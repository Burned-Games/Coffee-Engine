#pragma once
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Math/BoundingBox.h"
#include <CoffeeEngine/ImGui/ImGuiExtras.h>
#include <CoffeeEngine/Scene/PrimitiveMesh.h>

namespace Coffee
{
    class ParticleEmitter;


    struct BurstParticleEmitter
    {
        float initialTime;
        int count;
        float interval;
        float intervalTimer;

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const {
            archive(cereal::make_nvp("initialTime", initialTime));
            archive(cereal::make_nvp("count", count));
            archive(cereal::make_nvp("interval", interval));
        }

        template <class Archive> void load(Archive& archive, const std::uint32_t& version) {
            archive(cereal::make_nvp("initialTime", initialTime));
            archive(cereal::make_nvp("count", count));
            archive(cereal::make_nvp("interval", interval));
        }

    };


    /**
     * @brief Represents a particle in the particle system.
     */
    struct Particle
    {
        glm::mat4 transformMatrix; // Transformation matrix of the particle
        glm::vec3 direction;       // Direction of the particle
        glm::vec4 color;           // Color of the particle
        float size;                // Size of the particle
        float lifetime;            // Remaining lifetime of the particle
        float startLifetime;       // Initial lifetime of the particle
        float startSpeed;          // Initial speed of the particle
        glm::vec3 startSize;       // Initial size of the particle
        glm::vec3 startRotation;   // Initial rotation of the particle
        Ref<Texture2D> current_texture; //Current texture of the particle

        /**
         * @brief Default constructor for Particle.
         */
        Particle();

        /**
         * @brief Gets the world transformation matrix of the particle.
         * @return The transformation matrix.
         */
        glm::mat4 GetWorldTransform() const;

        /**
         * @brief Sets the position of the particle.
         * @param position The new position of the particle.
         */
        void SetPosition(glm::vec3 position);

        /**
         * @brief Sets the rotation of the particle.
         * @param rotation The new rotation of the particle.
         */
        void SetRotation(glm::vec3 rotation);

        /**
         * @brief Sets the size (scale) of the particle.
         * @param size The new size of the particle.
         */
        void SetSize(glm::vec3 size);

        /**
         * @brief Update the transform matrix of the particle.
         * 
         */
        void UpdateTransform();

        /**
         * @brief Gets the position of the particle.
         * @return The position of the particle.
         */
        glm::vec3 GetPosition() const;

        /**
         * @brief Gets the rotation of the particle.
         * @return The rotation of the particle.
         */
        glm::vec3 GetRotation() const;

        /**
         * @brief Gets the size (scale) of the particle.
         * @return The size of the particle.
         */
        glm::vec3 GetSize() const;
    };

    /**
     * @brief Represents a particle emitter in the particle system.
     */
    class ParticleEmitter
    {
      public:
        glm::mat4 transformComponentMatrix; // Transformation matrix of the emitter

        // Direction settings
        bool useDirectionRandom = false;                // Whether to use random direction
        glm::vec3 direction = {0.0f, 1.0f, 0.0f};       // Default direction
        glm::vec3 directionRandom = {0.0f, 1.0f, 0.0f}; // Random direction range

        // Color settings
        bool useColorRandom = false;                      // Whether to use random color
        glm::vec4 colorNormal = {1.0f, 1.0f, 1.0f, 1.0f}; // Default color
        glm::vec4 colorRandom = {1.0f, 1.0f, 1.0f, 1.0f}; // Random color range

        int amount = 1000;   // Maximum number of particles
        bool looping = true; // Whether the emitter should loop

        // Lifetime settings
        bool useRandomLifeTime = false; // Whether to use random lifetime
        float startLifeTimeMin = 5.0f;  // Minimum lifetime
        float startLifeTimeMax = 5.0f;  // Maximum lifetime
        float startLifeTime = 5.0f;     // Default lifetime

        // Speed settings
        bool useRandomSpeed = false; // Whether to use random speed
        float startSpeedMin = 5.0f;  // Minimum speed
        float startSpeedMax = 5.0f;  // Maximum speed
        float startSpeed = 5.0f;     // Default speed

        // Size settings
        bool useRandomSize = false;                  // Whether to use random size
        bool useSplitAxesSize = false;               // Whether to use separate axes for size
        glm::vec3 startSizeMin = glm::vec3(1, 1, 1); // Minimum size
        glm::vec3 startSizeMax = glm::vec3(1, 1, 1); // Maximum size
        glm::vec3 startSize = glm::vec3(1, 1, 1);    // Default size

        // Rotation settings
        bool useRandomRotation = false;                  // Whether to use random rotation
        glm::vec3 startRotationMin = glm::vec3(0, 0, 0); // Minimum rotation
        glm::vec3 startRotationMax = glm::vec3(0, 0, 0); // Maximum rotation
        glm::vec3 startRotation = glm::vec3(0, 0, 0);    // Default rotation

        /**
         * @brief Enum for simulation space types.
         */
        enum class SimulationSpace
        {
            Local = 0, // Local space
            World = 1, // World space
            Custom = 2 // Custom space
        };

        SimulationSpace simulationSpace = SimulationSpace::World; // Current simulation space

        bool useEmission = true;   // Whether emission is enabled
        float rateOverTime = 1.0f; // Emission rate over time
        float emitParticlesTest = 5.0f;


        bool useBurst = false;
        std::vector<Ref<BurstParticleEmitter>> bursts;


        /**
         * @brief Enum for shape types.
         */
        enum class ShapeType
        {
            Sphere, // Sphere shape
            Cone,   // Cone shape
            Box     // Box shape
        };

        ShapeType shape = ShapeType::Box;            // Current shape type
        glm::vec3 minSpread = {-0.1f, -0.1f, -0.1f}; // Minimum spread for emission
        glm::vec3 maxSpread = {0.1f, 0.1f, 0.1f};    // Maximum spread for emission
        bool useShape = true;                        // Whether to use a shape for emission
        float shapeAngle = 45.0f;                    // Angle for cone shape
        float shapeRadius = 1.0f;                    // Radius for sphere shape
        float shapeRadiusThickness = 0.1f;           // Thickness for sphere shape

        // Velocity over lifetime settings
        bool useVelocityOverLifetime = false;          // Whether to use velocity over lifetime
        bool velocityOverLifeTimeSeparateAxes = false; // Whether to use separate axes for velocity
        std::vector<CurvePoint> speedOverLifeTimeX = {{0.0f, 1.0f}, {1.0f, 0.5f}};       // Velocity curve for X axis
        std::vector<CurvePoint> speedOverLifeTimeY = {{0.0f, 1.0f}, {1.0f, 0.5f}};       // Velocity curve for Y axis
        std::vector<CurvePoint> speedOverLifeTimeZ = {{0.0f, 1.0f}, {1.0f, 0.5f}};       // Velocity curve for Z axis
        std::vector<CurvePoint> speedOverLifeTimeGeneral = {{0.0f, 1.0f}, {1.0f, 0.5f}}; // General velocity curve

        // Size over lifetime settings
        bool useSizeOverLifetime = false;          // Whether to use size over lifetime
        bool sizeOverLifeTimeSeparateAxes = false; // Whether to use separate axes for size
        std::vector<CurvePoint> sizeOverLifetimeX = {{0.0f, 1.0f}, {1.0f, 0.5f}};       // Size curve for X axis
        std::vector<CurvePoint> sizeOverLifetimeY = {{0.0f, 1.0f}, {1.0f, 0.5f}};       // Size curve for Y axis
        std::vector<CurvePoint> sizeOverLifetimeZ = {{0.0f, 1.0f}, {1.0f, 0.5f}};       // Size curve for Z axis
        std::vector<CurvePoint> sizeOverLifetimeGeneral = {{0.0f, 1.0f}, {1.0f, 0.5f}}; // General size curve

        // Rotation over lifetime settings
        bool useRotationOverLifetime = false; // Whether to use rotation over lifetime
        std::vector<CurvePoint> rotationOverLifetimeX = {{0.0f, 1.0f}, {1.0f, 0.5f}}; // Rotation curve for X axis
        std::vector<CurvePoint> rotationOverLifetimeY = {{0.0f, 1.0f}, {1.0f, 0.5f}}; // Rotation curve for Y axis
        std::vector<CurvePoint> rotationOverLifetimeZ = {{0.0f, 1.0f}, {1.0f, 0.5f}}; // Rotation curve for Z axis

        // Color over lifetime settings
        bool useColorOverLifetime = false; // Whether to use color over lifetime
        glm::vec4 overLifetimecolor;       // Color over lifetime
        std::vector<GradientPoint> colorOverLifetime_gradientPoints = {
            {0.0f, ImVec4(1, 1, 1, 1)}, {1.0f, ImVec4(1, 1, 1, 1)}}; // Gradient points for color

        bool useRenderer = true;    // Whether to use a renderer
        int renderMode = 0;         // Render mode
        glm::mat4 cameraViewMatrix; // Camera view matrix

        static Ref<Mesh> particleMesh;  // Mesh for particles
        Ref<Texture2D> particleTexture;

        /**
         * @brief Enum for render alignment types.
         */
        enum class RenderAligment
        {
            Billboard = 0, // Billboard alignment
            Custom         // Custom alignment
        };

        RenderAligment renderAlignment = RenderAligment::Billboard; // Current render alignment

        std::vector<Ref<Particle>> activeParticles; // List of active particles

      private:
        float accumulatedParticles = 0.0f; // Accumulated particles for emission
        float elapsedTime = 0.0f;          // Elapsed time since the emitter started

        /**
         * @brief Generates a new particle.
         */
        void GenerateParticle();

      public:
        /**
         * @brief Default constructor for ParticleEmitter.
         */
        ParticleEmitter();

        /**
         * @brief Initializes a particle with random values based on emitter settings.
         * @param particle The particle to initialize.
         */
        void InitParticle(Ref<Particle> particle);

        /**
         * @brief Updates the particle emitter and its particles.
         * @param deltaTime The time elapsed since the last frame.
         */
        void Update(float deltaTime);

        /**
         * @brief Updates a single particle.
         * @param particle The particle to update.
         * @param deltaTime The time elapsed since the last frame.
         */
        void UpdateParticle(Ref<Particle> particle, float deltaTime);


        /**
         * @brief Draw all particles.
         */
        void DrawParticles();

        /**
         * @brief Draw a single particle.
         */
        void DrawParticles(Ref<Particle> particle);


        /**
         * @brief Draws debug information for the particle emitter.
         */
        void DrawDebug();

        /**
         * @brief Emits a specified number of particles.
         * @param quantity The number of particles to emit.
         */
        void Emit(int quantity);

        /**
         * @brief Calculates the billboard transform for a particle.
         * @param particleTransform The particle's current transform.
         * @param viewMatrix The camera's view matrix.
         * @return The billboard transform matrix.
         */
        glm::mat4 CalculateBillboardTransform(const glm::mat4& particleTransform, const glm::mat4& viewMatrix);

        /**
         * @brief Serializes the ParticleEmitter object.
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
  
            // -------------------- Transform --------------------
           archive(cereal::make_nvp("TransformMatrix", transformComponentMatrix));

           // -------------------- Dirección --------------------
           archive(cereal::make_nvp("UseDirectionRandom", useDirectionRandom));
           archive(cereal::make_nvp("Direction", direction));
           archive(cereal::make_nvp("DirectionRandom", directionRandom));

           // -------------------- Color Aleatorio --------------------
           archive(cereal::make_nvp("UseColorRandom", useColorRandom));
           archive(cereal::make_nvp("ColorNormal", colorNormal));
           archive(cereal::make_nvp("ColorRandom", colorRandom));

           // -------------------- General --------------------
           archive(cereal::make_nvp("Amount", amount));
           archive(cereal::make_nvp("Looping", looping));

           // -------------------- Lifetime --------------------
           archive(cereal::make_nvp("UseRandomLifeTime", useRandomLifeTime));
           archive(cereal::make_nvp("StartLifeTimeMin", startLifeTimeMin));
           archive(cereal::make_nvp("StartLifeTimeMax", startLifeTimeMax));
           archive(cereal::make_nvp("StartLifeTime", startLifeTime));

           // -------------------- Speed --------------------
           archive(cereal::make_nvp("UseRandomSpeed", useRandomSpeed));
           archive(cereal::make_nvp("StartSpeedMin", startSpeedMin));
           archive(cereal::make_nvp("StartSpeedMax", startSpeedMax));
           archive(cereal::make_nvp("StartSpeed", startSpeed));

           // -------------------- Size --------------------
           archive(cereal::make_nvp("UseRandomSize", useRandomSize));
           archive(cereal::make_nvp("UseSplitAxesSize", useSplitAxesSize));
           archive(cereal::make_nvp("StartSizeMin", startSizeMin));
           archive(cereal::make_nvp("StartSizeMax", startSizeMax));
           archive(cereal::make_nvp("StartSize", startSize));

           // -------------------- Rotación --------------------
           archive(cereal::make_nvp("UseRandomRotation", useRandomRotation));
           archive(cereal::make_nvp("StartRotationMin", startRotationMin));
           archive(cereal::make_nvp("StartRotationMax", startRotationMax));
           archive(cereal::make_nvp("StartRotation", startRotation));

           // -------------------- Espacio de simulación --------------------
           archive(cereal::make_nvp("SimulationSpace", simulationSpace));

           // -------------------- Emisión --------------------
           archive(cereal::make_nvp("UseEmission", useEmission));
           archive(cereal::make_nvp("RateOverTime", rateOverTime));
           archive(cereal::make_nvp("UseBurst", useBurst));
           archive(cereal::make_nvp("Bursts", bursts));

           // -------------------- Forma --------------------
           archive(cereal::make_nvp("ShapeType", shape));
           archive(cereal::make_nvp("MinSpread", minSpread));
           archive(cereal::make_nvp("MaxSpread", maxSpread));
           archive(cereal::make_nvp("UseShape", useShape));
           archive(cereal::make_nvp("ShapeAngle", shapeAngle));
           archive(cereal::make_nvp("ShapeRadius", shapeRadius));
           archive(cereal::make_nvp("ShapeRadiusThickness", shapeRadiusThickness));

           // -------------------- Velocity over Lifetime --------------------
           archive(cereal::make_nvp("UseVelocityOverLifetime", useVelocityOverLifetime));
           archive(cereal::make_nvp("VelocitySeparateAxes", velocityOverLifeTimeSeparateAxes));
           archive(cereal::make_nvp("VelocityOverLifeX", speedOverLifeTimeX));
           archive(cereal::make_nvp("VelocityOverLifeY", speedOverLifeTimeY));
           archive(cereal::make_nvp("VelocityOverLifeZ", speedOverLifeTimeZ));
           archive(cereal::make_nvp("VelocityOverLifeGeneral", speedOverLifeTimeGeneral));

           // -------------------- Size over Lifetime --------------------
           archive(cereal::make_nvp("UseSizeOverLifetime", useSizeOverLifetime));
           archive(cereal::make_nvp("SizeSeparateAxes", sizeOverLifeTimeSeparateAxes));
           archive(cereal::make_nvp("SizeOverLifeX", sizeOverLifetimeX));
           archive(cereal::make_nvp("SizeOverLifeY", sizeOverLifetimeY));
           archive(cereal::make_nvp("SizeOverLifeZ", sizeOverLifetimeZ));
           archive(cereal::make_nvp("SizeOverLifeGeneral", sizeOverLifetimeGeneral));

           // -------------------- Rotation over Lifetime --------------------
           archive(cereal::make_nvp("UseRotationOverLifetime", useRotationOverLifetime));
           archive(cereal::make_nvp("RotationOverLifeX", rotationOverLifetimeX));
           archive(cereal::make_nvp("RotationOverLifeY", rotationOverLifetimeY));
           archive(cereal::make_nvp("RotationOverLifeZ", rotationOverLifetimeZ));

           // -------------------- Color over Lifetime --------------------
           archive(cereal::make_nvp("UseColorOverLifetime", useColorOverLifetime));
           archive(cereal::make_nvp("OverLifetimeColor", overLifetimecolor));
           archive(cereal::make_nvp("ColorGradientPoints", colorOverLifetime_gradientPoints));

           // -------------------- Render --------------------
           archive(cereal::make_nvp("UseRenderer", useRenderer));
           archive(cereal::make_nvp("RenderMode", renderMode));
           archive(cereal::make_nvp("RenderAlignment", renderAlignment));


           // -------------------- Misc --------------------
           archive(cereal::make_nvp("ElapsedTime", elapsedTime));
           archive(cereal::make_nvp("TextureUUID", particleTexture->GetUUID()));



        }


template <class Archive> void load(Archive& archive, const std::uint32_t& version) 
        {

            if (version == 0)
            {
                UUID textureUUID;
                glm::vec4 materialColor;

                archive(transformComponentMatrix, useDirectionRandom, direction, directionRandom, useColorRandom,
                        colorNormal, colorRandom, amount, looping, useRandomLifeTime, startLifeTimeMin,
                        startLifeTimeMax, startLifeTime, useRandomSpeed, startSpeedMin, startSpeedMax, startSpeed,
                        useRandomSize, useSplitAxesSize, startSizeMin, startSizeMax, startSize, useRandomRotation,
                        startRotationMin, startRotationMax, startRotation, simulationSpace, useEmission, rateOverTime,
                        shape, minSpread, maxSpread, useShape, shapeAngle, shapeRadius, shapeRadiusThickness,
                        useVelocityOverLifetime, velocityOverLifeTimeSeparateAxes, speedOverLifeTimeX,
                        speedOverLifeTimeY, speedOverLifeTimeZ, speedOverLifeTimeGeneral, useSizeOverLifetime,
                        sizeOverLifeTimeSeparateAxes, sizeOverLifetimeX, sizeOverLifetimeY, sizeOverLifetimeZ,
                        sizeOverLifetimeGeneral, useRotationOverLifetime, rotationOverLifetimeX, rotationOverLifetimeY,
                        rotationOverLifetimeZ, useColorOverLifetime, overLifetimecolor,
                        colorOverLifetime_gradientPoints, useRenderer, renderMode, renderAlignment, elapsedTime,
                        textureUUID, materialColor);

                if (textureUUID)
                {
                    particleTexture =
                        ResourceLoader::GetResource<Texture2D>(textureUUID);
                }
            
            }
            if (version >= 1)
            {
                // -------------------- Transform --------------------
                archive(cereal::make_nvp("TransformMatrix", transformComponentMatrix));

                // -------------------- Dirección --------------------
                archive(cereal::make_nvp("UseDirectionRandom", useDirectionRandom));
                archive(cereal::make_nvp("Direction", direction));
                archive(cereal::make_nvp("DirectionRandom", directionRandom));

                // -------------------- Color Aleatorio --------------------
                archive(cereal::make_nvp("UseColorRandom", useColorRandom));
                archive(cereal::make_nvp("ColorNormal", colorNormal));
                archive(cereal::make_nvp("ColorRandom", colorRandom));

                // -------------------- General --------------------
                archive(cereal::make_nvp("Amount", amount));
                archive(cereal::make_nvp("Looping", looping));

                // -------------------- Lifetime --------------------
                archive(cereal::make_nvp("UseRandomLifeTime", useRandomLifeTime));
                archive(cereal::make_nvp("StartLifeTimeMin", startLifeTimeMin));
                archive(cereal::make_nvp("StartLifeTimeMax", startLifeTimeMax));
                archive(cereal::make_nvp("StartLifeTime", startLifeTime));

                // -------------------- Speed --------------------
                archive(cereal::make_nvp("UseRandomSpeed", useRandomSpeed));
                archive(cereal::make_nvp("StartSpeedMin", startSpeedMin));
                archive(cereal::make_nvp("StartSpeedMax", startSpeedMax));
                archive(cereal::make_nvp("StartSpeed", startSpeed));

                // -------------------- Size --------------------
                archive(cereal::make_nvp("UseRandomSize", useRandomSize));
                archive(cereal::make_nvp("UseSplitAxesSize", useSplitAxesSize));
                archive(cereal::make_nvp("StartSizeMin", startSizeMin));
                archive(cereal::make_nvp("StartSizeMax", startSizeMax));
                archive(cereal::make_nvp("StartSize", startSize));

                // -------------------- Rotación --------------------
                archive(cereal::make_nvp("UseRandomRotation", useRandomRotation));
                archive(cereal::make_nvp("StartRotationMin", startRotationMin));
                archive(cereal::make_nvp("StartRotationMax", startRotationMax));
                archive(cereal::make_nvp("StartRotation", startRotation));

                // -------------------- Espacio de simulación --------------------
                archive(cereal::make_nvp("SimulationSpace", simulationSpace));

                // -------------------- Emisión --------------------
                archive(cereal::make_nvp("UseEmission", useEmission));
                archive(cereal::make_nvp("RateOverTime", rateOverTime));

                // -------------------- Forma --------------------
                archive(cereal::make_nvp("ShapeType", shape));
                archive(cereal::make_nvp("MinSpread", minSpread));
                archive(cereal::make_nvp("MaxSpread", maxSpread));
                archive(cereal::make_nvp("UseShape", useShape));
                archive(cereal::make_nvp("ShapeAngle", shapeAngle));
                archive(cereal::make_nvp("ShapeRadius", shapeRadius));
                archive(cereal::make_nvp("ShapeRadiusThickness", shapeRadiusThickness));

                // -------------------- Velocity over Lifetime --------------------
                archive(cereal::make_nvp("UseVelocityOverLifetime", useVelocityOverLifetime));
                archive(cereal::make_nvp("VelocitySeparateAxes", velocityOverLifeTimeSeparateAxes));
                archive(cereal::make_nvp("VelocityOverLifeX", speedOverLifeTimeX));
                archive(cereal::make_nvp("VelocityOverLifeY", speedOverLifeTimeY));
                archive(cereal::make_nvp("VelocityOverLifeZ", speedOverLifeTimeZ));
                archive(cereal::make_nvp("VelocityOverLifeGeneral", speedOverLifeTimeGeneral));

                // -------------------- Size over Lifetime --------------------
                archive(cereal::make_nvp("UseSizeOverLifetime", useSizeOverLifetime));
                archive(cereal::make_nvp("SizeSeparateAxes", sizeOverLifeTimeSeparateAxes));
                archive(cereal::make_nvp("SizeOverLifeX", sizeOverLifetimeX));
                archive(cereal::make_nvp("SizeOverLifeY", sizeOverLifetimeY));
                archive(cereal::make_nvp("SizeOverLifeZ", sizeOverLifetimeZ));
                archive(cereal::make_nvp("SizeOverLifeGeneral", sizeOverLifetimeGeneral));

                // -------------------- Rotation over Lifetime --------------------
                archive(cereal::make_nvp("UseRotationOverLifetime", useRotationOverLifetime));
                archive(cereal::make_nvp("RotationOverLifeX", rotationOverLifetimeX));
                archive(cereal::make_nvp("RotationOverLifeY", rotationOverLifetimeY));
                archive(cereal::make_nvp("RotationOverLifeZ", rotationOverLifetimeZ));

                // -------------------- Color over Lifetime --------------------
                archive(cereal::make_nvp("UseColorOverLifetime", useColorOverLifetime));
                archive(cereal::make_nvp("OverLifetimeColor", overLifetimecolor));
                archive(cereal::make_nvp("ColorGradientPoints", colorOverLifetime_gradientPoints));

                // -------------------- Render --------------------
                archive(cereal::make_nvp("UseRenderer", useRenderer));
                archive(cereal::make_nvp("RenderMode", renderMode));
                archive(cereal::make_nvp("RenderAlignment", renderAlignment));


                UUID textureUUID;

                // -------------------- Misc --------------------
                archive(cereal::make_nvp("ElapsedTime", elapsedTime));
                archive(cereal::make_nvp("TextureUUID", textureUUID));



                if (textureUUID)
                {
                    particleTexture = ResourceLoader::GetResource<Texture2D>(textureUUID);
                }

            }
            if (version >= 2)
            {
                archive(cereal::make_nvp("UseBurst", useBurst));
                archive(cereal::make_nvp("Bursts", bursts));
            
            }

           
  
        }
    };
} // namespace Coffee
CEREAL_CLASS_VERSION(Coffee::ParticleEmitter, 2);