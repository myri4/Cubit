#pragma once
#include <glm/glm.hpp>

namespace wc
{
	enum class BulletType : uint8_t { Blaster, Revolver, Shotgun, RedCircle, Slash };
	enum class WeaponType : uint8_t { Blaster, Laser, Revolver, Shotgun, RedBlaster, Sword };
	//enum class WeaponClass : uint8_t { Primary, Secondary, Melee, EnemyWeapon };

	struct BulletInfo
	{
		glm::vec4 Color = { 0, 0, 0, 0 };
		float Speed = 0.f;
		bool IsSensor = true; //physical body or a sensor
		bool IsPolygon = false; //is polygon or circle
		uint32_t Bounces = 0;
		glm::vec2 Size;
	};

	struct WeaponInfo
	{
		bool CloseRange = false;
		BulletType BulletType = BulletType::Blaster;
		//WeaponClass WeaponClass = WeaponClass::Primary;
		uint32_t Damage = 0;
		bool CanZoom = false;
		float FireRate = 0.f;
		float AltFireRate = 0.f;

		glm::vec4 BulletColor = { 0, 0, 0, 0 };
		float BulletSpeed = 0.f;
		bool IsBulletSensor = true; //physical body or a sensor
		bool BulletIgnoreWalls = false;
		uint32_t BulletBounces = 0;
		glm::vec2 BulletSize;
		bool IsBulletPolygon = false; //if true, bullet will be a polygon instead of a circle

		float Range = 0.f;
		float ReloadSpeed = 0.f;
		bool ReloadByOne = false;
		uint32_t MaxMag = 0;
		uint32_t MaxAmmo = 0;

		glm::vec2 RenderSize;
		glm::vec2 RenderOffset;

		glm::vec2 Recoil;

		uint32 TextureID = 0;
	};

	struct WeaponData
	{
		float Timer = 0.f;
		float AltFireTimer = 0.f;
		float ReloadTimer = 0.f;
		uint32_t Ammo = 0; // Reserved ammo
		uint32_t Magazine = 0;
	};

	WeaponInfo WeaponStats[magic_enum::enum_count<WeaponType>()];
}