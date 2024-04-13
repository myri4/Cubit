#pragma once

#include "../include/wc/Utils/YAML.h"
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Entities.h"
//#include "Raycasting.h"
#include "Tile.h"
#include <magic_enum.hpp>

// @TODO: Separate tilemap and scene
namespace wc
{
	int to1D(const glm::uvec3& pos, const glm::uvec3& size)
	{
		return (pos.z * size.x * size.y) + (pos.y * size.x) + pos.x;
	}

	Tileset m_Tileset;

	struct Map
	{
		Map()
		{
			Entities.push_back(&player);
			player.Size = glm::vec2(1.15f, 2.03f) * 0.5f;
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
				player.Size = glm::vec2(1.15f, 2.f) * 0.5f;
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

				if (mapMetaData["Gravity"]) Gravity = mapMetaData["Gravity"].as<glm::vec2>();
				if (mapMetaData["Tileset"])
				{
					TilesetName = mapMetaData["Tileset"].as<std::string>();
				}

				{
					auto objects = mapMetaData["Entities"];
					for (int i = 0; i < objects.size(); i++)
					{
						auto metaData = objects[i];
						EntityType Type = EntityType::UNDEFINED;
						Type = magic_enum::enum_cast<EntityType>(metaData["Type"].as<std::string>()).value();

						if (Type == EntityType::Player) Entities[0]->LoadMapBase(metaData);
					}
				}
			}
			else WC_CORE_ERROR("Could not find metadata file for {}", filepath);
		}

		void CreatePhysicsWorld(b2ContactListener* contactListener)
		{
			PhysicsWorld = new b2World({ Gravity.x, Gravity.y });
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

		void DestroyPhysicsWorld()
		{
			delete PhysicsWorld;
			PhysicsWorld = nullptr;
		}

		void UpdateAI()
		{
			for (auto& e : Entities)
			{
				switch (e->Type)
				{
				//case EntityType::TornadoEnemy:
				//{
				//	TornadoEnemy& entity = *reinterpret_cast<TornadoEnemy*>(e);
				//	entity.body->ApplyForceToCenter({ 500.f, 0.f }, true);
				//}
				break;
				default:
					break;
				}
			}
		}

		void Update()
		{
			UpdateAI();

			const int32_t velocityIterations = 6;
			const int32_t positionIterations = 2;

			PhysicsWorld->Step(Globals.deltaTime, velocityIterations, positionIterations);

			for (auto& entity : Entities)
				entity->UpdatePosition();
		}

		//HitInfo Intersect(const Ray& ray)
		//{
		//	HitInfo hitInfo;
		//	float t = FLT_MAX;
		//
		//	float fMaxDistance = 36.f;
		//
		//	glm::vec2 vRayLength1D;
		//	auto& vRayStart = ray.Origin;
		//	auto& vRayDir = ray.Direction;
		//	auto vRayUnitStepSize = abs(ray.InvDirection);
		//	glm::ivec2 vMapCheck = glm::floor(vRayStart);
		//	glm::ivec2 vMapLastCheck = vMapCheck;
		//	glm::ivec2 vStep = glm::ivec2(glm::sign(vRayDir));
		//
		//	// Establish Starting Conditions
		//	for (int i = 0; i < 2; i++)
		//		vRayLength1D[i] = ((vRayDir[i] < 0.f ? (vRayStart[i] - float(vMapCheck[i])) : (float(vMapCheck[i]) - vRayStart[i])) + 0.5f) * vRayUnitStepSize[i];
		//
		//	bool bTileFound = false;
		//	float fDistance = 0.f;
		//	while (!bTileFound && fDistance < fMaxDistance)
		//	{
		//		// Walk along shortest path
		//		int axis = 0;
		//		for (int i = 0; i < 2; i++)
		//		{
		//			int nextAxis = (i + 1) % 2;
		//			if (vRayLength1D[axis] > vRayLength1D[nextAxis]) axis = nextAxis;
		//		}
		//
		//		vMapCheck[axis] += vStep[axis];
		//		fDistance = vRayLength1D[axis];
		//		vRayLength1D[axis] += vRayUnitStepSize[axis];
		//
		//		// Test tile at new test point
		//		TileID tileID = GetTile(glm::uvec3(vMapCheck, 0));
		//		if (tileID > 0)
		//		{
		//			hitInfo.Hit = true;
		//			t = fDistance;
		//			hitInfo.N = glm::vec2(vMapLastCheck - vMapCheck);
		//			bTileFound = true;
		//		}
		//		vMapLastCheck = vMapCheck;
		//	}
		//
		//	hitInfo.Point = ray.Origin + t * ray.Direction;
		//
		//	return hitInfo;
		//}

		uint32_t Get1DSize() { return m_Size; }
		TileID* GetData() { return m_Data; }

	public:
		glm::uvec3 Size = glm::uvec3(1);

		std::vector<GameEntity*> Entities;

		glm::vec2 Gravity;

		Player player;

		b2World* PhysicsWorld = nullptr;

		std::string TilesetName;

	private:
		TileID* m_Data = nullptr;
		uint32_t m_Size = 1;

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