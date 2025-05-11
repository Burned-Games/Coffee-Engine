#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Math/BoundingBox.h"
#include "CoffeeEngine/Math/Frustum.h"
#include "CoffeeEngine/Renderer/Renderer2D.h"
#include <tracy/Tracy.hpp>
#include <unordered_map>
#include <vector>

namespace Coffee {

    template <typename T>
    struct ObjectContainer
    {
        const glm::mat4 transform;
        const AABB aabb;
        mutable AABB transformedAABB;
        const T object;
        mutable int id; //REMOVE

        ObjectContainer(const glm::mat4& transform, const AABB& aabb, const T& obj)
            : transform(transform), aabb(aabb), object(obj) {
            UpdateTransformedAABB();
        }

        void UpdateTransformedAABB() const {
            transformedAABB = aabb.CalculateTransformedAABB(transform);
        }
    };

    template <typename T>
    class OctreeNode
    {
    public:
        AABB aabb;
        bool isLeaf = true;
        int depth = 0; // Depth of the node in the octree
        std::vector<int> objectIDs; // Store only object IDs
        std::array<Scope<OctreeNode>, 8> children;

        void DebugDrawAABB(const std::unordered_map<int, Ref<ObjectContainer<T>>>& objectMap);
    };

    template <typename T>
    class Octree
    {
    public:
        Octree();
        Octree(const AABB& bounds, int maxObjectsPerNode = 8, int maxDepth = 5);
        ~Octree();

        void Insert(Ref<ObjectContainer<T>> object);
        void DebugDraw();
        void Clear();

        std::vector<T> Query(const Frustum& frustum) const;

    private:
        void Insert(OctreeNode<T>& node, int objectID);
        void InsertIntoLeaf(OctreeNode<T>& node, int objectID);
        void InsertIntoChild(OctreeNode<T>& node, int objectID);
        void RedistributeObjects(OctreeNode<T>& node);
        void Subdivide(OctreeNode<T>& node);
        void CreateChildren(OctreeNode<T>& node, const glm::vec3& center);

        void Query(const OctreeNode<T>& node, const Frustum& frustum, std::vector<T>& results) const;

        OctreeNode<T> rootNode;
        int maxObjectsPerNode;
        int maxDepth;
        int objectsCounter = 0;
        std::unordered_map<int, Ref<ObjectContainer<T>>> objectMap; // Centralized storage
    };

    template <typename T>
    void Octree<T>::Insert(Ref<ObjectContainer<T>> object)
    {
        object->id = objectsCounter++;
        object->UpdateTransformedAABB();
        objectMap[object->id] = object; // Store in centralized map
        Insert(rootNode, object->id);
    }

    template <typename T>
    void Octree<T>::DebugDraw()
    {
        rootNode.DebugDrawAABB(objectMap);
    }

    template <typename T>
    void Octree<T>::Insert(OctreeNode<T>& node, int objectID)
    {
        AABB objectTransformedAABB = objectMap.at(objectID)->transformedAABB;

        switch (node.aabb.Intersect(objectTransformedAABB))
        {
            using enum IntersectionType;
            case Outside:
                return;
            case Inside:
            case Intersect:
                if (node.isLeaf)
                {
                    InsertIntoLeaf(node, objectID);
                }
                else
                {
                    InsertIntoChild(node, objectID);
                }
        }
    }

    template <typename T>
    void Octree<T>::InsertIntoLeaf(OctreeNode<T>& node, int objectID)
    {
        node.objectIDs.push_back(objectID);
        if (node.objectIDs.size() > maxObjectsPerNode && node.depth < maxDepth)
        {
            Subdivide(node);
            RedistributeObjects(node);
        }
    }

    template <typename T>
    void Octree<T>::InsertIntoChild(OctreeNode<T>& node, int objectID)
    {
        for (auto& child : node.children)
        {
            if (!child)
                continue;
            if (child->aabb.Intersect(objectMap.at(objectID)->transformedAABB) !=
                IntersectionType::Outside)
            {
                Insert(*child, objectID);
            }
        }
    }

    template <typename T>
    void Octree<T>::RedistributeObjects(OctreeNode<T>& node)
    {
        for (int id : node.objectIDs)
        {
            InsertIntoChild(node, id);
        }
        node.objectIDs.clear();
    }

    template <typename T>
    void Octree<T>::Subdivide(OctreeNode<T>& node)
    {
        if (!node.isLeaf)
            return; // If the node is not a leaf, no need to subdivide

        glm::vec3 center = node.aabb.GetCenter(); // Calculate the center of the node's AABB
        CreateChildren(node, center);            // Create child nodes
        node.isLeaf = false;                     // Mark the node as no longer a leaf
    }

    template <typename T>
    void Octree<T>::CreateChildren(OctreeNode<T>& node, const glm::vec3& center)
    {
        const glm::vec3& min = node.aabb.min;
        const glm::vec3& max = node.aabb.max;

        for (int i = 0; i < 8; ++i)
        {
            glm::vec3 childMin = min;
            glm::vec3 childMax = max;

            // Adjust the min and max coordinates for each child based on the center
            if (i & 1) childMin.x = center.x; else childMax.x = center.x;
            if (i & 2) childMin.y = center.y; else childMax.y = center.y;
            if (i & 4) childMin.z = center.z; else childMax.z = center.z;

            // Create a new child node and set its AABB
            node.children[i] = CreateScope<OctreeNode<T>>();
            node.children[i]->aabb = AABB(childMin, childMax);
            node.children[i]->depth = node.depth + 1;
        }
    }

    template <typename T>
    void Octree<T>::Query(const OctreeNode<T>& node, const Frustum& frustum, std::vector<T>& results) const
    {
        ZoneScoped;

        if (!frustum.Contains(node.aabb))
            return;

        {
            ZoneScopedN("Object_Processing");
            for (int id : node.objectIDs)
            {
                const Ref<ObjectContainer<T>>& object = objectMap.at(id);

                if (frustum.Contains(object->transformedAABB))
                {
                    results.push_back(object->object);
                }
            }
        }

        if (node.isLeaf)
            return;

        {
            for (const auto& child : node.children)
            {
                if (child)
                {
                    Query(*child, frustum, results);
                }
            }
        }
    }

    template <typename T>
    Octree<T>::Octree(const AABB& bounds, int maxObjectsPerNode, int maxDepth) : maxObjectsPerNode(maxObjectsPerNode), maxDepth(maxDepth)
    {
        rootNode.aabb = bounds;
    }

    template <typename T>
    Octree<T>::~Octree()
    {
        Clear();
    }

    template <typename T>
    void OctreeNode<T>::DebugDrawAABB(const std::unordered_map<int, Ref<ObjectContainer<T>>>& objectMap)
    {
            int numObjects = objectIDs.size();

            float green = glm::clamp(numObjects / 10.0f, 0.0f, 1.0f);
            float red = glm::clamp(1.0f - (numObjects / 10.0f), 0.0f, 1.0f);
            glm::vec4 color(red, green, 0.0f, 1.0f);

            Renderer2D::DrawBox(aabb.min, aabb.max, color);

            for (int id : objectIDs)
            {
                const auto& obj = objectMap.at(id);
                AABB aabb = obj->transformedAABB;
                Renderer2D::DrawBox(aabb.min, aabb.max, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
            }

            for (const auto& child : children)
            {
                if (child)
                {
                    child->DebugDrawAABB(objectMap);
                }
            }
    }

    template <typename T>
    void Octree<T>::Clear()
    {
        rootNode.objectIDs.clear();
        for (auto& child : rootNode.children)
        {
            if (child)
            {
                child->objectIDs.clear();
                child.reset();
            }
        }
        rootNode.isLeaf = true;
        objectMap.clear(); // Clear centralized storage
    }

    template <typename T>
    std::vector<T> Octree<T>::Query(const Frustum& frustum) const
    {
        ZoneScopedN("Octree");
        std::vector<T> results;
        Query(rootNode, frustum, results);
        return results;
    }

} // namespace Coffee