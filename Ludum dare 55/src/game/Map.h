#pragma once

#include "../include/wc/Utils/YAML.h"
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <random>

#include "ParticleSystem.h"
#include "Entities.h"
//#include "Raycasting.h"
#include "Tile.h"
#include <magic_enum.hpp>

// @TODO: Separate tile map and scene
namespace wc
{
	int to1D(const glm::uvec3& pos, const glm::uvec3& size)
	{
		return (pos.z * size.x * size.y) + (pos.y * size.x) + pos.x;
	}

	Tileset m_Tileset; 
	
	RenderData m_RenderData;
	Renderer2D m_Renderer;

	ParticleSystem m_ParticleEmitter;
	ParticleProps m_Particle;
	ParticleProps m_SummonParticle;
	SvgImage svg;

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
				bool anyPlayer = entityA->Type == EntityType::Player || entityB->Type == EntityType::Player;

				if (entityA->Type == EntityType::Bullet)
				{
					if (entityB->Type == EntityType::Player)
						entityA->PlayerTouch = true;
					else if (entityB->Type == EntityType::RedCube)
					{
						entityA->ShotEnemy = true;
						entityA->EnemyID = entityB->ID;
					}
				}

				if (entityB->Type == EntityType::Bullet)
				{
					if (entityA->Type == EntityType::Player)
						entityB->PlayerTouch = true;
					else if (entityA->Type == EntityType::RedCube)
					{
						entityB->ShotEnemy = true;
						entityB->EnemyID = entityA->ID;
					}
				}
			}

			b2Vec2 bNormal = contact->GetManifold()->localNormal;
			glm::vec2 normal = { glm::round(bNormal.x), glm::round(bNormal.y) };
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

			if (entityA && entityB)
			{
				if (entityA->Type == EntityType::Bullet)
				{
					if (entityB->Type == EntityType::Player)
						entityA->PlayerTouch = false;
					else if (entityB->Type == EntityType::RedCube)
					{
						entityA->ShotEnemy = false;
						entityA->EnemyID = entityB->ID;
					}

				}

				if (entityB->Type == EntityType::Bullet)
				{
					if (entityA->Type == EntityType::Player)
						entityB->PlayerTouch = false;
					else if (entityA->Type == EntityType::RedCube)
					{
						entityB->ShotEnemy = false;
						entityB->EnemyID = entityA->ID;
					}
				}
			}

			b2Vec2 bNormal = contact->GetManifold()->localNormal;
			glm::vec2 normal = { bNormal.x, bNormal.y };

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
		Map()
		{
			Entities.emplace_back(&player);
			player.Size = glm::vec2(1.f, 1.f) * 0.5f;
			player.HitBoxSize = player.Size;
		}

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
			delete Entities[i];
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

		void Load(const std::string& filepath)
		{
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

						if (Type == EntityType::Player) player.LoadMapBase(metaData);
						else if (Type == EntityType::RedCube)
						{
							RedCube* e = new RedCube();
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

		void CreatePhysicsWorld()
		{
			PhysicsWorld = new b2World({ 0.f, -9.8f });
			PhysicsWorld->SetContactListener(&ContactListenerInstance);

			b2Vec2 vs[4];

			for (uint32_t i = 0; i < Entities.size(); i++)
			{
				GameEntity* e = (GameEntity*)Entities[i];
				b2BodyDef bodyDef;
				bodyDef.type = b2_dynamicBody;
				bodyDef.position.Set(e->Position.x, e->Position.y);
				bodyDef.fixedRotation = true;
				e->body = PhysicsWorld->CreateBody(&bodyDef);

				b2PolygonShape shape;
				shape.SetAsBox(e->HitBoxSize.x, e->HitBoxSize.y);


				b2FixtureDef fixtureDef;
				fixtureDef.density = e->Density;
				fixtureDef.friction = 0.f;
				fixtureDef.userData.pointer = (uintptr_t)e;

				fixtureDef.shape = &shape;
				e->body->CreateFixture(&fixtureDef);
				e->body->SetLinearDamping(e->LinearDamping);
			}
			uint32_t faceCount = 0;
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
			for (int y = 0; y < (int)Size.y; ++y)
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

						if (!m_Tileset.Tiles[checkTile].Solid && y + 1 < (int)Size.y)
						{
							if (!creatingTopLine)
							{
								TopStartPos = { x - 0.5f, y + 0.5f };
								TopEndPos = { x + 0.5f, y + 0.5f };
								creatingTopLine = true;
							}
							else
								TopEndPos.x++;
						}
						else
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
								BotEndPos.x++;
						}
						else
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
						}

						if (creatingBotLine)
						{
							b2Vec2 FACE[2] = { BotStartPos, BotEndPos };
							addFace(FACE);
						}

						creatingBotLine = false;
						creatingTopLine = false;
					}
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
								LeftEndPos.y++;
						}
						else
						{
							creatingLeftLine = false;
							b2Vec2 FACE[2] = { LeftEndPos, LeftStartPos };
							addFace(FACE);
						}

						// Right face
						checkTile = GetTileSafe({ x + 1, y, 0 });
						if (!m_Tileset.Tiles[checkTile].Solid)
						{
							if (!creatingRightLine && x + 1 < (int)Size.x)
							{
								RightStartPos = { x + 0.5f, y - 0.5f };
								RightEndPos = { x + 0.5f, y + 0.5f };
								creatingRightLine = true;
							}
							else
								RightEndPos.y++;
						}
						else
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
						}

						if (creatingRightLine)
						{
							b2Vec2 FACE[2] = { RightStartPos, RightEndPos };
							addFace(FACE);
						}

						creatingRightLine = false;
						creatingLeftLine = false;
					}
				}
			}
		}

		void Explode(glm::vec2 position, float radius, float blastPower)
		{
			for (auto& en : Entities)
			{
				auto& e = *en;
				float dist = glm::distance(position, e.Position);

				if (dist <= radius)
				{
					float invDistance = 1 / dist;
					glm::vec2 dir = glm::normalize(e.Position - position) * blastPower * invDistance * invDistance;
					if (e.Type != EntityType::Bullet) e.body->ApplyLinearImpulse(b2Vec2{ dir.x, dir.y }, b2Vec2{ position.x, position.y }, true);
				}
			}
		}

		void SpawnBullet(glm::vec2 position, glm::vec2 direction, float speed, float damage, glm::vec2 size, BulletType bulletType)
		{
			Bullet* bullet = new Bullet();
			bullet->Position = position;
			bullet->ShotPos = position;
			bullet->Size = size;
			bullet->Damage = damage;
			bullet->Speed = speed;
			bullet->Direction = direction;
			bullet->BulletType = bulletType;			
			bullet->Density = 20.f;
			bullet->LinearDamping = 0.f;
			bullet->CreateBody(PhysicsWorld);
			bullet->body->SetBullet(true);
			bullet->body->GetFixtureList()->SetSensor(true);
			Entities.emplace_back(bullet);
		}


		void DestroyPhysicsWorld()
		{
			delete PhysicsWorld;
			PhysicsWorld = nullptr;
		}

		void UpdateAI()
		{
			for (uint32_t i = 0; i < Entities.size(); i++)
			{
				auto& e = Entities[i];

				e->ID = i;
				switch (e->Type)
				{
				case EntityType::RedCube:
				{
					RedCube& entity = *(RedCube*)e;
					//timer
					if (entity.AttackTimer > 0.f) entity.AttackTimer -= Globals.deltaTime;

					if (player.SwordAttack && player.SwordCD <= 0 && glm::distance(entity.Position, player.Position) < 8.f) 
					{
						glm::vec2 direction = glm::normalize((player.Position - glm::vec2(0, player.Size.y * 0.5f)) - entity.Position);
						entity.body->ApplyLinearImpulseToCenter(-1500.f * b2Vec2(direction.x, direction.y), true);
						entity.Health -= player.SwordDamage;
					}

					if (!entity.Alive())
					{
						//Destroy
						entity.body->DestroyFixture(entity.body->GetFixtureList());
						PhysicsWorld->DestroyBody(entity.body);
						EnemyCount--;
						DestroyEntity(i);
					}


					//movement
					float distToPlayer = glm::distance(player.Position, entity.Position);
					if (distToPlayer < entity.DetectRange)
					{
						if (entity.Position.x > player.Position.x)entity.body->ApplyLinearImpulseToCenter(b2Vec2(-entity.Speed, 0), true);
						else entity.body->ApplyLinearImpulseToCenter(b2Vec2(entity.Speed, 0), true);

						/*if (entity.Position.x > player.Position.x)entity.body->SetLinearVelocity(b2Vec2(-entity.Speed, 0));
						else entity.body->SetLinearVelocity(b2Vec2(entity.Speed, 0));*/
					}
					//attack behavior
					if (distToPlayer < entity.ShootRange)
					{
						if (entity.AttackTimer <= 0 && entity.Alive())
						{
							//shoot
							glm::vec2 directionToPlayer = glm::normalize(player.Position + glm::vec2(0.f, 0.5f) - entity.Position);
							SpawnBullet(entity.Position + glm::vec2(0, 0.5f), directionToPlayer, 25.f, 0.5f, { 0.25f, 0.25f }, BulletType::RedCircle);

							entity.AttackTimer = 2.f;
						}
					}
				}
				break;

				case EntityType::Bullet:
				{
					Bullet& bullet = *(Bullet*)e;

					bullet.body->SetLinearVelocity(b2Vec2(bullet.Direction.x * bullet.Speed, bullet.Direction.y * bullet.Speed));
					//Eyeball Bullet Behavior
					if (bullet.BulletType == BulletType::RedCircle)
					{
						if (bullet.Contacts > 0 || glm::distance(bullet.ShotPos, bullet.Position) > 50.f || bullet.PlayerTouch)
						{
							if (bullet.PlayerTouch)
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

								ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/damage_enemy.wav", NULL);
								player.Health -= bullet.Damage;
							}

							//Destroy
							bullet.body->DestroyFixture(bullet.body->GetFixtureList());
							PhysicsWorld->DestroyBody(bullet.body);
							DestroyEntity(i);
						}
					}

					//BFG Bullet Behavior
					if (bullet.BulletType == BulletType::Blaster)
					{
						if (bullet.Contacts != 0 || glm::distance(bullet.ShotPos, bullet.Position) > 50.f || bullet.ShotEnemy)
						{
							if (bullet.ShotEnemy)
							{
								ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/damage_enemy.wav", NULL);
								GameEntity* shotEnt = (GameEntity*)Entities[bullet.EnemyID];
								shotEnt->Health -= bullet.Damage;
							}

							//Destroy
							bullet.body->DestroyFixture(bullet.body->GetFixtureList());
							PhysicsWorld->DestroyBody(bullet.body);
							DestroyEntity(i);
						}
					}
					
					//Shotgun Bullet Behavior
					if (bullet.BulletType == BulletType::Shotgun)
					{
						if (bullet.Contacts != 0 || glm::distance(bullet.ShotPos, bullet.Position) > 5.f || bullet.ShotEnemy)
						{
							if (bullet.ShotEnemy)
							{
								GameEntity* shotEnt = (GameEntity*)Entities[bullet.EnemyID];
								shotEnt->Health -= bullet.Damage;
							}

							if (bullet.Contacts != 0)
							{
								Explode(bullet.Position, 10.f, 130.f);
							}

							//Destroy
							bullet.body->DestroyFixture(bullet.body->GetFixtureList());
							PhysicsWorld->DestroyBody(bullet.body);
							DestroyEntity(i);
						}
					}
				}
				break;
				default:
					break;
				}
			}
		}

		glm::vec2 RandomOnHemisphere(const glm::vec2& normal, const glm::vec2& dir)
		{
			return dir * glm::sign(dot(normal, dir));
		}

#define PHYSICS_MODE_SEMI_FIXED 0
#define PHYSICS_MODE_CONSTANT 1
#define PHYSICS_MODE 0
		void Update()
		{
			UpdateAI();

			const int32_t velocityIterations = 8;
			const int32_t positionIterations = 4;

#if PHYSICS_MODE == PHYSICS_MODE_CONSTANT
			AccumulatedTime += Globals.deltaTime;
			while (AccumulatedTime >= SimulationTime)
			{
				PhysicsWorld->Step(SimulationTime, velocityIterations, positionIterations);
				AccumulatedTime -= SimulationTime;
			}
#elif PHYSICS_MODE == PHYSICS_MODE_SEMI_FIXED
			float frameTime = Globals.deltaTime;

			while (frameTime > 0.f)
			{
				float deltaTime = glm::min(frameTime, Globals.deltaTime);
				PhysicsWorld->Step(deltaTime, velocityIterations, positionIterations);
				frameTime -= deltaTime;
			}
#endif

			for (auto& entity : Entities)
				entity->UpdatePosition();
		}

		void UpdateGame()
		{
			Update();

			m_TargetPosition = player.Position;
			camera.Position += glm::vec3((m_TargetPosition - glm::vec2(camera.Position)) * 11.5f * Globals.deltaTime, 0.f);


			if (player.SwordCD > 0.f) player.SwordCD -= Globals.deltaTime;
			if (player.AttackCD > 0.f) player.AttackCD -= Globals.deltaTime;
			if (player.DashCD > 0.f) player.DashCD -= Globals.deltaTime;

			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && player.AttackCD <= 0)
			{
				if (player.weapon)
				{
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_real_distribution<float> dis(-0.08f, 0.12f);

					auto result = ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/gun.wav", NULL);

					if (result != MA_SUCCESS)
						WC_CORE_ERROR("sound play fail {}", result);

					glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);
					SpawnBullet(player.Position + dir * 0.75f, RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))), 25.f, 3.f, { 0.25f, 0.25f }, BulletType::Blaster);
					player.AttackCD = 0.3f;
				}
				else
				{
					ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/shotgun.wav", NULL);
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_real_distribution<float> dis(-0.35f, 0.35f);

					glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);
					for (uint32_t i = 0; i < 9; i++)
					{
						SpawnBullet(player.Position + dir * 0.45f, RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))), 25.f, 1.75f, { 0.1f, 0.1f }, BulletType::Shotgun);

						m_Particle.Position = player.Position + dir * 0.55f;
						auto& vel = player.body->GetLinearVelocity();
						m_Particle.ColorBegin = glm::vec4{ 254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f } *2.f;
						m_Particle.ColorEnd = { 254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f };
						m_Particle.Velocity = glm::vec2(vel.x, vel.y) * 0.45f;
						m_Particle.VelocityVariation = glm::normalize(player.Position + RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))) * 0.85f - player.Position) * 5.f;
						m_ParticleEmitter.Emit(m_Particle, 5);
					}
					player.AttackCD = 1.1f;
				}
			}

			if (player.Dash && player.DashCD <= 0)
			{
				ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/dash.wav", NULL);
				if (player.body->GetLinearVelocity().x > 0.7f) 
				{
					player.body->ApplyLinearImpulseToCenter({ 5000.f, 0.f }, true);
					player.Dash = false;
				}
				else if (player.body->GetLinearVelocity().x < -0.7f) 
				{
					player.body->ApplyLinearImpulseToCenter({ -5000.f, 0.f }, true);
					player.Dash = false;
				}
				player.DashCD = 2.f;
			}

			if (player.SwordAttack)
			{
				if (player.SwordCD <= 0.f)
				{
					//play sound
					ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/sword_swing.wav", NULL);
					m_RotateSword = true;
					player.SwordCD = 2.5f;
				}
				player.SwordAttack = false;
			}
			m_ParticleEmitter.OnUpdate();

			if (!player.Alive()) Globals.gameState = GameState::DEATH;
			if (EnemyCount == 0) Globals.gameState = GameState::WIN;
		}

		void RenderGame()
		{
			for (uint32_t x = 0; x < Size.x; x++)
				for (uint32_t y = 0; y < Size.y; y++)
				{
					TileID tileID = GetTile({ x,y, 0 });
					if (tileID != 0) m_RenderData.DrawQuad({ x, y , 0.f }, { 1.f, 1.f }, 0, glm::vec4(0.27f, 0.94f, 0.98f, 1.f));
				}

			//m_RenderData.DrawQuadSvg(glm::translate(glm::mat4(1.f), glm::vec3(1.f, 1.f, 0.f)), svg.textureID);
			for (int i = 0; i < Entities.size(); i++)
			{
				GameEntity& entity = *(GameEntity*)(Entities[i]);

				if (entity.Type == EntityType::Bullet)
				{
					Bullet& bullet = *(Bullet*)(Entities[i]);
					glm::vec4 bulletColor = glm::vec4(0.f);
					if (bullet.BulletType == BulletType::Blaster) bulletColor = glm::vec4(0.f, 1.f, 0.f, 1.f);
					if (bullet.BulletType == BulletType::RedCircle) bulletColor = glm::vec4(1.f, 0.f, 0.f, 1.f);
					if (bullet.BulletType == BulletType::Shotgun) bulletColor = glm::vec4(1.f, 1.f, 0.f, 1.f);
					m_RenderData.DrawCircle(glm::vec3(entity.Position, 0.f), entity.Size.x, 1.f, 0.05f, bulletColor * 1.3f);
					if (bullet.BulletType == BulletType::Blaster)
						if (bullet.Contacts != 0 || bullet.ShotEnemy)
						{
							m_Particle.LifeTime = 0.35f;
							m_Particle.ColorBegin = bulletColor * 2.f;
							m_Particle.ColorEnd = bulletColor;
							m_Particle.Position = entity.Position;
							m_Particle.Velocity = glm::vec2(0.5f);
							m_Particle.VelocityVariation = glm::normalize(entity.Position - player.Position) * 2.5f;
							m_ParticleEmitter.Emit(m_Particle, 6);
						}
				}
				else
				{
					//auto& fontsize = ImGui::CalcTextSize(std::format("HP: {}", entity.Health).c_str());
					//auto tra = glm::translate(glm::mat4(1.f), glm::vec3(entity.Position, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3{ , 0.5f} *6.f);
					//m_RenderData.DrawLineQuad(tra, glm::vec4(0.f, 1.f, 0.f, 1.f));
					m_RenderData.DrawString(std::format("HP: {}", entity.Health), font, entity.Position + glm::vec2(-0.5f, 1.f),
						entity.Type == EntityType::RedCube ? glm::vec4(1.f, 0, 0, 1.f) : glm::vec4(0.5f, 0.5f, 0.5f, 1.f));

					m_RenderData.DrawQuad(glm::vec3(entity.Position, 0.f), entity.Size * 2.f, 0, entity.Type == EntityType::RedCube ? glm::vec4(1.f, 0, 0, 1.f) : glm::vec4(0.27f, 0.94f, 0.98f, 1.f));
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
						glm::vec3{ 0.14f, 1.f, 0.5f } *6.f);

				m_RenderData.DrawQuad(transform, SwordTexture);
			}
			else
			{
				glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);
				glm::vec2 offset = glm::vec2(0.25f, -0.15f);
				float angle = atan2(dir.y, dir.x);
				if (dir.x < 0.f)
				{
					angle = glm::pi<float>() - angle;
					offset.x *= -1.f;
				}
				glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(player.Position, 0.f)) *
					glm::rotate(glm::mat4(1.f), angle, { 0.f, 0.f, dir.x < 0.f ? -1.f : 1.f }) *
					glm::translate(glm::mat4(1.f), glm::vec3(offset, 0.f)) * glm::scale(glm::mat4(1.f),
						{ (dir.x < 0.f ? -1.f : 1.f) * 1.f, 0.45f, 1.f });

				uint32_t tex = player.weapon ? PlasmaGunTexture : SawedOffTexture;

				m_RenderData.DrawQuad(transform, tex);
			}

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
		uint32_t PlasmaGunTexture = 0;
		uint32_t SawedOffTexture = 0;
		uint32_t SwordTexture = 0;
		Font font;

		bool m_RotateSword = false;
		float m_SwordRotation = 0.f;
		float AccumulatedTime = 0.f;
	private:
		TileID* m_Data = nullptr;
		uint32_t m_Size = 1;

		glm::vec2 m_TargetPosition;

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