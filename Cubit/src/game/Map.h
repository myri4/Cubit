#pragma once

#include "../include/wc/Utils/YAML.h"
#include <filesystem>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <random>

#include <wc/Math/Camera.h>

#include "ParticleSystem.h"
#include "Entities.h"
#include "Raycasting.h"
#include "Tile.h"
#include <magic_enum.hpp>

// @TODO: Separate tile map and scene
namespace wc
{
	int to1D(const glm::uvec3& pos, const glm::uvec3& size)	{ return (pos.z * size.x * size.y) + (pos.y * size.x) + pos.x; }

	Tileset m_Tileset; 
	
	RenderData m_RenderData;
	Renderer2D m_Renderer;

	ParticleSystem m_ParticleEmitter;
	ParticleProps m_Particle;
	ParticleProps m_SummonParticle;

	class ContactListener : public b2ContactListener
	{
		void BeginContact(b2Contact* contact) override
		{
			b2Fixture* fixtureA = contact->GetFixtureA();
			b2Fixture* fixtureB = contact->GetFixtureB();

			b2FixtureUserData fixtureUserDataA = fixtureA->GetUserData();
			b2FixtureUserData fixtureUserDataB = fixtureB->GetUserData();

			GameEntity* entityA = reinterpret_cast<GameEntity*>(fixtureUserDataA.pointer);
			GameEntity* entityB = reinterpret_cast<GameEntity*>(fixtureUserDataB.pointer);
			if (entityA && entityB)
			{
				if (entityA->Type == EntityType::Bullet)
				{
					Bullet& bullet = *(Bullet*)entityA;

					if (bullet.SourceEntity != entityB && entityB->Type != EntityType::Bullet)
					{
						bullet.HitEntityType = entityB->Type;
						if (entityB->Type > EntityType::Entity) bullet.HitEntity = entityB;
					}
				}

				if (entityB->Type == EntityType::Bullet)
				{
					Bullet& bullet = *(Bullet*)entityB;

					if (bullet.SourceEntity != entityA && entityA->Type != EntityType::Bullet)
					{
						bullet.HitEntityType = entityA->Type;
						if (entityA->Type > EntityType::Entity) bullet.HitEntity = entityA;
					}
				}
			}

			if (entityA && entityA->Type == EntityType::Bullet && !entityB)
			{
				Bullet& bullet = *(Bullet*)entityA;
				bullet.HitEntityType = EntityType::Tile;
			}

			if (entityB && entityB->Type == EntityType::Bullet && !entityA)
			{
				Bullet& bullet = *(Bullet*)entityB;
				bullet.HitEntityType = EntityType::Tile;
			}

			b2Vec2 bNormal = contact->GetManifold()->localNormal;
			glm::vec2 normal = glm::round(glm::vec2(bNormal.x, bNormal.y));
			if (entityA && entityA->Type > EntityType::Entity)
			{
				if (fixtureB->GetType() == b2Shape::e_chain)
				{
					entityA->Contacts++;
					if (normal == glm::vec2(0.f, -1.f)) entityA->UpContacts++;
					if (normal == glm::vec2(0.f, 1.f)) entityA->DownContacts++;
					if (normal == glm::vec2(1.f, 0.f)) entityA->LeftContacts++;
					if (normal == glm::vec2(-1.f, 0.f)) entityA->RightContacts++;
				}
			}

			if (entityB && entityB->Type > EntityType::Entity)
			{
				if (fixtureA->GetType() == b2Shape::e_chain)
				{
					entityB->Contacts++;
					if (normal == glm::vec2(0.f, -1.f)) entityB->UpContacts++;
					if (normal == glm::vec2(0.f, 1.f)) entityB->DownContacts++;
					if (normal == glm::vec2(1.f, 0.f)) entityB->LeftContacts++;
					if (normal == glm::vec2(-1.f, 0.f)) entityB->RightContacts++;
				}
			}
		}

		void EndContact(b2Contact* contact) override
		{
			b2Fixture* fixtureA = contact->GetFixtureA();
			b2Fixture* fixtureB = contact->GetFixtureB();

			b2FixtureUserData fixtureUserDataA = fixtureA->GetUserData();
			b2FixtureUserData fixtureUserDataB = fixtureB->GetUserData();

			GameEntity* entityA = reinterpret_cast<GameEntity*>(fixtureUserDataA.pointer);
			GameEntity* entityB = reinterpret_cast<GameEntity*>(fixtureUserDataB.pointer);

			b2Vec2 bNormal = contact->GetManifold()->localNormal;
			glm::vec2 normal = glm::round(glm::vec2(bNormal.x, bNormal.y));

			if (entityA && entityA->Type > EntityType::Entity)
			{
				if (fixtureB->GetType() == b2Shape::e_chain)
				{
					entityA->Contacts--;
					if (normal == glm::vec2(0.f, -1.f)) entityA->UpContacts--;
					if (normal == glm::vec2(0.f, 1.f)) entityA->DownContacts--;
					if (normal == glm::vec2(1.f, 0.f)) entityA->LeftContacts--;
					if (normal == glm::vec2(-1.f, 0.f)) entityA->RightContacts--;
				}
			}

			if (entityB && entityB->Type > EntityType::Entity)
			{
				if (fixtureA->GetType() == b2Shape::e_chain)
				{
					entityB->Contacts--;
					if (normal == glm::vec2(0.f, -1.f)) entityB->UpContacts--;
					if (normal == glm::vec2(0.f, 1.f)) entityB->DownContacts--;
					if (normal == glm::vec2(1.f, 0.f)) entityB->LeftContacts--;
					if (normal == glm::vec2(-1.f, 0.f)) entityB->RightContacts--;
				}
			}
		}
	} ContactListenerInstance;

	struct Map
	{
		Map() = default;

		~Map()
		{
			if (IsLoaded()) WC_CORE_WARN("Forgot to free the scene");
		}

		void Allocate()
		{
			if (IsLoaded()) Free();
			m_Size = Size.x * Size.y * Size.z;
			m_Data = new TileID[m_Size];
			memset(m_Data, 0, sizeof(TileID) * m_Size);
			if (Entities.size() == 0)
			{
				Entities.emplace_back(&player);
				player.Size = glm::vec2(1.f, 1.f) * 0.5f;
				player.HitBoxSize = player.Size;
			}
		}

		void Free(bool ResetSizes = false)
		{
			delete m_Data;
			m_Data = nullptr;

			if (ResetSizes)
			{
				Size.x = 1;
				Size.y = 1;
				Size.z = 1;
				m_Size = 1;
			}

			DestroyPhysicsWorld();

			if (Entities.size())
			{
				for (uint32_t i = 1; i < Entities.size(); i++) delete Entities[i];
				Entities.clear();
			}
		}

		void DestroyEntity(uint32_t i)
		{
			auto e = Entities[i];
			e->Body->DestroyFixture(e->Body->GetFixtureList());
			PhysicsWorld->DestroyBody(e->Body);

			delete e;
			Entities.erase(Entities.begin() + i);
		}

		bool IsLoaded() const { return m_Data != nullptr; }

		void SetTile(const glm::uvec3& coords, TileID tile) { m_Data[to1D(coords, Size)] = tile; }

		TileID GetTile(uint32_t id) { return m_Data[id]; }

		TileID GetTile(const glm::uvec3& coords) { return m_Data[to1D(coords, Size)]; }

		TileID GetTileSafe(const glm::ivec3& coords)
		{
			if (!IsLoaded()) return 0;
			for (uint32_t i = 0; i < 2; i++) if (coords[i] < 0 || coords[i] >= int(Size[i])) return 0;
			return GetTile(coords);
		}

		void Reset()
		{
			AccumulatedTime = 0.f;
			EnemyCount = 0;
			LevelTime = 0.f;
			//stopping sword animation
			m_RotateSword = false;
			m_SwordRotation = 0.f;

			m_ParticleEmitter.Reset();

			//resetting player and timers
			player.DashCD = 0.2f;
			player.Health = player.StartHealth;
			player.DownContacts = 0;
		}

		void Load(const std::string& filepath)
		{
			Reset();

			std::ifstream file(filepath);

			file >> Size.x >> Size.y >> Size.z;
			Allocate();

			std::vector<std::pair<TileID, uint16_t>> data;

			while (!file.eof())
			{
				uint32_t block;
				uint16_t count;

				file >> block >> count;
				data.emplace_back(block, count);
			}

			file.close();

			Decompress(data);

			std::filesystem::path filePath(filepath);
			std::string metaFilepath = filePath.replace_extension("metadata").string();
			if (std::filesystem::exists(metaFilepath))
			{
				YAML::Node mapMetaData = YAML::LoadFile(metaFilepath);

				{
					auto objects = mapMetaData["Entities"];
					for (int i = 0; i < objects.size(); i++)
					{
						auto metaData = objects[i];
						EntityType Type = EntityType::UNDEFINED;
						Type = magic_enum::enum_cast<EntityType>(metaData["Type"].as<std::string>()).value();

						if (Type == EntityType::Player)
						{
							player.Weapons[(int)WeaponType::Blaster].Ammo = 60;
							player.Weapons[(int)WeaponType::Shotgun].Ammo = 12;
							player.Weapons[(int)WeaponType::Revolver].Ammo = 24;
							player.Weapon = player.PrimaryWeapon;

							for (int i = 0; i < magic_enum::enum_count<WeaponType>(); i++) player.Weapons[i].Magazine = WeaponStats[i].MaxMag;

							player.LoadMapBase(metaData);
						}
						else if (Type == EntityType::RedCube)
						{
							RedCube* e = new RedCube();
							e->LoadMapBase(metaData);
							Entities.emplace_back(e);

							EnemyCount++;
						}
						else if (Type == EntityType::Fly)
						{
							Fly* e = new Fly();
							e->LoadMapBase(metaData);
							Entities.emplace_back(e);

							EnemyCount++;
						}
					}
				}

			}
			else WC_CORE_ERROR("Could not find metadata file for {}", filepath);
		}

		void LoadFull(const std::string& filepath) // @TODO: Rename?
		{
			Load(filepath);
			CreatePhysicsWorld();

			camera.Position = glm::vec3(player.Position, 0.f);			
		}

		float Gravity = -9.8f;
		void CreatePhysicsWorld()
		{
			PhysicsWorld = new b2World({ 0.f, Gravity });
			PhysicsWorld->SetContactListener(&ContactListenerInstance);

			b2Vec2 vs[4];

			for (uint32_t i = 0; i < Entities.size(); i++)
			{
				GameEntity* e = (GameEntity*)Entities[i];
				e->CreateBody(PhysicsWorld);
			}

			auto addFace = [&](const b2Vec2* face)
				{
					if ((face[0].x == face[1].x && face[1].x == 0.f) && face[0].y == face[1].y && face[1].y == 0.f) return;

					b2ChainShape chain;
					chain.CreateChain(face, 2, b2Vec2_zero, b2Vec2_zero);

					b2BodyDef bd;
					bd.type = b2_staticBody;
					b2Body* body = PhysicsWorld->CreateBody(&bd);

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &chain;
					fixtureDef.friction = 0.8f;

					body->CreateFixture(&fixtureDef);
				};


			// Top/down
			for (int y = 0; y < (int)Size.y; y++)
			{
				b2Vec2 TopStartPos = { 0.f, 0.f };
				b2Vec2 TopEndPos = { 0.f, 0.f };
				bool creatingTopLine = false;

				b2Vec2 BotStartPos = { 0.f, 0.f };
				b2Vec2 BotEndPos = { 0.f, 0.f };
				bool creatingBotLine = false;

				for (int x = 0; x < (int)Size.x; x++)
				{
					TileID tileID = GetTile({ x, y, 0 });
					TileID checkTile = 0;
					if (m_Tileset.Tiles[tileID].Solid)
					{
						// Top face
						checkTile = GetTileSafe({ x, y + 1, 0 });
						if (!m_Tileset.Tiles[checkTile].Solid)
						{
							if (!creatingTopLine)
							{
								TopStartPos = { x - 0.5f, y + 0.5f };
								TopEndPos = { x + 0.5f, y + 0.5f };
								creatingTopLine = true;
							}
							else
							{
								TopEndPos.x++;
							}
						}
						else if (creatingTopLine)
						{
							creatingTopLine = false;
							b2Vec2 FACE[2] = { TopEndPos, TopStartPos };
							addFace(FACE);
						}

						// Down face
						checkTile = GetTileSafe({ x, y - 1, 0 });
						if (!m_Tileset.Tiles[checkTile].Solid && y - 1 >= 0)
						{
							if (!creatingBotLine)
							{
								BotStartPos = { x - 0.5f, y - 0.5f };
								BotEndPos = { x + 0.5f, y - 0.5f };
								creatingBotLine = true;
							}
							else
							{
								BotEndPos.x++;
							}
						}
						else if (creatingBotLine)
						{
							creatingBotLine = false;
							b2Vec2 FACE[2] = { BotStartPos, BotEndPos };
							addFace(FACE);
						}
					}
					else
					{
						if (creatingTopLine)
						{
							b2Vec2 FACE[2] = { TopEndPos, TopStartPos };
							addFace(FACE);
							creatingTopLine = false;
						}

						if (creatingBotLine)
						{
							b2Vec2 FACE[2] = { BotStartPos, BotEndPos };
							addFace(FACE);
							creatingBotLine = false;
						}
					}
				}

				// Close any open top or bottom lines at the end of the row
				if (creatingTopLine)
				{
					b2Vec2 FACE[2] = { TopEndPos, TopStartPos };
					addFace(FACE);
					creatingTopLine = false;
				}

				if (creatingBotLine)
				{
					b2Vec2 FACE[2] = { BotStartPos, BotEndPos };
					addFace(FACE);
					creatingBotLine = false;
				}
			}

			// Left/right
			for (int x = 0; x < (int)Size.x; x++)
			{
				b2Vec2 LeftStartPos = { 0.f, 0.f };
				b2Vec2 LeftEndPos = { 0.f, 0.f };
				bool creatingLeftLine = false;

				b2Vec2 RightStartPos = { 0.f, 0.f };
				b2Vec2 RightEndPos = { 0.f, 0.f };
				bool creatingRightLine = false;

				for (int y = 0; y < (int)Size.y; y++)
				{
					TileID tileID = GetTileSafe({ x, y, 0 });
					TileID checkTile = 0;
					if (m_Tileset.Tiles[tileID].Solid)
					{
						// Left face
						checkTile = GetTileSafe({ x - 1, y, 0 });
						if (!m_Tileset.Tiles[checkTile].Solid && x - 1 >= 0)
						{
							if (!creatingLeftLine)
							{
								LeftStartPos = { x - 0.5f, y - 0.5f };
								LeftEndPos = { x - 0.5f, y + 0.5f };
								creatingLeftLine = true;
							}
							else
							{
								LeftEndPos.y++;
							}
						}
						else if (creatingLeftLine)
						{
							creatingLeftLine = false;
							b2Vec2 FACE[2] = { LeftEndPos, LeftStartPos };
							addFace(FACE);
						}

						// Right face
						checkTile = GetTileSafe({ x + 1, y, 0 });
						if (!m_Tileset.Tiles[checkTile].Solid && x + 1 < (int)Size.x)
						{
							if (!creatingRightLine)
							{
								RightStartPos = { x + 0.5f, y - 0.5f };
								RightEndPos = { x + 0.5f, y + 0.5f };
								creatingRightLine = true;
							}
							else
							{
								RightEndPos.y++;
							}
						}
						else if (creatingRightLine)
						{
							creatingRightLine = false;
							b2Vec2 FACE[2] = { RightStartPos, RightEndPos };
							addFace(FACE);
						}
					}
					else
					{
						if (creatingLeftLine)
						{
							b2Vec2 FACE[2] = { LeftEndPos, LeftStartPos };
							addFace(FACE);
							creatingLeftLine = false;
						}

						if (creatingRightLine)
						{
							b2Vec2 FACE[2] = { RightStartPos, RightEndPos };
							addFace(FACE);
							creatingRightLine = false;
						}
					}
				}

				// Close any open left or right lines at the end of the column
				if (creatingLeftLine)
				{
					b2Vec2 FACE[2] = { LeftEndPos, LeftStartPos };
					addFace(FACE);
					creatingLeftLine = false;
				}

				if (creatingRightLine)
				{
					b2Vec2 FACE[2] = { RightStartPos, RightEndPos };
					addFace(FACE);
					creatingRightLine = false;
				}
			}
		}

		HitInfo Intersect(const Ray& ray, uint32_t startIndex = 0)
		{
			HitInfo hitInfo;
			float t = FLT_MAX;

			float fMaxDistance = 36.f;

			glm::vec2 vRayLength1D;
			auto& vRayStart = ray.Origin;
			auto& vRayDir = ray.Direction;
			auto vRayUnitStepSize = abs(ray.InvDirection);
			glm::ivec2 vMapCheck = glm::round(vRayStart);
			glm::ivec2 vMapLastCheck = vMapCheck;
			glm::ivec2 vStep = glm::ivec2(glm::sign(vRayDir));

			// Establish Starting Conditions
			for (int i = 0; i < 2; i++)
				vRayLength1D[i] = ((vRayDir[i] < 0.f ? (vRayStart[i] - float(vMapCheck[i])) : (float(vMapCheck[i]) - vRayStart[i])) + 0.5f) * vRayUnitStepSize[i];

			bool bTileFound = false;
			float fDistance = 0.f;
			while (!bTileFound && fDistance < fMaxDistance)
			{
				// Walk along shortest path
				int axis = 0;
				for (int i = 0; i < 2; i++)
				{
					int nextAxis = (i + 1) % 2;
					if (vRayLength1D[axis] > vRayLength1D[nextAxis]) axis = nextAxis;
				}

				vMapCheck[axis] += vStep[axis];
				fDistance = vRayLength1D[axis];
				vRayLength1D[axis] += vRayUnitStepSize[axis];

				// Test tile at new test point
				TileID tileID = GetTile(glm::uvec3(vMapCheck, 0));
				if (tileID > 0)
				{
					hitInfo.Hit = true;
					t = fDistance;
					hitInfo.N = glm::vec2(vMapLastCheck - vMapCheck);
					bTileFound = true;
				}
				vMapLastCheck = vMapCheck;
			}

			for (uint32_t i = startIndex; i < Entities.size(); i++)
			{
				auto& entity = *Entities[i];

				auto oHitInfo = aabbIntersection(ray, -entity.Size + entity.Position, entity.Size + entity.Position);

				if (oHitInfo.Hit && oHitInfo.t < t)
				{
					hitInfo.Hit = true;
					t = oHitInfo.t;
					hitInfo.Entity = Entities[i];
					hitInfo.N = oHitInfo.N;
					hitInfo.Point = oHitInfo.Point;
				}
			}

			hitInfo.Point = ray.Origin + t * ray.Direction;
			hitInfo.t = t;

			return hitInfo;
		}

		void Explode(glm::vec2 position, float radius, float blastPower)
		{
			for (auto& en : Entities)
			{
				auto& e = *en;
				float dist = glm::distance(position, e.Position);

				if (dist <= radius)
				{
					float invDistance = 1.f / dist;
					glm::vec2 dir = glm::normalize(e.Position - position) * blastPower * invDistance * invDistance;
					if (e.Type != EntityType::Bullet) e.Body->ApplyLinearImpulse(b2Vec2{ dir.x, dir.y }, b2Vec2{ position.x, position.y }, true);
				}
			}
		}

		void SpawnBullet(glm::vec2 position, glm::vec2 direction, WeaponType weaponType, GameEntity* src)
		{
			auto& weaponInfo = WeaponStats[(int)weaponType];
			Bullet* bullet = new Bullet();
			bullet->SourceEntity = src;
			bullet->Position = position;
			bullet->ShotPos = position;
			bullet->Size = weaponInfo.BulletSize;
			bullet->Speed = weaponInfo.BulletSpeed;
			bullet->Direction = direction;
			bullet->BulletType = weaponInfo.BulletType;
			bullet->WeaponType = weaponType;
			bullet->Density = 0.f;
			bullet->LinearDamping = 0.f;
			bullet->CreateBody(PhysicsWorld);
			Entities.emplace_back(bullet);
		}

		void DestroyPhysicsWorld()
		{
			delete PhysicsWorld;
			PhysicsWorld = nullptr;
		}

		void UpdateAI()
		{
			for (uint32_t i = 1; i < Entities.size(); i++)
			{
				auto& e = Entities[i];

				e->ID = i;
				switch (e->Type)
				{
				case EntityType::RedCube:
				{
					RedCube& entity = *(RedCube*)e;

					if (entity.AttackTimer > 0.f) entity.AttackTimer -= Globals.deltaTime;
				}
				break;

				case EntityType::Fly:
				{
					Fly& entity = *(Fly*)e;

					entity.Body->SetGravityScale(0.f);

					if (entity.AttackTimer > 0.f) entity.AttackTimer -= Globals.deltaTime;
				}
				break;

				case EntityType::Bullet:
				{

				}
				break;
				default:
					break;
				}
			}
		}

		void UpdateAIFixed()
		{
			for (uint32_t i = 0; i < Entities.size(); i++)
			{
				auto& e = Entities[i];

				switch (e->Type)
				{
				case EntityType::RedCube:
				{
					RedCube& entity = *(RedCube*)e;	

					if (!entity.Alive())
					{
						EnemyCount--;
						DestroyEntity(i);
					}


					//movement
					float distToPlayer = glm::distance(player.Position, entity.Position);
					if (distToPlayer < entity.DetectRange)
					{
						if (entity.Position.x > player.Position.x) entity.Body->ApplyLinearImpulseToCenter(b2Vec2(-entity.Speed, 0), true);
						else entity.Body->ApplyLinearImpulseToCenter(b2Vec2(entity.Speed, 0), true);

						/*if (entity.Position.x > player.Position.x)entity.body->SetLinearVelocity(b2Vec2(-entity.Speed, 0));
						else entity.body->SetLinearVelocity(b2Vec2(entity.Speed, 0));*/
					}
					//attack behavior
					if (distToPlayer < entity.ShootRange)
					{
						if (entity.AttackTimer <= 0 && entity.Alive())
						{
							//shoot
							glm::vec2 dir = glm::normalize(player.Position - entity.Position);
							SpawnBullet(entity.Position + dir * 0.5f, dir, WeaponType::RedBlaster, e);

							entity.AttackTimer = 2.f;
						}
					}
				}
				break;

				case EntityType::Fly: 
				{
					Fly& entity = *(Fly*)e;

					entity.Body->SetGravityScale(0.f);

					if (!entity.Alive())
					{
						EnemyCount--;
						DestroyEntity(i);
					}

					//movement
					float distToPlayer = glm::distance(player.Position, entity.Position);
					if (distToPlayer < entity.DetectRange)
					{
						if (entity.Position.x > player.Position.x) entity.Body->ApplyLinearImpulseToCenter(b2Vec2(-entity.Speed, 0), true);
						else entity.Body->ApplyLinearImpulseToCenter(b2Vec2(entity.Speed, 0), true);

					}
					//attack behavior
					if (distToPlayer < entity.ShootRange)
					{
						if (entity.AttackTimer <= 0 && entity.Alive())
						{
							WC_CORE_INFO("Fly Attack");
						}
					}

				}
				break;

				case EntityType::Bullet:
				{
					Bullet& bullet = *(Bullet*)e;
					WeaponInfo& weapon = WeaponStats[(int)bullet.WeaponType];

					if (bullet.HitEntityType != EntityType::UNDEFINED)
					{
						if (bullet.BulletType == BulletType::RedCircle && bullet.HitEntityType == EntityType::Player)
						{
							std::random_device rd;
							std::mt19937 gen(rd());
							std::uniform_int_distribution<> dis(0, 1); // Define the range

							if (dis(gen) == 0)
							{
								RedCube* em = new RedCube();
								em->Position = bullet.Position + glm::vec2(0.f, 1.f);
								em->CreateBody(PhysicsWorld);

								m_SummonParticle.Position = em->Position;
								m_SummonParticle.Velocity = glm::vec2(0.5f);
								for (int i = 0; i < 6; i++)
								{
									m_SummonParticle.VelocityVariation = glm::normalize(glm::vec3(RandomValue(), RandomValue(), RandomValue()));

									m_ParticleEmitter.Emit(m_SummonParticle);
								}
								EnemyCount++;
								Entities.emplace_back(em);
							}

							ma_sound_start(&Globals.damageEnemy);
							player.DealDamage(weapon.Damage);
						}

						if (bullet.BulletType == BulletType::Blaster && bullet.HitEntityType > EntityType::Entity)
						{
							ma_sound_start(&Globals.damageEnemy);
							GameEntity* shotEnt = bullet.HitEntity;
							shotEnt->DealDamage(weapon.Damage);

							m_Particle.LifeTime = 0.35f;
							m_Particle.ColorBegin = glm::vec4(0.f, 1.f, 0.f, 1.f) * 2.f;
							m_Particle.ColorEnd = glm::vec4(0.f, 1.f, 0.f, 1.f);
							m_Particle.Position = shotEnt->Position;
							m_Particle.Velocity = glm::vec2(0.5f);
							m_Particle.VelocityVariation = glm::normalize(shotEnt->Position - player.Position) * 2.5f;
							m_ParticleEmitter.Emit(m_Particle, 6);
						}

						if (bullet.BulletType == BulletType::Shotgun && bullet.HitEntityType > EntityType::Entity)
						{
							GameEntity* shotEnt = bullet.HitEntity;
							shotEnt->DealDamage(weapon.Damage);
						}

						if (bullet.BulletType == BulletType::Revolver && bullet.HitEntityType > EntityType::Entity)
						{
							GameEntity* shotEnt = bullet.HitEntity;
							shotEnt->DealDamage(weapon.Damage);
						}

						DestroyEntity(i);
					}
					else if (glm::distance(bullet.ShotPos, bullet.Position) > weapon.Range)
					{
						DestroyEntity(i);
					}
				}
				break;
				default:
					break;
				}
			}
		}

		void FixedUpdate()
		{
			if (player.MoveDir != 0.f)
				player.Body->ApplyLinearImpulseToCenter({ player.MoveDir * player.Speed * player.Body->GetMass() / 10.f * (player.DownContacts > 0 ? 1.f : AirSpeedFactor), 0.f }, true);

			UpdateAIFixed();
		}

		void InputGame()
		{
			if (ImGui::IsKeyDown((ImGuiKey)Globals.settings.KeyLeft)) player.MoveDir = -1.f;
			else if (ImGui::IsKeyDown((ImGuiKey)Globals.settings.KeyRight)) player.MoveDir = 1.f;
			else player.MoveDir = 0.f;

			if (player.MoveDir != 0.f)
			{
				if (Key::GetKey(Key::LeftShift) == GLFW_PRESS && player.DashCD <= 0.f)
				{
					ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/dash.wav", NULL);
					player.Body->ApplyLinearImpulseToCenter({ 50.f * player.Body->GetMass() * player.MoveDir, 0.f }, true);
					player.DashCD = 2.f;
				}
			}

			if (ImGui::IsKeyReleased((ImGuiKey)Globals.settings.KeyFastSwich))
			{
				if (player.Weapon == player.PrimaryWeapon) player.Weapon = player.SecondaryWeapon;
				else player.Weapon = player.PrimaryWeapon;
			}

			if (ImGui::IsKeyPressed((ImGuiKey)Globals.settings.KeyMainWeapon))
				player.Weapon = player.PrimaryWeapon;

			else if (ImGui::IsKeyPressed((ImGuiKey)Globals.settings.KeySecondaryWeapon))
				player.Weapon = player.SecondaryWeapon;


			if (ImGui::IsKeyPressed((ImGuiKey)Globals.settings.KeyJump) && player.DownContacts != 0)
			{
				// @NOTE: if gravity is changed we should update this
				float jumHeight = 8.f;
				player.JumpForce = glm::sqrt(jumHeight * Gravity * player.Body->GetGravityScale() * -(2.f + player.LinearDamping)) * player.Body->GetMass();

				player.Body->ApplyLinearImpulseToCenter({ 0.f, player.JumpForce }, true);
			}

			if (player.DownContacts > 0)
			{
				auto vel = player.Body->GetLinearVelocity();

				player.Body->SetGravityScale(1.f);

				vel.x -= DragStrength * vel.x * Globals.deltaTime;

				if (abs(vel.x) < 0.01f) vel.x = 0.f;

				player.Body->SetLinearVelocity(vel);
			}

			if (player.Body->GetLinearVelocity().y < 0.f) player.Body->SetGravityScale(2.5f);


			glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);

			bool Zoom = Mouse::GetMouse(Mouse::RIGHT) != GLFW_RELEASE;

			m_TargetRotation = 0.f;
			if (Zoom && player.Weapon != WeaponType::Revolver)
			{
				m_TargetPosition = player.Position + dir * 3.f;
				m_TargetZoom = 0.6f;
			}
			else
			{
				m_TargetPosition = player.Position;
				m_TargetZoom = 1.f;
			}

			if (ImGui::IsKeyPressed(ImGuiKey_R))
				player.ReloadWeapon();

			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && player.CanShoot())
			{
				if (player.Weapon == WeaponType::Blaster)
				{
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_real_distribution<float> dis(-0.08f, 0.12f);

					ma_sound_start(&Globals.gun);

					//SpawnBullet(player.Position + dir * 0.75f, Zoom ? dir : RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))), player.weapon, (GameEntity*)&player);
					SpawnBullet(player.Position + dir * 0.75f, dir, player.Weapon, (GameEntity*)&player);
				}
				else if (player.Weapon == WeaponType::Shotgun)
				{
					camera.Shake(0.8f);

					glm::vec2 shootPos = player.Position + dir * 0.35f;
					auto hitInfo = Intersect({ shootPos, dir }, 1);

					if (hitInfo.t <= 5.f)
					{
						float invDist = 1.f / glm::max(hitInfo.t, 1.f);
						auto recoilForce = -dir * player.JumpForce * 2.f * invDist * invDist;
						player.Body->ApplyLinearImpulseToCenter({ recoilForce.x, recoilForce.y }, true);
					}

					ma_sound_start(&Globals.shotgun);
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_real_distribution<float> dis(-0.35f, 0.35f);

					for (uint32_t i = 0; i < 10; i++)
					{
						SpawnBullet(shootPos, RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))), player.Weapon, (GameEntity*)&player);

						m_Particle.Position = player.Position + dir * 0.55f;
						auto& vel = player.Body->GetLinearVelocity();
						m_Particle.ColorBegin = glm::vec4{ 254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f } *2.f;
						m_Particle.ColorEnd = { 254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f };
						m_Particle.Velocity = glm::vec2(vel.x, vel.y) * 0.45f;
						m_Particle.VelocityVariation = glm::normalize(player.Position + RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))) * 0.85f - player.Position) * 5.f;
						m_ParticleEmitter.Emit(m_Particle, 5);
					}
				}
				else if (player.Weapon == WeaponType::Revolver)
				{
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_real_distribution<float> dis(-0.08f, 0.12f);

					ma_sound_start(&Globals.gun);
					SpawnBullet(player.Position + dir * 0.75f, RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))), player.Weapon, (GameEntity*)&player);
				}

				player.Weapons[(int)player.Weapon].Magazine--;
				player.ResetWeaponTimer();
			}

			//alt fire
			if (ImGui::IsKeyDown(ImGuiKey_MouseRight) && player.CanShoot())
			{
				glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);

				player.ResetWeaponTimer(false);

				if (player.Weapon == WeaponType::Revolver)
				{
					ma_sound_start(&Globals.gun);

					player.Weapons[(int)player.Weapon].AltFireTimer = 1.2f;
					player.Weapons[(int)player.Weapon].Timer = 0.f;
				}
			}

			//revolver alt fire
			if (player.Weapons[(int)player.Weapon].AltFireTimer > 0.f && player.CanShoot())
			{
				if (player.Weapon == WeaponType::Revolver) 
				{
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_real_distribution<float> dis(-0.25f, 0.25f);

					if (player.CanShoot())
					{
						glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);

						SpawnBullet(player.Position + dir * 0.75f, RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))), player.Weapon, (GameEntity*)&player);
						player.Weapons[(int)player.Weapon].Magazine--;
						player.Weapons[(int)player.Weapon].Timer = 0.2f;
					}
				}
			}


			if (ImGui::IsKeyDown((ImGuiKey)Globals.settings.KeyMelee) && player.CanMelee()) 
			{
				if (player.MeleeWeapon == WeaponType::Sword)
				{
					ma_sound_start(&Globals.swordSwing);
					m_RotateSword = true;

					for (uint32_t i = 0; i < Entities.size(); i++)
					{
						auto& entity = *Entities[i];
						if (entity.Type == EntityType::RedCube || entity.Type == EntityType::Fly)
						{
							float dist = glm::distance(entity.Position, player.Position);
							if (dist < WeaponStats[(int)player.MeleeWeapon].Range)
							{
								glm::vec2 direction = glm::normalize(entity.Position - player.Position);

								auto hitInfo = Intersect({ player.Position, direction }, Entities.size()); // We skip entity intersection

								if ((hitInfo.Hit && dist <= hitInfo.t) || !hitInfo.Hit)
								{
									auto vel = glm::sign(direction.x) * 50.f * entity.Body->GetMass();
									entity.Body->ApplyLinearImpulseToCenter(b2Vec2(vel, 0.f), true);
									entity.DealDamage(WeaponStats[(int)player.MeleeWeapon].Damage);
								}
							}
						}
					}
				}

				player.ResetMeleeTimer();
			}
		}

		void Update()
		{
			const int32_t velocityIterations = 8;
			const int32_t positionIterations = 3;
			const int32_t MAX_STEPS = 5;

			AccumulatedTime += Globals.deltaTime;

			const int32_t nSteps = (int32_t)glm::floor(AccumulatedTime / SimulationTime);
			const int32_t nStepsClamped = glm::min(nSteps, MAX_STEPS);

			UpdateAI();
			if (nStepsClamped > 0)
			{
				AccumulatedTime -= nStepsClamped * SimulationTime;

				for (int i = 0; i < nStepsClamped; i++)
				{
					FixedUpdate();
					PhysicsWorld->Step(SimulationTime, velocityIterations, positionIterations);
				}
			}

			float AccumulatedTimeRatio = AccumulatedTime / SimulationTime;

			for (auto& entity : Entities)
				entity->UpdatePosition(AccumulatedTimeRatio);

			if (player.DashCD > 0.f) player.DashCD -= Globals.deltaTime;

			for (uint32_t i = 0; i < magic_enum::enum_count<WeaponType>(); i++)
			{
				auto& weapon = player.Weapons[i];
				if (weapon.Timer > 0.f) weapon.Timer -= Globals.deltaTime;
				if (weapon.AltFireTimer > 0.f) weapon.AltFireTimer -= Globals.deltaTime;
				if (weapon.ReloadTimer > 0.f) weapon.ReloadTimer -= Globals.deltaTime;
			}
		}

		void UpdateGame()
		{
			Update();

			LevelTime += Globals.deltaTime;
			m_TargetPosition = glm::clamp(m_TargetPosition, glm::vec2(8, 4), glm::vec2(Size) - glm::vec2(8, 4));

			camera.Position += glm::vec3((m_TargetPosition - glm::vec2(camera.Position)) * 11.5f * Globals.deltaTime, 0.f);
			camera.Rotation += (m_TargetRotation - camera.Rotation) * 11.5f * Globals.deltaTime;
			if (camera.Zoom != m_TargetZoom)
			{
				camera.Zoom += (m_TargetZoom - camera.Zoom) * 11.5f * Globals.deltaTime;
				camera.Update(m_Renderer.GetHalfSize());
			}
			m_Renderer.ChromaSettings.Falloff += (m_TargetChromaFallOff - m_Renderer.ChromaSettings.Falloff) * 11.5f * Globals.deltaTime;

			m_ParticleEmitter.OnUpdate();

			if (!player.Alive()) Globals.gameState = GameState::DEATH;
			if (EnemyCount == 0) Globals.gameState = GameState::WIN;
		}

		void RenderGame()
		{
			m_RenderData.ViewProjection = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, -1.f, 1.f);
			m_RenderData.DrawQuad({ 0.f, 0.f, 0.f }, { 1.f, 1.f }, m_Renderer.BackgroundTexture);
			m_RenderData.ViewProjection = camera.GetViewProjectionMatrix();
						
			for (uint32_t x = 0; x < Size.x; x++)
				for (uint32_t y = 0; y < Size.y; y++)
				{
					TileID tileID = GetTile({ x,y, 0 });
					if (tileID != 0) m_RenderData.DrawQuad({ x, y , 0.f }, { 1.f, 1.f }, 0, glm::vec4(0.27f, 0.94f, 0.98f, 1.f));
				}

			for (int i = 0; i < Entities.size(); i++)
			{
				GameEntity& entity = *(GameEntity*)(Entities[i]);

				if (entity.Type == EntityType::Bullet)
				{
					Bullet& bullet = *(Bullet*)(Entities[i]);
					glm::vec4 bulletColor = glm::vec4(0.f);
					if (bullet.BulletType == BulletType::Blaster) bulletColor = glm::vec4(0.f, 1.f, 0.f, 1.f);
					if (bullet.BulletType == BulletType::Revolver) bulletColor = glm::vec4(1.f, 1.f, 0.f, 1.f);
					if (bullet.BulletType == BulletType::Shotgun) bulletColor = glm::vec4(1.f, 1.f, 0.f, 1.f);
					if (bullet.BulletType == BulletType::RedCircle) bulletColor = glm::vec4(1.f, 0.f, 0.f, 1.f);
					m_RenderData.DrawCircle(glm::vec3(entity.Position, 0.f), entity.Size.x, 1.f, 0.05f, bulletColor * 1.3f);
				}
				else
				{
					if (entity.Type != EntityType::Player)
						m_RenderData.DrawString(std::format("HP: {}", entity.Health), font, entity.Position + glm::vec2(-0.5f, 1.f), glm::vec4(1.f, 0, 0, 1.f));

					m_RenderData.DrawQuad(glm::vec3(entity.Position, 0.f), entity.Size * 2.f, 0, entity.Type == EntityType::RedCube || entity.Type == EntityType::Fly ? glm::vec4(1.f, 0, 0, 1.f) : glm::vec4(0.27f, 0.94f, 0.98f, 1.f));
				}
			}

			if (m_RotateSword)
			{
				m_SwordRotation += (2.f * glm::pi<float>() - m_SwordRotation) * 11.5f * Globals.deltaTime;

				if (m_SwordRotation >= 2.f * glm::pi<float>() - 0.1f)
				{
					m_SwordRotation = 0.f;
					m_RotateSword = false;
				}
				glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(player.Position, 0.f)) *
					glm::rotate(glm::mat4(1.f), m_SwordRotation, { 0.f, 0.f, 1.f }) * glm::scale(glm::mat4(1.f),
						glm::vec3{ 0.14f, 1.f, 0.5f } * 6.f);

				m_RenderData.DrawQuad(transform, SwordTexture);
			}
			else
			{
				auto& weapon = WeaponStats[(int)player.Weapon];

				glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);
				glm::vec2 offset = weapon.Offset;
				float angle = atan2(dir.y, dir.x);
				if (dir.x < 0.f)
				{
					angle = glm::pi<float>() - angle;
					offset.x *= -1.f;
				}
				glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(player.Position, 0.f)) *
					glm::rotate(glm::mat4(1.f), angle, { 0.f, 0.f, dir.x < 0.f ? -1.f : 1.f }) *
					glm::translate(glm::mat4(1.f), glm::vec3(offset, 0.f)) * glm::scale(glm::mat4(1.f),
						{ (dir.x < 0.f ? -1.f : 1.f) * weapon.Size.x, weapon.Size.y, 1.f });

				m_RenderData.DrawQuad(transform, weapon.TextureID);
			}

			glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);
			glm::vec2 shootPos = player.Position + dir * 0.35f;

			m_ParticleEmitter.OnRender(m_RenderData);

			m_Renderer.Flush(m_RenderData);
			m_RenderData.Reset();
		}

		uint32_t Get1DSize() { return m_Size; }
		TileID* GetData() { return m_Data; }

	public:
		glm::uvec3 Size = glm::uvec3(1);

		std::vector<GameEntity*> Entities;

		Player player;

		b2World* PhysicsWorld = nullptr;

		uint32_t EnemyCount = 0;

		OrthographicCamera camera;

		float DragStrength = 2.f;
		float AirSpeedFactor = 0.7f;
		uint32_t SwordTexture = 0;
		Font font;

		bool m_RotateSword = false;
		float m_SwordRotation = 0.f;
		float AccumulatedTime = 0.f;

		float LevelTime = 0.f;
	private:
		TileID* m_Data = nullptr;
		uint32_t m_Size = 1;

		glm::vec2 m_TargetPosition;
		float m_TargetRotation = 0.f;
		float m_TargetZoom = 1.f;
		float m_TargetChromaFallOff = 10.f;


		const float SimulationTime = 1.f / 60.f;

	private:

		std::vector<std::pair<TileID, uint16_t>> Compress()
		{
			std::vector<std::pair<TileID, uint16_t>> compressed;
			TileID pBlockID = m_Data[0];
			uint16_t count = 0;

			for (uint32_t i = 0; i < Size.x * Size.y * Size.z; i++) // @TODO: optimize
			{
				TileID block = m_Data[i];
				if (block == pBlockID) count++;
				else
				{
					compressed.emplace_back(pBlockID, count);
					pBlockID = m_Data[i];
					count = 1;
				}
			}
			compressed.emplace_back(pBlockID, count);
			return compressed;
		}

		void Decompress(const std::vector<std::pair<TileID, uint16_t>>& tiles)
		{
			uint16_t counter = 0;
			for (auto& tile : tiles)
				for (uint16_t i = 0; i < tile.second; i++)
					m_Data[counter++] = tile.first;
		}
	};
}