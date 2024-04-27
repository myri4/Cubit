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

	ParticleSystem m_ParticleEmitter;
	ParticleProps m_Particle;
	ParticleProps m_SummonParticle;

	struct Map
	{
		Map()
		{
			Entities.push_back(&player);
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
				Entities.push_back(&player);
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

						if (Type == EntityType::Player) Entities[0]->LoadMapBase(metaData);
						else if (Type == EntityType::EyeballEnemy)
						{
							EyeballEnemy* e = new EyeballEnemy();
							e->LoadMapBase(metaData);
							Entities.push_back(e);
							EnemyCount++;
						}
					}
				}
			}
			else WC_CORE_ERROR("Could not find metadata file for {}", filepath);
		}

		void CreatePhysicsWorld(b2ContactListener* contactListener)
		{
			PhysicsWorld = new b2World({ 0.f, -9.8f });
			PhysicsWorld->SetContactListener(contactListener);

			b2Vec2 vs[4];

			for (auto& e : Entities)
			{
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
				e->body->SetLinearDamping(1.8f);
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

		void SpawnBullet(glm::vec2 position, glm::vec2 direction, float speed, float damage, glm::vec2 size, glm::vec4 color, BulletType bulletType)
		{
			Bullet* bullet = new Bullet();
			bullet->Position = position;
			bullet->Size = size;
			bullet->Color = color;
			bullet->Damage = damage;
			bullet->Speed = speed;
			bullet->direction = direction;
			bullet->bulletType = bulletType;
			bullet->CreateBody(PhysicsWorld);
			bullet->body->SetBullet(true);
			bullet->body->GetFixtureList()->SetSensor(true);
			Entities.push_back(bullet);
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
				case EntityType::EyeballEnemy:
				{
					EyeballEnemy& entity = *reinterpret_cast<EyeballEnemy*>(e);
					//timer
					if (entity.attackTimer > 0.f) entity.attackTimer -= Globals.deltaTime;

					if (player.swordAttack && player.swordCD <= 0 && glm::distance(entity.Position, player.Position) < 8.f) 
					{
						glm::vec2 direction = glm::normalize((player.Position - glm::vec2(0, player.Size.y * 0.5f)) - entity.Position);
						entity.body->ApplyLinearImpulseToCenter(-1500.f * b2Vec2(direction.x, direction.y), true);
						entity.Health -= player.SwordDamage;
					}

					if (!entity.Alive()) {

						//Destroy
						entity.body->DestroyFixture(entity.body->GetFixtureList());
						PhysicsWorld->DestroyBody(entity.body);
						EnemyCount--;
						Entities.erase(Entities.begin() + i);
					}


					//movement
					if (glm::distance(player.Position, entity.Position) < entity.detectRange) 
					{
						if (entity.Position.x > player.Position.x)entity.body->ApplyLinearImpulseToCenter(b2Vec2(-entity.Speed, 0), true);
						else entity.body->ApplyLinearImpulseToCenter(b2Vec2(entity.Speed, 0), true);
					}
					//attack behavior
					if (glm::distance(player.Position, entity.Position) < entity.shootRange)
					{
						if (entity.attackTimer <= 0 && entity.Alive())
						{
							//shoot
							Bullet* bullet = new Bullet();
							bullet->Position = entity.Position + glm::vec2(0, 0.5f);
							bullet->Damage = 0.5f;
							bullet->Color = { 1.f, 0.f, 0.f, 1.f };
							bullet->Size = { 0.25f, 0.25f };
							bullet->Speed = 435.f;
							bullet->Density = 75.f;
							bullet->bulletType = BulletType::Eyeball;
							bullet->CreateBody(PhysicsWorld);
							bullet->body->SetBullet(true);
							bullet->body->GetFixtureList()->SetSensor(true);
							Entities.push_back(bullet);

							entity.attackTimer = 2.f;
						}
					}
				}
				break;

				case EntityType::Bullet:
				{
					Bullet& bullet = *reinterpret_cast<Bullet*>(e);

					//Eyeball Bullet Behavior
					if (bullet.bulletType == BulletType::Eyeball)
					{
						if (!bullet.Shot)
						{
							glm::vec2 directionToPlayer = glm::normalize(player.Position + glm::vec2(0.f, 0.5f) - bullet.Position);

							//make it true so it only does it once
							bullet.Shot = true;
							bullet.body->ApplyLinearImpulseToCenter(bullet.Speed * b2Vec2(directionToPlayer.x, directionToPlayer.y), true);
						}

						if (bullet.Contacts > 0 || glm::distance(player.Position, bullet.Position) > 50.f || bullet.playerTouch)
						{
							if (bullet.playerTouch)
							{
								std::random_device rd;
								std::mt19937 gen(rd());
								std::uniform_int_distribution<> dis(0, 1); // Define the range

								if (dis(gen) == 0)
								{
									EyeballEnemy* em = new EyeballEnemy();
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
									Entities.push_back(em);
								}

								ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/damage_enemy.wav", NULL);
								player.Health -= bullet.Damage;
							}

							//Destroy
							bullet.body->DestroyFixture(bullet.body->GetFixtureList());
							PhysicsWorld->DestroyBody(bullet.body);
							Entities.erase(Entities.begin() + i);
						}
					}



					//BFG Bullet Behavior
					if (bullet.bulletType == BulletType::BFG)
					{
						bullet.body->SetLinearVelocity(b2Vec2(bullet.direction.x * bullet.Speed, bullet.direction.y * bullet.Speed));

						if (bullet.Contacts != 0 || glm::distance(player.Position, bullet.Position) > 50.f || bullet.shotEnemy)
						{
							if (bullet.shotEnemy)
							{
								ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/damage_enemy.wav", NULL);
								Entities[bullet.EnemyID]->Health -= bullet.Damage;
							}

							//Destroy
							bullet.body->DestroyFixture(bullet.body->GetFixtureList());
							PhysicsWorld->DestroyBody(bullet.body);
							Entities.erase(Entities.begin() + i);
						}
					}
					
					//Shotgun Bullet Behavior
					if (bullet.bulletType == BulletType::Shotgun)
					{
						bullet.body->SetLinearVelocity(b2Vec2(bullet.direction.x * bullet.Speed, bullet.direction.y * bullet.Speed));

						if (!bullet.Shot)
						{
							bullet.shotPos = bullet.Position;
							bullet.Shot = true;
							//make it true so it only does it once
						}

						if (bullet.Contacts != 0 || glm::distance(bullet.shotPos, bullet.Position) > 5 || bullet.shotEnemy)
						{
							if (bullet.shotEnemy)
							{
								Entities[bullet.EnemyID]->Health -= bullet.Damage;
							}

							if (bullet.Contacts != 0)
							{
								Explode(bullet.Position, 10.f, 130.f);
							}

							//Destroy
							bullet.body->DestroyFixture(bullet.body->GetFixtureList());
							PhysicsWorld->DestroyBody(bullet.body);
							Entities.erase(Entities.begin() + i);
						}
					}
				}
				break;
				default:
					break;
				}
			}
		}


		void Update()
		{
			UpdateAI();


			//AccumulatedTime += Globals.deltaTime;
			//while (AccumulatedTime >= 0.f)
			//{
				const int32_t velocityIterations = 6;
				const int32_t positionIterations = 2;

				PhysicsWorld->Step(/*SimulationTime*/Globals.deltaTime, velocityIterations, positionIterations);
			//	AccumulatedTime -= SimulationTime;
			//}

				for (auto& entity : Entities)
					entity->UpdatePosition();
		}

		uint32_t Get1DSize() { return m_Size; }
		TileID* GetData() { return m_Data; }

	public:
		glm::uvec3 Size = glm::uvec3(1);

		std::vector<GameEntity*> Entities;

		Player player;

		b2World* PhysicsWorld = nullptr;

		uint32_t EnemyCount = 0;

	private:
		TileID* m_Data = nullptr;
		uint32_t m_Size = 1;

		float AccumulatedTime = 0.f;
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