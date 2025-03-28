#include "ParticleManager.h"
#include <cstdlib>
#include <ctime>
#include <glm/gtc/random.hpp>
#include <CoffeeEngine/Renderer/Renderer3D.h>
#include "CoffeeEngine/Renderer/Renderer2D.h"

namespace Coffee
{
    Particle::Particle() : transformMatrix(glm::mat4(1.0f)), direction(0.0f), color(1.0f), size(1.0f), lifetime(1.0f) {}

    glm::mat4 Particle::GetWorldTransform() const
    {
        return transformMatrix;
    }

    void Particle::SetPosition(glm::vec3 position)
    {
        transformMatrix[3] = glm::vec4(position, 1.0f);
    }

    void Particle::SetRotation(glm::vec3 rotation)
    {
        glm::vec3 position = GetPosition();
        glm::vec3 scale = GetSize();

        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, rotation.x, glm::vec3(1, 0, 0));
        rotationMatrix = glm::rotate(rotationMatrix, rotation.y, glm::vec3(0, 1, 0));
        rotationMatrix = glm::rotate(rotationMatrix, rotation.z, glm::vec3(0, 0, 1));

        transformMatrix = glm::scale(rotationMatrix, scale);
        SetPosition(position);
    }

    void Particle::SetSize(glm::vec3 scale)
    {
        glm::vec3 position = GetPosition();
        glm::mat3 rotationMatrix = glm::mat3(glm::normalize(transformMatrix[0]), glm::normalize(transformMatrix[1]),
                                             glm::normalize(transformMatrix[2]));
        transformMatrix = glm::mat4(rotationMatrix);
        transformMatrix = glm::scale(transformMatrix, scale);
        SetPosition(position);
    }

    glm::vec3 Particle::GetPosition() const
    {
        return glm::vec3(transformMatrix[3]);
    }

    glm::vec3 Particle::GetRotation() const
    {
        return glm::vec3(atan2(transformMatrix[1][2], transformMatrix[2][2]),
                         atan2(-transformMatrix[0][2], sqrt(transformMatrix[1][2] * transformMatrix[1][2] +
                                                            transformMatrix[2][2] * transformMatrix[2][2])),
                         atan2(transformMatrix[0][1], transformMatrix[0][0]));
    }

    glm::vec3 Particle::GetSize() const
    {
        return glm::vec3(glm::length(glm::vec3(transformMatrix[0])), glm::length(glm::vec3(transformMatrix[1])),
                         glm::length(glm::vec3(transformMatrix[2])));
    }

    Ref<Mesh> ParticleEmitter::particleMesh = nullptr;

    ParticleEmitter::ParticleEmitter()
    {
        if (!particleMesh)
        {
            particleMesh = Coffee::PrimitiveMesh::CreatePlane(glm::vec2(1,1));
        }
        particleMaterial = CreateRef<Material>("Default Particle Material");
    }

    void ParticleEmitter::InitParticle(Ref<Particle> particle)
    {
        glm::vec3 startPos = glm::linearRand(minSpread, maxSpread);
        glm::vec4 startPosVec4 = glm::vec4(startPos.x, startPos.y, startPos.z, 0);

        glm::mat4 auxMatrix = transformComponentMatrix;
        auxMatrix[3] = transformComponentMatrix[3] + startPosVec4;

        particle->transformMatrix = auxMatrix;


        //Relative to Transform Component rotation
        glm::vec3 worldDirection = useDirectionRandom ? glm::linearRand(direction, directionRandom) : direction;
        particle->direction = glm::vec3(transformComponentMatrix * glm::vec4(worldDirection, 0));



        particle->color = useColorRandom ? glm::linearRand(colorNormal, colorRandom) : colorNormal;
        if (particleMaterial)
            particleMaterial->GetMaterialProperties().color = particle->color;

        particle->lifetime = useRandomLifeTime ? glm::linearRand(startLifeTimeMin, startLifeTimeMax) : startLifeTime;
        particle->startLifetime = particle->lifetime;

        particle->startSpeed = useRandomSpeed ? glm::linearRand(startSpeedMin, startSpeedMax) : startSpeed;

        particle->startSize = useRandomSize ? glm::linearRand(startSizeMin, startSizeMax) : startSize;
        if (!useSplitAxesSize)
        {
            particle->startSize = glm::vec3(particle->startSize.x);
        }
        particle->SetSize(particle->startSize);

        particle->startRotation =
            useRandomRotation ? glm::linearRand(startRotationMin, startRotationMax) : startRotation;
        particle->SetRotation(particle->startRotation);
    }

    void ParticleEmitter::GenerateParticle()
    {
        Ref<Particle> particle = CreateRef<Particle>();
        InitParticle(particle);
        activeParticles.push_back(particle);
    }

    void ParticleEmitter::Update(float deltaTime)
    {
        elapsedTime += deltaTime;

        if (looping)
        {
            accumulatedParticles += rateOverTime * deltaTime;
            while (accumulatedParticles >= 1.0f && activeParticles.size() < amount)
            {
                GenerateParticle();
                accumulatedParticles -= 1.0f;
            }
        }

        for (size_t i = 0; i < activeParticles.size();)
        {
            UpdateParticle(activeParticles[i], deltaTime);
            if (activeParticles[i]->lifetime <= 0)
            {
                activeParticles.erase(activeParticles.begin() + i);
            }
            else
            {
                ++i;
            }
        }
    }

    void ParticleEmitter::UpdateParticle(Ref<Particle> particle, float deltaTime)
    {
        float normalizedLife = 1.0f - (particle->lifetime / particle->startLifetime);

        if (renderAlignment == RenderAligment::Billboard) // Assume 0 is billboarding
        {
            glm::mat4 viewMatrix = cameraViewMatrix; // Get the camera's view matrix
            glm::mat4 billboardTransform = CalculateBillboardTransform(particle->transformMatrix, viewMatrix);


            if (!useRotationOverLifetime)
            {
                glm::mat4 localRotationX =
                    glm::rotate(glm::mat4(1.0f), glm::radians(particle->startRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                billboardTransform = billboardTransform * localRotationX;

                glm::mat4 localRotationY =
                    glm::rotate(glm::mat4(1.0f), glm::radians(particle->startRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                billboardTransform = billboardTransform * localRotationY;

                glm::mat4 localRotationZ =
                    glm::rotate(glm::mat4(1.0f), glm::radians(particle->startRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                billboardTransform = billboardTransform * localRotationZ;
            
            }
            else
            {
                glm::vec3 newRotation;
                newRotation.x = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, rotationOverLifetimeX), -1, 1);

                newRotation.y = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, rotationOverLifetimeY), -1, 1);

                newRotation.z = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, rotationOverLifetimeZ), -1, 1);


                glm::mat4 localRotationX = glm::rotate(glm::mat4(1.0f), glm::radians(newRotation.x * particle->startRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                billboardTransform = billboardTransform * localRotationX;

                glm::mat4 localRotationY = glm::rotate(glm::mat4(1.0f), glm::radians(newRotation.z * particle->startRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                billboardTransform = billboardTransform * localRotationY;

                glm::mat4 localRotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(newRotation.y * particle->startRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                billboardTransform = billboardTransform * localRotationZ;
            }

            
           

            //Fix for the rotation primitive plane
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            billboardTransform = billboardTransform * rotationMatrix;
            //End fix

            particle->transformMatrix = billboardTransform;
           
        }
        else
        {
            // Handle other alignment modes if needed
        }
        particle->SetSize(particle->startSize);

        glm::vec3 newVelocity = glm::vec3(1, 1, 1);

        if (useVelocityOverLifetime)
        {
            if (velocityOverLifeTimeSeparateAxes)
            {
                newVelocity.x = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, speedOverLifeTimeX), -1, 1);
                
                newVelocity.z = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, speedOverLifeTimeY), -1, 1);
                
                newVelocity.y = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, speedOverLifeTimeZ), -1, 1);
                
            }
            else
            {
                
                float uniformSize = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, speedOverLifeTimeGeneral),
                                                 -1, 1);
                newVelocity = glm::vec3(uniformSize);
            }
        }

        if (useSizeOverLifetime)
        {
            glm::vec3 newSize;
            if (sizeOverLifeTimeSeparateAxes)
            {

                newSize.x = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, sizeOverLifetimeX), -1, 1);

                newSize.z = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, sizeOverLifetimeY), -1, 1);

                newSize.y = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, sizeOverLifetimeZ), -1, 1);


            }
            else
            {
                float uniformSize = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, sizeOverLifetimeGeneral), -1, 1);
                newSize = glm::vec3(uniformSize);
            }
            particle->SetSize(newSize * particle->startSize);
        }

        

        if (useColorOverLifetime)
        {
            ImVec4 newColor = GradientEditor::GetGradientValue(normalizedLife, colorOverLifetime_gradientPoints);
            particle->color = glm::vec4(newColor.x, newColor.y, newColor.z, newColor.w);
        }


        if (simulationSpace == SimulationSpace::Local)
        {
            particle->SetPosition(particle->GetPosition() +
                                  particle->direction * deltaTime * newVelocity * particle->startSpeed);
        }
        else
        {
            particle->SetPosition(particle->GetPosition() +
                                  particle->direction * deltaTime * newVelocity * particle->startSpeed);
        }

        particle->lifetime -= deltaTime;



        DrawParticles(particle);

    }


    glm::mat4 ParticleEmitter::CalculateBillboardTransform(const glm::mat4& particleTransform,
                                                           const glm::mat4& viewMatrix)
    {
        // Extract the particle's position
        glm::vec3 position = glm::vec3(particleTransform[3]);

        // Remove the particle's rotation (keep only scale and position)
        glm::mat4 billboardTransform = glm::mat4(1.0f);
        billboardTransform[3] = glm::vec4(position, 1.0f);

        // Apply the inverse of the camera's rotation to make the particle face the camera
        glm::mat4 inverseView = glm::inverse(viewMatrix);
        inverseView[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Ignore the camera's translation

        billboardTransform = billboardTransform * inverseView;

        return billboardTransform;
    }


    void ParticleEmitter::DrawParticles() {
        for (size_t i = 0; i < activeParticles.size(); i++)
        {
            Ref<Particle> p = activeParticles.at(i);
            DrawParticles(p);
        }
    }

    void ParticleEmitter::DrawParticles(Ref<Particle> p)
    {  
        RenderCommand renderCommand;
        renderCommand.transform = p->GetWorldTransform();
        renderCommand.mesh = ParticleEmitter::particleMesh;
        renderCommand.material = particleMaterial;
        renderCommand.animator = nullptr;

       Renderer3D::Submit(renderCommand); 
    }

    void ParticleEmitter::DrawDebug()
    {
        glm::vec3 auxTransformPosition = glm::vec3(transformComponentMatrix[3]);
        glm::vec3 direction = glm::normalize(glm::vec3(transformComponentMatrix[1]));


        Renderer2D::DrawBox(minSpread + auxTransformPosition, maxSpread + auxTransformPosition, glm::vec4(1.0f), 1.0f);
        Renderer2D::DrawArrow(transformComponentMatrix[3], direction, 1.5f);

    }

    void ParticleEmitter::Emit(int quantity)
    {
        for (int i = 0; i < quantity; i++)
        {
            GenerateParticle();
            if (activeParticles.size() >= amount)
            {
                break;
            }
        }
    }

} // namespace Coffee