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

#include <wc/vk/SyncContext.h>

#include <wc/Utils/Time.h>
#include <wc/Utils/box2d.h>
#include <wc/Utils/List.h>
#include <wc/Utils/Window.h>
#include <wc/Utils/YAML.h>

// GUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

#include "../Globals.h"
#include "../Rendering/Renderer2D.h"
#include "../UI/Widgets.h"

#include "Map.h"

namespace wc
{
	struct GameInstance
	{
	protected:
		Map m_Map;
		uint32_t m_LevelID = 0;

		bool m_RotateSword = false;
		float m_SwordRotation = 0.f;

		OrthographicCamera camera;

		uint32_t SwordTexture = 0;
		Font font;

		glm::vec2 m_TargetPosition;
		float m_TargetRotation = 0.f;
		float m_TargetZoom = 1.f;
		float m_TargetChromaFallOff = 10.f;

		void LoadFull(const std::string& filepath)
		{
			m_Map.LoadFull(filepath);

			Globals.serverTime = 0.f;
			m_RotateSword = false;
			m_SwordRotation = 0.f;
			camera.Position = glm::vec3(m_Map.player.Position, 0.f);
			m_Map.player.UpContact = false;
			m_Map.player.DownContact = false;
			m_Map.player.LeftContact = false;
			m_Map.player.RightContact = false;
		}

		std::string levelPath;
	public:	
		bool Overlay = true;

		void Create(glm::vec2 renderSize)
		{
			m_RenderData.Create();
			m_Renderer.Init(camera);
			font.Load("assets/fonts/ST-SimpleSquare.ttf", m_RenderData);

			m_Tileset.Load();

			SwordTexture = m_RenderData.LoadTexture("assets/textures/Sword.png");

			{
				auto& player = EntityStats[(int)EntityType::Player];
				player.Density = 100.f;
				player.Speed = 3.5f;
				player.LinearDamping = 1.5f;
			}

			{
				auto& blaster = WeaponStats[(int)WeaponType::Blaster];
				blaster.BulletType = BulletType::Blaster;
				blaster.WeaponClass = WeaponClass::Primary;
				blaster.CanZoom = true;
				blaster.Damage = 30;
				blaster.FireRate = 0.3f;
				blaster.MaxMag = 15;
				blaster.ReloadSpeed = 1.5f;
				blaster.Range = 50.f;
				blaster.BulletColor = { 0.f, 1.f, 0.f, 1.f };
				blaster.BulletSpeed = 45.f;
				blaster.BulletSize = { 0.25f, 0.25f };
				blaster.Recoil = { 0.25f, -0.15f };
				blaster.RenderOffset = { 0.25f, -0.15f };
				blaster.RenderSize = { 1.f, 0.45f };
				blaster.TextureID = m_RenderData.LoadTexture("assets/textures/Plasma_Rifle.png");
			}

			{
				auto& laser = WeaponStats[(int)WeaponType::Laser];
				laser.WeaponClass = WeaponClass::Primary;
				laser.CanZoom = true;
				laser.Damage = 60;
				laser.FireRate = 1.5f;
				laser.MaxMag = 5;
				laser.ReloadSpeed = 2.5f;
				laser.Range = 50.f;
				laser.BulletSize = { 0.25f, 0.25f };
				laser.Recoil = { 0.15f, -0.15f };
				laser.RenderOffset = { 0.0f, -0.0f };
				laser.RenderSize = { 1.5f, 0.45f };
				laser.TextureID = m_RenderData.LoadTexture("assets/textures/LaserGun.png");
			}

			{
				auto& shotgun = WeaponStats[(int)WeaponType::Shotgun];
				shotgun.BulletType = BulletType::Shotgun;
				shotgun.WeaponClass = WeaponClass::Secondary;
				shotgun.Damage = 18;
				shotgun.FireRate = 1.1f;
				shotgun.MaxMag = 4;
				shotgun.ReloadSpeed = 0.5f;
				shotgun.ReloadByOne = true;
				shotgun.CanZoom = true;
				shotgun.Range = 5.5f;
				shotgun.BulletColor = { 1.f, 1.f, 0.f, 1.f };
				shotgun.BulletSpeed = 25.f;
				shotgun.BulletSize = { 0.1f, 0.1f };
				shotgun.Recoil = { 0.25f, -0.15f };
				shotgun.RenderOffset = { 0.25f, -0.15f };
				shotgun.RenderSize = { 1.f, 0.45f };
				shotgun.TextureID = m_RenderData.LoadTexture("assets/textures/Sawed-Off.png");
			}

			{
				auto& revolver = WeaponStats[(int)WeaponType::Revolver];
				revolver.BulletType = BulletType::Revolver;
				revolver.WeaponClass = WeaponClass::Secondary;
				revolver.Damage = 25;
				revolver.FireRate = 1.f;
				revolver.AltFireRate = 1.5f;
				revolver.MaxMag = 99;
				revolver.ReloadSpeed = 0.35f;
				revolver.ReloadByOne = true;
				revolver.Range = 125.f;
				revolver.RenderOffset = { 0.25f, -0.15f };
				revolver.RenderSize = { 1.f, 0.45f };
				revolver.IsBulletSensor = false;
				revolver.BulletColor = { 1.f, 1.f, 0.f, 1.f };
				revolver.BulletSpeed = 25.f;
				revolver.BulletSize = { 0.1f, 0.1f };
				revolver.BulletBounces = 3;
				revolver.TextureID = m_RenderData.LoadTexture("assets/textures/Revolver.png");
			}

			{
				auto& redBlaster = WeaponStats[(int)WeaponType::RedBlaster];
				redBlaster.WeaponClass = WeaponClass::EnemyWeapon;
				redBlaster.BulletType = BulletType::RedCircle;
				redBlaster.Damage = 5;
				redBlaster.FireRate = 0.3f;
				redBlaster.Range = 50.f;
				redBlaster.BulletColor = { 1.f, 0, 0, 1.f };
				redBlaster.BulletSpeed = 45.f;
				redBlaster.Recoil = { 0.25f, -0.15f };
				redBlaster.BulletSize = { 0.25f, 0.25f };
				redBlaster.RenderOffset = { 0.25f, -0.15f };
				redBlaster.RenderSize = { 1.f, 0.45f };
				redBlaster.TextureID = m_RenderData.LoadTexture("assets/textures/Plasma_Rifle.png");
			}

			{
				auto& sword = WeaponStats[(int)WeaponType::Sword];
				sword.WeaponClass = WeaponClass::Melee;
				sword.Damage = 50;
				sword.FireRate = 2.5f;
				sword.Range = 15.f;
				sword.RenderSize = { 1.f, 0.45f };
				sword.TextureID = m_RenderData.LoadTexture("assets/textures/Sword.png");
			}

			m_Renderer.CreateScreen(renderSize, m_RenderData);
			m_ParticleEmitter.Init();
			m_Particle.ColorBegin = { 0.99f, 0.83f, 0.48f, 1.f };
			m_Particle.ColorEnd = { 0.99f, 0.42f, 0.16f, 1.f };
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
			auto& player = m_Map.player;
	
			//if (Mouse::IsMouseReleased(MouseCode::LEFT))WC_CORE_INFO("Mouse Pressed");

			//if (Key::IsKeyPressed(KeyCode::C))
			//{
			//	WC_CORE_INFO(Key::DownDuration(KeyCode::C))
			//	//Key::downDuration(KeyCode::C);
			//}

			//WC_CORE_INFO(player.Position);

			if (Key::IsKeyDown(Globals.settings.KeyLeft) && player.LeftContact == 0) player.MoveDir = -1.f;
			else if (Key::IsKeyDown(Globals.settings.KeyRight) && player.RightContact == 0) player.MoveDir = 1.f;
			else player.MoveDir = 0.f;

			if (player.MoveDir != 0.f)
			{
				if (Key::IsKeyPressed(Globals.settings.KeyDash) && player.DashCD <= 0.f)
				{
					ma_engine_play_sound(&Globals.sfx_engine, "assets/sound/sfx/dash.wav", NULL);
					player.Body.ApplyLinearImpulseToCenter({ 50.f * player.Body.GetMass() * player.MoveDir, 0.f }, true);
					player.DashCD = 2.f;
				}
			}

			if (Key::IsKeyPressed(Globals.settings.KeyFastSwich))
			{
				if (player.Weapon == player.PrimaryWeapon) player.Weapon = player.SecondaryWeapon;
				else player.Weapon = player.PrimaryWeapon;
			}

			if (Key::IsKeyPressed(Globals.settings.KeyMainWeapon))
				player.Weapon = player.PrimaryWeapon;

			else if (Key::IsKeyPressed(Globals.settings.KeySecondaryWeapon))
				player.Weapon = player.SecondaryWeapon;

			if (player.DownContact)
			{
				auto vel = player.Body.GetLinearVelocity();

				player.Body.SetGravityScale(1.f);

				vel.x -= m_Map.DragStrength * vel.x * Globals.deltaTime;

				if (abs(vel.x) < 0.01f) vel.x = 0.f;

				player.Body.SetLinearVelocity(vel);
			}
			else if (player.Body.GetLinearVelocity().y < 0.f) player.Body.SetGravityScale(2.5f);

			if (Key::IsKeyPressed(Globals.settings.KeyJump) && player.DownContact)
			{
				// @NOTE: if gravity is changed we should update this
				float jumHeight = 8.f;
				player.JumpForce = glm::sqrt(jumHeight * m_Map.Gravity * player.Body.GetGravityScale() * -(2.f + EntityStats[(int)EntityType::Player].LinearDamping)) * player.Body.GetMass();

				player.Body.ApplyLinearImpulseToCenter({ 0.f, player.JumpForce }, true);
			}


			glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);

			bool Zoom = WeaponStats[(int)player.Weapon].CanZoom && Mouse::IsMouseDown(MouseCode::Right);

			m_TargetRotation = 0.f;
			if (Zoom)
			{
				//auto offset = dir * 8.f;
				//offset.y = glm::clamp(offset.y, -3.f, 3.f);
				m_TargetPosition = player.Position + dir * 3.f;
				m_TargetZoom = 0.6f;
			}
			else
			{
				m_TargetPosition = player.Position;
				//m_TargetPosition = glm::vec2(m_Map.Size.x * 0.5f, m_Map.Size.y * 0.5f);
				m_TargetZoom = 1.f;
			}

			if (Key::IsKeyPressed(Globals.settings.KeyReload)) player.ReloadWeapon();

			if (Mouse::IsMouseDown(Globals.settings.MouseShoot) && player.CanShoot())
			{
				if (player.Weapon == WeaponType::Blaster)
				{
					std::random_device rd;
					std::mt19937 gen(rd());
					auto& offset = WeaponStats[(int)WeaponType::Blaster].Recoil;
					std::uniform_real_distribution<float> dis(offset.x, offset.y);

					ma_engine_play_sound(&Globals.sfx_engine, Globals.gun.c_str(), nullptr);

					m_Map.SpawnBullet(player.Position + dir * 0.25f, Zoom ? dir : RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))), player.Weapon, (Entity*)&player);
				}
				else if (player.Weapon == WeaponType::Laser)
				{
					ma_engine_play_sound(&Globals.sfx_engine, Globals.gun.c_str(), nullptr);

					glm::vec2 shootPos = player.Position + dir * 0.25f;
					auto hitInfo = m_Map.Intersect({ shootPos, dir }, 1, EntityType::Bullet);

					if (glm::distance(hitInfo.Point, player.Position) < WeaponStats[(int)WeaponType::Laser].Range) {
						if (hitInfo.Hit && hitInfo.Entity) {
							if (static_cast<Entity*>(hitInfo.Entity)->Type > EntityType::Entity)
								static_cast<Entity*>(hitInfo.Entity)->TakeDamage(WeaponStats[(int)player.Weapon].Damage);
						}

						m_Particle.Position = hitInfo.Point;
						m_Particle.ColorBegin = glm::vec4{ 254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f } *2.f;
						m_Particle.ColorEnd = { 254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f };
						m_Particle.Velocity = -dir;
						m_Particle.VelocityVariation = -dir * 3.5f;
						m_ParticleEmitter.Emit(m_Particle, 25);
					}
				}
				else if (player.Weapon == WeaponType::Shotgun)
				{
					camera.Shake(0.8f);

					glm::vec2 shootPos = player.Position + dir * 0.25f;
					auto hitInfo = m_Map.Intersect({ shootPos, dir }, 1);

					if (hitInfo.t <= 5.f)
					{
						float invDist = 1.f / glm::max(hitInfo.t, 1.f);
						auto recoilForce = -dir * player.JumpForce * 2.f * invDist * invDist;
						player.Body.ApplyLinearImpulseToCenter(-dir * player.JumpForce * 2.f * invDist * invDist, true);
					}

					ma_engine_play_sound(&Globals.sfx_engine, Globals.shotgun.c_str(), nullptr);
					std::random_device rd;
					std::mt19937 gen(rd());
					auto& offset = WeaponStats[(int)WeaponType::Shotgun].Recoil;
					std::uniform_real_distribution<float> dis(offset.x, offset.y);

					for (uint32_t i = 0; i < 10; i++)
					{
						m_Map.SpawnBullet(shootPos, RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))), player.Weapon, (Entity*)&player);

						m_Particle.Position = player.Position + dir * 0.55f;
						auto vel = player.Body.GetLinearVelocity();
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
					auto& offset = WeaponStats[(int)WeaponType::Revolver].Recoil;
					std::uniform_real_distribution<float> dis(offset.x, offset.y);

					ma_engine_play_sound(&Globals.sfx_engine, Globals.gun.c_str(), nullptr);
					m_Map.SpawnBullet(player.Position + dir * 1.25f, RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))), player.Weapon, (Entity*)&player);
				}

				player.Weapons[(int)player.Weapon].Magazine--;
				player.ResetWeaponTimer();
			}

			//alt fire
			if (Mouse::IsMouseDown(Globals.settings.MouseAlt) && player.CanShoot())
			{
				glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);

				player.ResetWeaponTimer(false);

				if (player.Weapon == WeaponType::Revolver)
				{
					ma_engine_play_sound(&Globals.sfx_engine, Globals.gun.c_str(), nullptr);

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
						ma_engine_play_sound(&Globals.sfx_engine, Globals.gun.c_str(), nullptr);
						glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);

						m_Map.SpawnBullet(player.Position + dir * 0.75f, RandomOnHemisphere(dir, glm::normalize(dir + glm::vec2(dis(gen), dis(gen)))), player.Weapon, (Entity*)&player);
						player.Weapons[(int)player.Weapon].Magazine--;
						player.Weapons[(int)player.Weapon].Timer = 0.2f;
					}
				}
			}


			if (Key::IsKeyPressed(Globals.settings.KeyMelee) && player.CanMelee())
			{
				if (player.MeleeWeapon == WeaponType::Sword)
				{
					ma_engine_play_sound(&Globals.sfx_engine, Globals.swordSwing.c_str(), nullptr);
					m_RotateSword = true;

					for (uint32_t i = 0; i < m_Map.Entities.size(); i++)
					{
						auto& entity = *m_Map.Entities[i];
#if 0
						if (entity.Type == EntityType::RedCube)
						{
							float dist = glm::distance(entity.Position, player.Position);
							if (dist < WeaponStats[(int)player.MeleeWeapon].Range)
							{
								glm::vec2 direction = glm::normalize(entity.Position - player.Position);

								auto hitInfo = m_Map.Intersect({ player.Position, direction }, m_Map.Entities.size()); // We skip entity intersection

								if ((hitInfo.Hit && dist <= hitInfo.t) || !hitInfo.Hit)
								{
									auto vel = glm::sign(direction.x) * 15.f * entity.Body->GetMass();
									entity.Body->ApplyLinearImpulseToCenter(b2Vec2(vel, 0.f), true);
									entity.DealDamage(WeaponStats[(int)player.MeleeWeapon].Damage);
								}
							}
						}
#endif
					}
				}

				player.ResetMeleeTimer();
			}

			//if (ImGui::IsKeyPressed(ImGuiKey_F2))
			//{
			//	VulkanContext::GetLogicalDevice().WaitIdle();

			//	uint32_t width = m_Renderer.GetRenderSize().x;
			//	uint32_t height = m_Renderer.GetRenderSize().y;

			//	StagingBuffer stagingBuffer;
			//	stagingBuffer.Allocate(sizeof(float) * 4 * width * height, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			//	SyncContext::immediate_submit([&](VkCommandBuffer cmd) {
			//		m_Renderer.GetFinalImage().setLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			//		VkBufferImageCopy copyRegion = {};
			//		copyRegion.bufferOffset = 0;
			//		copyRegion.bufferRowLength = 0;
			//		copyRegion.bufferImageHeight = 0;

			//		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			//		copyRegion.imageSubresource.mipLevel = 0;
			//		copyRegion.imageSubresource.baseArrayLayer = 0;
			//		copyRegion.imageSubresource.layerCount = 1;
			//		copyRegion.imageExtent.width = width;
			//		copyRegion.imageExtent.height = height;
			//		copyRegion.imageExtent.depth = 1;

			//		//copy the buffer into the image
			//		vkCmdCopyImageToBuffer(cmd, m_Renderer.GetFinalImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &copyRegion);

			//		m_Renderer.GetFinalImage().setLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
			//		});

			//	float* imagedata = (float*)stagingBuffer.Map();

			//	const auto now = std::chrono::system_clock::now();
			//	auto filename = std::format("screenshots/screenshot_{:%d-%m-%Y-%H-%M-%OS}.png", now);

			//	int bytes_per_pixel = 4;
			//	uint32_t bytes_per_scanline = bytes_per_pixel * width;
			//	uint8_t* imageOutput = new uint8_t[width * height * bytes_per_pixel];

			//	for (int i = 0; i < width * height * bytes_per_pixel; i++)
			//		imageOutput[i] = (uint8_t)(imagedata[i] * 255.f);

			//	stbi_write_png(filename.c_str(), width, height, bytes_per_pixel, imageOutput, bytes_per_scanline);
			//	delete[] imageOutput;
			//	stagingBuffer.Unmap();
			//	stagingBuffer.Free();
			//}
		}

		void RenderGame()
		{
			auto& player = m_Map.player;

			m_RenderData.ViewProjection = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, -1.f, 1.f);
			m_RenderData.DrawQuad({ 0.f, 0.f, 0.f }, { 1.f, 1.f }, m_Renderer.BackgroundTexture);
			m_RenderData.ViewProjection = camera.GetViewProjectionMatrix();

			for (uint32_t x = 0; x < m_Map.Size.x; x++)
				for (uint32_t y = 0; y < m_Map.Size.y; y++)
				{
					TileID tileID = m_Map.GetTile({ x,y, 0 });
					if (tileID != 0) m_RenderData.DrawQuad({ x, y , 0.f }, { 1.f, 1.f }, 0, glm::vec4(0.27f, 0.94f, 0.98f, 1.f));
				}

			for (int i = 0; i < m_Map.Entities.size(); i++)
			{
				Entity& entity = *(Entity*)(m_Map.Entities[i]);

				if (entity.Type == EntityType::Bullet)
				{
					Bullet& bullet = *(Bullet*)(m_Map.Entities[i]);
					m_RenderData.DrawCircle(glm::vec3(entity.Position, 0.f), entity.Size.x, 1.f, 0.05f, bullet.Color * 1.3f);
				}
				else
				{
					m_RenderData.DrawQuad(glm::vec3(entity.Position, 0.f), entity.Size * 2.f, player.Body.GetAngle(), 0, glm::vec4(0.27f, 0.94f, 0.98f, 1.f));

					if (entity.Type != EntityType::Player)
					{
						m_RenderData.DrawString(std::format("HP: {}", entity.Health), font, entity.Position + glm::vec2(-0.5f, 1.f), glm::vec4(1.f, 0, 0, 1.f));

						auto& weapon = WeaponStats[(int)WeaponType::Blaster];

						glm::vec2 dir = glm::normalize(player.Position - entity.Position);
						glm::vec2 offset = weapon.RenderOffset;
						float angle = atan2(dir.y, dir.x);
						if (dir.x < 0.f)
						{
							angle = glm::pi<float>() - angle;
							offset.x *= -1.f;
						}
						glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(entity.Position, 0.f)) *
							glm::rotate(glm::mat4(1.f), angle, { 0.f, 0.f, dir.x < 0.f ? -1.f : 1.f }) *
							glm::translate(glm::mat4(1.f), glm::vec3(offset, 0.f)) * glm::scale(glm::mat4(1.f),
								{ (dir.x < 0.f ? -1.f : 1.f) * weapon.RenderSize.x, weapon.RenderSize.y, 1.f });

						m_RenderData.DrawQuad(transform, weapon.TextureID);
					}
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
				auto& weapon = WeaponStats[(int)player.Weapon];

				glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);
				glm::vec2 offset = weapon.RenderOffset;
				float angle = atan2(dir.y, dir.x);
				if (dir.x < 0.f)
				{
					angle = glm::pi<float>() - angle;
					offset.x *= -1.f;
				}
				glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(player.Position, 0.f)) *
					glm::rotate(glm::mat4(1.f), angle, { 0.f, 0.f, dir.x < 0.f ? -1.f : 1.f }) *
					glm::translate(glm::mat4(1.f), glm::vec3(offset, 0.f)) * glm::scale(glm::mat4(1.f),
						{ (dir.x < 0.f ? -1.f : 1.f) * weapon.RenderSize.x, weapon.RenderSize.y, 1.f });

				m_RenderData.DrawQuad(transform, weapon.TextureID);
			}

			glm::vec2 dir = glm::normalize(glm::vec2(camera.Position) + m_Renderer.ScreenToWorld(Globals.window.GetCursorPos()) - player.Position);
			glm::vec2 shootPos = player.Position + dir * 0.35f;

			m_ParticleEmitter.OnRender(m_RenderData);

			m_Renderer.Flush(m_RenderData);
			m_RenderData.Reset();
		}

		void Update()
		{
			m_Map.Update();

			Globals.serverTime += Globals.deltaTime;
			//m_TargetPosition = glm::clamp(m_TargetPosition, glm::vec2(8, 4), glm::vec2(Size) - glm::vec2(8, 4));

			camera.Position += glm::vec3((m_TargetPosition - glm::vec2(camera.Position)) * 11.5f * Globals.deltaTime, 0.f);
			camera.Rotation += (m_TargetRotation - camera.Rotation) * 11.5f * Globals.deltaTime;
			if (camera.Zoom != m_TargetZoom)
			{
				camera.Zoom += (m_TargetZoom - camera.Zoom) * 11.5f * Globals.deltaTime;
				camera.Update(m_Renderer.GetHalfSize());
			}
			m_Renderer.ChromaSettings.Falloff += (m_TargetChromaFallOff - m_Renderer.ChromaSettings.Falloff) * 11.5f * Globals.deltaTime;

			m_ParticleEmitter.OnUpdate();

			if (!m_Map.player.Alive()) Globals.gameState = GameState::DEATH;

			RenderGame();
		}

		void UI_Data()
		{
			auto color = ImVec4(57 / 255.f, 255 / 255.f, 20 / 255.f, 1.f);
			ImGui::SetWindowFontScale(0.5f);

			ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2 - 75);
			ImGui::TextColored(color, std::format("{:.2f} sec.", Globals.serverTime).c_str());


			ImGui::SetCursorPos(ImVec2(10.f, ImGui::GetWindowSize().y - 70.f));
			ImGui::TextColored(color, std::format("Current Level: {}", m_LevelID).c_str());
			ImGui::SetCursorPos({ 10.f, 10.f });
			ImGui::TextColored(color, std::format("FPS: {}", 1 / Globals.deltaTime).c_str());
			ImGui::SetCursorPosX(10.f);
			ImGui::TextColored(color, std::format("Left: {}", m_Map.player.LeftContact ? "+" : "").c_str());
			ImGui::SetCursorPosX(10.f);
			ImGui::TextColored(color, std::format("Right: {}", m_Map.player.RightContact ? "+" : "").c_str());
			ImGui::SetCursorPosX(10.f);
			ImGui::TextColored(color, std::format("Down: {}", m_Map.player.DownContact ? "+" : "").c_str());
			ImGui::SetCursorPosX(10.f);
			ImGui::TextColored(color, std::format("Up: {}", m_Map.player.UpContact ? "+" : "").c_str());
			
			ImGui::SetWindowFontScale(1.2f);
			float offset = 0.f;
			if (m_Map.player.Weapons[(int)m_Map.player.Weapon].Ammo >= 100)offset = 305.f;
			else offset = 255.f;
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - offset, ImGui::GetWindowSize().y - 70.f));
			int mag = m_Map.player.Weapons[(int)m_Map.player.Weapon].Magazine;
			if (mag >= 10) ImGui::TextColored(color, std::format("{}/{}", mag, m_Map.player.Weapons[(int)m_Map.player.Weapon].Ammo).c_str());
			else ImGui::TextColored(color, std::format("  {}/{}", mag, m_Map.player.Weapons[(int)m_Map.player.Weapon].Ammo).c_str());
			//ImGui::SetCursorPosX(10.f);
			//ImGui::TextColored(color, std::format("Accumulator: {}", m_Map.player.Weapons[(int)m_Map.player.MeleeWeapon].Timer).c_str());
			//ImGui::SetCursorPosX(10.f);
			//ImGui::TextColored(color, std::format("Clamped steps: {}", glm::min(m_Map.nSteps, m_Map.MAX_STEPS)).c_str());

			//big hp-bar
			ImGui::SetWindowFontScale(0.2f);
			ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - 10));
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
			ImGui::ProgressBar((float)m_Map.player.Health / m_Map.player.StartHealth, ImVec2(ImGui::GetWindowSize().x, 10), std::format("{}", m_Map.player.Health).c_str());

			ImGui::SetWindowFontScale(1.f);
		}

		void MAIN_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.7f));

			ImGui::Begin("MENU", NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
			ImGui::SetCursorPos(ImVec2(0, 0));

			ImGui::SetWindowFontScale(2.f);
			char buf[128];
			sprintf(buf, "%c Cubit %c", "|/-\\"[(int)(ImGui::GetTime() / 0.25f) & 3], "|/-\\"[(int)(ImGui::GetTime() / 0.25f) & 3]);
			ImVec2 CubitSize = ImGui::CalcTextSize(buf);
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - CubitSize.x) * 0.5f, (ImGui::GetWindowSize().y - CubitSize.y) * 0.5f - 200));
			ImGui::TextColored(ImVec4(0, 1, 1, 1), buf);
			ImGui::SetWindowFontScale(1.f);


			ImVec2 PlaySize = ImGui::CalcTextSize("PLAY");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - PlaySize.x) * 0.5f, (ImGui::GetWindowSize().y - PlaySize.y) * 0.5f));
			if (ImGui::Button("PLAY") || ImGui::IsKeyPressed(ImGuiKey_Space))
			{
				m_Map.LoadFull("levels/level1.malen");
				Globals.gameState = GameState::PLAY;
			}

			ImVec2 LoadoutSize = ImGui::CalcTextSize("Loadout");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - LoadoutSize.x) * 0.5f, (ImGui::GetWindowSize().y - LoadoutSize.y) * 0.5f + 100));
			if (ImGui::Button("Loadout")) Globals.gameState = GameState::LOADOUT;

			ImVec2 SettingsSize = ImGui::CalcTextSize("Settings");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - SettingsSize.x) * 0.5f, (ImGui::GetWindowSize().y - SettingsSize.y) * 0.5f + 200));
			if (ImGui::Button("Settings")) {
				Globals.settings.Load();
				Globals.gameState = GameState::SETTINGS;
			}

			ImVec2 QuitSize = ImGui::CalcTextSize("Quit");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - QuitSize.x) * 0.5f, (ImGui::GetWindowSize().y - QuitSize.y) * 0.5f + 300));
			if (ImGui::Button("Quit")) Globals.window.Close();


			ImGui::PopStyleVar(6);
			ImGui::End();
		}

		void PAUSE_MENU()
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

			ImGui::SetNextWindowBgAlpha(0.75f);
			ImGui::Begin("Screen Render", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
			if (Overlay) UI_Data();

			ImGui::SetWindowFontScale(1.25f);
			ImVec2 PausedSize = ImGui::CalcTextSize("! PAUSED !");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - PausedSize.x) * 0.5f, (ImGui::GetWindowSize().y - PausedSize.y) * 0.5f - 100));
			ImGui::TextColored(ImVec4(57 / 255.f, 255 / 255.f, 20 / 255.f, 1.f), "! PAUSED !");

			ImGui::SetWindowFontScale(1.f);
			ImVec2 ResumeSize = ImGui::CalcTextSize("Resume");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ResumeSize.x) * 0.5f, (ImGui::GetWindowSize().y - ResumeSize.y) * 0.5f));
			if (ImGui::Button("Resume"))Globals.gameState = GameState::PLAY;

			ImVec2 ResetSize = ImGui::CalcTextSize("Reset");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - ResetSize.x) * 0.5f, (ImGui::GetWindowSize().y - ResetSize.y) * 0.5f + 100));
			if (ImGui::Button("Reset")) 
			{
				LoadFull(m_Map.levelPath);
				Globals.gameState = GameState::PLAY;
			}

			ImVec2 SettingsSize = ImGui::CalcTextSize("Settings");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - SettingsSize.x) * 0.5f, (ImGui::GetWindowSize().y - SettingsSize.y) * 0.5f + 200));
			if (ImGui::Button("Settings")) {
				Globals.settings.Load();
				Globals.gameState = GameState::SETTINGS;
			}


			ImVec2 MainMenuSize = ImGui::CalcTextSize("Main Menu");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - MainMenuSize.x) * 0.5f, (ImGui::GetWindowSize().y - MainMenuSize.y) * 0.5f + 300));
			if (ImGui::Button("Main Menu"))Globals.gameState = GameState::MENU;

			auto windowPos = (glm::vec2)Globals.window.GetPos();
			ImGui::GetBackgroundDrawList()->AddImage(m_Renderer.GetRenderImageID(), ImVec2(windowPos.x, windowPos.y), ImVec2((float)Globals.window.GetSize().x + windowPos.x, (float)Globals.window.GetSize().y + windowPos.y));
			ImGui::End();
			ImGui::PopStyleVar(3);
		}

		void LOADOUT_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
			ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 7.f);

			ImGui::Begin("LOADOUT", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetWindowFontScale(0.8f);
			if (ImGui::BeginTabBar("Tab", ImGuiTabBarFlags_None))
			{
				//@TODO - optimize this
				// ? put primaries in one class, secondaries in another and melee in another
				// - make the loops shorter -

				if (ImGui::BeginTabItem("Primary"))
				{
					ImGui::SeparatorText("Primary Weapon");
					for (int i = 0; i < magic_enum::enum_count<WeaponType>(); i++)
					{
						if (WeaponStats[i].WeaponClass == WeaponClass::Primary)UI::WeaponChoose((WeaponType)i, m_Map.player.PrimaryWeapon);
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Secondary"))
				{
					ImGui::SeparatorText("Secondary Weapon");
					for (int i = 0; i < magic_enum::enum_count<WeaponType>(); i++)
					{
						if (WeaponStats[i].WeaponClass == WeaponClass::Secondary) {
							UI::WeaponChoose((WeaponType)i, m_Map.player.SecondaryWeapon);
						}
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Melee"))
				{
					ImGui::SeparatorText("Melee Weapon");
					for (int i = 0; i < magic_enum::enum_count<WeaponType>(); i++)
					{
						if (WeaponStats[i].WeaponClass == WeaponClass::Melee) {
							UI::WeaponChoose((WeaponType)i, m_Map.player.MeleeWeapon);
						}
					}
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImVec2 BackSize = ImGui::CalcTextSize("Save and Go Back");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - BackSize.x) * 0.5f + 300, (ImGui::GetWindowSize().y + BackSize.y + 400) * 0.5f));
			if (ImGui::Button("Save and Go Back"))
			{
				Globals.gameState = GameState::MENU;
				Globals.settings.Save();
			}
			ImGui::End();
			ImGui::PopStyleVar(5);
		}

		bool buttonChecks[10] = { false };
		void SETTINGS_MENU()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.f);
			ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 7.f);

			ImGui::Begin("SETTINGS", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetWindowFontScale(0.8f);
			if (ImGui::BeginTabBar("Tab", ImGuiTabBarFlags_None)) 
			{
				if (ImGui::BeginTabItem("Screen")) 
				{
					ImGui::SeparatorText("Window");
					const char* windowSizes[] = { "1920x1080", "1366x768", "1280x1024", "1024x768", "1280x720" };
					ImGui::Combo("Window Size", &Globals.settings.iWindowSize, windowSizes, IM_ARRAYSIZE(windowSizes));
					UI::Checkbox("Fullscreen", Globals.settings.Fullscreen);
					UI::Checkbox("VSync", Globals.settings.VSync);
					UI::Separator("Game");
					ImGui::SliderFloat("Brightness", &Globals.settings.Brighness, 0.f, 1.f);
					UI::Checkbox("Screen Effects", Globals.settings.CRTEffect);
					UI::Checkbox("Bloom", Globals.settings.Bloom);
					UI::Checkbox("Vignete", Globals.settings.Vignete);
					UI::Checkbox("Background", Globals.settings.Background);
					UI::Checkbox("Overlay", Overlay);
					//UI::Checkbox("Hide Particles", Globals.settings.HideParticles);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Audio")) 
				{
					UI::Separator("Volume");					
					if (UI::Drag("Master Volume", Globals.settings.MasterVolume, 0.25f, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) 
					{ 
						ma_engine_set_volume(&Globals.music_engine, Globals.settings.MasterVolume); 
						ma_engine_set_volume(&Globals.music_engine, (Globals.settings.MusicVolume / 100.f) * (Globals.settings.MasterVolume / 100.0f));
						ma_engine_set_volume(&Globals.sfx_engine, (Globals.settings.SFXVolume / 100.f) * (Globals.settings.MasterVolume / 100.0f));
					}
					if (UI::Drag("Music Volume", Globals.settings.MusicVolume, 0.25f, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) { ma_engine_set_volume(&Globals.music_engine, (Globals.settings.MusicVolume / 100.f) * (Globals.settings.MasterVolume / 100.0f)); }
					if (UI::Drag("SFX Volume", Globals.settings.SFXVolume, 0.25f, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) { ma_engine_set_volume(&Globals.sfx_engine, (Globals.settings.SFXVolume / 100.f) * (Globals.settings.MasterVolume / 100.0f)); }

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Controls")) 
				{
					//UI::Separator("Movement");
					//UI::GetKey("Move Left", Globals.settings.KeyLeft, buttonChecks[0]);
					//UI::GetKey("Move Right", Globals.settings.	KeyRight, buttonChecks[1]);
					//UI::GetKey("Jump", Globals.settings.KeyJump, buttonChecks[2]);
					//UI::GetKey("Dash", Globals.settings.KeyDash, buttonChecks[3]);
					//UI::Separator("Weapons");
					//UI::GetKey("Shoot", Globals.settings.KeyShoot, buttonChecks[4]);
					//UI::GetKey("Melee", Globals.settings.KeyMelee, buttonChecks[5]);
					//UI::GetKey("Main Weapon", Globals.settings.KeyMainWeapon, buttonChecks[6]);
					//UI::GetKey("Secondary Weapon", Globals.settings.KeySecondaryWeapon, buttonChecks[7]);
					//UI::GetKey("Fast Switch", Globals.settings.KeyFastSwich, buttonChecks[8]);
					//UI::Separator("Game");

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImVec2 BackSize = ImGui::CalcTextSize("Save and Go Back");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - BackSize.x) * 0.5f + 300, (ImGui::GetWindowSize().y + BackSize.y + 400) * 0.5f));
			if (ImGui::Button("Save and Go Back")) 
			{
				Globals.gameState = GameState::MENU;
				Globals.settings.Save();
				Globals.window.SetSize(Globals.settings.WindowSize);
				Globals.window.RequestAttention();
				Globals.window.resized = true;
			}
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

			ImVec2 BackSize = ImGui::CalcTextSize("Go Back");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - BackSize.x) * 0.5f, (ImGui::GetWindowSize().y + BackSize.y + 300) * 0.5f));
			if (ImGui::Button("Go Back")) Globals.gameState = GameState::MENU;
			
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

			ImVec2 WinSize = ImGui::CalcTextSize("CONGRATS You Won!");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - WinSize.x) * 0.5f, (ImGui::GetWindowSize().y - WinSize.y) * 0.5f));
			ImGui::TextColored(ImVec4(95.f / 255.f, 14.f / 255.f, 61.f / 255.f, 1.f), "CONGRATS! You Won!");

			ImVec2 TimeSize = ImGui::CalcTextSize("Level Time: {} sec.");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - TimeSize.x) * 0.5f, (ImGui::GetWindowSize().y - TimeSize.y) * 0.5f - 100));
			ImGui::TextColored(ImVec4(95.f / 255.f, 14.f / 255.f, 61.f / 255.f, 1.f), std::format("Level Time: {:.2f} sec.", Globals.serverTime).c_str());

			ImVec2 LevelSize = ImGui::CalcTextSize("Current Level: {}");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - TimeSize.x) * 0.5f, (ImGui::GetWindowSize().y - TimeSize.y) * 0.5f - 200));
			ImGui::TextColored(ImVec4(95.f / 255.f, 14.f / 255.f, 61.f / 255.f, 1.f), std::format("Current Level: {}", m_LevelID).c_str());

			ImVec2 NextSize = ImGui::CalcTextSize("Go Next");
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - NextSize.x) * 0.5f, (ImGui::GetWindowSize().y + NextSize.y + 300) * 0.5f));
			if (ImGui::Button("Go Next")) 
			{
				Globals.gameState = GameState::PLAY;
				m_Map.LoadFull("levels/level2.malen");
				m_Map.player.Health = m_Map.player.StartHealth;
				m_LevelID++;
			}
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
			if (Overlay) UI_Data();

			//glm::vec2 pos = m_Renderer.WorldToScreen(m_Map.player.Position - (glm::vec2)m_Map.camera.Position - m_Map.player.Size.x);
			//ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
			//ImGui::ProgressBar(m_Map.player.DashCD / 2, ImVec2(65, 10), "");

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