#include "CoffeeEngine/Physics/PhysicsWorld.h"
#include "CoffeeEngine/Physics/CollisionSystem.h"
#include "CoffeeEngine/Renderer/Renderer2D.h"
#include "CoffeeEngine/Scene/SceneManager.h"

#include <glm/fwd.hpp>

namespace Coffee {

    PhysicsWorld::PhysicsWorld() {
        collisionConfig = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfig);
        broadphase = new btDbvtBroadphase();
        solver = new btSequentialImpulseConstraintSolver();
        dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
        dynamicsWorld->setGravity(btVector3(0, GRAVITY, 0));
    }

    PhysicsWorld::~PhysicsWorld() {
        for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
            btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body) {
                dynamicsWorld->removeRigidBody(body);
            }
        }

        delete dynamicsWorld;
        delete solver;
        delete broadphase;
        delete dispatcher;
        delete collisionConfig;
    }

    void PhysicsWorld::addRigidBody(btRigidBody* body) const {
        dynamicsWorld->addRigidBody(body);
    }

    void PhysicsWorld::removeRigidBody(btRigidBody* body) const {
        dynamicsWorld->removeRigidBody(body);
    }

    void PhysicsWorld::stepSimulation(const float dt) const {
        dynamicsWorld->stepSimulation(dt);
        CollisionSystem::checkCollisions(*this);
    }

    void PhysicsWorld::setGravity(const float gravity) const {
        dynamicsWorld->setGravity(btVector3(0, gravity, 0));
    }

    void PhysicsWorld::setGravity(const btVector3& gravity) const {
        dynamicsWorld->setGravity(gravity);
    }

    btDiscreteDynamicsWorld* PhysicsWorld::getDynamicsWorld() const
    {
        return dynamicsWorld;
    }

    void PhysicsWorld::drawCollisionShapes() const
    {
        if (!dynamicsWorld)
            return;
        const int numCollisionObjects = dynamicsWorld->getNumCollisionObjects();
        for (int i = 0; i < numCollisionObjects; i++)
        {
            constexpr float margin = 0.05f;
            const btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];

            if (!obj)
                continue;

            const btCollisionShape* shape = obj->getCollisionShape();

            if (!shape)
                continue;

            const btTransform& transform = obj->getWorldTransform();

            btVector3 origin = transform.getOrigin();
            btQuaternion rotation = transform.getRotation();
            glm::vec3 position(origin.x(), origin.y(), origin.z());
            glm::quat orientation(rotation.w(), rotation.x(), rotation.y(), rotation.z());

            switch (shape->getShapeType())
            {
            case BOX_SHAPE_PROXYTYPE: {
                const btBoxShape* boxShape = static_cast<const btBoxShape*>(shape);
                if (!boxShape)
                    continue;

                btVector3 halfExtents = boxShape->getHalfExtentsWithMargin();
                glm::vec3 size((halfExtents.x() + margin) * 2.0f, (halfExtents.y() + margin) * 2.0f,
                               (halfExtents.z() + margin) * 2.0f);
                Renderer2D::DrawBox(position, orientation, size, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
                break;
            }
            case SPHERE_SHAPE_PROXYTYPE: {
                const btSphereShape* sphereShape = static_cast<const btSphereShape*>(shape);
                if (!sphereShape)
                    continue;

                const float radius = sphereShape->getRadius() + margin;
                Renderer2D::DrawSphere(position, radius, orientation, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
                break;
            }
            case CAPSULE_SHAPE_PROXYTYPE: {
                const btCapsuleShape* capsuleShape = static_cast<const btCapsuleShape*>(shape);
                if (!capsuleShape)
                    continue;

                const float radius = capsuleShape->getRadius() + margin;
                const float cylinderHeight = capsuleShape->getHalfHeight() * 2.0f + margin;

                Renderer2D::DrawCapsule(position, orientation, radius, cylinderHeight,
                                        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
                break;
            }
            default:
                continue;
            }
        }
    }

    void PhysicsWorld::DebugDrawRaycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, 
                                        const glm::vec4& rayColor, const glm::vec4& hitColor) const 
    {
        // Normalize direction
        glm::vec3 normalizedDir = glm::normalize(direction);
        
        // Perform the raycast to get hit info
        RaycastHit hit = Raycast(origin, normalizedDir, maxDistance);
        
        // Calculate end point
        glm::vec3 endPoint;
        if (hit.hasHit) {
            // Draw only up to the hit point
            endPoint = hit.hitPoint;
            
            // Draw the hit point
            const float hitPointSize = 0.1f;
            Renderer2D::DrawSphere(hit.hitPoint, hitPointSize, glm::quat(), hitColor);
            
            // Draw the hit normal
            const float normalLength = 0.5f;
            Renderer2D::DrawLine(
                hit.hitPoint, 
                hit.hitPoint + hit.hitNormal * normalLength,
                glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 
                0.02f
            );
        } else {
            // No hit, draw the full ray
            endPoint = origin + normalizedDir * maxDistance;
        }
        
        // Draw the ray itself
        Renderer2D::DrawLine(origin, endPoint, rayColor, 0.03f);
    }

    RaycastHit PhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
        btVector3 btFrom(origin.x, origin.y, origin.z);
        btVector3 btDir(direction.x, direction.y, direction.z);
        btDir.normalize();
        btVector3 btTo = btFrom + btDir * maxDistance;

        btCollisionWorld::ClosestRayResultCallback rayCallback(btFrom, btTo);
        dynamicsWorld->rayTest(btFrom, btTo, rayCallback);

        RaycastHit result;
        if (rayCallback.hasHit()) {
            result.hasHit = true;
            auto& hitPoint = rayCallback.m_hitPointWorld;
            auto& hitNormal = rayCallback.m_hitNormalWorld;

            result.hitPoint = glm::vec3(hitPoint.x(), hitPoint.y(), hitPoint.z());
            result.hitNormal = glm::vec3(hitNormal.x(), hitNormal.y(), hitNormal.z());
            result.hitFraction = rayCallback.m_closestHitFraction;

            // Get the entity associated with the hit body
            const btCollisionObject* obj = rayCallback.m_collisionObject;
            if (obj && obj->getUserPointer()) {
                result.hitEntity = CreateRef<Entity>(static_cast<entt::entity>(reinterpret_cast<size_t>(obj->getUserPointer())), SceneManager::GetActiveScene().get());
            }
        }

        return result;
    }

    // this function is not correctly tested yet
    std::vector<RaycastHit> PhysicsWorld::RaycastAll(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const
    {
        btVector3 btFrom(origin.x, origin.y, origin.z);
        btVector3 btDir(direction.x, direction.y, direction.z);
        btDir.normalize();
        btVector3 btTo = btFrom + btDir * maxDistance;

        btCollisionWorld::AllHitsRayResultCallback rayCallback(btFrom, btTo);
        dynamicsWorld->rayTest(btFrom, btTo, rayCallback);

        std::vector<RaycastHit> results;

        for (int i = 0; i < rayCallback.m_collisionObjects.size(); i++) {
            RaycastHit result;
            result.hasHit = true;

            auto& hitPoint = rayCallback.m_hitPointWorld[i];
            auto& hitNormal = rayCallback.m_hitNormalWorld[i];

            result.hitPoint = glm::vec3(hitPoint.x(), hitPoint.y(), hitPoint.z());
            result.hitNormal = glm::vec3(hitNormal.x(), hitNormal.y(), hitNormal.z());
            result.hitFraction = rayCallback.m_hitFractions[i];

            const btCollisionObject* obj = rayCallback.m_collisionObjects[i];
            if (obj && obj->getUserPointer()) {
                result.hitEntity = CreateRef<Entity>(static_cast<entt::entity>(reinterpret_cast<size_t>(obj->getUserPointer())), SceneManager::GetActiveScene().get());
            }

            results.push_back(result);
        }

        // Sort results by distance (fraction)
        std::sort(results.begin(), results.end(),
                  [](const RaycastHit& a, const RaycastHit& b) {
                      return a.hitFraction < b.hitFraction;
                  });

        return results;
    }

    bool PhysicsWorld::RaycastAny(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const
    {
        btVector3 btFrom(origin.x, origin.y, origin.z);
        btVector3 btDir(direction.x, direction.y, direction.z);
        btDir.normalize();
        btVector3 btTo = btFrom + btDir * maxDistance;

        btCollisionWorld::ClosestRayResultCallback rayCallback(btFrom, btTo);

        dynamicsWorld->rayTest(btFrom, btTo, rayCallback);

        return rayCallback.hasHit();
    }

} // namespace Coffee