#pragma once

#define BT_NO_SIMD_OPERATOR_OVERLOADS

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Math/BoundingBox.h"
#include "Collider.h"
#include "PhysicsWorld.h"

#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <glm/glm.hpp>
#include <memory>

namespace Coffee {

    class RigidBody
    {
    public:
        enum class Type
        {
            Static,
            Dynamic,
            Kinematic
        };

        struct Properties
        {
            public:
                Type type = Type::Dynamic;
                float mass = 1.0f;
                bool useGravity = true;
                bool freezeX = false;
                bool freezeY = false;
                bool freezeZ = false;
                bool freezeRotX = false;  
                bool freezeRotY = false;
                bool freezeRotZ = false;
                bool isTrigger = false;
                glm::vec3 velocity = {0.0f, 0.0f, 0.0f};
                float friction = 0.5f;     // Default friction value
                float linearDrag = 0.01f;  // Default linear damping
                float angularDrag = 0.01f; // Default angular damping

            private:
                friend class cereal::access;
                friend class RigidBody;

                template<class Archive> void save(Archive& archive, std::uint32_t const version) const
                {
                    archive(
                        CEREAL_NVP(type),
                        CEREAL_NVP(mass),
                        CEREAL_NVP(useGravity),
                        CEREAL_NVP(freezeX),
                        CEREAL_NVP(freezeY),
                        CEREAL_NVP(freezeZ),
                        CEREAL_NVP(freezeRotX),
                        CEREAL_NVP(freezeRotY),
                        CEREAL_NVP(freezeRotZ),
                        CEREAL_NVP(isTrigger),
                        CEREAL_NVP(velocity),
                        CEREAL_NVP(friction),
                        CEREAL_NVP(linearDrag),
                        CEREAL_NVP(angularDrag)
                    );
                }

                template<class Archive> void load(Archive& archive, std::uint32_t const version)
                {
                    archive(
                        CEREAL_NVP(type),
                        CEREAL_NVP(mass),
                        CEREAL_NVP(useGravity),
                        CEREAL_NVP(freezeX),
                        CEREAL_NVP(freezeY),
                        CEREAL_NVP(freezeZ),
                        CEREAL_NVP(freezeRotX),
                        CEREAL_NVP(freezeRotY),
                        CEREAL_NVP(freezeRotZ),
                        CEREAL_NVP(isTrigger),
                        CEREAL_NVP(velocity),
                        CEREAL_NVP(friction),
                        CEREAL_NVP(linearDrag),
                        CEREAL_NVP(angularDrag)
                    );
                }
        };

        static Ref<RigidBody> Create(const Properties& props, const Ref<Collider>& collider);
        RigidBody() = default;
        ~RigidBody();

        // Kinematics
        void SetPosition(const glm::vec3& position) const;
        void SetRotation(const glm::vec3& rotation) const;
        void SetVelocity(const glm::vec3& velocity) const;
        void AddVelocity(const glm::vec3& deltaVelocity) const;
        glm::vec3 GetPosition() const;
        glm::vec3 GetRotation() const;
        glm::vec3 GetVelocity() const;

        // Angular movement functions
        void ApplyTorque(const glm::vec3& torque) const;
        void ApplyTorqueImpulse(const glm::vec3& torque) const;
        void SetAngularVelocity(const glm::vec3& velocity) const;
        glm::vec3 GetAngularVelocity() const;
        void ResizeColliderToFitAABB(const AABB& aabb);

        // Physics
        void ApplyForce(const glm::vec3& force) const;
        void ApplyImpulse(const glm::vec3& impulse) const;
        void SetTrigger(bool isTrigger);
        void ResetVelocity() const;
        void ClearForces() const;

        // Property accessors
        Type GetBodyType() const { return m_Properties.type; }
        void SetBodyType(Type type);

        float GetMass() const { return m_Properties.mass; }
        void SetMass(float mass);

        bool GetUseGravity() const { return m_Properties.useGravity; }
        void SetUseGravity(bool useGravity);

        bool GetFreezeX() const { return m_Properties.freezeX; }
        void SetFreezeX(bool freezeX);

        bool GetFreezeY() const { return m_Properties.freezeY; }
        void SetFreezeY(bool freezeY);

        bool GetFreezeZ() const { return m_Properties.freezeZ; }
        void SetFreezeZ(bool freezeZ);

        bool GetIsTrigger() const { return m_Properties.isTrigger; }
        
        float GetFriction() const { return m_Properties.friction; }
        void SetFriction(float friction);

        float GetLinearDrag() const { return m_Properties.linearDrag; }
        void SetLinearDrag(float linearDrag);

        float GetAngularDrag() const { return m_Properties.angularDrag; }
        void SetAngularDrag(float angularDrag);

        bool GetFreezeRotX() const { return m_Properties.freezeRotX; }
        void SetFreezeRotX(bool freezeRotX);

        bool GetFreezeRotY() const { return m_Properties.freezeRotY; }
        void SetFreezeRotY(bool freezeRotY);

        bool GetFreezeRotZ() const { return m_Properties.freezeRotZ; }
        void SetFreezeRotZ(bool freezeRotZ);

        // Access to properties (needed for UI)
        const Properties& GetProperties() const { return m_Properties; }
        Properties& GetPropertiesMutable() { return m_Properties; }

        // Internal use
        btRigidBody* GetNativeBody() const { return m_Body; }
        Ref<Collider> GetCollider() const { return m_Collider; }

    private:
        void Initialize(const Properties& props, const Ref<Collider>& collider);

        void UpdateLinearFactor();
        void UpdateAngularFactor();
        
        btRigidBody* m_Body = nullptr;
        btMotionState* m_MotionState = nullptr;
        Ref<Collider> m_Collider;
        Properties m_Properties;

    private:
        friend class cereal::access;
        template<class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(
                CEREAL_NVP(m_Properties),
                CEREAL_NVP(m_Collider)
            );
        }

        template<class Archive> 
        void load(Archive& archive, std::uint32_t const version)
        {
            archive(
                CEREAL_NVP(m_Properties),
                CEREAL_NVP(m_Collider)
            );
            Initialize(m_Properties, m_Collider);
        }
    };

} // namespace Coffee
CEREAL_CLASS_VERSION(Coffee::RigidBody, 0);
CEREAL_CLASS_VERSION(Coffee::RigidBody::Properties, 0);
