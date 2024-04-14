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
        EyeballEnemy,

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

        inline void SetPosition() { body->SetTransform({ Position.x, Position.y }, 0.f); }

        inline void UpdatePosition()
        {
            auto& bPos = body->GetPosition();
            Position = { bPos.x, bPos.y };
        }
    };

    struct GameEntity : public DynamicEntity
    {
        // Properties
        float StartHealth = 10.f;
        float Damage = 0.5f;
        float Speed = 8.5f;
        float Density = 55.f;

        uint32_t ID;

        float Health = 10.f;

        uint32_t UpContacts = 0;
        uint32_t DownContacts = 0;
        uint32_t LeftContacts = 0;
        uint32_t RightContacts = 0;
        uint32_t Contacts = 0;

        bool playerTouch = false;
        bool shotEnemy = false;
        uint32_t EnemyID;

        glm::vec2 HitBoxSize = glm::vec2(0.5f);

        bool Alive() { return Health > 0.f; }

        void LoadMapBase(const YAML::Node& node)
        {
            YAML_LOAD_VAR(node, Position);
        }

        void CreateBody(b2World* PhysicsWorld)
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
            body->SetLinearDamping(2.8f);
        }

        GameEntity() = default;
    };

    struct Player : public GameEntity
    {
        bool weapon = true;

        float JumpForce = 2200.f;

        bool dash = false;
        float dashCD = 0.f;

        float swordCD = 1.5f;
        bool swordAttack = false;
        float SwordDamage = 5.5f;

        float attackCD = 0.5f;

        Player()
        {
            Type = EntityType::Player;
            Density = 100.f;
        }

    };

    struct EyeballEnemy : public GameEntity
    {
        float attackTimer = 2.f;
        float range = 8.f;

        EyeballEnemy() { Type = EntityType::EyeballEnemy; Speed = 1.1f; Damage = 1.f; }
    };

    enum class BulletType { BFG, Eyeball, Shotgun };
    struct Bullet : public GameEntity
    {
        glm::vec2 direction;
        glm::vec4 Color;
        BulletType bulletType;
        bool Shot = false; // bool for one time change at the start

        Bullet() { Type = EntityType::Bullet; }
    };
}