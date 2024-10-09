#pragma once

#include <box2d/box2d.h>

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

#include <flecs.h>

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
			if (!IsLoaded()) return;

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

		auto CreateEntity() { return m_Entities.entity(); }
		auto CreateEntity(const std::string& name) { return m_Entities.entity(name.c_str()); }
		auto LookupEntity(const std::string& name) { return m_Entities.lookup(name.c_str()); }


		void DestroyEntity(uint32_t i)
		{
			auto e = Entities[i];
			e->DestroyBody();

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
			//stopping sword animation

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
							player.Weapons[(int)WeaponType::Laser].Ammo = 10;
							player.Weapons[(int)WeaponType::Shotgun].Ammo = 12;
							player.Weapons[(int)WeaponType::Revolver].Ammo = 24;
							player.Weapon = player.PrimaryWeapon;

							for (int i = 0; i < magic_enum::enum_count<WeaponType>(); i++) player.Weapons[i].Magazine = WeaponStats[i].MaxMag;

							player.LoadMapBase(metaData);
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
		}

		float Gravity = -9.8f;
		void CreatePhysicsWorld()
		{
			b2WorldDef worldDef = b2DefaultWorldDef();
			worldDef.gravity = { 0.f, Gravity };

			PhysicsWorld.Create(worldDef);

			for (uint32_t i = 0; i < Entities.size(); i++)
			{
				Entity* e = (Entity*)Entities[i];
				e->CreateBody(PhysicsWorld);
			}

			auto addFace = [&](glm::vec2 p1, glm::vec2 p2)
				{
					if ((p1.x == p2.x && p2.x == 0.f) && p1.y == p2.y && p2.y == 0.f) return;

					b2BodyDef bodyDef = b2DefaultBodyDef();
					bodyDef.type = b2_staticBody;
					b2Body body(PhysicsWorld, bodyDef);

					b2ShapeDef shapeDef = b2DefaultShapeDef();
					shapeDef.friction = 0.8f;

					b2Segment segment = { {p1.x, p1.y}, {p2.x, p2.y} };

					b2CreateSegmentShape(body, &shapeDef, &segment);
				};

			const float offset = 0.05f;

			// Top/down
			for (int y = 0; y < (int)Size.y; y++)
			{
				glm::vec2 TopStartPos = { 0.f, 0.f };
				glm::vec2 TopEndPos = { 0.f, 0.f };
				bool creatingTopLine = false;

				glm::vec2 BotStartPos = { 0.f, 0.f };
				glm::vec2 BotEndPos = { 0.f, 0.f };
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
							addFace({ TopStartPos.x + offset, TopStartPos.y }, { TopEndPos.x - offset, TopEndPos.y });
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
							addFace({ BotStartPos.x + offset, BotStartPos.y }, { BotEndPos.x - offset, BotEndPos.y });
						}
					}
					else
					{
						if (creatingTopLine)
						{
							addFace({ TopStartPos.x + offset, TopStartPos.y }, { TopEndPos.x - offset, TopEndPos.y });
							creatingTopLine = false;
						}

						if (creatingBotLine)
						{
							addFace({ BotStartPos.x + offset, BotStartPos.y }, { BotEndPos.x - offset, BotEndPos.y });
							creatingBotLine = false;
						}
					}
				}

				// Close any open top or bottom lines at the end of the row
				if (creatingTopLine)
				{
					addFace({ TopStartPos.x + offset, TopStartPos.y }, { TopEndPos.x - offset, TopEndPos.y });
					creatingTopLine = false;
				}

				if (creatingBotLine)
				{
					addFace({ BotStartPos.x + offset, BotStartPos.y }, { BotEndPos.x - offset, BotEndPos.y });
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
							addFace({ LeftStartPos.x, LeftStartPos.y + offset }, { LeftEndPos.x, LeftEndPos.y - offset });
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
							addFace({ RightStartPos.x, RightStartPos.y + offset }, { RightEndPos.x, RightEndPos.y - offset });
						}
					}
					else
					{
						if (creatingLeftLine)
						{
							addFace({ LeftStartPos.x, LeftStartPos.y + offset }, { LeftEndPos.x, LeftEndPos.y - offset });
							creatingLeftLine = false;
						}

						if (creatingRightLine)
						{
							addFace({ RightStartPos.x, RightStartPos.y + offset }, { RightEndPos.x, RightEndPos.y - offset });
							creatingRightLine = false;
						}
					}
				}

				// Close any open left or right lines at the end of the column
				if (creatingLeftLine)
				{
					addFace({ LeftStartPos.x, LeftStartPos.y + offset }, { LeftEndPos.x, LeftEndPos.y - offset });
					creatingLeftLine = false;
				}

				if (creatingRightLine)
				{
					addFace({ RightStartPos.x, RightStartPos.y + offset }, { RightEndPos.x, RightEndPos.y - offset });
					creatingRightLine = false;
				}
			}
		}

		HitInfo IntersectMap(const Ray& ray, float maxDist = 0.f)
		{
			HitInfo hitInfo;

			float fMaxDistance = maxDist <= 0.f ? glm::length(glm::vec2(Size)) : maxDist;

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
					hitInfo.N = glm::vec2(vMapLastCheck - vMapCheck);
					bTileFound = true;
				}
				vMapLastCheck = vMapCheck;
			}

			hitInfo.t = fDistance;
			hitInfo.Point = ray.Origin + hitInfo.t * ray.Direction;

			return hitInfo;
		}

		HitInfo IntersectEntities(const Ray& ray, uint32_t startIndex = 0, EntityType ignoreType = EntityType::UNDEFINED)
		{
			HitInfo hitInfo;
			float t = FLT_MAX;

			for (uint32_t i = startIndex; i < Entities.size(); i++)
			{
				auto& entity = *Entities[i];
				if (entity.Type == ignoreType) continue;

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

			hitInfo.t = t;
			hitInfo.Point = ray.Origin + hitInfo.t * ray.Direction;

			return hitInfo;
		}

		HitInfo Intersect(const Ray& ray, float maxDist = 0.f, uint32_t startIndex = 0, EntityType ignoreType = EntityType::UNDEFINED)
		{
			auto hitInfo = IntersectMap(ray, 0.f);

			auto entityInfo = IntersectEntities(ray, startIndex, ignoreType);

			if (entityInfo.t < hitInfo.t) hitInfo = entityInfo;

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
					if (e.Type != EntityType::Bullet)e.Body.ApplyLinearImpulse(glm::normalize(e.Position - position) * blastPower * invDistance * invDistance, position, true);
				}
			}
		}

		void SpawnBullet(glm::vec2 position, glm::vec2 direction, WeaponType weaponType, Entity* src)
		{
			auto& weaponInfo = WeaponStats[(int)weaponType];
			Bullet* bullet = new Bullet();
			bullet->SourceEntity = src;
			bullet->Position = position;
			bullet->PrevPos = bullet->Position;
			bullet->Size = weaponInfo.BulletSize;
			//bullet->Speed = weaponInfo.BulletSpeed;
			bullet->Direction = direction;
			bullet->Color = weaponInfo.BulletColor;
			bullet->BulletType = weaponInfo.BulletType;
			bullet->WeaponType = weaponType;
			//bullet->Density = 0.f;
			//bullet->LinearDamping = 0.f;
			bullet->Bounces = weaponInfo.BulletBounces;

			{
				b2BodyDef bodyDef;
				bodyDef.type = b2_dynamicBody;
				bodyDef.position = { bullet->Position.x, bullet->Position.y };
				bodyDef.fixedRotation = true;
				bodyDef.isBullet = true;
				bodyDef.isEnabled = true;
				bodyDef.enableSleep = false;
				bodyDef.gravityScale = 0.f;
				bodyDef.linearVelocity = { bullet->Direction.x * weaponInfo.BulletSpeed, bullet->Direction.y * weaponInfo.BulletSpeed };
				bullet->Body.Create(PhysicsWorld, bodyDef);

				b2Circle shape;
				shape.center = { 0.f, 0.f };
				shape.radius = bullet->Size.x;

				b2ShapeDef shapeDef = b2DefaultShapeDef();
				shapeDef.density = 0.01f;
				shapeDef.friction = 0.f;
				shapeDef.isSensor = weaponInfo.IsBulletSensor;
				shapeDef.userData = (void*)bullet;

				bullet->ShapeID.CreateCircleShape(bullet->Body, shapeDef, shape);

				auto massData = bullet->Body.GetMassData();
				massData.mass = 0.01f;
				bullet->Body.SetMassData(massData);
			}
			Entities.emplace_back(bullet);
		}

		void DestroyPhysicsWorld()
		{
			PhysicsWorld.Destroy();
		}

		void UpdateAI()
		{
#if 0
			for (uint32_t i = 1; i < Entities.size(); i++)
			{
				auto& e = Entities[i];

				e->ID = i;
				switch (e->Type)
				{
				case EntityType::Bullet:
				{

				}
				break;
				default:
					break;
				}
			}
#endif
		}

		void UpdateAIFixed()
		{
			for (uint32_t i = 0; i < Entities.size(); i++)
			{
				auto& e = Entities[i];
				
				switch (e->Type)
				{
				case EntityType::Bullet:
				{
					Bullet& bullet = *(Bullet*)e;
					WeaponInfo& weapon = WeaponStats[(int)bullet.WeaponType];
					bullet.DistanceTraveled += weapon.BulletSpeed * SimulationTime;
					//WC_CORE_INFO(bullet.DistanceTraveled);
					bool destroy = false;
					if (bullet.HitAnyEntity())
					{
						Entity* shotEnt = bullet.HitEntity;

						if (bullet.DidHitEntity())
						{
							bool hit = shotEnt != bullet.SourceEntity; // && shotEnt->Type != bullet.SourceEntity->Type

							if (hit)
							{
								shotEnt->TakeDamage(weapon.Damage);

								if (bullet.BulletType == BulletType::Blaster)
								{
									//ma_engine_play_sound(&Globals.sfx_engine, Globals.damageEnemy.c_str(), nullptr);

									m_Particle.LifeTime = 0.35f;
									m_Particle.SizeBegin = bullet.Size.x * 2.f;
									m_Particle.ColorBegin = glm::vec4(0.f, 1.f, 0.f, 1.f) * 2.f;
									m_Particle.ColorEnd = glm::vec4(0.f, 1.f, 0.f, 1.f);
									m_Particle.Position = bullet.Position;
									m_Particle.Velocity = glm::vec2(0.5f);
									m_Particle.VelocityVariation = glm::normalize(shotEnt->Position - player.Position) * 2.5f;
									m_ParticleEmitter.Emit(m_Particle, 6);
								}
								else if (bullet.BulletType == BulletType::RedCircle) // && bullet.HitEntityType == EntityType::Player
								{
									std::random_device rd;
									std::mt19937 gen(rd());
									std::uniform_int_distribution<> dis(0, 1); // Define the range

									//if (dis(gen) == 0 && false) // stopped duplicating on hit
									//{
									//	RedCube* em = new RedCube();
									//	em->Position = bullet.Position + glm::vec2(0.f, 1.f);
									//	em->CreateBody(PhysicsWorld);
									//
									//	m_SummonParticle.Position = em->Position;
									//	m_SummonParticle.Velocity = glm::vec2(0.5f);
									//	for (int i = 0; i < 6; i++)
									//	{
									//		m_SummonParticle.VelocityVariation = glm::normalize(glm::vec3(RandomValue(), RandomValue(), RandomValue()));
									//
									//		m_ParticleEmitter.Emit(m_SummonParticle);
									//	}
									//	EnemyCount++;
									//	Entities.emplace_back(em);
									//}

									ma_engine_play_sound(&Globals.sfx_engine, Globals.damageEnemy.c_str(), nullptr);
								}
							}							


							//destroy itself if collides with source entity after one bounce
							if (bullet.HitEntity == bullet.SourceEntity && bullet.Bounces != WeaponStats[(int)bullet.WeaponType].BulletBounces) {
								destroy = true;
							}
						}
						
						// destroy both bullets if they are from different sources and both are physical
						if (!bullet.IsSensor() && bullet.HitBullet()) 
						{
							Bullet& hitBullet = *(Bullet*)bullet.HitEntity;
							if (!hitBullet.IsSensor() && bullet.SourceEntity != hitBullet.SourceEntity)
							{
								//animation from both physical bullets destroying each other
								m_Particle.LifeTime = 0.45f;
								m_Particle.ColorBegin = bullet.Color;
								m_Particle.ColorEnd = hitBullet.Color;
								m_Particle.Position = bullet.Position;
								m_Particle.Velocity = glm::vec2(0.0f);
								m_Particle.VelocityVariation = glm::vec2(0.0f);
								m_ParticleEmitter.Emit(m_Particle, 6);
								destroy = true;
							}
						}
						else if (shotEnt != bullet.SourceEntity && bullet.HitEntity->Type != EntityType::Bullet) 
						{
							destroy = true;
						}						
					}
					else 
					{
						if (bullet.HitEntityType == EntityType::Tile && bullet.Bounces == 0) destroy = true;
						//if (bullet.BulletType == BulletType::Slash && bullet.DistanceTraveled > 0.0001f) destroy = true;
					}

					if (bullet.DistanceTraveled > weapon.Range) destroy = true;
					if (bullet.Position.x <= 0.f || bullet.Position.x >= Size.x + 1.f ||
						bullet.Position.y <= 0.f || bullet.Position.y >= Size.y + 1.f) destroy = true;
					
					if (destroy) DestroyEntity(i);
					
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
				player.Body.ApplyLinearImpulseToCenter({ player.MoveDir * EntityStats[(int)EntityType::Player].Speed * player.Body.GetMass() / 10.f * (player.DownContacts > 0 ? 1.f : AirSpeedFactor), 0.f }, true);

			UpdateAIFixed();
		}

		void Update()
		{
			AccumulatedTime += Globals.deltaTime;

			UpdateAI();

			while (AccumulatedTime >= SimulationTime)
			{
				for (auto& entity : Entities)
					entity->PrevPos = entity->Body.GetPosition();

				FixedUpdate();

				PhysicsWorld.Step(SimulationTime, 4);
				//PhysicsWorld.SetHitEventThreshold(10.f);
				
				//Physical bullets go to contact events and sensor bullets go to sensor events

				// Sensor Events
				b2SensorEvents sensorEvents = PhysicsWorld.GetSensorEvents();

				for (uint32_t i = 0; i < sensorEvents.beginCount; i++)
				{
					auto& contact = sensorEvents.beginEvents[i];

					Entity* entityS = static_cast<Entity*>(b2Shape_GetUserData(contact.sensorShapeId));  // sensor
					Entity* entityV = static_cast<Entity*>(b2Shape_GetUserData(contact.visitorShapeId)); // visitor
					
					// Sensor Bullets Begin Touch
					if (entityS && entityV) {
						BulletHit(entityS, entityV);
					}

					//sensor bullets dont reflect off walls SO normal is {0, 0}
					BulletHitTile(entityS, entityV, {0, 0});


				}

				for (uint32_t i = 0; i < sensorEvents.endCount; i++)
				{
					auto& contact = sensorEvents.beginEvents[i];

					Entity* entityS = static_cast<Entity*>(b2Shape_GetUserData(contact.sensorShapeId));  // sensor
					Entity* entityV = static_cast<Entity*>(b2Shape_GetUserData(contact.visitorShapeId)); // visitor

					// Sensor Bullet End Touch
					BulletClear(entityS);
				}


				// Contact Events
				b2ContactEvents contactEvents = PhysicsWorld.GetContactEvents();

				//Begin Touch
				for (uint32_t i = 0; i < contactEvents.beginCount; i++)
				{
					auto& contact = contactEvents.beginEvents[i];

					Entity* entityA = static_cast<Entity*>(b2Shape_GetUserData(contact.shapeIdA));
					Entity* entityB = static_cast<Entity*>(b2Shape_GetUserData(contact.shapeIdB));

					
					// Physical Bullets Begin Contact
					if (entityB && entityB->Type == EntityType::Bullet)
					{
						if (entityA) BulletHit(entityA, entityB);
						
						b2ContactData contactData;
						b2Shape_GetContactData(contact.shapeIdA, &contactData, 1);
						glm::vec2 normal = glm::vec2(contactData.manifold.normal.x, contactData.manifold.normal.y);
						BulletHitTile(entityA, entityB, normal);
						BulletHitTile(entityB, entityA, normal);
					}

					// Player Begin Contacts
					if ((!entityA && entityB && entityB->Type == EntityType::Player) || (!entityB && entityA && entityA->Type == EntityType::Player)) 
					{
						b2Body body;
						b2Segment segment;
						Entity* entity = nullptr;	

						// Determine which is wall and which is entity
						if (!entityA)
						{
							segment = b2Shape_GetSegment(contact.shapeIdA);
							body = b2Shape_GetBody(contact.shapeIdB);
							entity = entityB;
						}
						if (!entityB)
						{
							segment = b2Shape_GetSegment(contact.shapeIdB);
							body = b2Shape_GetBody(contact.shapeIdA);
							entity = entityA;
						}

						// Horizontal
						if (segment.point1.y == segment.point2.y)
						{
							if (segment.point1.y > body.GetPosition().y) entity->UpContacts++;
							else entity->DownContacts++;
						}

						// Vertical
						if (segment.point1.x == segment.point2.x)
						{
							if (segment.point1.x > body.GetPosition().x) entity->RightContacts++;
							else entity->LeftContacts++;
						}
					}
				}

				//End Touch
				for (uint32_t i = 0; i < contactEvents.endCount; i++)
				{
					auto& contact = contactEvents.endEvents[i];

					Entity* entityA = static_cast<Entity*>(b2Shape_GetUserData(contact.shapeIdA));
					Entity* entityB = static_cast<Entity*>(b2Shape_GetUserData(contact.shapeIdB));

					// Physical Bullets End Contact
					if (entityB && entityB->Type == EntityType::Bullet)
					{

					}

					// Player End Contacts
					if ((!entityA && entityB && entityB->Type == EntityType::Player) || (!entityB && entityA && entityA->Type == EntityType::Player))
					{
						b2Body body;
						b2Segment segment;
						Entity* entity = nullptr;

						// Determine which is wall and which is entity
						if (!entityA)
						{
							segment = b2Shape_GetSegment(contact.shapeIdA);
							body = b2Shape_GetBody(contact.shapeIdB);
							entity = entityB;
						}
						if (!entityB)
						{
							segment = b2Shape_GetSegment(contact.shapeIdB);
							body = b2Shape_GetBody(contact.shapeIdA);
							entity = entityA;
						}

						// Horizontal
						if (segment.point1.y == segment.point2.y)
						{
							if (segment.point1.y > body.GetPosition().y) entity->UpContacts--;
							else entity->DownContacts--;
						}

						// Vertical
						if (segment.point1.x == segment.point2.x)
						{
							if (segment.point1.x > body.GetPosition().x) entity->RightContacts--;
							else entity->LeftContacts--;
						}
					}
				}

				for (uint32_t i = 0; i < contactEvents.hitCount; i++)
				{
					b2ContactHitEvent contact = contactEvents.hitEvents[i];
					WC_CORE_INFO("HIT");

				}

				AccumulatedTime -= SimulationTime;
			}			

			float alpha = AccumulatedTime / SimulationTime;

			for (auto& entity : Entities)
			{
				auto b2Pos = entity->Body.GetPosition();
				auto bPos = glm::vec2(b2Pos.x, b2Pos.y);

				entity->Position = glm::mix(entity->PrevPos, bPos, alpha);
			}


			if (player.DashCD > 0.f) player.DashCD -= Globals.deltaTime;

			for (uint32_t i = 0; i < magic_enum::enum_count<WeaponType>(); i++)
			{
				auto& weapon = player.Weapons[i];
				if (weapon.Timer > 0.f) weapon.Timer -= Globals.deltaTime;
				if (weapon.AltFireTimer > 0.f) weapon.AltFireTimer -= Globals.deltaTime;
				if (weapon.ReloadTimer > 0.f) weapon.ReloadTimer -= Globals.deltaTime;
			}
		}

		uint32_t Get1DSize() { return m_Size; }
		TileID* GetData() { return m_Data; }

	public:
		glm::uvec3 Size = glm::uvec3(1);

		std::vector<Entity*> Entities;

		Player player;

		b2World PhysicsWorld;

		float DragStrength = 2.f;
		float AirSpeedFactor = 0.7f;

		
		float AccumulatedTime = 0.f;
	private:
		TileID* m_Data = nullptr;
		uint32_t m_Size = 1;		
		flecs::world m_Entities;

		const float SimulationTime = 1.f / 200.f;

	private: 
		void BulletHit(Entity* entityA, Entity* entityB)
		{
			if (entityA->Type == EntityType::Bullet)
			{
				Bullet& bullet = *(Bullet*)entityA;

				bullet.HitEntityType = entityB->Type;
				bullet.HitEntity = entityB;
				//if (entityB->Type > EntityType::Entity)
				//{
					//entityB->Body->ApplyLinearImpulseToCenter(-bullet.Body->GetLinearVelocity(), true);						
				//}
			}
			if (entityB->Type == EntityType::Bullet)
			{
				Bullet& bullet = *(Bullet*)entityB;

				bullet.HitEntityType = entityA->Type;
				bullet.HitEntity = entityA;
			}
		}

		void BulletHitTile(Entity* entityA, Entity* entityB, glm::vec2 normal)
		{

			if (entityA && entityA->Type == EntityType::Bullet && !entityB)
			{
				Bullet& bullet = *(Bullet*)entityA;
				bullet.HitEntityType = EntityType::Tile;

				if (bullet.Bounces > 0 && !bullet.IsSensor())
				{
					bullet.Direction = glm::reflect(bullet.Direction, normal);
					bullet.Body.SetLinearVelocity(bullet.Direction * WeaponStats[(int)bullet.WeaponType].BulletSpeed);
					bullet.HitEntityType = EntityType::UNDEFINED;
					bullet.Bounces--;
				}
			}
		}

		void BulletClear(Entity* entity)
		{
			if (entity->Type == EntityType::Bullet)
			{
				Bullet& bullet = *(Bullet*)entity;
				bullet.HitEntityType = EntityType::UNDEFINED;
				bullet.HitEntity = nullptr;
			}
		}		

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