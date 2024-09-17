#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../Globals.h"
#include "Entities.h"

namespace wc
{
	enum class EventType : uint8_t { UNDEFINED, SpawnPlayer, DestroyPlayer, SpawnBullet, DestroyBullet };

	struct ServerEvent
	{
		EventType type = EventType::UNDEFINED; // Corrected initialization
		Entity* entity = nullptr;
		float serverTime = 0.f;

		int ID = 0.f;

		bool operator==(const ServerEvent& other) const
		{
			return ID == other.ID;
		}

		// add more data if needed

	};

	void AddEvent(std::vector<ServerEvent>& vector, EventType type, Entity* entity, float time)
	{
		ServerEvent event;
		event.type = type;
		event.entity = entity;
		event.serverTime = time;
		event.ID = (int)vector.size();
		//event.serverTime = Globals.ServerTime; not working
		vector.push_back(event);
	}

	//can use ClearEventat(vector, event.ID) to remove event
	void ClearEventAt(std::vector<ServerEvent>& vector, int id)
	{
		if (id <= vector.size())vector.erase(vector.begin() + id);
	}

	void ClearAllEvents(std::vector<ServerEvent>& vector)
	{
		vector.clear();
	}

	//void SpawnPlayer(std::vector<ServerEvent>& vector, Entity* player)
	//{
	//	ServerEvent event;
	//	event.type = EventType::SpawnPlayer;
	//	event.serverTime = Globals.ServerTime;
	//	vector.push_back(event);
	//}
	//
	//void DestroyPlayer(std::vector<ServerEvent>& vector, Entity* player)
	//{
	//	ServerEvent event;
	//	event.type = EventType::DestroyPlayer;
	//	event.serverTime = Globals.ServerTime;
	//	vector.push_back(event);
	//}
	//
	//void SpawnBullet(std::vector<ServerEvent>& vector, Entity* bullet)
	//{
	//	ServerEvent event;
	//	event.type = EventType::SpawnBullet;
	//	event.serverTime = Globals.ServerTime;
	//	vector.push_back(event);
	//}
	//
	//void DestroyBullet(std::vector<ServerEvent>& vector, Entity* bullet)
	//{
	//	ServerEvent event;
	//	event.type = EventType::DestroyBullet;
	//	event.serverTime = Globals.ServerTime;
	//	vector.push_back(event);
	//}
	//
	//void LoadedMap(std::vector<ServerEvent>& vector)
	//{
	//	ServerEvent event;
	//	event.type = EventType::LoadedMap;
	//	event.serverTime = Globals.ServerTime;
	//	vector.push_back(event);
	//}
}