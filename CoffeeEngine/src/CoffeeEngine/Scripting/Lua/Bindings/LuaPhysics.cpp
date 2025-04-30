#include "LuaPhysics.h"

#include "CoffeeEngine/Physics/RigidBody.h"
#include "CoffeeEngine/Scene/SceneManager.h"
#include "CoffeeEngine/Scene/Entity.h"

void Coffee::RegisterPhysicsBindings(sol::state& luaState)
{
    // Bind RigidBody::Type enum
    luaState.new_enum<RigidBody::Type>("RigidBodyType",
    {
        {"Static", RigidBody::Type::Static},
        {"Dynamic", RigidBody::Type::Dynamic},
        {"Kinematic", RigidBody::Type::Kinematic}
    });

    // Bind RigidBody properties
    luaState.new_usertype<RigidBody::Properties>("RigidBodyProperties",
        sol::constructors<RigidBody::Properties()>(),
        "type", &RigidBody::Properties::type,
        "mass", &RigidBody::Properties::mass,
        "useGravity", &RigidBody::Properties::useGravity,
        "freezeX", &RigidBody::Properties::freezeX,
        "freezeY", &RigidBody::Properties::freezeY,
        "freezeZ", &RigidBody::Properties::freezeZ,
        "freezeRotX", &RigidBody::Properties::freezeRotX,
        "freezeRotY", &RigidBody::Properties::freezeRotY,
        "freezeRotZ", &RigidBody::Properties::freezeRotZ,
        "isTrigger", &RigidBody::Properties::isTrigger,
        "velocity", &RigidBody::Properties::velocity,
        "friction", &RigidBody::Properties::friction,
        "linearDrag", &RigidBody::Properties::linearDrag,
        "angularDrag", &RigidBody::Properties::angularDrag
    );

    // Bind RigidBody methods
    luaState.new_usertype<RigidBody>("RigidBody",
        // Position and rotation
        "set_position", &RigidBody::SetPosition,
        "get_position", &RigidBody::GetPosition,
        "set_rotation", &RigidBody::SetRotation,
        "get_rotation", &RigidBody::GetRotation,

        // Velocity and forces
        "set_velocity", &RigidBody::SetVelocity,
        "get_velocity", &RigidBody::GetVelocity,
        "add_velocity", &RigidBody::AddVelocity,
        "apply_force", &RigidBody::ApplyForce,
        "apply_impulse", &RigidBody::ApplyImpulse,
        "reset_velocity", &RigidBody::ResetVelocity,
        "clear_forces", &RigidBody::ClearForces,

        // Torque and angular velocity methods
        "apply_torque", &RigidBody::ApplyTorque,
        "apply_torque_impulse", &RigidBody::ApplyTorqueImpulse,
        "set_angular_velocity", &RigidBody::SetAngularVelocity,
        "get_angular_velocity", &RigidBody::GetAngularVelocity,

        // Collisions and triggers
        "set_trigger", &RigidBody::SetTrigger,

        // Body type
        "get_body_type", &RigidBody::GetBodyType,
        "set_body_type", &RigidBody::SetBodyType,

        // Mass
        "get_mass", &RigidBody::GetMass,
        "set_mass", &RigidBody::SetMass,

        // Gravity
        "get_use_gravity", &RigidBody::GetUseGravity,
        "set_use_gravity", &RigidBody::SetUseGravity,

        // Constraints
        "get_freeze_x", &RigidBody::GetFreezeX,
        "set_freeze_x", &RigidBody::SetFreezeX,
        "get_freeze_y", &RigidBody::GetFreezeY,
        "set_freeze_y", &RigidBody::SetFreezeY,
        "get_freeze_z", &RigidBody::GetFreezeZ,
        "set_freeze_z", &RigidBody::SetFreezeZ,
        "get_freeze_rot_x", &RigidBody::GetFreezeRotX,
        "set_freeze_rot_x", &RigidBody::SetFreezeRotX,
        "get_freeze_rot_y", &RigidBody::GetFreezeRotY,
        "set_freeze_rot_y", &RigidBody::SetFreezeRotY,
        "get_freeze_rot_z", &RigidBody::GetFreezeRotZ,
        "set_freeze_rot_z", &RigidBody::SetFreezeRotZ,

        // Physical properties
        "get_friction", &RigidBody::GetFriction,
        "set_friction", &RigidBody::SetFriction,
        "get_linear_drag", &RigidBody::GetLinearDrag,
        "set_linear_drag", &RigidBody::SetLinearDrag,
        "get_angular_drag", &RigidBody::GetAngularDrag,
        "set_angular_drag", &RigidBody::SetAngularDrag,

        // Utility
        "get_is_trigger", &RigidBody::GetIsTrigger,

        // Add a function to get the collider
        "get_collider", &RigidBody::GetCollider,

        // Add a function to get the collider type
        "get_collider_type", [](const RigidBody& self) -> std::string {
            auto collider = self.GetCollider();
            if (!collider) return "None";

            if (std::dynamic_pointer_cast<BoxCollider>(collider)) return "Box";
            if (std::dynamic_pointer_cast<SphereCollider>(collider)) return "Sphere";
            if (std::dynamic_pointer_cast<CapsuleCollider>(collider)) return "Capsule";
            if (std::dynamic_pointer_cast<ConeCollider>(collider)) return "Cone";
            if (std::dynamic_pointer_cast<CylinderCollider>(collider)) return "Cylinder";

            return "Unknown";
        }
    );

    // Add Collider usertype bindings
    luaState.new_usertype<Collider>("Collider",
        // Add the following functions to the Collider binding
        "set_box_size", [](const Collider& self, const glm::vec3& size) {
            Ref<BoxCollider> boxCollider = CreateRef<BoxCollider>(size);

            // Get the scene
            auto scene = SceneManager::GetActiveScene();

            // Find the entity with this collider
            Entity entity;

            // Get a view of entities with RigidbodyComponent
            const auto view = scene->GetAllEntitiesWithComponents<RigidbodyComponent>();
            bool found = false;

            // Iteration with early termination
            for (const auto entityID : view) {
                Entity e(entityID, scene.get());
                if (const auto& rb = e.GetComponent<RigidbodyComponent>(); rb.rb && rb.rb->GetCollider().get() == &self) {
                    entity = e;
                    found = true;
                    break;
                }
            }

            if (!found) return;  // No matching entity found

            auto& rbComponent = entity.GetComponent<RigidbodyComponent>();

            // Store current rigidbody properties
            const RigidBody::Properties props = rbComponent.rb->GetProperties();
            const glm::vec3 position = rbComponent.rb->GetPosition();
            const glm::vec3 rotation = rbComponent.rb->GetRotation();
            const glm::vec3 velocity = rbComponent.rb->GetVelocity();

            // Remove from physics world
            scene->GetPhysicsWorld().removeRigidBody(rbComponent.rb->GetNativeBody());;

            // Create new box collider with updated size
            const Ref<Collider> newCollider = CreateRef<BoxCollider>(size);

            // Create new rigidbody with new collider
            rbComponent.rb = RigidBody::Create(props, newCollider);
            rbComponent.rb->SetPosition(position);
            rbComponent.rb->SetRotation(rotation);
            rbComponent.rb->SetVelocity(velocity);

            // Add back to physics world
            scene->GetPhysicsWorld().addRigidBody(rbComponent.rb->GetNativeBody());
            rbComponent.rb->GetNativeBody()->setUserPointer(
                reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<entt::entity>(entity))));
        },

        "set_sphere_radius", [](Collider& self, float radius) {
            Ref<SphereCollider> boxCollider = CreateRef<SphereCollider>(radius);

            // Get the scene
            auto scene = SceneManager::GetActiveScene();

            // Find the entity with this collider
            Entity entity;

            // Get a view of entities with RigidbodyComponent
            const auto view = scene->GetAllEntitiesWithComponents<RigidbodyComponent>();
            bool found = false;

            // Iteration with early termination
            for (const auto entityID : view) {
                Entity e(entityID, scene.get());
                if (const auto& rb = e.GetComponent<RigidbodyComponent>(); rb.rb && rb.rb->GetCollider().get() == &self) {
                    entity = e;
                    found = true;
                    break;
                }
            }

            if (!found) return;  // No matching entity found

            auto& rbComponent = entity.GetComponent<RigidbodyComponent>();

            // Store current rigidbody properties
            const RigidBody::Properties props = rbComponent.rb->GetProperties();
            const glm::vec3 position = rbComponent.rb->GetPosition();
            const glm::vec3 rotation = rbComponent.rb->GetRotation();
            const glm::vec3 velocity = rbComponent.rb->GetVelocity();

            // Remove from physics world
            scene->GetPhysicsWorld().removeRigidBody(rbComponent.rb->GetNativeBody());;

            // Create new box collider with updated size
            const Ref<SphereCollider> newCollider = CreateRef<SphereCollider>(radius);

            // Create new rigidbody with new collider
            rbComponent.rb = RigidBody::Create(props, newCollider);
            rbComponent.rb->SetPosition(position);
            rbComponent.rb->SetRotation(rotation);
            rbComponent.rb->SetVelocity(velocity);

            // Add back to physics world
            scene->GetPhysicsWorld().addRigidBody(rbComponent.rb->GetNativeBody());
            rbComponent.rb->GetNativeBody()->setUserPointer(
                reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<entt::entity>(entity))));
        },

        "set_capsule_dimensions", [](Collider& self, float radius, float height) {
            Ref<CapsuleCollider> boxCollider = CreateRef<CapsuleCollider>(radius, height);

            // Get the scene
            auto scene = SceneManager::GetActiveScene();

            // Find the entity with this collider
            Entity entity;

            // Get a view of entities with RigidbodyComponent
            const auto view = scene->GetAllEntitiesWithComponents<RigidbodyComponent>();
            bool found = false;

            // Iteration with early termination
            for (const auto entityID : view) {
                Entity e(entityID, scene.get());
                if (const auto& rb = e.GetComponent<RigidbodyComponent>(); rb.rb && rb.rb->GetCollider().get() == &self) {
                    entity = e;
                    found = true;
                    break;
                }
            }

            if (!found) return;  // No matching entity found

            auto& rbComponent = entity.GetComponent<RigidbodyComponent>();

            // Store current rigidbody properties
            const RigidBody::Properties props = rbComponent.rb->GetProperties();
            const glm::vec3 position = rbComponent.rb->GetPosition();
            const glm::vec3 rotation = rbComponent.rb->GetRotation();
            const glm::vec3 velocity = rbComponent.rb->GetVelocity();

            // Remove from physics world
            scene->GetPhysicsWorld().removeRigidBody(rbComponent.rb->GetNativeBody());;

            // Create new box collider with updated size
            const Ref<CapsuleCollider> newCollider = CreateRef<CapsuleCollider>(radius, height);

            // Create new rigidbody with new collider
            rbComponent.rb = RigidBody::Create(props, newCollider);
            rbComponent.rb->SetPosition(position);
            rbComponent.rb->SetRotation(rotation);
            rbComponent.rb->SetVelocity(velocity);

            // Add back to physics world
            scene->GetPhysicsWorld().addRigidBody(rbComponent.rb->GetNativeBody());
            rbComponent.rb->GetNativeBody()->setUserPointer(
                reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<entt::entity>(entity))));
        },
        "set_cone_dimensions", [](Collider& self, float radius, float height) {
            // Get the scene
            auto scene = SceneManager::GetActiveScene();
        
            // Find the entity with this collider
            Entity entity;
        
            // Get a view of entities with RigidbodyComponent
            const auto view = scene->GetAllEntitiesWithComponents<RigidbodyComponent>();
            bool found = false;
        
            // Iteration with early termination
            for (const auto entityID : view) {
                Entity e(entityID, scene.get());
                if (const auto& rb = e.GetComponent<RigidbodyComponent>(); rb.rb && rb.rb->GetCollider().get() == &self) {
                    entity = e;
                    found = true;
                    break;
                }
            }
        
            if (!found) return;  // No matching entity found
        
            auto& rbComponent = entity.GetComponent<RigidbodyComponent>();
        
            // Store current rigidbody properties
            const RigidBody::Properties props = rbComponent.rb->GetProperties();
            const glm::vec3 position = rbComponent.rb->GetPosition();
            const glm::vec3 rotation = rbComponent.rb->GetRotation();
            const glm::vec3 velocity = rbComponent.rb->GetVelocity();
        
            // Remove from physics world
            scene->GetPhysicsWorld().removeRigidBody(rbComponent.rb->GetNativeBody());
        
            // Create new cone collider with updated dimensions
            const Ref<ConeCollider> newCollider = CreateRef<ConeCollider>(radius, height);
        
            // Create new rigidbody with new collider
            rbComponent.rb = RigidBody::Create(props, newCollider);
            rbComponent.rb->SetPosition(position);
            rbComponent.rb->SetRotation(rotation);
            rbComponent.rb->SetVelocity(velocity);
        
            // Add back to physics world
            scene->GetPhysicsWorld().addRigidBody(rbComponent.rb->GetNativeBody());
            rbComponent.rb->GetNativeBody()->setUserPointer(
                reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<entt::entity>(entity))));
        },
        "set_cylinder_dimensions", [](Collider& self, float radius, float height) {
            // Get the scene
            auto scene = SceneManager::GetActiveScene();

            // Find the entity with this collider
            Entity entity;

            // Get a view of entities with RigidbodyComponent
            const auto view = scene->GetAllEntitiesWithComponents<RigidbodyComponent>();
            bool found = false;

            // Iteration with early termination
            for (const auto entityID : view) {
                Entity e(entityID, scene.get());
                if (const auto& rb = e.GetComponent<RigidbodyComponent>(); rb.rb && rb.rb->GetCollider().get() == &self) {
                    entity = e;
                    found = true;
                    break;
                }
            }

            if (!found) return;  // No matching entity found

            auto& rbComponent = entity.GetComponent<RigidbodyComponent>();

            // Store current rigidbody properties
            const RigidBody::Properties props = rbComponent.rb->GetProperties();
            const glm::vec3 position = rbComponent.rb->GetPosition();
            const glm::vec3 rotation = rbComponent.rb->GetRotation();
            const glm::vec3 velocity = rbComponent.rb->GetVelocity();

            // Remove from physics world
            scene->GetPhysicsWorld().removeRigidBody(rbComponent.rb->GetNativeBody());

            // Create new cylinder collider with updated dimensions
            const Ref<CylinderCollider> newCollider = CreateRef<CylinderCollider>(radius, height);

            // Create new rigidbody with new collider
            rbComponent.rb = RigidBody::Create(props, newCollider);
            rbComponent.rb->SetPosition(position);
            rbComponent.rb->SetRotation(rotation);
            rbComponent.rb->SetVelocity(velocity);

            // Add back to physics world
            scene->GetPhysicsWorld().addRigidBody(rbComponent.rb->GetNativeBody());
            rbComponent.rb->GetNativeBody()->setUserPointer(
                reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<entt::entity>(entity))));
        }
    );

    luaState.new_usertype<BoxCollider>("BoxCollider",
        sol::constructors<BoxCollider(), BoxCollider(const glm::vec3&)>(),
        sol::base_classes, sol::bases<Collider>()
    );

    luaState.new_usertype<SphereCollider>("SphereCollider",
        sol::constructors<SphereCollider(), SphereCollider(float)>(),
        sol::base_classes, sol::bases<Collider>()
    );

    luaState.new_usertype<CapsuleCollider>("CapsuleCollider",
        sol::constructors<CapsuleCollider(), CapsuleCollider(float, float)>(),
        sol::base_classes, sol::bases<Collider>()
    );

    luaState.new_usertype<ConeCollider>("ConeCollider",
        sol::constructors<ConeCollider(), ConeCollider(float, float)>(),
        sol::base_classes, sol::bases<Collider>()
    );

    luaState.new_usertype<CylinderCollider>("CylinderCollider",
        sol::constructors<CylinderCollider(), CylinderCollider(float, float)>(),
        sol::base_classes, sol::bases<Collider>()
    );

    // Helper functions for creating colliders and rigidbodies
    luaState.set_function("create_box_collider", [](const glm::vec3& size) {
        return CreateRef<BoxCollider>(size);
    });

    luaState.set_function("create_sphere_collider", [](float radius) {
        return CreateRef<SphereCollider>(radius);
    });

    luaState.set_function("create_capsule_collider", [](float radius, float height) {
        return CreateRef<CapsuleCollider>(radius, height);
    });

    luaState.set_function("create_cone_collider", [](float radius, float height) {
        return CreateRef<ConeCollider>(radius, height);
    });

    luaState.set_function("create_cylinder_collider", [](float radius, float height) {
        return CreateRef<CylinderCollider>(radius, height);
    });

    luaState.set_function("create_rigidbody", [](const RigidBody::Properties& props, const Ref<Collider>& collider) {
        return RigidBody::Create(props, collider);
    });

    sol::table physicsTable = luaState.create_table();
    luaState["Physics"] = physicsTable;

    // Bind RaycastHit type to Lua
    luaState.new_usertype<RaycastHit>(
        "RaycastHit",
        "hasHit", &RaycastHit::hasHit,
        "hitEntity", &RaycastHit::hitEntity,
        "hitPoint", &RaycastHit::hitPoint,
        "hitNormal", &RaycastHit::hitNormal,
        "hitFraction", &RaycastHit::hitFraction
    );

    // Bind Raycast functions
    physicsTable["Raycast"] = [](const glm::vec3& origin, const glm::vec3& direction, float maxDistance) -> RaycastHit {
        auto scene = SceneManager::GetActiveScene();
        if (!scene)
            return RaycastHit{};

        return scene->GetPhysicsWorld().Raycast(origin, direction, maxDistance);
    };

    physicsTable["RaycastAll"] = [](const glm::vec3& origin, const glm::vec3& direction, float maxDistance) -> std::vector<RaycastHit> {
        auto scene = SceneManager::GetActiveScene();
        if (!scene)
            return std::vector<RaycastHit>{};

        return scene->GetPhysicsWorld().RaycastAll(origin, direction, maxDistance);
    };

    physicsTable["RaycastAny"] = [](const glm::vec3& origin, const glm::vec3& direction, float maxDistance) -> bool {
        auto scene = SceneManager::GetActiveScene();
        if (!scene)
            return false;

        return scene->GetPhysicsWorld().RaycastAny(origin, direction, maxDistance);
    };

    physicsTable["DebugDrawRaycast"] = [](const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
        sol::optional<glm::vec4> rayColor, sol::optional<glm::vec4> hitColor) {
        auto scene = SceneManager::GetActiveScene();
        if (!scene)
        return;

        // Default colors if not provided
        glm::vec4 rColor = rayColor.value_or(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        glm::vec4 hColor = hitColor.value_or(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

        scene->GetPhysicsWorld().DebugDrawRaycast(origin, direction, maxDistance, rColor, hColor);
    };
}