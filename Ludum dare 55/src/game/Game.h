#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

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
		glm::vec2 m_TargetPosition;

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

				if (entityA && entityB)
				{
					bool anyPlayer = entityA->Type == EntityType::Player || entityB->Type == EntityType::Player;

					if (entityA->Type == EntityType::Bullet)
					{
						if (entityB->Type == EntityType::Player)
							entityA->playerTouch = true;
						else if (entityB->Type == EntityType::EyeballEnemy)
						{
							entityA->shotEnemy = true;
							entityA->EnemyID = entityB->ID;
						}
					}

					if (entityB->Type == EntityType::Bullet)
					{
						if (entityA->Type == EntityType::Player)
							entityB->playerTouch = true;
						else if (entityA->Type == EntityType::EyeballEnemy)
						{
							entityB->shotEnemy = true;
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
							entityA->playerTouch = false;
						else if (entityB->Type == EntityType::EyeballEnemy)
						{
							entityA->shotEnemy = false;
							entityA->EnemyID = entityB->ID;
						}

					}

					if (entityB->Type == EntityType::Bullet)
					{
						if (entityA->Type == EntityType::Player)
							entityB->playerTouch = false;
						else if (entityA->Type == EntityType::EyeballEnemy)
						{
							entityB->shotEnemy = false;
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
		} m_ContactListenerInstance;

		RenderData m_RenderData;

		Renderer2D m_Renderer;
	public:
		float DragStrength = 2.f;
		float AirSpeedFactor = 0.7f;
		uint32_t PlasmaGunTexture = 0;
		uint32_t SawedOffTexture = 0;
		uint32_t SwordTexture = 0;

		////sound

		//float sfx_volume = 0.6f;

		//ma_result result;

		//ma_engine sfx_engine;

		bool m_RotateSword = false;
		float m_SwordRotation = 0.f;

	public:

		void LoadMap(const std::string& filePath)
		{
			m_Map.Load(filePath);
			m_Map.CreatePhysicsWorld(&m_ContactListenerInstance);
		}

		void Create(glm::vec2 renderSize)
		{

			//sound
			Globals.result = ma_engine_init(NULL, &Globals.sfx_engine);
			if (Globals.result != MA_SUCCESS) {
				std::cout << "sound engine fail\n";
				std::cout << Globals.result;
			}
			ma_engine_set_volume(&Globals.sfx_engine, Globals.sfx_volume);

			m_RenderData.Create();
			m_Renderer.Init(camera);
			m_Tileset.Load();

			PlasmaGunTexture = m_RenderData.LoadTexture("assets/textures/Plasma_Rifle.png");
			SawedOffTexture = m_RenderData.LoadTexture("assets/textures/Sawed-Off.png");
			SwordTexture = m_RenderData.LoadTexture("assets/textures/Sword.png");

			LoadMap("levels/level2.malen");

			m_Renderer.CreateScreen(renderSize, m_RenderData);
			m_ParticleEmitter.Init();
			m_Particle.ColorBegin = { 254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f };
			m_Particle.ColorEnd = { 254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f };
			m_Particle.SizeBegin = 0.5f, m_Particle.SizeVariation = 0.3f, m_Particle.SizeEnd = 0.0f;
			m_Particle.LifeTime = 0.6f;
			m_Particle.Velocity = { 0.0f, 0.0f };
			m_Particle.VelocityVariation = { 6.f, 6.f };
			m_Particle.Position = { 0.0f, 0.0f };

			m_SummonParticle.ColorBegin = glm::vec4{ 1.f, 0.37f, 0.12f, 1.f } * 2.f;
			m_SummonParticle.ColorEnd = { 1.f, 0.37f, 0.12f, 1.0f };
			m_SummonParticle.SizeBegin = 0.5f, m_SummonParticle.SizeVariation = 0.3f, m_SummonParticle.SizeEnd = 0.0f;
			m_SummonParticle.LifeTime = 0.35f;
			m_SummonParticle.Velocity = { 0.0f, 0.0f };
			m_SummonParticle.VelocityVariation = { 6.f, 6.f };
			m_SummonParticle.Position = { 0.0f, 0.0f };

			//m_SummonParticle.VelocityVariation = glm::normalize(entity.Position - m_Map.player.Position) * 2.5f;
		}

		
		void InputGame()
		{
			glm::vec2 moveDir;
			Player& player = m_Map.player;

			bool keyPressed = false;

			if (Key::GetKey(Key::A)) { moveDir.x = -1.f; keyPressed = true; }
			else if (Key::GetKey(Key::D)) { moveDir.x = 1.f;  keyPressed = true; }

			if (ImGui::IsKeyPressed(ImGuiKey_E)) player.swordAttack = true;
			player.dash = Key::GetKey(Key::LeftShift) == GLFW_PRESS;
			if (ImGui::IsKeyReleased(ImGuiKey_Q)) 
			{
				player.weapon = !player.weapon;
				player.attackCD = 0.2f;
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Space))
			{
				if (player.DownContacts != 0)
				{
					player.body->ApplyLinearImpulseToCenter({ 0.f, player.JumpForce }, true); //normal jump
				}
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

		void UpdateGame()
		{
			m_Map.Update();

			Player& player = m_Map.player;
			m_TargetPosition = player.Position;
			camera.Position += glm::vec3((m_TargetPosition - glm::vec2(camera.Position)) * 11.5f * Globals.deltaTime, 0.f);


			if (player.swordCD > 0.f) player.swordCD -= Globals.deltaTime;
			if (player.attackCD > 0.f) player.attackCD -= Globals.deltaTime;
			if (player.dashCD > 0.f) player.dashCD -= Globals.deltaTime;

			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && player.attackCD <= 0)
			{
				if (player.weapon)
				{
					Globals.result = ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/gun.wav", NULL);
					if (Globals.result != MA_SUCCESS) {
						std::cout << "sound play fail\n";
						std::cout << Globals.result;
					}
					glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - m_Map.player.Position);
					m_Map.SpawnBullet(player.Position + dir * 0.75f, dir, 25.f, 3.f, { 0.25f, 0.25f }, glm::vec4(0, 1.f, 0, 1.f), BulletType::BFG);
					player.attackCD = 0.3f;
				}
				else
				{
					ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/shotgun.wav", NULL);
					std::random_device rd;
					std::mt19937 gen(rd());
					std::uniform_real_distribution<float> dis(-1.75f, 2.25f);

					for (uint32_t i = 0; i < 9; i++)
					{
						glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - m_Map.player.Position + glm::vec2(0.f, dis(gen)));
						m_Map.SpawnBullet(player.Position + dir * 0.45f, dir, 25.f, 1.5f, { 0.15f, 0.15f }, glm::vec4(1.f, 1.f, 0, 1.f), BulletType::Shotgun);

						m_Particle.Position = player.Position + dir * 0.55f;
						auto& vel = player.body->GetLinearVelocity();
						m_Particle.ColorBegin = glm::vec4{ 254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f } * 2.f;
						m_Particle.ColorEnd = { 254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f };
						m_Particle.Velocity = glm::vec2(vel.x, vel.y) * 0.5f;
						m_Particle.VelocityVariation = glm::normalize(player.Position + dir * 0.55f - player.Position) * 5.f;
						for (int i = 0; i < 6; i++)
							m_ParticleEmitter.Emit(m_Particle);
					}
					player.attackCD = 0.8f;
				}
			}

			if (player.dash && player.dashCD <= 0)
			{
				ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/dash.wav", NULL);
				if (player.body->GetLinearVelocity().x > 0.1f) {
					player.body->ApplyLinearImpulseToCenter({ 5000.f, 0.f }, true);
					player.dash = false;
				}
				else if (player.body->GetLinearVelocity().x < -0.1f) {
					player.body->ApplyLinearImpulseToCenter({ -5000.f, 0.f }, true);
					player.dash = false;
				}
				player.dashCD = 2.f;
			}

			if (player.swordAttack)
			{
				if (player.swordCD <= 0.f)
				{
				//play sound
				ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/sword_swing.wav", NULL);
				m_RotateSword = true;
				player.swordCD = 2.5f;
				}
				player.swordAttack = false;
			}
			m_ParticleEmitter.OnUpdate();

			if (!player.Alive()) Globals.gameState = GameState::DEATH;
		}

		void RenderGame()
		{
			for (uint32_t x = 0; x < m_Map.Size.x; x++)
				for (uint32_t y = 0; y < m_Map.Size.y; y++)
				{
					TileID tileID = m_Map.GetTile({ x,y, 0 });
					if (tileID != 0) m_RenderData.DrawQuad({ x, y , 0.f }, { 1.f, 1.f }, 0, glm::vec4(0.27f, 0.94f, 0.98f, 1.f));
				}

			for (int i = 0; i < m_Map.Entities.size(); i++)
			{
				GameEntity& entity = *reinterpret_cast<GameEntity*>(m_Map.Entities[i]);

				if (entity.Type == EntityType::Bullet)
				{
					Bullet& bullet = *reinterpret_cast<Bullet*>(m_Map.Entities[i]);
					m_RenderData.DrawCircle(glm::vec3(entity.Position, 0.f), entity.Size.x, 1.f, 0.05f, bullet.Color * 1.3f);
					if(bullet.bulletType == BulletType::BFG)
					if (bullet.Contacts != 0 || bullet.shotEnemy) {
						m_Particle.LifeTime = 0.35f;
						m_Particle.ColorBegin = bullet.Color * 2.f;
						m_Particle.ColorEnd = bullet.Color;
						m_Particle.Position = entity.Position;
						m_Particle.Velocity = glm::vec2(0.5f);
						m_Particle.VelocityVariation = glm::normalize(entity.Position - m_Map.player.Position) * 2.5f;
						for (int i = 0; i < 6; i++)
							m_ParticleEmitter.Emit(m_Particle);
					}
				}
				else
					m_RenderData.DrawQuad(glm::vec3(entity.Position, 0.f), entity.Size * 2.f, 0, entity.Type == EntityType::EyeballEnemy ? glm::vec4(1.f, 0, 0, 1.f) : glm::vec4(0.5f, 0.5f, 0.5f, 1.f));
			}
			
			if (m_RotateSword)
			{
				m_SwordRotation += (2.f * glm::pi<float>() - m_SwordRotation) * 11.5f * Globals.deltaTime;

				if (m_SwordRotation >= 2.f * glm::pi<float>() - 0.1f)
				{
					m_SwordRotation = 0.f;
					m_RotateSword = false;
				}
				glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(m_Map.player.Position, 0.f)) *
					glm::rotate(glm::mat4(1.f), m_SwordRotation, { 0.f, 0.f, 1.f }) * glm::scale(glm::mat4(1.f),
						glm::vec3{ 0.14f, 1.f, 0.5f } * 6.f);

				m_RenderData.DrawQuad(transform, SwordTexture);
			}
			else
			{
				glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - m_Map.player.Position);
				float angle = atan2(dir.y, dir.x);
				if (dir.x < 0.f) angle = glm::pi<float>() - angle;
				glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(m_Map.player.Position + glm::vec2(0.25f, 0), 0.f)) *
					glm::rotate(glm::mat4(1.f), angle, { 0.f, 0.f, dir.x < 0.f ? -1.f : 1.f }) * glm::scale(glm::mat4(1.f), 
						{ (dir.x < 0.f ? -1.f : 1.f) * 1.f, 0.45f, 1.f });

				uint32_t tex = m_Map.player.weapon ? PlasmaGunTexture : SawedOffTexture;

				m_RenderData.DrawQuad(transform, tex);
			}

			m_ParticleEmitter.OnRender(m_RenderData);

			m_Renderer.Flush(m_RenderData);
			m_RenderData.Reset();
		}

		void Update()
		{
			UpdateGame();
			RenderGame();
		}

		void MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_Button,  ImVec4(95.f / 255.f, 14.f / 255.f, 61.f / 255.f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(95.f / 255.f, 35.f / 255.f, 61.f / 255.f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(95.f / 255.f, 35.f / 255.f, 61.f / 255.f, 0.7f));

			ImGui::Begin("Screen Render", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - 200) * 0.5f, (ImGui::GetWindowSize().y - 300) * 0.5f));
			if (ImGui::Button("Play", ImVec2(200, 100)))Globals.gameState = GameState::PLAY;
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize("- Solo Summoned -").x) * 0.5f, (ImGui::GetWindowSize().y - ImGui::CalcTextSize("- Solo Summoned -").y) * 0.5f));
			ImGui::TextColored(ImVec4(95.f / 255.f, 14.f / 255.f, 61.f / 255.f, 1.f), "- Solo Summoned -");
			ImGui::End();
			ImGui::PopStyleVar(5);
		}

		void DEATH_MENU()
		{
			WC_CORE_INFO("You died")
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
			ImGui::SetWindowFontScale(0.8f);
			ImGui::SetCursorPos(ImVec2(10, 10));
			ImGui::TextColored(ImVec4(57 / 255.f, 255 / 255.f, 20 / 255.f, 1.f), std::format("HP: {}", m_Map.player.Health).c_str());
			//ImGui::TextColored(ImVec4(57 / 255.f, 255 / 255.f, 20 / 255.f, 1.f), std::format("HP: {}", m_Map.Entities[1].Health).c_str());
			ImGui::GetBackgroundDrawList()->AddImage(m_Renderer.GetRenderImageID(), ImVec2(0, 0), ImVec2((float)Globals.window.GetSize().x, (float)Globals.window.GetSize().y));
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