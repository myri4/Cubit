#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <wc/vk/SyncContext.h>

#include <wc/Utils/Time.h>

#include <wc/Math/Camera.h>
#include <wc/Utils/List.h>
#include <wc/Utils/Window.h>
#include <wc/Utils/YAML.h>

// Physics
#include <box2d/box2d.h>

// GUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include "../Globals.h"
#include "../Rendering/Renderer2D.h"

#include "Map.h"

namespace wc
{
	struct GameInstance
	{
	protected:
		// Game objects
		OrthographicCamera camera;
		Map m_Map;

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
				
				b2Vec2 bNormal = contact->GetManifold()->localNormal;
				glm::vec2 normal = { bNormal.x, bNormal.y };
				
				if (entityA && entityA->Type > EntityType::Entity)
				{
					if (fixtureB->GetType() == b2Shape::e_chain)
					{
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
				glm::vec2 normal = { bNormal.x, bNormal.y };
				
				if (entityA && entityA->Type > EntityType::Entity)
				{
					if (fixtureB->GetType() == b2Shape::e_chain)
					{
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
						if (normal == glm::vec2(0.f, -1.f)) entityB->UpContacts--;
						if (normal == glm::vec2(0.f, 1.f)) entityB->DownContacts--;
						if (normal == glm::vec2(1.f, 0.f)) entityB->LeftContacts--;
						if (normal == glm::vec2(-1.f, 0.f)) entityB->RightContacts--;
					}
				}
			}
		} m_ContactListenerInstance;

		RenderData m_RenderData;

		Renderer2D m_Renderer;
	public:
		float DragStrength = 3.f;
		float AirSpeedFactor = 0.7f;

	public:

		void LoadMap(const std::string& filePath)
		{
			m_Map.Load(filePath);
			m_Tileset.Load(m_Map.TilesetName, m_RenderData);
			m_Map.CreatePhysicsWorld(&m_ContactListenerInstance);
		}

		void Create(glm::vec2 renderSize)
		{
			m_RenderData.Create();
			m_Renderer.Init(camera);

			//LoadAssets();

			LoadMap("levels/level1.malen");

			m_Renderer.CreateScreen(renderSize, m_RenderData);
		}


		void InputGame()
		{
			glm::vec2 moveDir;
			Player& player = m_Map.player;

			bool keyPressed = false;

			if (Key::GetKey(Key::A)) { moveDir.x = -1.f; keyPressed = true; }
			else if (Key::GetKey(Key::D)) { moveDir.x = 1.f;  keyPressed = true; }

			if (ImGui::IsKeyPressed(ImGuiKey_Space))
			{
				if (player.DownContacts == 0)
				{
					if (player.LeftContacts && !player.RightContacts)
					{
						//player.body->SetLinearVelocity(b2Vec2(0, 0));
						glm::vec2 f = glm::vec2(0.5f, 0.5f) * player.JumpForce * 2.f;
						player.body->ApplyLinearImpulseToCenter({ f.x,f.y }, true); //wall on left, jump to right
					}
					if (!player.LeftContacts && player.RightContacts)
					{
						//player.body->SetLinearVelocity(b2Vec2(0, 0));
						glm::vec2 f = glm::vec2(-0.5f, 0.5f) * player.JumpForce * 2.f;
						player.body->ApplyLinearImpulseToCenter({ f.x,f.y }, true); //wall on left, jump to right
					}
					if (!player.LeftContacts && !player.RightContacts)
					{
						player.body->ApplyLinearImpulseToCenter({ 0.f, player.JumpForce }, true);//air jump
					}
				}
				else
					player.body->ApplyLinearImpulseToCenter({ 0.f, player.JumpForce }, true); //normal jump
			}

			if (moveDir != glm::vec2(0.f))
			{
				moveDir = glm::normalize(moveDir) * player.Speed * (player.DownContacts > 0 ? 1.f : AirSpeedFactor);
				player.body->ApplyLinearImpulseToCenter({ moveDir.x, moveDir.y }, true);
			}

			if (player.DownContacts > 0)
			{
				auto vel = player.body->GetLinearVelocity();

				vel.x -= DragStrength * vel.x * Globals.deltaTime;

				if (abs(vel.x) < 0.01f) vel.x = 0.f;

				player.body->SetLinearVelocity(vel);
			}
		}

		void UpdateGame(glm::vec2 cursorPos)
		{
			m_Map.Update();

			Player& player = m_Map.player;
			camera.Position = glm::vec3(player.Position, camera.Position.z);


			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{

			}
		}

		void RenderGame()
		{
			for (uint32_t x = 0; x < m_Map.Size.x; x++)
				for (uint32_t y = 0; y < m_Map.Size.y; y++)
				{
					TileID tileID = m_Map.GetTile({ x,y, 0 });
					if (tileID != 0) m_RenderData.DrawQuad({ x, y , 0.f }, { 1.f, 1.f }, m_Tileset.Tiles[tileID].TextureID);
				}

			for (int i = 0; i < m_Map.Entities.size(); i++)
			{
				GameEntity& entity = *reinterpret_cast<GameEntity*>(m_Map.Entities[i]);

				m_RenderData.DrawQuad(glm::vec3(entity.Position, 0.f), entity.Size * 2.f, entity.TextureID, entity.Alive() ?
					glm::vec4(1.f) : glm::vec4(0.5f, 0.5f, 0.5f, 1.f));
			}

			m_Renderer.Flush(m_RenderData);
			m_RenderData.Reset();
		}

		void Update()
		{
			UpdateGame(glm::vec2(0.f));
			RenderGame();
		}

		void UI()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("Screen Render", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::Image(m_Renderer.GetRenderImageID(), ImVec2((float)Globals.window.GetSize().x, (float)Globals.window.GetSize().y));
			ImGui::End();
			ImGui::PopStyleVar(3);
		}

		void Resize(glm::vec2 size)
		{
			m_Renderer.Resize(size, m_RenderData);
		}

		void DestroyGame()
		{
			m_Renderer.Deinit();
			m_RenderData.Destroy();
			m_Map.Free();
		}
	};
}