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
		Map m_Map;		
	public:		

		void Create(glm::vec2 renderSize)
		{
			m_RenderData.Create();
			m_Renderer.Init(m_Map.camera);
			m_Map.font.Load("assets/fonts/ST-SimpleSquare.ttf", m_RenderData);

			m_Tileset.Load();

			m_Map.PlasmaGunTexture = m_RenderData.LoadTexture("assets/textures/Plasma_Rifle.png");
			m_Map.SawedOffTexture = m_RenderData.LoadTexture("assets/textures/Sawed-Off.png");
			m_Map.SwordTexture = m_RenderData.LoadTexture("assets/textures/Sword.png");

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

			if (ImGui::IsKeyPressed(ImGuiKey_E)) player.SwordAttack = true;
			player.Dash = Key::GetKey(Key::LeftShift) == GLFW_PRESS;
			if (ImGui::IsKeyReleased(ImGuiKey_Q)) 
			{
				player.weapon = !player.weapon;
				player.AttackCD = 0.2f;
			}

			//jump
			if (ImGui::IsKeyPressed(ImGuiKey_Space) && player.DownContacts != 0) 
				player.body->ApplyLinearImpulseToCenter({ 0.f, player.JumpForce }, true);
			

			if (moveDir != glm::vec2(0.f))
			{
				moveDir = glm::normalize(moveDir) * player.Speed * (player.DownContacts > 0 ? 1.f : m_Map.AirSpeedFactor);
				player.body->ApplyLinearImpulseToCenter({ moveDir.x, moveDir.y }, true);
			}

			if (player.DownContacts > 0)
			{
				auto vel = player.body->GetLinearVelocity();

				vel.x -= m_Map.DragStrength * vel.x * Globals.deltaTime;

				if (abs(vel.x) < 0.01f) vel.x = 0.f;

				player.body->SetLinearVelocity(vel);
			}
		}		

		void Update()
		{
			m_Map.UpdateGame();
			m_Map.RenderGame();
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

			ImGui::Begin("MENU", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - 200) * 0.5f, (ImGui::GetWindowSize().y - 350) * 0.5f));
			ImGui::SetWindowFontScale(1.f);
			if (ImGui::Button("Play", ImVec2(200, 100))) {
				Globals.gameState = GameState::PLAY;
				m_Map.EnemyCount = 0;
				m_Map.LoadFull("levels/level2.malen");
				m_Map.player.Health = 10;
			}
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize("- Cube's Calling -").x) * 0.5f, (ImGui::GetWindowSize().y - ImGui::CalcTextSize("- Cube's Calling -").y) * 0.5f));
			ImGui::TextColored(ImVec4(0, 1.f, 1.f, 1.f), "- Cube's Calling -");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - 200) * 0.5f, (ImGui::GetWindowSize().y + 400) * 0.5f));
			if (ImGui::Button("Credits", ImVec2(200, 100)))Globals.gameState = GameState::CREDITS;
			ImGui::End();
			ImGui::PopStyleVar(5);
		}

		

		void DEATH_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
			ImGui::Begin("DEATH", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetWindowFontScale(1.8f);
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize("You Died").x) * 0.5f, (ImGui::GetWindowSize().y - ImGui::CalcTextSize("You Died").y) * 0.5f));
			ImGui::TextColored(ImVec4(1.f, 0, 0, 1.f), "You Died");
			ImGui::SetWindowFontScale(1.f);
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - 270) * 0.5f, (ImGui::GetWindowSize().y + 300) * 0.5f));
			if (ImGui::Button("Go Back", ImVec2(270, 100)))Globals.gameState = GameState::MENU;
			ImGui::End();
			ImGui::PopStyleVar(5);
		}

		void WIN_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
 
			ImGui::Begin("WIN", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ImGui::CalcTextSize("CONGRATS You Won!").x) * 0.5f, (ImGui::GetWindowSize().y - ImGui::CalcTextSize("CONGRATS You Won!").y) * 0.5f));
			ImGui::TextColored(ImVec4(95.f / 255.f, 14.f / 255.f, 61.f / 255.f, 1.f), "CONGRATS! You Won!");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - 270) * 0.5f, (ImGui::GetWindowSize().y + 300) * 0.5f));
			if (ImGui::Button("Go Back", ImVec2(270, 100)))Globals.gameState = GameState::MENU;
			ImGui::End();
			ImGui::PopStyleVar(5);
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
			ImGui::SetWindowFontScale(0.5f);
			ImGui::SetCursorPos(ImVec2(10, 10));
			ImGui::TextColored(ImVec4(57 / 255.f, 255 / 255.f, 20 / 255.f, 1.f), std::format("FPS: {}", int(1.f / Globals.deltaTime)).c_str());
			ImGui::TextColored(ImVec4(57 / 255.f, 255 / 255.f, 20 / 255.f, 1.f), std::format("AT: {}", CURRENT_FRAME).c_str());
			ImGui::TextColored(ImVec4(57 / 255.f, 255 / 255.f, 20 / 255.f, 1.f), std::format("Enemy count: {}", m_Map.EnemyCount).c_str());
			auto windowPos = (glm::vec2)Globals.window.GetPos();
			ImGui::GetBackgroundDrawList()->AddImage(m_Renderer.GetRenderImageID(), ImVec2(windowPos.x, windowPos.y), ImVec2((float)Globals.window.GetSize().x + windowPos.x, (float)Globals.window.GetSize().y + windowPos.y));
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