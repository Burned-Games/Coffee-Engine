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
        particleTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
    }

    void ParticleEmitter::InitParticle(Ref<Particle> particle)
    {
        glm::vec3 startPos = GetRandomPointByShape(shape);
        glm::vec4 startPosVec4 = glm::vec4(startPos.x, startPos.y, startPos.z, 0);

        glm::mat4 auxMatrix = transformComponentMatrix;
        auxMatrix[3] = transformComponentMatrix[3] + startPosVec4;

        particle->transformMatrix = auxMatrix;

        if (shape != ShapeType::Cone)
        {
            if (goCenter)
            {
                glm::vec3 localCenter = glm::vec3(0.0f);
                glm::vec3 dirLocal = glm::normalize(localCenter - startPos);
                particle->direction = dirLocal; // <- En espacio local, sin transformar
            }
            else
            {
                glm::vec3 worldDirection = useDirectionRandom ? glm::linearRand(direction, directionRandom) : direction;
                particle->direction = glm::vec3(transformComponentMatrix * glm::vec4(worldDirection, 0));
            }
        }
        else
        {
            float cosAngle = cos(shapeAngle);
            glm::vec3 direction;
            do
            {
                direction = glm::normalize(glm::vec3(glm::linearRand(-1.0f, 1.0f),
                                                     glm::linearRand(0.0f, 1.0f), 
                                                     glm::linearRand(-1.0f, 1.0f)));
            } while (glm::dot(direction, glm::vec3(0, 1, 0)) < cosAngle); 
            particle->direction = glm::vec3(transformComponentMatrix * glm::vec4(direction,0));
        }

        particle->localPosition = startPos;
        particle->color = useColorRandom ? glm::linearRand(colorNormal, colorRandom) : colorNormal;
        particle->current_texture = particleTexture;
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


            for (int i = 0; i < bursts.size(); i++)
            {
                Ref<BurstParticleEmitter> burst = bursts[i];

                if (burst->initialTime <= elapsedTime)
                {
                    burst->intervalTimer += deltaTime;

                    if (burst->intervalTimer > burst->interval)
                    {
                        Emit(burst->count);
                        burst->intervalTimer = 0;
                    }
                }
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
                newRotation.x = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, rotationOverLifetimeX),
                                                 -rotationMultiplier, rotationMultiplier);

                newRotation.y = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, rotationOverLifetimeY),
                                                 -rotationMultiplier, rotationMultiplier);

                newRotation.z = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, rotationOverLifetimeZ),
                                                 -rotationMultiplier, rotationMultiplier);


                glm::mat4 localRotationX = glm::rotate(glm::mat4(1.0f), glm::radians(newRotation.x * particle->startRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                billboardTransform = billboardTransform * localRotationX;

                glm::mat4 localRotationY = glm::rotate(glm::mat4(1.0f), glm::radians(newRotation.z * particle->startRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                billboardTransform = billboardTransform * localRotationY;

                glm::mat4 localRotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(newRotation.y * particle->startRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                billboardTransform = billboardTransform * localRotationZ;
            }

            
           

            //Fix for the rotation primitive plane
            //glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            //billboardTransform = billboardTransform * rotationMatrix;
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
                newVelocity.x = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, speedOverLifeTimeX),
                                                 -velocityMultiplier, velocityMultiplier);
                
                newVelocity.z = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, speedOverLifeTimeY),
                                                 -velocityMultiplier, velocityMultiplier);
                
                newVelocity.y = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, speedOverLifeTimeZ),
                                                 -velocityMultiplier, velocityMultiplier);
                
            }
            else
            {
                
                float uniformSpeed = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife,speedOverLifeTimeGeneral),
                                                    -velocityMultiplier, velocityMultiplier);
                newVelocity = glm::vec3(uniformSpeed);
            }
        }
        
        if (useSizeOverLifetime)
        {
            glm::vec3 newSize;
            if (sizeOverLifeTimeSeparateAxes)
            {

                newSize.x = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, sizeOverLifetimeX),
                                                         -sizeMultiplier, sizeMultiplier);

                newSize.z = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, sizeOverLifetimeY),
                                                         -sizeMultiplier, sizeMultiplier);

                newSize.y = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, sizeOverLifetimeZ),
                                                         -sizeMultiplier, sizeMultiplier);


            }
            else
            {
                float uniformSize = CurveEditor::ScaleCurveValue(CurveEditor::GetCurveValue(normalizedLife, sizeOverLifetimeGeneral),
                                                 -sizeMultiplier, sizeMultiplier);
                newSize = glm::vec3(uniformSize);
            }
            particle->SetSize(newSize * particle->startSize);
        }

        

        if (useColorOverLifetime)
        {
            ImVec4 newColor = GradientEditor::GetGradientValue(normalizedLife, colorOverLifetime_gradientPoints);
            particle->color = glm::vec4(newColor.x, newColor.y, newColor.z, newColor.w);
        }

        newVelocity += gravity * (particle->lifetime - particle->startLifetime);


        
        particle->localPosition += particle->direction * deltaTime * newVelocity * particle->startSpeed;
        if (simulationSpace == SimulationSpace::Local)
        {
            glm::vec3 emissorPosition = transformComponentMatrix[3];
            particle->SetPosition(emissorPosition + particle->localPosition);
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
        glm::vec3 position = glm::vec3(particleTransform[3]);

        glm::mat4 rotationMatrix = glm::mat4(glm::mat3(viewMatrix));

        glm::mat4 billboardTransform = glm::inverse(rotationMatrix);
        billboardTransform[3] = glm::vec4(position, 1.0f);

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
        if (p->current_texture)
        {
            Renderer2D::DrawQuad(p->GetWorldTransform(), p->current_texture, 1, p->color,
                                 Renderer2D::RenderMode::World);
        }
    }

    void ParticleEmitter::DrawDebug()
    {
        glm::vec4 colorDebug = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);

        glm::vec3 auxTransformPosition = glm::vec3(transformComponentMatrix[3]);
        glm::vec3 direction = glm::normalize(glm::vec3(transformComponentMatrix[1]));
        glm::mat3 rotationMatrix;
        glm::quat rotation90X;
        glm::quat rotation;
        float distance;
        float dispersionRadius;

        switch (shape)
        {
        case Coffee::ParticleEmitter::ShapeType::Circle:
            rotationMatrix = glm::mat3(transformComponentMatrix);
            rotation90X = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
            rotation = glm::quat_cast(rotationMatrix) * rotation90X;
            
            Renderer2D::DrawCircle(auxTransformPosition, shapeRadius, rotation, colorDebug);
            Renderer2D::DrawCircle(auxTransformPosition, shapeRadiusThickness, rotation, colorDebug);
            break;
        case Coffee::ParticleEmitter::ShapeType::Cone:

            rotationMatrix = glm::mat3(transformComponentMatrix);
            rotation90X = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
            rotation = glm::quat_cast(rotationMatrix) * rotation90X;


            distance = 3.0f;
            dispersionRadius = tan(shapeAngle) * distance;

            /*Renderer2D::DrawCircle(auxTransformPosition, shapeRadius, rotation, colorDebug);

            Renderer2D::DrawCone(auxTransformPosition + (rotation * glm::vec3(0, 0, -distance)),
                                 glm::quat_cast(rotationMatrix) * rotation90X * rotation90X, dispersionRadius, distance,
                                 colorDebug);*/

            Renderer2D::DrawTruncatedCone(auxTransformPosition,
                                          glm::quat_cast(rotationMatrix), shapeRadius,
                                          dispersionRadius, distance, colorDebug);

            break;
        case Coffee::ParticleEmitter::ShapeType::Box:
            Renderer2D::DrawBox(minSpread + auxTransformPosition, maxSpread + auxTransformPosition, colorDebug);
            break;
        default:
            break;
        }
       
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


    glm::vec3 ParticleEmitter::GetRandomPointInCircle() {

        //float minRadius = shapeRadius * (1.0f - shapeRadiusThickness);
        float randomRadius = glm::mix(shapeRadiusThickness, shapeRadius, static_cast<float>(rand()) / RAND_MAX);
        float angle = glm::linearRand(0.0f, glm::two_pi<float>());

        float x = randomRadius * cos(angle);
        float z = randomRadius * sin(angle);

        return glm::mat3(transformComponentMatrix) * glm::vec3(x, 0.0f, z);
    }

    glm::vec3 ParticleEmitter::GetRandomPointInCone()
    {

        // float minRadius = shapeRadius * (1.0f - shapeRadiusThickness);
        float randomRadius = glm::mix(0.0f, shapeRadius, static_cast<float>(rand()) / RAND_MAX);
        float angle = glm::linearRand(0.0f, glm::two_pi<float>());

        float x = randomRadius * cos(angle);
        float z = randomRadius * sin(angle);

        return glm::mat3(transformComponentMatrix) * glm::vec3(x, 0.0f, z);
    }


    glm::vec3 ParticleEmitter::GetRandomPointInBox() {
        return glm::linearRand(minSpread, maxSpread);
    }

    glm::vec3 ParticleEmitter::GetRandomPointByShape(ShapeType type) {
        switch (type)
        {
        case Coffee::ParticleEmitter::ShapeType::Circle:
            return GetRandomPointInCircle();
        case Coffee::ParticleEmitter::ShapeType::Cone:
            return GetRandomPointInCone();
        case Coffee::ParticleEmitter::ShapeType::Box:
            return GetRandomPointInBox();
        default:
            return glm::vec3(0, 0, 0);
        }

    }


} // namespace Coffee