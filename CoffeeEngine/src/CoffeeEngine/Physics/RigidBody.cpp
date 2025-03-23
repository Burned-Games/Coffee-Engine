#include "RigidBody.h"

#include "CoffeeEngine/Math/BoundingBox.h"

namespace Coffee {

    Ref<RigidBody> RigidBody::Create(const Properties& props, const Ref<Collider>& collider) {
        auto rb = CreateRef<RigidBody>();
        rb->Initialize(props, collider);
        return rb;
    }

    void RigidBody::Initialize(const Properties& props, const Ref<Collider>& collider) {
        m_Properties = props;
        m_Collider = collider;

        btTransform transform;
        transform.setIdentity();
        m_MotionState = new btDefaultMotionState(transform);

        btVector3 localInertia(0, 0, 0);
        if (props.type != Type::Static)
        {
            m_Collider->getShape()->calculateLocalInertia(props.mass, localInertia);
        }

        const btRigidBody::btRigidBodyConstructionInfo rbInfo(
            props.mass,
            m_MotionState,
            m_Collider->getShape(),
            localInertia
        );

        m_Body = new btRigidBody(rbInfo);
        m_Body->setActivationState(DISABLE_DEACTIVATION);
        m_Body->setUserPointer(nullptr);

        // Apply friction and damping
        m_Body->setFriction(props.friction);
        m_Body->setDamping(props.linearDrag, props.angularDrag);

        if (props.type == Type::Static) {
            m_Body->setCollisionFlags(m_Body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
            m_Body->setMassProps(0, btVector3(0, 0, 0)); // Zero mass for static objects
        }

        // Apply linear factor based on freeze settings
        UpdateLinearFactor();

        // Apply angular factor based on freeze rotation settings
        UpdateAngularFactor();

        // Apply gravity based on useGravity property
        if (props.useGravity) {
            m_Body->setGravity(btVector3(0, GRAVITY, 0)); // Default gravity
        } else {
            m_Body->setFlags(m_Body->getFlags() | BT_DISABLE_WORLD_GRAVITY);
            m_Body->setGravity(btVector3(0, 0, 0)); // No gravity
        }
    }

    RigidBody::~RigidBody() {
        if (m_Body) {
            delete m_Body->getMotionState();
            delete m_Body;
        }

        if (m_Collider) {
            m_Collider.reset();
        }
    }

    void RigidBody::SetPosition(const glm::vec3& position) const
    {
        const glm::vec3& offset = m_Collider->getOffset();
        btVector3 btPosition(
            position.x + offset.x,
            position.y + offset.y,
            position.z + offset.z
        );
        
        btTransform transform = m_Body->getWorldTransform();
        transform.setOrigin(btPosition);
        m_Body->setWorldTransform(transform);
        m_Collider->setOffset(offset);
    }

    void RigidBody::SetVelocity(const glm::vec3& velocity) const
    {
        m_Body->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
        m_Body->activate(true);
    }
    
    void RigidBody::AddVelocity(const glm::vec3& deltaVelocity) const
    {
        btVector3 currentVel = m_Body->getLinearVelocity();
        btVector3 newVel(
            currentVel.x() + deltaVelocity.x,
            currentVel.y() + deltaVelocity.y,
            currentVel.z() + deltaVelocity.z
        );
        m_Body->setLinearVelocity(newVel);
        m_Body->activate(true); 
    }

    glm::vec3 RigidBody::GetPosition() const {
        btVector3 pos = m_Body->getWorldTransform().getOrigin();
        const glm::vec3& offset = m_Collider->getOffset();
        return {pos.x() - offset.x, pos.y() - offset.y, pos.z() - offset.z};
    }

    void RigidBody::ApplyForce(const glm::vec3& force) const
    {
        m_Body->activate(true);
        m_Body->applyCentralForce(btVector3(force.x, force.y, force.z));
    }

    void RigidBody::SetRotation(const glm::vec3& rotation) const
    {
        btTransform transform = m_Body->getWorldTransform();
        btQuaternion quat;
        
        glm::vec3 rotationRadians = glm::radians(rotation);
        quat.setEulerZYX(rotationRadians.z, rotationRadians.y, rotationRadians.x);
        
        transform.setRotation(quat);
        m_Body->setWorldTransform(transform);
    }

    glm::vec3 RigidBody::GetRotation() const
    {
        const btTransform transform = m_Body->getWorldTransform();
        const btQuaternion quat = transform.getRotation();
        btScalar x, y, z;
        quat.getEulerZYX(z, y, x);
        
        return glm::degrees(glm::vec3(x, y, z));
    }

    glm::vec3 RigidBody::GetVelocity() const
    {
        btVector3 vel = m_Body->getLinearVelocity();
        return {vel.x(), vel.y(), vel.z()};
    }

    void RigidBody::ApplyImpulse(const glm::vec3& impulse) const
    {
        m_Body->activate(true);
        m_Body->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
    }

    void RigidBody::SetTrigger(const bool isTrigger)
    {
        m_Properties.isTrigger = isTrigger;
        if (isTrigger)
        {
            m_Body->setCollisionFlags(m_Body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        }
        else
        {
            m_Body->setCollisionFlags(m_Body->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
        }
    }

    void RigidBody::ResetVelocity() const
    {
        // Check this function (ResetVelocity(); bullet?)
        m_Body->setLinearVelocity(btVector3(0, 0, 0));
        m_Body->setAngularVelocity(btVector3(0, 0, 0));
    }

    void RigidBody::ClearForces() const
    {
        m_Body->clearForces();
    }

    void RigidBody::SetBodyType(Type type) 
    {
        m_Properties.type = type;
        
        if (type == Type::Static) 
        {
            m_Body->setCollisionFlags(m_Body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
            m_Body->setMassProps(0, btVector3(0, 0, 0));
        }
        else 
        {
            m_Body->setCollisionFlags(m_Body->getCollisionFlags() & ~btCollisionObject::CF_STATIC_OBJECT);
            
            if (type == Type::Dynamic) 
            {
                btVector3 inertia(0, 0, 0);
                m_Collider->getShape()->calculateLocalInertia(m_Properties.mass, inertia);
                m_Body->setMassProps(m_Properties.mass, inertia);
            }
        }
        
        m_Body->activate(true);
    }

    void RigidBody::SetMass(float mass) 
    {
        m_Properties.mass = mass;
        
        if (m_Properties.type == Type::Dynamic) 
        {
            btVector3 inertia(0, 0, 0);
            m_Collider->getShape()->calculateLocalInertia(mass, inertia);
            m_Body->setMassProps(mass, inertia);
            m_Body->activate(true);
        }
    }

    void RigidBody::SetUseGravity(bool useGravity) 
    {
        m_Properties.useGravity = useGravity;
        
        // Apply or remove gravity effect
        if (m_Body) {
            if (useGravity) {
                m_Body->setFlags(m_Body->getFlags() & ~BT_DISABLE_WORLD_GRAVITY);
                m_Body->setGravity(btVector3(0, GRAVITY, 0)); // Standard gravity
            } else {
                m_Body->setFlags(m_Body->getFlags() | BT_DISABLE_WORLD_GRAVITY);
                m_Body->setGravity(btVector3(0, 0, 0)); // No gravity
            }
            m_Body->activate(true);
        }
    }

    void RigidBody::SetFreezeX(bool freezeX) 
    {
        m_Properties.freezeX = freezeX;
        UpdateLinearFactor();
        m_Body->activate(true);
    }

    void RigidBody::SetFreezeY(bool freezeY) 
    {
        m_Properties.freezeY = freezeY;
        UpdateLinearFactor();
        m_Body->activate(true);
    }

    void RigidBody::SetFreezeZ(bool freezeZ) 
    {
        m_Properties.freezeZ = freezeZ;
        UpdateLinearFactor();
        m_Body->activate(true);
    }

    void RigidBody::UpdateLinearFactor()
    {
        btVector3 linearFactor(
            m_Properties.freezeX ? 0 : 1,
            m_Properties.freezeY ? 0 : 1,
            m_Properties.freezeZ ? 0 : 1
        );
        m_Body->setLinearFactor(linearFactor);
    }

    void RigidBody::SetFriction(float friction) 
    {
        m_Properties.friction = friction;
        if (m_Body) {
            m_Body->setFriction(friction);
            m_Body->activate(true);
        }
    }

    void RigidBody::SetLinearDrag(float linearDrag) 
    {
        m_Properties.linearDrag = linearDrag;
        if (m_Body) {
            m_Body->setDamping(linearDrag, m_Properties.angularDrag);
            m_Body->activate(true);
        }
    }

    void RigidBody::SetAngularDrag(float angularDrag) 
    {
        m_Properties.angularDrag = angularDrag;
        if (m_Body) {
            m_Body->setDamping(m_Properties.linearDrag, angularDrag);
            m_Body->activate(true);
        }
    }

    void RigidBody::SetFreezeRotX(bool freezeRotX) 
    {
        m_Properties.freezeRotX = freezeRotX;
        UpdateAngularFactor();
        m_Body->activate(true);
    }

    void RigidBody::SetFreezeRotY(bool freezeRotY) 
    {
        m_Properties.freezeRotY = freezeRotY;
        UpdateAngularFactor();
        m_Body->activate(true);
    }

    void RigidBody::SetFreezeRotZ(bool freezeRotZ) 
    {
        m_Properties.freezeRotZ = freezeRotZ;
        UpdateAngularFactor();
        m_Body->activate(true);
    }

    void RigidBody::UpdateAngularFactor()
    {
        btVector3 angularFactor(
            m_Properties.freezeRotX ? 0 : 1,
            m_Properties.freezeRotY ? 0 : 1,
            m_Properties.freezeRotZ ? 0 : 1
        );
        m_Body->setAngularFactor(angularFactor);
    }

    void RigidBody::ApplyTorque(const glm::vec3& torque) const
    {
        m_Body->activate(true);
        m_Body->applyTorque(btVector3(torque.x, torque.y, torque.z));
    }

    void RigidBody::ApplyTorqueImpulse(const glm::vec3& torque) const
    {
        m_Body->activate(true);
        m_Body->applyTorqueImpulse(btVector3(torque.x, torque.y, torque.z));
    }

    void RigidBody::SetAngularVelocity(const glm::vec3& velocity) const
    {
        m_Body->activate(true);
        m_Body->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    }

    glm::vec3 RigidBody::GetAngularVelocity() const
    {
        btVector3 vel = m_Body->getAngularVelocity();
        return {vel.x(), vel.y(), vel.z()};
    }

    void RigidBody::ResizeColliderToFitAABB(const AABB& aabb) {
        if (m_Collider) {
            const glm::vec3 position = GetPosition();
            const glm::vec3 rotation = GetRotation();
            const glm::vec3 velocity = GetVelocity();
            
            m_Collider->ResizeToFitAABB(aabb);
            
            // We need to recreate the rigid body with the updated collider
            btVector3 localInertia(0, 0, 0);
            if (m_Properties.type != Type::Static) {
                m_Collider->getShape()->calculateLocalInertia(m_Properties.mass, localInertia);
            }
            
            // Update the rigidbody construction info
            btRigidBody::btRigidBodyConstructionInfo rbInfo(
                m_Properties.mass,
                m_MotionState,
                m_Collider->getShape(),
                localInertia
            );
            
            // Remove the old body from the physics world if it exists
            if (m_Body) {
                delete m_Body;
            }
            
            // Create new body
            m_Body = new btRigidBody(rbInfo);
            m_Body->setActivationState(DISABLE_DEACTIVATION);
            
            // Restore state
            SetPosition(position);
            SetRotation(rotation);
            SetVelocity(velocity);
            
            // Apply other properties
            m_Body->setFriction(m_Properties.friction);
            m_Body->setDamping(m_Properties.linearDrag, m_Properties.angularDrag);
            UpdateLinearFactor();
            UpdateAngularFactor();
            
            if (m_Properties.isTrigger) {
                m_Body->setCollisionFlags(m_Body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
            }
            
            if (!m_Properties.useGravity) {
                m_Body->setFlags(m_Body->getFlags() | BT_DISABLE_WORLD_GRAVITY);
                m_Body->setGravity(btVector3(0, 0, 0));
            }
        }
    }

} // namespace Coffee