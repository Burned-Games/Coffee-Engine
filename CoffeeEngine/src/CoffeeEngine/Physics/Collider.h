#pragma once

#define BT_NO_SIMD_OPERATOR_OVERLOADS

#include <btBulletDynamicsCommon.h>
#include <cereal/access.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include <glm/glm.hpp>

namespace Coffee {

    class BoxCollider;
    class SphereCollider;
    class CapsuleCollider;
    class ConeCollider;
    class CylinderCollider;

    class Collider {
    public:
        virtual ~Collider() {
            if (m_Shape) {
                delete m_Shape;
                m_Shape = nullptr;
            }
        }

        virtual void ResizeToFitAABB(const AABB& aabb) = 0;

        btCollisionShape* getShape() const { return m_Shape; }
        
        const glm::vec3& getOffset() const { return m_Offset; }
        void setOffset(const glm::vec3& offset) { m_Offset = offset; }

    protected:
        btCollisionShape* m_Shape = nullptr;
        glm::vec3 m_Offset = {0.0f, 0.0f, 0.0f}; // Offset from the rigidbody's center
        
        friend class cereal::access;
        
        template <class Archive> void save(Archive& archive, const std::uint32_t& version) const
        {
            btCollisionShape* shape = getShape();
            int shapeType = shape ? shape->getShapeType() : -1;
            archive(cereal::make_nvp("ShapeType", shapeType));
            archive(cereal::make_nvp("Offset", m_Offset));

            if (shape) {
                switch (shapeType) {
                    case BOX_SHAPE_PROXYTYPE: {
                        auto* boxShape = static_cast<btBoxShape*>(shape);
                        btVector3 halfExtents = boxShape->getHalfExtentsWithoutMargin();
                        glm::vec3 size(halfExtents.x(), halfExtents.y(), halfExtents.z());
                        archive(cereal::make_nvp("Size", size));
                        break;
                    }
                    case SPHERE_SHAPE_PROXYTYPE: {
                        auto* sphereShape = static_cast<btSphereShape*>(shape);
                        float radius = sphereShape->getRadius();
                        archive(cereal::make_nvp("Radius", radius));
                        break;
                    }
                    case CAPSULE_SHAPE_PROXYTYPE: {
                        auto* capsuleShape = static_cast<btCapsuleShape*>(shape);
                        float radius = capsuleShape->getRadius();
                        float height = capsuleShape->getHalfHeight() * 2.0f;
                        archive(cereal::make_nvp("Radius", radius));
                        archive(cereal::make_nvp("Height", height));
                        break;
                    }
                    case CONE_SHAPE_PROXYTYPE: {
                        auto* coneShape = static_cast<btConeShape*>(shape);
                        float radius = coneShape->getRadius();
                        float height = coneShape->getHeight();
                        archive(cereal::make_nvp("Radius", radius));
                        archive(cereal::make_nvp("Height", height));
                        break;
                    }
                    case CYLINDER_SHAPE_PROXYTYPE: {
                        auto* cylinderShape = static_cast<btCylinderShape*>(shape);
                        float radius = cylinderShape->getRadius();
                        float height = cylinderShape->getHalfExtentsWithoutMargin().y() * 2.0f;
                        archive(cereal::make_nvp("Radius", radius));
                        archive(cereal::make_nvp("Height", height));
                        break;
                    }
                }
            }
        }

        template <class Archive> void load(Archive& archive, const std::uint32_t& version)
        {

            int shapeType;
            archive(cereal::make_nvp("ShapeType", shapeType));
            if(version >= 0) archive(cereal::make_nvp("Offset", m_Offset));

            if (m_Shape) {
                delete m_Shape;
                m_Shape = nullptr;
            }

            switch (shapeType) {
                case BOX_SHAPE_PROXYTYPE: {
                    glm::vec3 size;
                    archive(cereal::make_nvp("Size", size));
                    m_Shape = new btBoxShape(btVector3(size.x, size.y, size.z));
                    break;
                }
                case SPHERE_SHAPE_PROXYTYPE: {
                    float radius;
                    archive(cereal::make_nvp("Radius", radius));
                    m_Shape = new btSphereShape(radius);
                    break;
                }
                case CAPSULE_SHAPE_PROXYTYPE: {
                    float radius, height;
                    archive(cereal::make_nvp("Radius", radius));
                    archive(cereal::make_nvp("Height", height));
                    m_Shape = new btCapsuleShape(radius, height);
                    break;
                }
                case CONE_SHAPE_PROXYTYPE: {
                    float radius, height;
                    archive(cereal::make_nvp("Radius", radius));
                    archive(cereal::make_nvp("Height", height));
                    m_Shape = new btConeShape(radius, height);
                    break;
                }
                case CYLINDER_SHAPE_PROXYTYPE: {
                    float radius, height;
                    archive(cereal::make_nvp("Radius", radius));
                    archive(cereal::make_nvp("Height", height));
                    m_Shape = new btCylinderShape(btVector3(radius, height, radius));
                    break;
                }
            }
        }
    };
    

    class BoxCollider : public Collider {
    public:
        BoxCollider(const glm::vec3& size = glm::vec3(1.0f, 1.0f, 1.0f)) {
            m_Size = size;
            m_Shape = new btBoxShape(btVector3(size.x / 2.0f, size.y / 2.0f, size.z / 2.0f));
        }

        const glm::vec3& GetSize() const { return m_Size; }

        void ResizeToFitAABB(const AABB& aabb) override {
            m_Size = aabb.max - aabb.min;
            
            if (m_Shape) {
                delete m_Shape;
            }

            m_Shape = new btBoxShape(btVector3(m_Size.x / 2.0f, m_Size.y / 2.0f, m_Size.z / 2.0f));
        }

    private:
        glm::vec3 m_Size;

        friend class cereal::access;

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::base_class<Collider>(this));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::base_class<Collider>(this));

            if (m_Shape && m_Shape->getShapeType() == BOX_SHAPE_PROXYTYPE) {
                const auto* boxShape = static_cast<btBoxShape*>(m_Shape);
                const btVector3 halfExtents = boxShape->getHalfExtentsWithoutMargin();
                m_Size = glm::vec3(halfExtents.x() * 2.0f, halfExtents.y() * 2.0f, halfExtents.z() * 2.0f);
            }
        }
    };

    class SphereCollider : public Collider {
    public:
        SphereCollider(float radius = 0.5f) {
            m_Radius = radius;
            m_Shape = new btSphereShape(radius);
        }

        float GetRadius() const { return m_Radius; }

        void ResizeToFitAABB(const AABB& aabb) override {
            // Calculate the maximum distance from center to any corner
            glm::vec3 center = (aabb.min + aabb.max) * 0.5f;
            glm::vec3 extents = aabb.max - center;
            m_Radius = glm::length(extents);
            
            if (m_Shape) {
                delete m_Shape;
            }
            m_Shape = new btSphereShape(m_Radius);
        }

    private:
        float m_Radius;

        friend class cereal::access;

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::base_class<Collider>(this));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::base_class<Collider>(this));

            if (m_Shape && m_Shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE) {
                const auto* sphereShape = static_cast<btSphereShape*>(m_Shape);
                m_Radius = sphereShape->getRadius();
            }
        }
    };

    class CapsuleCollider : public Collider {
    public:
        CapsuleCollider(float radius = 0.5f, float height = 1.0f) {
            m_Radius = radius;
            m_Height = height;
            m_Shape = new btCapsuleShape(radius, height);
        }

        float GetRadius() const { return m_Radius; }
        float GetHeight() const { return m_Height; }

        void ResizeToFitAABB(const AABB& aabb) override {
            glm::vec3 size = aabb.max - aabb.min;
            
            // Find the longest axis for the capsule direction
            float maxAxis = glm::max(size.x, glm::max(size.y, size.z));
            
            // Set height to longest dimension, radius to half of the average of the other two dimensions
            if (maxAxis == size.y) {
                // Y-axis oriented capsule
                m_Height = size.y;
                m_Radius = (size.x + size.z) / 4.0f;
            } else if (maxAxis == size.x) {
                // X-axis oriented capsule
                m_Height = size.x;
                m_Radius = (size.y + size.z) / 4.0f;
            } else {
                // Z-axis oriented capsule
                m_Height = size.z;
                m_Radius = (size.x + size.y) / 4.0f;
            }
            
            // Update the shape with the new dimensions
            if (m_Shape) {
                delete m_Shape;
            }
            m_Shape = new btCapsuleShape(m_Radius, m_Height);
        }

    private:
        float m_Radius;
        float m_Height;

        friend class cereal::access;

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::base_class<Collider>(this));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::base_class<Collider>(this));

            if (m_Shape && m_Shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE) {
                const auto* capsuleShape = static_cast<btCapsuleShape*>(m_Shape);
                m_Radius = capsuleShape->getRadius();
                m_Height = capsuleShape->getHalfHeight() * 2.0f;
            }
        }
    };

    class ConeCollider : public Collider
    {
    public:
        ConeCollider(float radius = 0.5f, float height = 1.0f)
        {
            m_Radius = radius;
            m_Height = height;
            m_Shape = new btConeShape(radius, height);
        }
    
        float GetRadius() const { return m_Radius; }
        float GetHeight() const { return m_Height; }
    
        void ResizeToFitAABB(const AABB& aabb) override {
            glm::vec3 size = aabb.max - aabb.min;
    
            // Find the longest axis for the cone direction
            float maxAxis = glm::max(size.x, glm::max(size.y, size.z));
    
            // Set height to longest dimension, radius to half of the average of the other two dimensions
            if (maxAxis == size.y) {
                // Y-axis oriented cone
                m_Height = size.y;
                m_Radius = (size.x + size.z) / 4.0f;
            } else if (maxAxis == size.x) {
                // X-axis oriented cone
                m_Height = size.x;
                m_Radius = (size.y + size.z) / 4.0f;
            } else {
                // Z-axis oriented cone
                m_Height = size.z;
                m_Radius = (size.x + size.y) / 4.0f;
            }
    
            // Update the shape with the new dimensions
            if (m_Shape) {
                delete m_Shape;
            }
            m_Shape = new btConeShape(m_Radius, m_Height);
        }
    
    private:
        float m_Radius;
        float m_Height;
    
        friend class cereal::access;
    
        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::base_class<Collider>(this));
        }
    
        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::base_class<Collider>(this));

            if (m_Shape && m_Shape->getShapeType() == CONE_SHAPE_PROXYTYPE) {
                const auto* coneShape = static_cast<btConeShape*>(m_Shape);
                m_Radius = coneShape->getRadius();
                m_Height = coneShape->getHeight();
            }
        }
    };

    class CylinderCollider : public Collider
    {
    public:
        CylinderCollider(float radius = 0.5f, float height = 1.0f)
        {
            m_Radius = radius;
            m_Height = height;
            m_Shape = new btCylinderShape(btVector3(radius, height * 0.5f, radius));
        }
    
        float GetRadius() const { return m_Radius; }
        float GetHeight() const { return m_Height; }
    
        void ResizeToFitAABB(const AABB& aabb) override {
            glm::vec3 size = aabb.max - aabb.min;
    
            // Find the longest axis for the cylinder direction
            float maxAxis = glm::max(size.x, glm::max(size.y, size.z));
    
            // Set height to longest dimension, radius to half of the average of the other two dimensions
            if (maxAxis == size.y) {
                // Y-axis oriented cylinder
                m_Height = size.y;
                m_Radius = (size.x + size.z) / 4.0f;
            } else if (maxAxis == size.x) {
                // X-axis oriented cylinder
                m_Height = size.x;
                m_Radius = (size.y + size.z) / 4.0f;
            } else {
                // Z-axis oriented cylinder
                m_Height = size.z;
                m_Radius = (size.x + size.y) / 4.0f;
            }
    
            // Update the shape with the new dimensions
            if (m_Shape) {
                delete m_Shape;
            }
            m_Shape = new btCylinderShape(btVector3(m_Radius, m_Height * 0.5f, m_Radius));
        }
    
    private:
        float m_Radius;
        float m_Height;
    
        friend class cereal::access;
    
        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::base_class<Collider>(this));
        }
    
        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::base_class<Collider>(this));

            if (m_Shape && m_Shape->getShapeType() == CYLINDER_SHAPE_PROXYTYPE) {
                const auto* cylinderShape = static_cast<btCylinderShape*>(m_Shape);
                m_Radius = cylinderShape->getRadius();
                m_Height = cylinderShape->getHalfExtentsWithoutMargin().y() * 2.0f;
            }
        }
    };

} // namespace Coffee

CEREAL_REGISTER_TYPE(Coffee::BoxCollider)
CEREAL_REGISTER_TYPE(Coffee::SphereCollider)
CEREAL_REGISTER_TYPE(Coffee::CapsuleCollider)
CEREAL_REGISTER_TYPE(Coffee::ConeCollider)
CEREAL_REGISTER_TYPE(Coffee::CylinderCollider)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Collider, Coffee::BoxCollider)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Collider, Coffee::SphereCollider)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Collider, Coffee::CapsuleCollider)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Collider, Coffee::ConeCollider)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Collider, Coffee::CylinderCollider)
CEREAL_CLASS_VERSION(Coffee::Collider, 0)
CEREAL_CLASS_VERSION(Coffee::BoxCollider, 0)
CEREAL_CLASS_VERSION(Coffee::SphereCollider, 0)
CEREAL_CLASS_VERSION(Coffee::CapsuleCollider, 0)
CEREAL_CLASS_VERSION(Coffee::ConeCollider, 0)
CEREAL_CLASS_VERSION(Coffee::CylinderCollider, 0)