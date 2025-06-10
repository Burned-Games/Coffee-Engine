#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "Skeleton.h"

#include <glm/gtc/type_ptr.hpp>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>

namespace Coffee {
    class Animation;
    struct AnimationLayer;
    struct AnimatorComponent;

    /**
     * @brief System responsible for handling animations.
     */
    class AnimationSystem
    {
    public:

        /**
         * @brief Updates the animation system.
         * @param deltaTime The time elapsed since the last update.
         * @param animator The animator component to update.
         */
        static void Update(float deltaTime, AnimatorComponent* animator);

        /**
         * @brief Sets the bone transformations for the shader.
         * @param shader The shader to set the bone transformations for.
         * @param animator The animator component.
         */
        static void SetBoneTransformations(const Ref<Shader>& shader, const AnimatorComponent* animator);

        /**
         * @brief Sets the current animation for a specific layer.
         * @param index The index of the animation to set.
         * @param animator The animator component.
         * @param layer The animation layer to update.
         */
        static void SetCurrentAnimation(unsigned int index, AnimatorComponent* animator, AnimationLayer* layer);

        /**
         * @brief Adds an animator component to the system.
         * @param animatorComponent The animator component to add.
         */
        static void AddAnimator(AnimatorComponent* animatorComponent);

        /**
         * @brief Gets the list of animators.
         * @return A vector of animator components.
         */
        static std::vector<AnimatorComponent*> GetAnimators() { return m_Animators; }

        /**
         * @brief Resets the animators vector.
         */
        static void ResetAnimators() { m_Animators.clear(); }

        /**
         * @brief Loads the animator.
         * @param animator The animator component.
         */
        static void LoadAnimator(AnimatorComponent* animator);

        /**
         * @brief Sets up partial blending for upper and lower body animations.
         * @param upperBodyAnimIndex The index of the upper body animation.
         * @param lowerBodyAnimIndex The index of the lower body animation.
         * @param upperBodyJointName The name of the upper body root joint.
         * @param animator The animator component.
         */
        static void SetupPartialBlending(unsigned int upperBodyAnimIndex, unsigned int lowerBodyAnimIndex, const std::string& upperBodyJointName, AnimatorComponent* animator);

    private:
        /**
         * @brief Updates blending for a specific animation layer.
         * @param deltaTime The time elapsed since the last update.
         * @param animator The animator component.
         * @param layer The animation layer to update.
         */
        static void UpdateBlending(float deltaTime, const AnimatorComponent* animator, AnimationLayer* layer);

        /**
         * @brief Blends transforms between two sets of animations.
         * @param currentTransforms The current animation transforms.
         * @param nextTransforms The next animation transforms.
         * @param blendRatio The ratio for blending between the two animations.
         */
        static void BlendTransforms(std::vector<ozz::math::SoaTransform>& currentTransforms, const std::vector<ozz::math::SoaTransform>& nextTransforms, float blendRatio);

        /**
         * @brief Updates partial blending for upper and lower body animations.
         * @param deltaTime The time elapsed since the last update.
         * @param animator The animator component.
         */
        static void UpdatePartialBlending(float deltaTime, AnimatorComponent* animator);

        /**
         * @brief Sets up per-joint weights for partial blending.
         * @param animator The animator component.
         * @param upperBodyRootIndex The index of the upper body root joint.
         */
        static void SetupPerJointWeights(const AnimatorComponent* animator, int upperBodyRootIndex);

        /**
         * @brief Updates the animation times for a specific layer.
         * @param deltaTime The time elapsed since the last update.
         * @param animator The animator component.
         * @param layer The animation layer to update.
         * @param currentAnim The current animation.
         */
        static void UpdateLayerTimes(float deltaTime, const AnimatorComponent* animator, AnimationLayer* layer, const Animation* currentAnim);

        /**
         * @brief Samples and blends animations for a specific layer.
         * @param animator The animator component.
         * @param layer The animation layer.
         * @param currentAnim The current animation.
         * @param nextAnim The next animation.
         * @param outputTransforms The output transforms for the layer.
         */
        static void SampleAndBlendLayerAnimations(AnimatorComponent* animator, AnimationLayer* layer, const Animation* currentAnim, const Animation* nextAnim, std::vector<ozz::math::SoaTransform>& outputTransforms);

        /**
         * @brief Samples the transforms for the animation.
         * @param animator The animator component.
         * @param animationIndex The index of the animation.
         * @param timeRatio The time ratio for the animation.
         * @return A vector of sampled transforms.
         */
        static std::vector<ozz::math::SoaTransform> SampleTransforms(AnimatorComponent* animator, unsigned int animationIndex, float timeRatio);

        /**
         * @brief Converts local transforms to model space.
         * @param animator The animator component.
         * @param localTransforms The local transforms.
         * @return A vector of transforms in model space.
         */
        static std::vector<ozz::math::Float4x4> ConvertToModelSpace(AnimatorComponent* animator, const std::vector<ozz::math::SoaTransform>& localTransforms);

        /**
         * @brief Converts an Ozz matrix to a GLM matrix.
         * @param from The Ozz matrix.
         * @return The GLM matrix.
         */
        static glm::mat4 OzzToGlmMat4(const ozz::math::Float4x4& from) {
            glm::mat4 to;
            memcpy(glm::value_ptr(to), &from.cols[0], sizeof(glm::mat4));
            return to;
        }

    private:
        static std::vector<AnimatorComponent*> m_Animators; ///< The list of animator components.
    };
} // namespace Coffee
