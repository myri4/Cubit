#pragma once
#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <wc/Utils/Time.h>
#include <wc/Utils/YAML.h>

#include "Components.h"

namespace wc
{
    enum class EntityType : int8_t
    {
        Tile = -1,
        UNDEFINED = 0,
        // Entities
        Bullet,
        Entity,
        RedCube,
        Fly,

        Player,
    };

	enum class BulletType : uint8_t { Blaster, Revolver, Shotgun, RedCircle };
	enum class WeaponType : uint8_t { Blaster, Laser, Revolver, Shotgun, RedBlaster, Sword };
	enum class WeaponClass : uint8_t { Primary, Secondary, Melee, EnemyWeapon };

    struct BaseEntity
    {
        EntityType Type = EntityType::UNDEFINED;
    };    

    struct DynamicEntity : public BaseEntity, public TransformComponent, public Rigidbody2DComponent
	{
		uint32_t ID = 0;

        inline void SetPosition() { Body->SetTransform({ Position.x, Position.y }, 0.f); }

        inline void UpdatePosition()
        {
            auto& bPos = Body->GetPosition();
            Position = { bPos.x, bPos.y };
        }

		inline void UpdatePosition(float alpha) // @NOTE: We don't interpolate angles yet
		{
			auto bPos = glm::vec2(Body->GetPosition().x, Body->GetPosition().y);

			Position = glm::mix(Position, bPos, alpha);
		}

        void LoadMapBase(const YAML::Node& node)
        {
            YAML_LOAD_VAR(node, Position);
        }
    };

    struct GameEntity : public DynamicEntity
    {
        // Properties
        float Density = 55.f;
        float LinearDamping = 1.4f;        
        float Speed = 7.f;

        uint32_t StartHealth = 100;
        uint32_t Health = 100;

        glm::vec2 HitBoxSize = glm::vec2(0.5f);

        // Character controller components
		float MoveDir = 0.f;

        void DealDamage(uint32_t Damage)
        {
            if (Alive()) Health -= glm::min(Damage, Health);
        }

        bool Alive() { return Health > 0; }        

        virtual void CreateBody(b2World* PhysicsWorld)
        {
            b2BodyDef bodyDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.position.Set(Position.x, Position.y);
            bodyDef.fixedRotation = true;
            bodyDef.linearDamping = LinearDamping;
            Body = PhysicsWorld->CreateBody(&bodyDef);

            b2PolygonShape shape;
            shape.SetAsBox(HitBoxSize.x, HitBoxSize.y);


            b2FixtureDef fixtureDef;
            fixtureDef.density = Density;
            fixtureDef.friction = 0.f;

            fixtureDef.userData.pointer = (uintptr_t)this;

            fixtureDef.shape = &shape;
            Body->CreateFixture(&fixtureDef);
        }

        GameEntity() = default;
    };

    struct BulletInfo
    {

    };

    struct WeaponInfo
    {
        bool CloseRange = false;
        BulletType BulletType = BulletType::Blaster;
        WeaponClass WeaponClass = WeaponClass::Primary;
        uint32_t Damage = 0;
        bool canZoom = false;
        float FireRate = 0.f;
        float AltFireRate = 0.f;
        float BulletSpeed = 0.f;
        float Range = 0.f;
		float ReloadSpeed = 0.f;
		bool ReloadByOne = false;
		uint32_t MaxMag = 0;

        glm::vec2 Size;
        glm::vec2 Offset;

        glm::vec2 BulletSize;

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


    struct Player : public GameEntity
    {

		WeaponType PrimaryWeapon = WeaponType::Laser;
		WeaponType SecondaryWeapon = WeaponType::Shotgun;
		WeaponType MeleeWeapon = WeaponType::Sword;

        WeaponType Weapon = PrimaryWeapon;

        WeaponData Weapons[magic_enum::enum_count<WeaponType>()];

        float JumpForce = 0.f;

        float DashCD = 0.f;

        inline bool CanShoot() { return Weapons[(int)Weapon].Timer <= 0.f && Weapons[(int)Weapon].Magazine > 0; }

		inline bool CanMelee() { return Weapons[(int)MeleeWeapon].Timer <= 0; }
		inline void ResetMeleeTimer() { Weapons[(int)MeleeWeapon].Timer = WeaponStats[(int)MeleeWeapon].FireRate; }

		inline void ResetWeaponTimer(const bool Reload = true) { Weapons[(int)Weapon].Timer = Reload ? WeaponStats[(int)Weapon].FireRate : WeaponStats[(int)Weapon].AltFireRate; }

		void ReloadWeapon()
		{
            auto& weapon = Weapons[(int)Weapon];
            auto& weaponStats = WeaponStats[(int)Weapon];
			if (weaponStats.ReloadByOne)
			{
				weapon.Timer = weaponStats.ReloadSpeed;
				if (weapon.ReloadTimer <= 0 && weapon.Magazine < weaponStats.MaxMag && weapon.Ammo > 0)
				{
					weapon.Magazine++;
					weapon.Ammo--;
					weapon.ReloadTimer = weaponStats.ReloadSpeed;
				}
			}
			else
			{
				weapon.Timer = weaponStats.ReloadSpeed;
				if (weapon.Ammo >= weaponStats.MaxMag - weapon.Magazine) 
                {
					weapon.Ammo -= weaponStats.MaxMag - weapon.Magazine;
					weapon.Magazine = weaponStats.MaxMag;
				}
				else 
                {
					weapon.Magazine += weapon.Ammo;
					weapon.Ammo = 0;
				}
			}
		}

        Player()
        {
            Type = EntityType::Player;
            Density = 100.f;
        }
    };
    	
    struct RedCube : public GameEntity
    {
        float AttackTimer = 2.f; 
        float ShootRange = 8.f;
        float DetectRange = 15.f;
        uint32_t Damage = 10;

        RedCube() { Type = EntityType::RedCube; Health = 150; Speed = 1.1f; }
    };    

	struct Fly : public GameEntity
	{
		float AttackTimer = 5.f;
		float ShootRange = 4.f;
		float DetectRange = 10.f;
		uint32_t Damage = 5;

		Fly() { Type = EntityType::Fly; Health = 50; Speed = 1.5f; }
	};

    struct Bullet : public GameEntity
    {
        glm::vec2 Direction;
        BulletType BulletType;
        WeaponType WeaponType = WeaponType::Blaster;
        glm::vec2 ShotPos;

        EntityType HitEntityType = EntityType::UNDEFINED;
		GameEntity* HitEntity = nullptr;
        GameEntity* SourceEntity = nullptr;

        Bullet() { Type = EntityType::Bullet; }

        void CreateBody(b2World* PhysicsWorld) override
        {
            b2BodyDef bodyDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.position.Set(Position.x, Position.y);
            bodyDef.fixedRotation = true;
            bodyDef.bullet = true;
            Body = PhysicsWorld->CreateBody(&bodyDef);

            b2CircleShape shape;
            shape.m_radius = Size.x;

            b2FixtureDef fixtureDef;
            fixtureDef.density = Density;
            fixtureDef.friction = 0.f;
            fixtureDef.isSensor = true;

            fixtureDef.userData.pointer = (uintptr_t)this;

            fixtureDef.shape = &shape;
            Body->CreateFixture(&fixtureDef); 
            Body->SetLinearVelocity(b2Vec2(Direction.x * Speed, Direction.y * Speed));
            Body->SetGravityScale(0.f);
        }
    };
}