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
        TestEnemy,

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
        float Speed = 7.f;
        float Density = 55.f;

        uint32_t TextureID = 0;

        float Health = 10.f;

        uint32_t UpContacts = 0;
        uint32_t DownContacts = 0;
        uint32_t LeftContacts = 0;
        uint32_t RightContacts = 0;

        glm::vec2 HitBoxSize = glm::vec2(0.5f);

        bool Alive() { return Health > 0.f; }

        void LoadMapBase(const YAML::Node& node)
        {
            YAML_LOAD_VAR(node, Position);
        }

        GameEntity() = default;
    };

    struct Player : public GameEntity
    {
        float JumpForce = 1750.f;

        Player()
        {
            Type = EntityType::Player;
            Density = 47.11f;
        }
    };

    struct Enemy : public GameEntity
    {
        Enemy() { Type = EntityType::TestEnemy; }
    };
}