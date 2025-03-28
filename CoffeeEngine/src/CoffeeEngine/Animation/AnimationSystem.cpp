#include "AnimationSystem.h"

#define BT_NO_SIMD_OPERATOR_OVERLOADS

#include "CoffeeEngine/Renderer/Model.h"
#include "CoffeeEngine/Scene/Components.h"
#include "ozz/animation/runtime/skeleton_utils.h"

#include <iostream>

namespace Coffee {

    std::vector<AnimatorComponent*> AnimationSystem::m_Animators;

    void AnimationSystem::Update(float deltaTime, AnimatorComponent* animator)
    {
        Ref<Skeleton> animatorSkeleton = animator->GetSkeleton();

        UpdateBlending(deltaTime * animator->AnimationSpeed, animator, animator->UpperAnimation.get());
        UpdateBlending(deltaTime * animator->AnimationSpeed, animator, animator->LowerAnimation.get());

        UpdatePartialBlending(deltaTime, animator);
    }

    void AnimationSystem::SetupPartialBlending(unsigned int upperBodyAnimIndex, unsigned int lowerBodyAnimIndex, const std::string& upperBodyJointName, AnimatorComponent* animator)
    {
        if (!animator->GetAnimationController()) return;

        auto* skeleton = animator->GetSkeleton()->GetSkeleton();
        if (!skeleton) return;

        int upperBodyRootIndex = -1;
        for (int i = 0; i < skeleton->num_joints(); i++)
        {
            if (skeleton->joint_names()[i] == upperBodyJointName)
            {
                upperBodyRootIndex = i;
                break;
            }
        }

        if (upperBodyRootIndex < 0)
        {
            std::cerr << "Failed to find upper body root joint: " << upperBodyJointName << std::endl;
            return;
        }

        animator->UpperAnimation->CurrentAnimation = upperBodyAnimIndex;
        animator->LowerAnimation->CurrentAnimation = lowerBodyAnimIndex;
        animator->UpperAnimation->AnimationTime = 0.0f;
        animator->LowerAnimation->AnimationTime = 0.0f;
        animator->UpperBodyRootJoint = upperBodyRootIndex;

        const int numSoaJoints = skeleton->num_soa_joints();

        animator->UpperAnimation->JointWeights.resize(numSoaJoints);
        animator->LowerAnimation->JointWeights.resize(numSoaJoints);
        animator->UpperAnimation->LocalTransforms.resize(numSoaJoints);
        animator->LowerAnimation->LocalTransforms.resize(numSoaJoints);
        animator->PartialBlendOutput.resize(numSoaJoints);

        SetupPerJointWeights(animator, upperBodyRootIndex);

        animator->GetContext().Resize(std::max(
            animator->GetAnimationController()->GetAnimation(upperBodyAnimIndex)->GetAnimation()->num_tracks(),
            animator->GetAnimationController()->GetAnimation(lowerBodyAnimIndex)->GetAnimation()->num_tracks()
        ));
    }

    void AnimationSystem::SetupPerJointWeights(const AnimatorComponent* animator, const int upperBodyRootIndex)
    {
        auto* skeleton = animator->GetSkeleton()->GetSkeleton();
        if (!skeleton) return;

        for (int i = 0; i < skeleton->num_soa_joints(); ++i)
        {
            animator->LowerAnimation->JointWeights[i] = ozz::math::simd_float4::one();
            animator->UpperAnimation->JointWeights[i] = ozz::math::simd_float4::zero();
        }

        auto setUpperBodyWeights = [&](int jointIndex, int)
        {
            int soaIndex = jointIndex / 4;
            int offset = jointIndex % 4;

            animator->UpperAnimation->JointWeights[soaIndex] = ozz::math::SetI(animator->UpperAnimation->JointWeights[soaIndex], ozz::math::simd_float4::Load1(1.0f), offset);
            animator->LowerAnimation->JointWeights[soaIndex] = ozz::math::SetI(animator->LowerAnimation->JointWeights[soaIndex], ozz::math::simd_float4::Load1(0.0f), offset);
        };

        ozz::animation::IterateJointsDF(*skeleton, setUpperBodyWeights, upperBodyRootIndex);
    }

   void AnimationSystem::UpdatePartialBlending(const float deltaTime, AnimatorComponent* animator)
    {
        Ref<AnimationController> animController = animator->GetAnimationController();
        if (!animController) return;

        auto& upperLayer = animator->UpperAnimation;
        auto& lowerLayer = animator->LowerAnimation;

        Animation* upperBodyCurrentAnim = animController->GetAnimation(animator->UpperAnimation->CurrentAnimation);
        Animation* upperBodyNextAnim = animator->UpperAnimation->IsBlending ? animController->GetAnimation(animator->UpperAnimation->NextAnimation) : nullptr;

        Animation* lowerBodyCurrentAnim = animController->GetAnimation(animator->LowerAnimation->CurrentAnimation);
        Animation* lowerBodyNextAnim = animator->LowerAnimation->IsBlending ? animController->GetAnimation(animator->LowerAnimation->NextAnimation) : nullptr;

        if (!upperBodyCurrentAnim || !lowerBodyCurrentAnim)
        {
            std::cerr << "Invalid animations for partial blending" << std::endl;
            return;
        }

        UpdateLayerTimes(deltaTime, animator, upperLayer.get(), upperBodyCurrentAnim);
        UpdateLayerTimes(deltaTime, animator, lowerLayer.get(), lowerBodyCurrentAnim);

        SampleAndBlendLayerAnimations(animator, upperLayer.get(), upperBodyCurrentAnim, upperBodyNextAnim, animator->UpperAnimation->LocalTransforms);
        SampleAndBlendLayerAnimations(animator, lowerLayer.get(), lowerBodyCurrentAnim, lowerBodyNextAnim, animator->LowerAnimation->LocalTransforms);

        animator->GetBlendLayers()[0].transform = ozz::make_span(animator->LowerAnimation->LocalTransforms);
        animator->GetBlendLayers()[0].weight = animator->LowerBodyWeight;
        animator->GetBlendLayers()[0].joint_weights = ozz::make_span(animator->LowerAnimation->JointWeights);

        animator->GetBlendLayers()[1].transform = ozz::make_span(animator->UpperAnimation->LocalTransforms);
        animator->GetBlendLayers()[1].weight = animator->UpperBodyWeight;
        animator->GetBlendLayers()[1].joint_weights = ozz::make_span(animator->UpperAnimation->JointWeights);

        animator->GetBlendJob().threshold = animator->PartialBlendThreshold;
        animator->GetBlendJob().layers = ozz::span(animator->GetBlendLayers(), 2);
        animator->GetBlendJob().rest_pose = animator->GetSkeleton()->GetSkeleton()->joint_rest_poses();
        animator->GetBlendJob().output = ozz::make_span(animator->PartialBlendOutput);

        if (!animator->GetBlendJob().Run())
        {
            std::cerr << "Failed to blend partial animations" << std::endl;
            return;
        }

        std::vector<ozz::math::Float4x4> modelSpaceTransforms = ConvertToModelSpace(animator, animator->PartialBlendOutput);

        for (size_t i = 0; i < modelSpaceTransforms.size(); ++i)
        {
            animator->JointMatrices[i] = OzzToGlmMat4(modelSpaceTransforms[i]) * animator->GetSkeleton()->GetJoints()[i].invBindPose;
        }
    }

    void AnimationSystem::UpdateLayerTimes(const float deltaTime, const AnimatorComponent* animator, AnimationLayer* layer, const Animation* currentAnim)
    {
        layer->AnimationTime += deltaTime * animator->AnimationSpeed;

        if (layer->IsBlending && layer->NextAnimationTime <= 0.0f)
            layer->NextAnimationTime = 0.0f;

        if (layer->IsBlending)
            layer->NextAnimationTime += deltaTime * animator->AnimationSpeed;

        if (float currentDuration = currentAnim->GetAnimation()->duration(); layer->AnimationTime > currentDuration)
        {
            if (animator->Loop)
                layer->AnimationTime = std::fmod(layer->AnimationTime, currentDuration);
            else
                layer->AnimationTime = currentDuration;
        }
    }

    void AnimationSystem::SampleAndBlendLayerAnimations(AnimatorComponent* animator, AnimationLayer* layer, const Animation* currentAnim, const Animation* nextAnim, std::vector<ozz::math::SoaTransform>& outputTransforms)
    {
        float currentDuration = currentAnim->GetAnimation()->duration();

        if (layer->IsBlending && nextAnim)
        {
            ozz::animation::SamplingJob currentSamplingJob;
            currentSamplingJob.animation = currentAnim->GetAnimation();
            currentSamplingJob.context = &animator->GetContext();
            currentSamplingJob.ratio = layer->AnimationTime / currentDuration;
            currentSamplingJob.output = ozz::make_span(outputTransforms);

            if (!currentSamplingJob.Run())
            {
                std::cerr << "Failed to sample current animation" << std::endl;
                return;
            }

            float nextDuration = nextAnim->GetAnimation()->duration();
            if (layer->NextAnimationTime > nextDuration)
                layer->NextAnimationTime = std::fmod(layer->NextAnimationTime, nextDuration);

            std::vector<ozz::math::SoaTransform> nextTransforms(outputTransforms.size());
            ozz::animation::SamplingJob nextSamplingJob;
            nextSamplingJob.animation = nextAnim->GetAnimation();
            nextSamplingJob.context = &animator->GetContext();
            nextSamplingJob.ratio = layer->NextAnimationTime / nextDuration;
            nextSamplingJob.output = ozz::make_span(nextTransforms);

            if (!nextSamplingJob.Run())
            {
                std::cerr << "Failed to sample next animation" << std::endl;
                return;
            }

            float blendRatio = layer->BlendTime / animator->BlendDuration;

            ozz::animation::BlendingJob transitionBlendJob;
            ozz::animation::BlendingJob::Layer transitionLayers[2];

            transitionLayers[0].transform = ozz::make_span(outputTransforms);
            transitionLayers[0].weight = 1.0f - blendRatio;

            transitionLayers[1].transform = ozz::make_span(nextTransforms);
            transitionLayers[1].weight = blendRatio;

            transitionBlendJob.layers = ozz::make_span(transitionLayers);
            transitionBlendJob.rest_pose = animator->GetSkeleton()->GetSkeleton()->joint_rest_poses();
            transitionBlendJob.output = ozz::make_span(outputTransforms);
            transitionBlendJob.threshold = animator->PartialBlendThreshold;

            if (!transitionBlendJob.Run())
            {
                std::cerr << "Failed to blend animations during transition" << std::endl;
                return;
            }
        }
        else
        {
            ozz::animation::SamplingJob samplingJob;
            samplingJob.animation = currentAnim->GetAnimation();
            samplingJob.context = &animator->GetContext();
            samplingJob.ratio = layer->AnimationTime / currentDuration;
            samplingJob.output = ozz::make_span(outputTransforms);

            if (!samplingJob.Run())
            {
                std::cerr << "Failed to sample animation" << std::endl;
                return;
            }
        }
    }

    void AnimationSystem::BlendTransforms(std::vector<ozz::math::SoaTransform>& currentTransforms, const std::vector<ozz::math::SoaTransform>& nextTransforms, float blendRatio)
    {
        if (currentTransforms.size() != nextTransforms.size())
        {
            std::cerr << "Cannot blend transforms with different sizes" << std::endl;
            return;
        }

        ozz::animation::BlendingJob blendingJob;
        ozz::animation::BlendingJob::Layer layers[2];
        layers[0].transform = ozz::make_span(currentTransforms);
        layers[0].weight = 1.0f - blendRatio;

        layers[1].transform = ozz::make_span(nextTransforms);
        layers[1].weight = blendRatio;

        blendingJob.layers = ozz::make_span(layers);
        blendingJob.output = ozz::make_span(currentTransforms);
        blendingJob.threshold = 0.01f;

        if (!blendingJob.Run())
            std::cerr << "Failed to blend transforms" << std::endl;
    }

    void AnimationSystem::UpdateBlending(float deltaTime, const AnimatorComponent* animator, AnimationLayer* layer)
    {
        if (!layer || !layer->IsBlending) return;

        layer->BlendTime += deltaTime;

        if (layer->BlendTime >= animator->BlendDuration)
        {
            layer->CurrentAnimation = layer->NextAnimation;
            layer->AnimationTime = layer->NextAnimationTime;
            layer->IsBlending = false;
        }
    }

    void AnimationSystem::LoadAnimator(AnimatorComponent* animator)
    {
        Ref<Model> model = ResourceRegistry::Get<Model>(animator->modelUUID);
        animator->SetSkeleton(model->GetSkeleton());
        animator->SetAnimationController(model->GetAnimationController());
        animator->JointMatrices = animator->GetSkeleton()->GetJointMatrices();
    }

    std::vector<ozz::math::SoaTransform> AnimationSystem::SampleTransforms(AnimatorComponent* animator, unsigned int animationIndex, float timeRatio)
    {
        const int numJoints = animator->GetSkeleton()->GetSkeleton()->num_joints();
        std::vector<ozz::math::SoaTransform> localTransforms(numJoints);

        ozz::animation::SamplingJob samplingJob;
        samplingJob.animation = animator->GetAnimationController()->GetAnimation(animationIndex)->GetAnimation();
        samplingJob.context = &animator->GetContext();
        samplingJob.ratio = timeRatio;
        samplingJob.output = ozz::make_span(localTransforms);

        if (!samplingJob.Run())
        {
            std::cerr << "Failed to sample animation" << std::endl;
        }

        return localTransforms;
    }

    std::vector<ozz::math::Float4x4> AnimationSystem::ConvertToModelSpace(AnimatorComponent* animator, const std::vector<ozz::math::SoaTransform>& localTransforms)
    {
        const int numJoints = animator->GetSkeleton()->GetSkeleton()->num_joints();
        std::vector<ozz::math::Float4x4> modelSpaceTransforms(numJoints);

        ozz::animation::LocalToModelJob localToModelJob;
        localToModelJob.skeleton = animator->GetSkeleton()->GetSkeleton();
        localToModelJob.input = ozz::make_span(localTransforms);
        localToModelJob.output = ozz::make_span(modelSpaceTransforms);

        if (!localToModelJob.Run())
        {
            std::cerr << "Failed to convert local to model transforms" << std::endl;
            std::fill(animator->JointMatrices.begin(), animator->JointMatrices.end(), glm::mat4(1.0f));
        }

        return modelSpaceTransforms;
    }

    void AnimationSystem::SetCurrentAnimation(unsigned int index, AnimatorComponent* animator, AnimationLayer* layer)
    {
        if (index < animator->GetAnimationController()->GetAnimationCount())
        {
            layer->NextAnimation = index;
            layer->BlendTime = 0.f;
            layer->IsBlending = true;
            layer->NextAnimationTime = (layer == animator->UpperAnimation.get() && index == animator->LowerAnimation->CurrentAnimation)
                                    || (layer == animator->LowerAnimation.get() && index == animator->UpperAnimation->CurrentAnimation)
                                    ? (layer == animator->UpperAnimation.get() ? animator->LowerAnimation->AnimationTime : animator->UpperAnimation->AnimationTime)
                                    : 0.0f;
            animator->GetContext().Resize(animator->GetAnimationController()->GetAnimation(index)->GetAnimation()->num_tracks());
        }
    }

    void AnimationSystem::SetBoneTransformations(const Ref<Shader>& shader, const AnimatorComponent* animator)
    {
        shader->setBool("animated", true);

        const std::vector<glm::mat4>& jointMatrices = animator->JointMatrices;
        shader->setMat4v("finalBonesMatrices", jointMatrices);
    }

    void AnimationSystem::AddAnimator(AnimatorComponent* animatorComponent)
    {
        m_Animators.push_back(animatorComponent);
    }
}