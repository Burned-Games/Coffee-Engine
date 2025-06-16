#include "Scene.h"

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/DataStructures/Octree.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/Math/Frustum.h"
#include "CoffeeEngine/Physics/Collider.h"
#include "CoffeeEngine/Physics/CollisionCallback.h"
#include "CoffeeEngine/Physics/CollisionSystem.h"
#include "CoffeeEngine/Physics/PhysicsWorld.h"
#include "CoffeeEngine/Renderer/EditorCamera.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include "CoffeeEngine/Renderer/Model.h"
#include "CoffeeEngine/Renderer/Renderer.h"
#include "CoffeeEngine/Renderer/Renderer3D.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/Entity.h"
#include "CoffeeEngine/Scene/SceneCamera.h"
#include "CoffeeEngine/Scene/SceneManager.h"
#include "CoffeeEngine/Scene/SceneTree.h"
#include <CoffeeEngine/Scripting/Script.h>
#include "CoffeeEngine/Scripting/Lua/LuaScript.h"
#include "CoffeeEngine/UI/UIManager.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"

#include <cstdint>
#include <cstdlib>
#include <glm/detail/type_quat.hpp>
#include <glm/fwd.hpp>
#include <memory>
#include <string>
#include <tracy/Tracy.hpp>

#include <cereal/archives/json.hpp>
#include <fstream>



namespace Coffee {

    std::map <UUID, UUID> Scene::s_UUIDMap;
    std::vector<MeshComponent*> Scene::s_MeshComponents;
    std::vector<AnimatorComponent*> Scene::s_AnimatorComponents;

    Scene::Scene()
    {
        m_SceneTree = CreateScope<SceneTree>(this);

        AnimationSystem::ResetAnimators();
    }

    template <typename T>
    static void CopyComponentIfExists(entt::entity destinyEntity, entt::entity sourceEntity, entt::registry& registry)
    {
        if(registry.all_of<T>(sourceEntity))
        {
            auto srcComponent = registry.get<T>(sourceEntity);
            registry.emplace_or_replace<T>(destinyEntity, srcComponent);
        }
    }

    template <>
    void CopyComponentIfExists<ActiveComponent>(entt::entity destinyEntity, entt::entity sourceEntity, entt::registry& registry)
    {
        // ActiveComponent is empty, just place it
        if (!registry.all_of<ActiveComponent>(sourceEntity)) {
            registry.remove<ActiveComponent>(destinyEntity);
        }
    }

    template <>
    void CopyComponentIfExists<StaticComponent>(entt::entity destinyEntity, entt::entity sourceEntity, entt::registry& registry)
    {
        // StaticComponent is empty, just place it
        if (registry.all_of<StaticComponent>(sourceEntity)) {
            registry.emplace<StaticComponent>(destinyEntity);
        }
    }

    template <>
    void CopyComponentIfExists<HierarchyComponent>(entt::entity destinyEntity, entt::entity sourceEntity, entt::registry& registry)
    {
        // We don't need to copy the hierarchy component directly
        // The hierarchy will be set up by SetParent() calls in DuplicateEntityRecursive
        
        // Just make sure we have a fresh hierarchy component
        if (!registry.all_of<HierarchyComponent>(destinyEntity)) {
            registry.emplace<HierarchyComponent>(destinyEntity);
        }
    }

    template <>
    void CopyComponentIfExists<AnimatorComponent>(entt::entity destinyEntity, entt::entity sourceEntity, entt::registry& registry)
    {
        if(registry.all_of<AnimatorComponent>(sourceEntity))
        {
            const auto& srcComponent = registry.get<AnimatorComponent>(sourceEntity);

            AnimatorComponent newComponent = srcComponent;

            newComponent.UpperAnimation = CreateRef<AnimationLayer>(*srcComponent.UpperAnimation);
            newComponent.LowerAnimation = CreateRef<AnimationLayer>(*srcComponent.LowerAnimation);

            AnimationSystem::LoadAnimator(&newComponent);

            UUID newUUID = UUID();
            Scene::s_UUIDMap[srcComponent.animatorUUID] = newUUID;
            newComponent.animatorUUID = newUUID;

            const std::string rootJointName = newComponent.GetSkeleton()->GetJoints()[newComponent.UpperBodyRootJoint].name;
            AnimationSystem::SetupPartialBlending(
                newComponent.UpperAnimation->CurrentAnimation,
                newComponent.LowerAnimation->CurrentAnimation,
                rootJointName,
                &newComponent
            );

            registry.emplace<AnimatorComponent>(destinyEntity, std::move(newComponent));

            Scene::s_AnimatorComponents.push_back(&registry.get<AnimatorComponent>(destinyEntity));
        }
    }

    template <>
    void CopyComponentIfExists<MeshComponent>(entt::entity destinyEntity, entt::entity sourceEntity, entt::registry& registry)
    {
        if(registry.all_of<MeshComponent>(sourceEntity))
        {
            const auto& srcComponent = registry.get<MeshComponent>(sourceEntity);

            MeshComponent newComponent(srcComponent);

            registry.emplace<MeshComponent>(destinyEntity, std::move(newComponent));

            if (newComponent.animator != nullptr)
            {
                Scene::s_MeshComponents.push_back(&registry.get<MeshComponent>(destinyEntity));
            }
        }
    }

    template <>
    void CopyComponentIfExists<AudioSourceComponent>(entt::entity destinyEntity, entt::entity sourceEntity, entt::registry& registry)
    {
        if(registry.all_of<AudioSourceComponent>(sourceEntity))
        {
            const auto& srcComponent = registry.get<AudioSourceComponent>(sourceEntity);

            AudioSourceComponent newComponent = AudioSourceComponent::CreateCopy(srcComponent);

            registry.emplace<AudioSourceComponent>(destinyEntity, std::move(newComponent));

            auto& audioSourceComponent = registry.get<AudioSourceComponent>(destinyEntity);
            Audio::RegisterAudioSourceComponent(audioSourceComponent);
            AudioZone::RegisterObject(audioSourceComponent.gameObjectID, audioSourceComponent.transform[3]);
        }
    }

    template <>
    void CopyComponentIfExists<AudioListenerComponent>(entt::entity destinyEntity, entt::entity sourceEntity, entt::registry& registry)
    {
        if(registry.all_of<AudioListenerComponent>(sourceEntity))
        {
            const auto& srcComponent = registry.get<AudioListenerComponent>(sourceEntity);

            AudioListenerComponent newComponent = AudioListenerComponent::CreateCopy(srcComponent);

            registry.emplace<AudioListenerComponent>(destinyEntity, std::move(newComponent));

            auto& audioListenerComponent = registry.get<AudioListenerComponent>(destinyEntity);
            Audio::RegisterAudioListenerComponent(audioListenerComponent);
        }
    }

    template <>
    void CopyComponentIfExists<RigidbodyComponent>(entt::entity destinyEntity, entt::entity sourceEntity, entt::registry& registry)
    {
        if(registry.all_of<RigidbodyComponent>(sourceEntity))
        {
            const auto& srcComponent = registry.get<RigidbodyComponent>(sourceEntity);

            try {
                RigidBody::Properties props = srcComponent.rb->GetProperties();

                Ref<Collider> collider;
                if (auto boxCollider = std::dynamic_pointer_cast<BoxCollider>(srcComponent.rb->GetCollider())) {
                    collider = CreateRef<BoxCollider>(boxCollider->GetSize());
                }
                else if (auto sphereCollider = std::dynamic_pointer_cast<SphereCollider>(srcComponent.rb->GetCollider())) {
                    collider = CreateRef<SphereCollider>(sphereCollider->GetRadius());
                }
                else if (auto capsuleCollider = std::dynamic_pointer_cast<CapsuleCollider>(srcComponent.rb->GetCollider())) {
                    collider = CreateRef<CapsuleCollider>(capsuleCollider->GetRadius(), capsuleCollider->GetHeight());
                }
                else if (auto cylinderCollider = std::dynamic_pointer_cast<CylinderCollider>(srcComponent.rb->GetCollider())) {
                    collider = CreateRef<CylinderCollider>(cylinderCollider->GetRadius(), cylinderCollider->GetHeight());
                }
                else if (auto coneCollider = std::dynamic_pointer_cast<ConeCollider>(srcComponent.rb->GetCollider())) {
                    collider = CreateRef<ConeCollider>(coneCollider->GetRadius(), coneCollider->GetHeight());
                }
                else {
                    collider = CreateRef<BoxCollider>(glm::vec3(1.0f, 1.0f, 1.0f));
                }

                auto& newComponent = registry.emplace<RigidbodyComponent>(destinyEntity, props, collider);

                newComponent.callback = srcComponent.callback;

                if (registry.all_of<TransformComponent>(destinyEntity)) {
                    auto& transform = registry.get<TransformComponent>(destinyEntity);
                    newComponent.rb->SetPosition(transform.GetLocalPosition());
                    newComponent.rb->SetRotation(transform.GetLocalRotation());
                }
            }
            catch (const std::exception& e) {
                COFFEE_CORE_ERROR("Exception copying rigidbody component: {0}", e.what());
                if (registry.all_of<RigidbodyComponent>(destinyEntity)) {
                    registry.remove<RigidbodyComponent>(destinyEntity);
                }
            }
        }
    }

    template <>
    void CopyComponentIfExists<ParticlesSystemComponent>(entt::entity destinyEntity, entt::entity sourceEntity,
                                              entt::registry& registry){

        if (registry.all_of<ParticlesSystemComponent>(sourceEntity))
            {
            auto& srcComponent = registry.get<ParticlesSystemComponent>(sourceEntity);
            ParticlesSystemComponent newComponent(srcComponent);
            newComponent.SetParticleEmitter(CreateRef<ParticleEmitter>(*srcComponent.GetParticleEmitter()));
            registry.emplace<ParticlesSystemComponent>(destinyEntity, std::move(newComponent));
            
        }
    }

    template <typename... Components>
    static void CopyEntity(entt::entity destinyEntity, entt::entity sourceEntity, entt::registry& registry)
    {
        (CopyComponentIfExists<Components>(destinyEntity, sourceEntity, registry), ...);
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        ZoneScoped;

        Entity entity = { m_Registry.create(), this };
        entity.AddComponent<TransformComponent>();
        auto& nameTag = entity.AddComponent<TagComponent>();
        nameTag.Tag = name.empty() ? "Entity" : name;
        entity.AddComponent<HierarchyComponent>();
        entity.AddComponent<ActiveComponent>();
        return entity;
    }

    Entity Scene::DuplicateEntityRecursive(Entity& sourceEntity, Entity* parentEntity = nullptr)
    {
        Entity newEntity = CreateEntity(sourceEntity.GetComponent<TagComponent>().Tag);
        CopyEntity<ALL_COMPONENTS>(newEntity, sourceEntity, m_Registry);

        if (parentEntity)
            newEntity.SetParent(*parentEntity);
        
        auto children = sourceEntity.GetChildren();
        for (auto& child : children)
        {
            DuplicateEntityRecursive(child, &newEntity);
        }
        
        return newEntity;
    }
    
    Entity Scene::Duplicate(Entity& entity)
    {
        s_AnimatorComponents.clear();
        s_MeshComponents.clear();
        s_UUIDMap.clear();

        Entity duplicatedEntity = DuplicateEntityRecursive(entity);

        if (s_AnimatorComponents.empty())
            return duplicatedEntity;

        for (auto mesh : s_MeshComponents)
        {
            auto it = s_UUIDMap.find(mesh->animator->animatorUUID);
            if (it != s_UUIDMap.end())
            {
                for (const auto& animator : s_AnimatorComponents)
                {
                    if (animator->animatorUUID == it->second)
                    {
                        mesh->animatorUUID = animator->animatorUUID;
                        mesh->animator = animator;
                        break;
                    }
                }
            }
        }

        return duplicatedEntity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        // TODO think where we can put this. If we don't remove first the rigidbody from the physics world we will have a crash
        if (entity.HasComponent<RigidbodyComponent>())
        {
            auto& rbComponent = entity.GetComponent<RigidbodyComponent>();
            if (rbComponent.rb && rbComponent.rb->GetNativeBody())
            {
                m_PhysicsWorld.removeRigidBody(rbComponent.rb->GetNativeBody());
                rbComponent.rb->GetNativeBody()->setUserPointer(nullptr);
                rbComponent.rb.reset();
            }
        }

        if (entity.HasComponent<AudioSourceComponent>())
        {
            auto& audioSourceComponent = entity.GetComponent<AudioSourceComponent>();
            Audio::UnregisterAudioSourceComponent(audioSourceComponent);
        }

        if (entity.HasComponent<AudioListenerComponent>())
        {
            auto& audioListenerComponent = entity.GetComponent<AudioListenerComponent>();
            Audio::UnregisterAudioListenerComponent(audioListenerComponent);
        }

        auto& hierarchyComponent = m_Registry.get<HierarchyComponent>(entity);
        auto curr = hierarchyComponent.m_First;

        while(curr != entt::null)
        {
            Entity e{curr, this};
            curr = m_Registry.get<HierarchyComponent>(curr).m_Next;
            DestroyEntity(e);
        }

        // Finally destroy the entity itself
        m_Registry.destroy((entt::entity)entity);
    }

    Entity Scene::GetEntityByName(const std::string& name)
    {
        auto view = m_Registry.view<TagComponent>();

        for(auto entity : view)
        {
            auto& tag = view.get<TagComponent>(entity).Tag;
            if(tag == name)
                return Entity{entity, this};
        }

        return Entity{entt::null, this};
    }

    std::vector<Entity> Scene::GetAllEntities()
    {
        std::vector<Entity> entities;

        auto view = m_Registry.view<entt::entity>();

        for(auto entity : view)
        {
            entities.push_back(Entity{entity, this});
        }

        return entities;
    }

    void Scene::OnInitEditor()
    {
        ZoneScoped;

        CollisionSystem::Initialize(this);

        auto audioListenerView = m_Registry.view<AudioListenerComponent>();
        for (auto& entity : audioListenerView)
        {
            auto& audioListenerComponent = audioListenerView.get<AudioListenerComponent>(entity);
            Audio::RegisterAudioListenerComponent(audioListenerComponent);
        }
        auto audioSourceView = m_Registry.view<AudioSourceComponent>();
        for (auto& entity : audioSourceView)
        {
            auto& audioSourceComponent = audioSourceView.get<AudioSourceComponent>(entity);
            Audio::RegisterAudioSourceComponent(audioSourceComponent);
            AudioZone::RegisterObject(audioSourceComponent.gameObjectID, audioSourceComponent.transform[3]);
        }
    }

    void Scene::OnInitRuntime()
    {
        ZoneScoped;

        m_SceneTree->Update();

        CollisionSystem::Initialize(this);

        auto staticView = m_Registry.view<StaticComponent, TransformComponent>();

        // Create octree bounds based on the static entities
        glm::vec3 minOctreeBounds = glm::vec3(0.0f);
        glm::vec3 maxOctreeBounds = glm::vec3(0.0f);

        for (auto entity : staticView)
        {
            auto& transformComponent = staticView.get<TransformComponent>(entity);
            auto meshComponent = m_Registry.try_get<MeshComponent>(entity);

            AABB aabb;

            if(meshComponent)
            {
                Ref<Mesh> mesh = meshComponent->GetMesh();
                aabb = mesh->GetAABB();
            }
            else
            {
                aabb = AABB(glm::vec3(-0.5f), glm::vec3(0.5f));
            }

            AABB transformedAABB = aabb.CalculateTransformedAABB(transformComponent.GetWorldTransform());

            minOctreeBounds = glm::min(minOctreeBounds, transformedAABB.min);
            maxOctreeBounds = glm::max(maxOctreeBounds, transformedAABB.max);


        }

        m_Octree = CreateScope<Octree<entt::entity>>(AABB(minOctreeBounds, maxOctreeBounds), 10, 5);

        // Static entities octree
        for (auto entity : staticView)
        {
            auto& transformComponent = staticView.get<TransformComponent>(entity);
            auto meshComponent = m_Registry.try_get<MeshComponent>(entity);

            AABB aabb;

            if(meshComponent)
            {
                Ref<Mesh> mesh = meshComponent->GetMesh();
                aabb = mesh->GetAABB();
            }
            else
            {
                aabb = AABB(glm::vec3(-0.5f), glm::vec3(0.5f));
            }

            Ref<ObjectContainer<entt::entity>> object = CreateRef<ObjectContainer<entt::entity>>(transformComponent.GetWorldTransform(), aabb, entity);
            m_Octree->Insert(object);
        }

        auto audioListenerView = m_Registry.view<AudioListenerComponent>();
        for (auto& entity : audioListenerView)
        {
            auto& audioListenerComponent = audioListenerView.get<AudioListenerComponent>(entity);
            Audio::RegisterAudioListenerComponent(audioListenerComponent);
        }
        auto audioSourceView = m_Registry.view<AudioSourceComponent>();
        for (auto& entity : audioSourceView)
        {
            auto& audioSourceComponent = audioSourceView.get<AudioSourceComponent>(entity);
            Audio::RegisterAudioSourceComponent(audioSourceComponent);
            AudioZone::RegisterObject(audioSourceComponent.gameObjectID, audioSourceComponent.transform[3]);
        }
        Audio::PlayInitialAudios();

        // Get all entities with ScriptComponent
        auto scriptView = m_Registry.view<ScriptComponent>();

        for (auto& entity : scriptView)
        {
            Entity scriptEntity{entity, this};

            auto& scriptComponent = scriptView.get<ScriptComponent>(entity);

            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable("self", scriptEntity);
            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable("current_scene", this);

            scriptComponent.script->OnReady();
        }
    }

    void Scene::OnUpdateEditor(EditorCamera& camera, float dt)
    {
        ZoneScoped;

        m_SceneTree->Update();

        Renderer::GetCurrentRenderTarget()->SetCamera(camera, glm::inverse(camera.GetViewMatrix()));

        // TODO test change cubemap
        auto cubemapView = m_Registry.view<WorldEnvironmentComponent>();
        if (!cubemapView.empty<WorldEnvironmentComponent>())
        {
            auto& firstWorldEnv = cubemapView.get<WorldEnvironmentComponent>(cubemapView.front());
            Renderer3D::SetEnvironmentMap(firstWorldEnv.Skybox);
            Renderer3D::GetRenderSettings().EnvironmentExposure = firstWorldEnv.SkyboxIntensity;

            // FOG
            Renderer3D::GetRenderSettings().DepthFog = firstWorldEnv.Fog;
            Renderer3D::GetRenderSettings().FogColor = firstWorldEnv.FogColor;
            Renderer3D::GetRenderSettings().FogDensity = firstWorldEnv.FogDensity;
            Renderer3D::GetRenderSettings().FogHeight = firstWorldEnv.FogHeight;
            Renderer3D::GetRenderSettings().FogHeightDensity = firstWorldEnv.FogHeightDensity;

            // Bloom
            Renderer3D::GetRenderSettings().Bloom = firstWorldEnv.Bloom;
            Renderer3D::GetRenderSettings().BloomIntensity = firstWorldEnv.BloomIntensity;
            Renderer3D::GetRenderSettings().BloomRadius = firstWorldEnv.BloomRadius;
            Renderer3D::GetRenderSettings().BloomMaxMipLevels = firstWorldEnv.BloomMaxMipLevels;
        }

        // TEMPORAL - Navigation
        {
            auto navMeshView = m_Registry.view<ActiveComponent, NavMeshComponent>();
            ZoneScopedN("NavMeshComponent View");

            for (auto& entity : navMeshView)
            {
                auto& navMeshComponent = navMeshView.get<NavMeshComponent>(entity);
                if (navMeshComponent.ShowDebug && navMeshComponent.GetNavMesh() &&
                    navMeshComponent.GetNavMesh()->IsCalculated())
                {
                    navMeshComponent.GetNavMesh()->RenderWalkableAreas();
                }
            }
        }


        {
            auto viewRigidbody = m_Registry.view<ActiveComponent, RigidbodyComponent, TransformComponent>();
            ZoneScopedN("RigidbodyComponent View");

            for (auto entity : viewRigidbody)
            {
                auto [rb, transform] = viewRigidbody.get<RigidbodyComponent, TransformComponent>(entity);
                if (rb.rb)
                {
                    rb.rb->SetPosition(transform.GetLocalPosition());
                    rb.rb->SetRotation(transform.GetLocalRotation());
                }
            }
        }

        {
            auto animatorView = m_Registry.view<ActiveComponent, AnimatorComponent>();
            ZoneScopedN("AnimatorComponent View");

            for (auto& entity : animatorView)
            {
                AnimatorComponent* animatorComponent = &animatorView.get<AnimatorComponent>(entity);
                if (animatorComponent->NeedsUpdate)
                {
                    AnimationSystem::Update(dt, animatorComponent);
                    animatorComponent->NeedsUpdate = false;
                }
            }
        }

        {
            ZoneScopedN("UpdateAudioComponentsPositions");
            UpdateAudioComponentsPositions();
        }

        {
            // Get all entities with ModelComponent and TransformComponent
            auto view = m_Registry.view<ActiveComponent, MeshComponent, TransformComponent>();
            ZoneScopedN("MeshComponent View");

            // Loop through each entity with the specified components
            for (auto& entity : view)
            {
                // Get the ModelComponent and TransformComponent for the current entity
                auto& meshComponent = view.get<MeshComponent>(entity);
                auto& transformComponent = view.get<TransformComponent>(entity);
                auto materialComponent = m_Registry.try_get<MaterialComponent>(entity);

                Ref<Mesh> mesh = meshComponent.GetMesh();
                Ref<Material> material = (materialComponent) ? materialComponent->material : nullptr;

                //Renderer::Submit(material, mesh, transformComponent.GetWorldTransform(), (uint32_t)entity);
                Renderer3D::Submit(RenderCommand{transformComponent.GetWorldTransform(), mesh, material, (uint32_t)entity, meshComponent.animator});
            }
        }

        {
            //Get all entities with LightComponent and TransformComponent
            auto lightView = m_Registry.view<ActiveComponent, LightComponent, TransformComponent>();
            ZoneScopedN("LightComponent View");

            //Loop through each entity with the specified components
            for(auto& entity : lightView)
            {
                auto& lightComponent = lightView.get<LightComponent>(entity);
                auto& transformComponent = lightView.get<TransformComponent>(entity);

                lightComponent.Position = transformComponent.GetWorldTransform()[3];
                lightComponent.Direction = glm::normalize(glm::vec3(-transformComponent.GetWorldTransform()[1]));

                Renderer3D::Submit(lightComponent);
            }
        }

        {
            // Get all entities with ParticlesSystemComponent and TransformComponent
            auto particleSystemView = m_Registry.view<ActiveComponent, ParticlesSystemComponent, TransformComponent>();
            ZoneScopedN("ParticlesSystemComponent View");

            for (auto& entity : particleSystemView)
            {
                auto& particlesSystemComponent = particleSystemView.get<ParticlesSystemComponent>(entity);
                auto& transformComponent = particleSystemView.get<TransformComponent>(entity);
                if (particlesSystemComponent.NeedsUpdate)
                {
                    particlesSystemComponent.NeedsUpdate = false;
                    particlesSystemComponent.GetParticleEmitter()->transformComponentMatrix = transformComponent.GetWorldTransform();
                    particlesSystemComponent.GetParticleEmitter()->cameraViewMatrix = camera.GetViewMatrix();
                    particlesSystemComponent.GetParticleEmitter()->Update(dt);
                    particlesSystemComponent.GetParticleEmitter()->DrawDebug();
                }
                else {
                    Renderer2D::DrawQuad(transformComponent.GetWorldTransform(),
                        particlesSystemComponent.GetParticleEmitter()->particleTexture, 1,
                        particlesSystemComponent.GetParticleEmitter()->colorNormal, Renderer2D::RenderMode::World);
                }
            }
        }

        {
            auto spriteView = m_Registry.view<ActiveComponent, SpriteComponent, TransformComponent>();
            ZoneScopedN("SpriteComponent View");

            for (auto& entity : spriteView)
            {
                auto& spriteComponent = spriteView.get<SpriteComponent>(entity);
                auto& transformComponent = spriteView.get<TransformComponent>(entity);

                if (spriteComponent.texture)
                {
                    Renderer2D::DrawQuad(transformComponent.GetWorldTransform(), spriteComponent.texture,
                                         spriteComponent.tilingFactor, spriteComponent.tintColor,
                                         Renderer2D::RenderMode::World);
                }
            }
        }

        {
            ZoneScopedN("UIManager::UpdateUI");
            UIManager::UpdateUI(m_Registry);
        }

        // Debug Draw
        // if (m_SceneDebugFlags.ShowOctree) m_Octree->DebugDraw();
        if (m_SceneDebugFlags.ShowColliders) m_PhysicsWorld.drawCollisionShapes();
        if (m_SceneDebugFlags.ShowNavMesh) {
            auto navMeshViewDebug = m_Registry.view<ActiveComponent, NavMeshComponent>();
            ZoneScopedN("NavMesh Debug View");

            for (auto& entity : navMeshViewDebug) {
                auto& navMeshComponent = navMeshViewDebug.get<NavMeshComponent>(entity);
                if (navMeshComponent.GetNavMesh() && navMeshComponent.GetNavMesh()->IsCalculated()) {
                    navMeshComponent.ShowDebug = m_SceneDebugFlags.ShowNavMesh;
                }
            }
        } else {
            auto navMeshViewDebug = m_Registry.view<ActiveComponent, NavMeshComponent>();
            ZoneScopedN("NavMesh Debug View - Disable");

            for (auto& entity : navMeshViewDebug) {
                auto& navMeshComponent = navMeshViewDebug.get<NavMeshComponent>(entity);
                navMeshComponent.ShowDebug = false;
            }
        }

        if (m_SceneDebugFlags.ShowNavMeshPath) {
            auto navigationAgentViewDebug = m_Registry.view<ActiveComponent, NavigationAgentComponent>();
            ZoneScopedN("NavigationAgent Debug View");

            for (auto& agent : navigationAgentViewDebug) {
                auto& navAgentComponent = navigationAgentViewDebug.get<NavigationAgentComponent>(agent);
                if (navAgentComponent.GetNavMeshComponent())
                    navAgentComponent.ShowDebug = m_SceneDebugFlags.ShowNavMeshPath;
            }
        } else {
            auto navigationAgentViewDebug = m_Registry.view<ActiveComponent, NavigationAgentComponent>();
            ZoneScopedN("NavigationAgent Debug View - Disable");

            for (auto& agent : navigationAgentViewDebug) {
                auto& navAgentComponent = navigationAgentViewDebug.get<NavigationAgentComponent>(agent);
                navAgentComponent.ShowDebug = false;
            }
        }
    }

    void Scene::OnUpdateRuntime(float dt)
    {
        ZoneScoped;

        m_SceneTree->Update();

        auto cubemapView = m_Registry.view<WorldEnvironmentComponent>();
        if (!cubemapView.empty<WorldEnvironmentComponent>())
        {
            auto& firstWorldEnv = cubemapView.get<WorldEnvironmentComponent>(cubemapView.front());
            Renderer3D::SetEnvironmentMap(firstWorldEnv.Skybox);
            Renderer3D::GetRenderSettings().EnvironmentExposure = firstWorldEnv.SkyboxIntensity;

            // FOG
            Renderer3D::GetRenderSettings().DepthFog = firstWorldEnv.Fog;
            Renderer3D::GetRenderSettings().FogColor = firstWorldEnv.FogColor;
            Renderer3D::GetRenderSettings().FogDensity = firstWorldEnv.FogDensity;
            Renderer3D::GetRenderSettings().FogHeight = firstWorldEnv.FogHeight;
            Renderer3D::GetRenderSettings().FogHeightDensity = firstWorldEnv.FogHeightDensity;

            // Bloom
            Renderer3D::GetRenderSettings().Bloom = firstWorldEnv.Bloom;
            Renderer3D::GetRenderSettings().BloomIntensity = firstWorldEnv.BloomIntensity;
            Renderer3D::GetRenderSettings().BloomRadius = firstWorldEnv.BloomRadius;
            Renderer3D::GetRenderSettings().BloomMaxMipLevels = firstWorldEnv.BloomMaxMipLevels;
        }

        Camera* camera = nullptr;
        glm::mat4 cameraTransform;
        {
            auto cameraView = m_Registry.view<ActiveComponent, TransformComponent, CameraComponent>();
            ZoneScopedN("CameraComponent View");

            for (auto entity : cameraView)
            {
                auto [transform, cameraComponent] = cameraView.get<TransformComponent, CameraComponent>(entity);

                // TODO: Multiple cameras support (for now, the last camera found will be used)
                camera = &cameraComponent.Camera;
                cameraTransform = transform.GetWorldTransform();
            }
        }

        if (!camera)
        {
            COFFEE_ERROR("No camera entity found!");

            SceneCamera sceneCamera;
            camera = &sceneCamera;

            cameraTransform = glm::mat4(1.0f);
        }

        {
            auto navMeshView = m_Registry.view<ActiveComponent, NavMeshComponent>();
            ZoneScopedN("NavMeshComponent View");

            for (auto& entity : navMeshView)
            {
                auto& navMeshComponent = navMeshView.get<NavMeshComponent>(entity);
                if (navMeshComponent.ShowDebug && navMeshComponent.GetNavMesh() && navMeshComponent.GetNavMesh()->IsCalculated())
                {
                    navMeshComponent.GetNavMesh()->RenderWalkableAreas();
                }
            }
        }

        {
            auto navigationAgentView = m_Registry.view<ActiveComponent, NavigationAgentComponent>();
            ZoneScopedN("NavigationAgentComponent View");

            for (auto& agent : navigationAgentView)
            {
                auto& navAgentComponent = navigationAgentView.get<NavigationAgentComponent>(agent);
                if (navAgentComponent.ShowDebug && navAgentComponent.GetPathFinder())
                    navAgentComponent.GetPathFinder()->RenderPath(navAgentComponent.Path);
            }
        }

        m_PhysicsWorld.stepSimulation(dt);

        {
            // Update transforms from physics
            auto viewPhysics = m_Registry.view<ActiveComponent, RigidbodyComponent, TransformComponent>();
            ZoneScopedN("Physics Update View");

            for (auto entity : viewPhysics) {
                auto [rb, transform] = viewPhysics.get<RigidbodyComponent, TransformComponent>(entity);
                if (rb.rb) {
                    transform.SetLocalPosition(rb.rb->GetPosition());
                    transform.SetLocalRotation(rb.rb->GetRotation());
                }
            }
        }

        {
            ZoneScopedN("UpdateAudioComponentsPositions");
            // TODO: Ask to Guillem if this is a candidate for frustum culling
            UpdateAudioComponentsPositions();
        }


        // Get all the static entities from the Octree
        Frustum frustum = Frustum(camera->GetProjection() * glm::inverse(cameraTransform));
        auto visibleStaticEntities = m_Octree->Query(frustum);
        std::unordered_set<entt::entity> visibleEntitySet(visibleStaticEntities.begin(), visibleStaticEntities.end());

        auto staticView = m_Registry.view<StaticComponent>();
        {
            // Get all entities with ScriptComponent
            auto scriptView = m_Registry.view<ActiveComponent, ScriptComponent>();
            ZoneScopedN("ScriptComponent View");

            for (auto& entity : scriptView)
            {
                /*if (staticView.contains(entity) && visibleEntitySet.find(entity) == visibleEntitySet.end())
                    continue;*/

                auto& scriptComponent = scriptView.get<ScriptComponent>(entity);
                ZoneScoped;
                //ZoneText(scriptComponent.script->GetPath().filename().string().c_str(), scriptComponent.script->GetPath().filename().string().length());
                scriptComponent.script->OnUpdate(dt);
                if(SceneManager::GetActiveScene().get() != this)
                    return;
            }
        }

        if(SceneManager::GetActiveScene().get() != this)
            return;

        //TODO: Add this to a function bc it is repeated in OnUpdateEditor
        Renderer::GetCurrentRenderTarget()->SetCamera(*camera, cameraTransform);

        {
            auto animatorView = m_Registry.view<ActiveComponent, AnimatorComponent>();
            ZoneScopedN("AnimatorComponent View");

            for (auto& entity : animatorView)
            {
                /*if (staticView.contains(entity) && visibleEntitySet.find(entity) == visibleEntitySet.end())
                    continue;*/

                AnimatorComponent* animatorComponent = &animatorView.get<AnimatorComponent>(entity);
                AnimationSystem::Update(dt, animatorComponent);
            }
        }

        {
            // Get all entities with ModelComponent and TransformComponent
            auto view = m_Registry.view<ActiveComponent, MeshComponent, TransformComponent>();
            ZoneScopedN("MeshComponent View");

            // Loop through each entity with the specified components
            for (auto& entity : view)
            {
                if (staticView.contains(entity) && visibleEntitySet.find(entity) == visibleEntitySet.end())
                    continue;

                // Get the ModelComponent and TransformComponent for the current entity
                auto& meshComponent = view.get<MeshComponent>(entity);
                auto& transformComponent = view.get<TransformComponent>(entity);
                auto materialComponent = m_Registry.try_get<MaterialComponent>(entity);

                Ref<Mesh> mesh = meshComponent.GetMesh();
                Ref<Material> material = (materialComponent) ? materialComponent->material : nullptr;

                Renderer3D::Submit(RenderCommand{transformComponent.GetWorldTransform(), mesh, material, (uint32_t)entity, meshComponent.animator});
            }
        }

        {
            //Get all entities with LightComponent and TransformComponent
            auto lightView = m_Registry.view<ActiveComponent, LightComponent, TransformComponent>();
            ZoneScopedN("LightComponent View");

            //Loop through each entity with the specified components
            for(auto& entity : lightView)
            {
                if (staticView.contains(entity) && visibleEntitySet.find(entity) == visibleEntitySet.end())
                    continue;

                auto& lightComponent = lightView.get<LightComponent>(entity);
                auto& transformComponent = lightView.get<TransformComponent>(entity);

                lightComponent.Position = transformComponent.GetWorldTransform()[3];
                lightComponent.Direction = glm::normalize(glm::vec3(-transformComponent.GetWorldTransform()[1]));

                Renderer3D::Submit(lightComponent);
            }
        }

        {
            // Get all entities with ParticlesSystemComponent and TransformComponent
            auto particleSystemView = m_Registry.view<ActiveComponent, ParticlesSystemComponent, TransformComponent>();
            ZoneScopedN("ParticlesSystemComponent View");

            for (auto& entity : particleSystemView)
            {
                if (staticView.contains(entity) && visibleEntitySet.find(entity) == visibleEntitySet.end())
                    continue;

                auto& particlesSystemComponent = particleSystemView.get<ParticlesSystemComponent>(entity);
                auto& transformComponent = particleSystemView.get<TransformComponent>(entity);

                particlesSystemComponent.GetParticleEmitter()->transformComponentMatrix = transformComponent.GetWorldTransform();
                particlesSystemComponent.GetParticleEmitter()->cameraViewMatrix = glm::inverse(cameraTransform);
                particlesSystemComponent.GetParticleEmitter()->Update(dt);
            }
        }

        {
            auto spriteView = m_Registry.view<ActiveComponent, SpriteComponent, TransformComponent>();
            ZoneScopedN("SpriteComponent View");

            for (auto& entity : spriteView)
            {
                if (staticView.contains(entity) && visibleEntitySet.find(entity) == visibleEntitySet.end())
                    continue;

                auto& spriteComponent = spriteView.get<SpriteComponent>(entity);
                auto& transformComponent = spriteView.get<TransformComponent>(entity);

                if (spriteComponent.texture) {
                    Renderer2D::DrawQuad(transformComponent.GetWorldTransform(), spriteComponent.texture,
                                         spriteComponent.tilingFactor, spriteComponent.tintColor,
                                         Renderer2D::RenderMode::World);
                }
            }
        }

        // Debug Draw
        if (m_SceneDebugFlags.ShowOctree) m_Octree->DebugDraw();
        if (m_SceneDebugFlags.ShowColliders) m_PhysicsWorld.drawCollisionShapes();
        if (m_SceneDebugFlags.ShowNavMesh) {
            auto navMeshViewDebug = m_Registry.view<ActiveComponent, NavMeshComponent>();
            ZoneScopedN("NavMesh Debug View");

            for (auto& entity : navMeshViewDebug) {
                auto& navMeshComponent = navMeshViewDebug.get<NavMeshComponent>(entity);
                if (navMeshComponent.GetNavMesh() && navMeshComponent.GetNavMesh()->IsCalculated()) {
                    navMeshComponent.ShowDebug = m_SceneDebugFlags.ShowNavMesh;
                }
            }
        } else {
            auto navMeshViewDebug = m_Registry.view<ActiveComponent, NavMeshComponent>();
            ZoneScopedN("NavMesh Debug View - Disable");

            for (auto& entity : navMeshViewDebug) {
                auto& navMeshComponent = navMeshViewDebug.get<NavMeshComponent>(entity);
                navMeshComponent.ShowDebug = false;
            }
        }

        if (m_SceneDebugFlags.ShowNavMeshPath) {
            auto navigationAgentViewDebug = m_Registry.view<ActiveComponent, NavigationAgentComponent>();
            ZoneScopedN("NavigationAgent Debug View");

            for (auto& agent : navigationAgentViewDebug) {
                auto& navAgentComponent = navigationAgentViewDebug.get<NavigationAgentComponent>(agent);
                if (navAgentComponent.GetNavMeshComponent())
                    navAgentComponent.ShowDebug = m_SceneDebugFlags.ShowNavMeshPath;
            }
        } else {
            auto navigationAgentViewDebug = m_Registry.view<ActiveComponent, NavigationAgentComponent>();
            ZoneScopedN("NavigationAgent Debug View - Disable");

            for (auto& agent : navigationAgentViewDebug) {
                auto& navAgentComponent = navigationAgentViewDebug.get<NavigationAgentComponent>(agent);
                navAgentComponent.ShowDebug = false;
            }
        }

        UIManager::UpdateUI(m_Registry);
    }

    void Scene::OnEvent(Event& e)
    {
        ZoneScoped;
    }

    void Scene::OnExitEditor()
    {
        ZoneScoped;

        auto view = m_Registry.view<ScriptComponent>();
        for (auto entity : view)
        {
            auto& scriptComponent = view.get<ScriptComponent>(entity);
            if (scriptComponent.script)
            {
                scriptComponent.script->OnExit();
                scriptComponent.script.reset();
            }
        }
    }

    void Scene::OnExitRuntime()
    {
        // Clear collision system state
        CollisionSystem::Shutdown();
    }

    Ref<Scene> Scene::Load(const std::filesystem::path& path)
    {
        ZoneScoped;

        Ref<Scene> scene = CreateRef<Scene>();

        std::ifstream sceneFile(path);
        cereal::JSONInputArchive archive(sceneFile);

        archive(*scene);

        scene->m_FilePath = path;

        // TODO: Think where this could be done instead of the Load function

        // Add rigidbodies back to physics world
        auto view = scene->m_Registry.view<ActiveComponent, RigidbodyComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto [rb, transform] = view.get<RigidbodyComponent, TransformComponent>(entity);
            if (rb.rb && rb.rb->GetNativeBody())
            {
                // Set initial transform
                rb.rb->SetPosition(transform.GetLocalPosition());
                rb.rb->SetRotation(transform.GetLocalRotation());

                // Add to physics world
                scene->m_PhysicsWorld.addRigidBody(rb.rb->GetNativeBody());

                // Set user pointer for collision callbacks
                rb.rb->GetNativeBody()->setUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)));
            }
        }

        // TODO: Think where this could be done instead of the Load function
        for (auto& audioSource : Audio::audioSources)
        {
            Audio::SetVolume(audioSource->gameObjectID, audioSource->mute ? 0.f : audioSource->volume);
        }

        return scene;
    }

    void Scene::Save(const std::filesystem::path& path, Ref<Scene> scene)
    {
        ZoneScoped;

        std::ofstream sceneFile(path);
        cereal::JSONOutputArchive archive(sceneFile);

        archive(*scene);

        scene->m_FilePath = path;
    }

    // Is possible that this function will be moved to the SceneTreePanel but for now it will stay here
    void AddModelToTheSceneTree(Scene* scene, Ref<Model> model, AnimatorComponent* animatorComponent)
    {
        static Entity parent;

        Entity modelEntity = scene->CreateEntity(model->GetName());

        if (model->HasAnimations())
        {
            animatorComponent = &modelEntity.AddComponent<AnimatorComponent>(model->GetSkeleton(), model->GetAnimationController());

            std::string jointName = "Chest";
            const auto& joints = animatorComponent->GetSkeleton()->GetJoints();
            auto it = std::ranges::find_if(joints, [](const auto& joint) { return joint.name == "Chest"; });
            if (it == joints.end())
                jointName = joints[0].name;

            AnimationSystem::SetupPartialBlending(0, 0, jointName, animatorComponent);

            animatorComponent->modelUUID = model->GetUUID();
            animatorComponent->animatorUUID = UUID();
        }

        if((entt::entity)parent != entt::null)modelEntity.SetParent(parent);
        modelEntity.GetComponent<TransformComponent>().SetLocalTransform(model->GetTransform());

        auto& meshes = model->GetMeshes();
        bool hasMultipleMeshes = meshes.size() > 1;

        for(auto& mesh : meshes)
        {
            Entity entity = hasMultipleMeshes ? scene->CreateEntity(mesh->GetName()) : modelEntity;

            entity.AddComponent<MeshComponent>(mesh);

            if (animatorComponent)
            {
                entity.GetComponent<MeshComponent>().animator = animatorComponent;
                entity.GetComponent<MeshComponent>().animatorUUID = animatorComponent->animatorUUID;
            }

            if(mesh->GetMaterial())
            {
                entity.AddComponent<MaterialComponent>(mesh->GetMaterial());
            }

            if(hasMultipleMeshes)
            {
                entity.SetParent(modelEntity);
            }
        }

        for(auto& c : model->GetChildren())
        {
            parent = modelEntity;
            AddModelToTheSceneTree(scene, c, animatorComponent);
        }

        parent = Entity{entt::null, scene};
    }

    void Scene::AssignAnimatorsToMeshes(const std::vector<AnimatorComponent*> animators)
    {
        std::vector<Entity> entities = GetAllEntities();
        for (auto entity : entities)
        {
            if (entity.HasComponent<MeshComponent>())
            {
                for (auto animator : animators)
                {
                    MeshComponent* meshComponent = &entity.GetComponent<MeshComponent>();
                    if (meshComponent->animatorUUID == animator->animatorUUID && !meshComponent->animator)
                        meshComponent->animator = animator;
                }
            }
        }
    }
    void Scene::UpdateAudioComponentsPositions()
    {
        ZoneScopedN("Scene::UpdateAudioComponentsPositions");

        auto audioSourceView = m_Registry.view<AudioSourceComponent, TransformComponent>();

        for (auto& entity : audioSourceView)
        {
            auto& audioSourceComponent = audioSourceView.get<AudioSourceComponent>(entity);
            auto& transformComponent = audioSourceView.get<TransformComponent>(entity);

            if (audioSourceComponent.transform != transformComponent.GetWorldTransform())
            {
                audioSourceComponent.transform = transformComponent.GetWorldTransform();

                Audio::Set3DPosition(audioSourceComponent.gameObjectID,
                transformComponent.GetWorldTransform()[3],
                glm::normalize(glm::vec3(transformComponent.GetWorldTransform()[2])),
                glm::normalize(glm::vec3(transformComponent.GetWorldTransform()[1]))
                );
                AudioZone::UpdateObjectPosition(audioSourceComponent.gameObjectID, transformComponent.GetWorldTransform()[3]);
            }
        }

        auto audioListenerView = m_Registry.view<AudioListenerComponent, TransformComponent>();

        for (auto& entity : audioListenerView)
        {
            auto& audioListenerComponent = audioListenerView.get<AudioListenerComponent>(entity);
            auto& transformComponent = audioListenerView.get<TransformComponent>(entity);

            if (audioListenerComponent.transform != transformComponent.GetWorldTransform())
            {
                audioListenerComponent.transform = transformComponent.GetWorldTransform();

                Audio::Set3DPosition(audioListenerComponent.gameObjectID,
                    transformComponent.GetWorldTransform()[3],
                    glm::normalize(glm::vec3(transformComponent.GetWorldTransform()[2])),
                    glm::normalize(glm::vec3(transformComponent.GetWorldTransform()[1]))
                );
            }
        }
    }
}
