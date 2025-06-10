#pragma once

#include <cereal/details/helpers.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>

namespace Coffee
{
    /**
     * @defgroup renderer Renderer
     * @brief Renderer components of the CoffeeEngine.
     * @{
     */

    /**
     * @brief The Camera class is responsible for managing the camera's projection matrix.
     */
    class Camera
    {
    public:

        /**
         * @brief Enum class representing the type of projection.
         */
        enum class ProjectionType
        {
            PERSPECTIVE, ///< Perspective projection.
            ORTHOGRAPHIC ///< Orthographic projection.
        };

    public:
        /**
         * @brief Default constructor for the Camera class.
         */
        Camera() = default;

        /**
         * @brief Constructs a Camera object with the specified projection matrix.
         * @param projection The projection matrix.
         */
        Camera(const glm::mat4& projection)
            : m_Projection(projection) {}

        /**
         * @brief Virtual destructor for the Camera class.
         */
        virtual ~Camera() = default;

        /**
         * @brief Gets the field of view of the camera.
         * @return The field of view.
         */
        float GetFOV() const { return m_FOV; }

        /**
         * @brief Sets the field of view of the camera.
         * @param fov The field of view.
         */
        void SetFOV(float fov) { m_FOV = fov; UpdateProjection(); }

        /**
         * @brief Gets the aspect ratio of the camera.
         * @return The aspect ratio.
         */
        float GetAspectRatio() const { return m_AspectRatio; }

        /**
         * @brief Sets the aspect ratio of the camera.
         * @param aspectRatio The aspect ratio.
         */
        void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; UpdateProjection(); }

        /**
         * @brief Gets the near clipping plane distance of the camera.
         * @return The near clipping plane distance.
         */
        float GetNearClip() const { return m_NearClip; }

        /**
         * @brief Sets the near clipping plane distance of the camera.
         * @param nearClip The near clipping plane distance.
         */
        void SetNearClip(float nearClip) { m_NearClip = nearClip; UpdateProjection(); }

        /**
         * @brief Gets the far clipping plane distance of the camera.
         * @return The far clipping plane distance.
         */
        float GetFarClip() const { return m_FarClip; }

        /**
         * @brief Sets the far clipping plane distance of the camera.
         * @param farClip The far clipping plane distance.
         */
        void SetFarClip(float farClip) { m_FarClip = farClip; UpdateProjection(); }

        /**
         * @brief Gets the projection matrix of the camera.
         * @return The projection matrix.
         */
        const glm::mat4& GetProjection() const { return m_Projection; }


        /**
         * @brief Gets the projection type of the camera.
         * @return The projection type.
         */
        ProjectionType GetProjectionType() const { return m_ProjectionType; }

        /**
         * @brief Sets the projection type of the camera.
         * @param projectionType The projection type.
         */
        void SetProjectionType(ProjectionType projectionType) { m_ProjectionType = projectionType; UpdateProjection(); }

        glm::vec2 GetViewportSize() const { return { m_ViewportWidth, m_ViewportHeight }; }

         /**
         * @brief Sets the size of the viewport and updates the projection matrix.
         * @param width The width of the viewport.
         * @param height The height of the viewport.
         */
        inline void SetViewportSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; UpdateProjection(); }

    public:

        /**
         * @brief Converts the projection type to a projection matrix.
         * @param projection The type of projection.
         * @return The projection matrix.
         */
        glm::mat4 ProjectionTypeToMat4(ProjectionType projection)
        {
            switch (projection) {
                case ProjectionType::PERSPECTIVE:
                    return glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
                break;
                case ProjectionType::ORTHOGRAPHIC:
                    float orthoHeight = 10.0f; // Define the height of the orthographic view
                    float orthoWidth = orthoHeight * m_AspectRatio; // Calculate width based on aspect ratio
                    return glm::ortho(-orthoWidth / 2, orthoWidth / 2, -orthoHeight / 2, orthoHeight / 2, m_NearClip, m_FarClip);
                break;
            }
        }

        /**
         * @brief Updates the projection matrix based on the current viewport dimensions and projection type.
         */
        void UpdateProjection()
        {
            m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
            m_Projection = ProjectionTypeToMat4(m_ProjectionType);
        }


        
    private:
        friend class cereal::access;

        /**
         * @brief Serializes the camera to an archive.
         * @tparam Archive The type of the archive.
         * @param archive The archive to save the camera to.
         */
        template <class Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("FOV", m_FOV), cereal::make_nvp("AspectRatio", m_AspectRatio),
                    cereal::make_nvp("NearClip", m_NearClip), cereal::make_nvp("FarClip", m_FarClip),
                    cereal::make_nvp("ProjectionType", m_ProjectionType), cereal::make_nvp("ViewportWidth", m_ViewportWidth),
                    cereal::make_nvp("ViewportHeight", m_ViewportHeight));
        }

    protected:
        float m_FOV = 45.0f; ///< The field of view for the perspective projection.
        float m_AspectRatio = 1.778; ///< The aspect ratio of the viewport.
        float m_NearClip = 0.1f; ///< The near clipping plane distance.
        float m_FarClip = 1000.0f; ///< The far clipping plane distance.

        glm::mat4 m_Projection = glm::mat4(1.0f); ///< The projection matrix.

        ProjectionType m_ProjectionType = ProjectionType::PERSPECTIVE; ///< The type of projection.

        float m_ViewportWidth = 1280; ///< The width of the viewport.
        float m_ViewportHeight = 720; ///< The height of the viewport.
    };

    /** @} */
}

CEREAL_REGISTER_TYPE(Coffee::Camera);
CEREAL_CLASS_VERSION(Coffee::Camera, 0);