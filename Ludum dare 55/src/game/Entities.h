#pragma once
#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <vector>
#include <wc/Utils/Time.h>
#include <wc/Utils/YAML.h>

namespace wc
{
    enum class EntityType : int32_t
    {
        Tile = -1,
        UNDEFINED = 0,
        // Entities
        Entity,
        Bullet,
        RedCube,

        Player,
    };

    struct BaseEntity
    {
        EntityType Type = EntityType::UNDEFINED;
    };

    struct TransformComponent
    {
        glm::vec2 Position;
        glm::vec2 Size = glm::vec2(0.5f);
        float Rotation = 0.f;

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec2& translation) { Position = translation; }

        glm::mat4 GetTransform() const
        {
            auto Translation = glm::vec3(Position, 0.f);
            auto Scale = glm::vec3(Size, 1.f);

            return glm::translate(glm::mat4(1.f), Translation)
                * glm::rotate(glm::mat4(1.f), Rotation, { 0.f, 0.f, 1.f })
                * glm::scale(glm::mat4(1.f), Scale);
        }
    };

    struct DynamicEntity : public BaseEntity, public TransformComponent
    {
        b2Body* body = nullptr;
        // @TODO: Improve on contacts
        uint32_t UpContacts = 0;
        uint32_t DownContacts = 0;
        uint32_t LeftContacts = 0;
        uint32_t RightContacts = 0;
        uint32_t Contacts = 0;

        uint32_t ID = 0;

        inline void SetPosition() { body->SetTransform({ Position.x, Position.y }, 0.f); }

        inline void UpdatePosition()
        {
            auto& bPos = body->GetPosition();
            Position = { bPos.x, bPos.y };
        }

        void LoadMapBase(const YAML::Node& node)
        {
            YAML_LOAD_VAR(node, Position);
        }
    };

    struct GameEntity : public DynamicEntity
    {
        // Properties
        float StartHealth = 10.f;
        float Damage = 0.5f;
        float Speed = 8.5f;
        float Density = 55.f;

        float Health = 10.f;
        float LinearDamping = 1.4f;        

        bool PlayerTouch = false;
        bool ShotEnemy = false;
        uint32_t EnemyID;

        glm::vec2 HitBoxSize = glm::vec2(0.5f);

        bool Alive() { return Health > 0.f; }        

        virtual void CreateBody(b2World* PhysicsWorld)
        {
            b2BodyDef bodyDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.position.Set(Position.x, Position.y);
            bodyDef.fixedRotation = true;
            body = PhysicsWorld->CreateBody(&bodyDef);

            b2PolygonShape shape;
            shape.SetAsBox(HitBoxSize.x, HitBoxSize.y);


            b2FixtureDef fixtureDef;
            fixtureDef.density = Density;
            fixtureDef.friction = 0.f;

            fixtureDef.userData.pointer = (uintptr_t)this;

            fixtureDef.shape = &shape;
            body->CreateFixture(&fixtureDef);
            body->SetLinearDamping(LinearDamping);
        }

        GameEntity() = default;
    };

    struct Player : public GameEntity
    {
        bool weapon = true;

        float JumpForce = 1900.f;

        bool Dash = false;
        float DashCD = 0.f;

        float SwordCD = 1.5f;
        bool SwordAttack = false;
        float SwordDamage = 5.5f;

        float AttackCD = 0.5f;

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

        RedCube() { Type = EntityType::RedCube; Health = 15.f;  Speed = 1.1f; Damage = 1.f; }
    };

    enum class BulletType { Blaster, Shotgun, RedCircle };
    enum class WeaponType { Blaster, Shotgun, Sword };


    struct WeaponStats
    {
        bool CloseRange = false;
        BulletType BulletType = BulletType::Blaster;
        float Damage = 0.f;
        union
        {
            float FireRate;
            float Range;
        };

        uint32 TextureID = 0;
    };

    struct Bullet : public GameEntity
    {
        glm::vec2 Direction;
        BulletType BulletType;
        glm::vec2 ShotPos;

        Bullet() { Type = EntityType::Bullet; }

		void CreateBody(b2World* PhysicsWorld) override
		{
			b2BodyDef bodyDef;
			bodyDef.type = b2_dynamicBody;
			bodyDef.position.Set(Position.x, Position.y);
			bodyDef.fixedRotation = true;
			body = PhysicsWorld->CreateBody(&bodyDef);

			//b2PolygonShape shape;
			//shape.SetAsBox(HitBoxSize.x, HitBoxSize.y);
            b2CircleShape shape;
            shape.m_radius = Size.x;

			b2FixtureDef fixtureDef;
			fixtureDef.density = Density;
			fixtureDef.friction = 0.f;

			fixtureDef.userData.pointer = (uintptr_t)this;

			fixtureDef.shape = &shape;
			body->CreateFixture(&fixtureDef);
			body->SetLinearDamping(2.f);
		}
    };    
}