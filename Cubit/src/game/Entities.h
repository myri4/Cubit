#pragma once
#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <wc/Utils/Time.h>
#include <wc/Utils/YAML.h>

#include "Components.h"

#include "Weapons.h"

namespace wc
{
    enum class EntityType : int8_t
    {
        Tile = -1,
        UNDEFINED = 0,
        // Entities
        Bullet,
        Entity,

        Player,
    };	

    struct BaseEntity
    {
        EntityType Type = EntityType::UNDEFINED;
    };    

    struct DynamicEntity : public BaseEntity, public TransformComponent, public Rigidbody2DComponent
	{
		uint32_t ID = 0;

        inline void SetPosition() { b2Body_SetTransform(Body, { Position.x, Position.y }, { 0.f, 0.f }); }

        inline void UpdatePosition()
        {
            auto bPos = b2Body_GetPosition(Body);
            Position = { bPos.x, bPos.y };
        }

		inline void UpdatePosition(float alpha) // @NOTE: We don't interpolate angles yet
		{
			
		}

        void LoadMapBase(const YAML::Node& node)
        {
            YAML_LOAD_VAR(node, Position);
        }
    };

    struct EntityProperties
    {
		float Density = 55.f;
		float LinearDamping = 1.4f;
		float Speed = 7.f;

		uint32_t StartHealth = 100;
		uint32_t MaxHealth = 100;
    };

    EntityProperties EntityStats[magic_enum::enum_count<EntityType>()];

    struct Entity : public DynamicEntity
    {
        // Properties

        uint32_t StartHealth = 100;
        //uint32_t MaxHealth = Properties.MaxHealth;
        glm::vec2 HitBoxSize = glm::vec2(0.5f);

        uint32_t Health = 100;
                
        void TakeDamage(uint32_t Damage)
        {
            if (Alive()) Health -= glm::min(Damage, Health);
        }

        bool Alive() { return Health > 0; }        

        virtual void CreateBody(b2WorldId PhysicsWorld)
        {
            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = b2_dynamicBody;
            bodyDef.position = { Position.x, Position.y };
            bodyDef.fixedRotation = true;
            bodyDef.linearDamping = EntityStats[(int)Type].LinearDamping;//
            Body.Create(PhysicsWorld, bodyDef);

            b2Polygon shape = b2MakeBox(HitBoxSize.x, HitBoxSize.y);


			b2ShapeDef shapeDef = b2DefaultShapeDef();
			shapeDef.density = EntityStats[(int)Type].Density;
			shapeDef.friction = 0.1f;
            shapeDef.userData = this;

            ShapeID.CreatePolygonShape(Body, shapeDef, shape);
        }

        void DestroyBody()
        {
			ShapeID.Destroy();
			Body.Destroy();
        }

        Entity() = default;
    };

    struct Player : public Entity
    {
		WeaponType PrimaryWeapon = WeaponType::Blaster;
		WeaponType SecondaryWeapon = WeaponType::Revolver;
		WeaponType MeleeWeapon = WeaponType::Sword;

        WeaponType Weapon = PrimaryWeapon;

        WeaponData Weapons[magic_enum::enum_count<WeaponType>()];

        float JumpForce = 0.f;

        float DashCD = 0.f;

		// Character controller components
		float MoveDir = 0.f;

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
				if (weapon.Magazine < weaponStats.MaxMag) weapon.Timer = weaponStats.ReloadSpeed;
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
            //Density = 100.f;
        }
    };

    struct Bullet : public Entity
    {
        glm::vec2 Direction;
        BulletType BulletType;
        WeaponType WeaponType = WeaponType::Blaster;

        glm::vec4 Color;

        EntityType HitEntityType = EntityType::UNDEFINED;
		Entity* HitEntity = nullptr;
        Entity* SourceEntity = nullptr;
        uint32_t Bounces = 0;
        float DistanceTraveled = 0.f;

        Bullet() { Type = EntityType::Bullet; }

		bool IsSensor() { return WeaponStats[(int)WeaponType].IsBulletSensor; }
		//bool IsPolygon() { return WeaponStats[(int)WeaponType].IsBulletPolygon; }
		//bool IgnoreWalls() { return WeaponStats[(int)WeaponType].BulletIgnoreWalls; }

		bool HitAnyEntity() { return HitEntityType > EntityType::UNDEFINED; }
		bool DidHitEntity() { return HitEntityType > EntityType::Entity; } // To prevent name collision
		bool HitBullet() { return HitEntityType == EntityType::Bullet; }
		bool HitTile() { return HitEntityType == EntityType::Tile; }
    };
}