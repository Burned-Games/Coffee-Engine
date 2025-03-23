#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Math/BoundingBox.h"
#include "CoffeeEngine/Math/Frustum.h"
#include "CoffeeEngine/Renderer/Renderer2D.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include <vector>
#include <memory>
#include "CoffeeEngine/Renderer/Renderer2D.h"

namespace Coffee {

    template <typename T>
    struct ObjectContainer
    {
        const glm::mat4& transform;
        const AABB& aabb;
        const T& object;
    };

    template <typename T>
    class OctreeNode
    {
    public:
        AABB aabb;
        bool isLeaf = true;
        std::vector<ObjectContainer<T>> objectList;
        std::array<Scope<OctreeNode>, 8> children;

        void DebugDrawAABB();
        int GetChildIndex(const AABB& bounds, const glm::vec3& point) const;
    };

    template <typename T>
    class Octree
    {
    public:
        Octree();
        Octree(const AABB& bounds, int maxObjectsPerNode = 8, int maxDepth = 5);
        ~Octree();

        void Insert(const ObjectContainer<T>& object);
        void DebugDraw();
        void Clear();

        std::vector<ObjectContainer<T>> Query(const Frustum& frustum) const;

    private:
        void Insert(OctreeNode<T>& node, const ObjectContainer<T>& object);
        void InsertIntoLeaf(OctreeNode<T>& node, const ObjectContainer<T>& object);
        void InsertIntoChild(OctreeNode<T>& node, const ObjectContainer<T>& object);
        void RedistributeObjects(OctreeNode<T>& node);
        void Subdivide(OctreeNode<T>& node);
        void CreateChildren(OctreeNode<T>& node, const glm::vec3& center);

        void Query(const OctreeNode<T>& node, const Frustum& frustum, std::vector<ObjectContainer<T>>& results) const;

        OctreeNode<T> rootNode;
        int maxObjectsPerNode;
        int maxDepth;
    };

    template <typename T>
    void Octree<T>::Insert(OctreeNode<T>& node, const ObjectContainer<T>& object)
    {
        AABB objectTransformedAABB = object.aabb.CalculateTransformedAABB(object.transform);

        switch (node.aabb.Intersect(objectTransformedAABB))
        {
            using enum IntersectionType;
            case Outside:
                return;
            case Inside:
            case Intersect:
                if (node.isLeaf)
                {
                    InsertIntoLeaf(node, object);
                }
                else
                {
                    InsertIntoChild(node, object);
                }
        }
    }

    template <typename T>
    void Octree<T>::InsertIntoLeaf(OctreeNode<T>& node, const ObjectContainer<T>& object) {
        node.objectList.push_back(object);
        if (node.objectList.size() > maxObjectsPerNode && maxDepth > 0) {
            Subdivide(node);
            RedistributeObjects(node);
        }
    }

    template <typename T>
    void Octree<T>::InsertIntoChild(OctreeNode<T>& node, const ObjectContainer<T>& object) {
        int childIndex = node.GetChildIndex(node.aabb, object.transform[3]);
        Insert(*node.children[childIndex], object);
    }

    template <typename T>
    void Octree<T>::RedistributeObjects(OctreeNode<T>& node) {
        for (const auto& obj : node.objectList) {
            int childIndex = node.GetChildIndex(node.aabb, obj.transform[3]);
            Insert(*node.children[childIndex], obj);
        }
        node.objectList.clear();
    }

    template <typename T>
    void Octree<T>::Insert(const ObjectContainer<T>& object) {
        Insert(rootNode, object);
    }

    template <typename T>
    void Octree<T>::Subdivide(OctreeNode<T>& node)
    {
        glm::vec3 center = (node.aabb.min + node.aabb.max) * 0.5f;
        CreateChildren(node, center);
        node.isLeaf = false;
    }

    template <typename T>
    void Octree<T>::CreateChildren(OctreeNode<T>& node, const glm::vec3& center) {
        node.children[0] = CreateScope<OctreeNode<T>>(AABB(node.aabb.min, center));
        node.children[1] = CreateScope<OctreeNode<T>>(AABB(glm::vec3(center.x, node.aabb.min.y, node.aabb.min.z),
                                                        glm::vec3(node.aabb.max.x, center.y, center.z)));
        node.children[2] = CreateScope<OctreeNode<T>>(AABB(glm::vec3(node.aabb.min.x, center.y, node.aabb.min.z),
                                                        glm::vec3(center.x, node.aabb.max.y, center.z)));
        node.children[3] = CreateScope<OctreeNode<T>>(AABB(glm::vec3(center.x, center.y, node.aabb.min.z),
                                                        glm::vec3(node.aabb.max.x, node.aabb.max.y, center.z)));
        node.children[4] = CreateScope<OctreeNode<T>>(AABB(glm::vec3(node.aabb.min.x, node.aabb.min.y, center.z),
                                                        glm::vec3(center.x, center.y, node.aabb.max.z)));
        node.children[5] = CreateScope<OctreeNode<T>>(AABB(glm::vec3(center.x, node.aabb.min.y, center.z),
                                                        glm::vec3(node.aabb.max.x, center.y, node.aabb.max.z)));
        node.children[6] = CreateScope<OctreeNode<T>>(AABB(glm::vec3(node.aabb.min.x, center.y, center.z),
                                                        glm::vec3(center.x, node.aabb.max.y, node.aabb.max.z)));
        node.children[7] = CreateScope<OctreeNode<T>>(AABB(center, node.aabb.max));
    }

    template <typename T>
    void Octree<T>::Query(const OctreeNode<T>& node, const Frustum& frustum, std::vector<ObjectContainer<T>>& results) const
    {
        if (!frustum.Contains(node.aabb))
            return;

        for (const auto& object : node.objectList)
        {
            if (frustum.Contains(object.aabb.CalculateTransformedAABB(object.transform)))
                results.push_back(object);
        }

        if (node.isLeaf)
            return;

        for (const auto& child : node.children)
        {
            if (child)
            {
                Query(*child, frustum, results);
            }
        }
    }

    template <typename T>
    void OctreeNode<T>::DebugDrawAABB()
    {
        // Assuming you have a function to get the number of objects in the node
        int numObjects = objectList.size();

        // Calculate the color based on the number of objects
        float green = glm::clamp(numObjects / 10.0f, 0.0f, 1.0f);
        float red = glm::clamp(1.0f - (numObjects / 10.0f), 0.0f, 1.0f);
        glm::vec4 color(red, green, 0.0f, 1.0f);

        // Draw the box with the calculated color
        Renderer2D::DrawBox(aabb.min, aabb.max, color);
        if (!isLeaf)
        {
            for (auto& child : children)
            {
                if (child)
                {
                    child->DebugDrawAABB();
                }
            }
        }

        for (auto& obj : objectList)
        {
            AABB aabb = obj.aabb.CalculateTransformedAABB(obj.transform);
            Renderer2D::DrawBox(aabb.min, aabb.max, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        }
    }

    template <typename T>
    int OctreeNode<T>::GetChildIndex(const AABB& bounds, const glm::vec3& point) const
    {
        glm::vec3 center = (bounds.min + bounds.max) * 0.5f;
        int index = 0;
        if (point.x > center.x) index |= 1;
        if (point.y > center.y) index |= 2;
        if (point.z > center.z) index |= 4;
        return index;
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
    void Octree<T>::DebugDraw()
    {
        rootNode.DebugDrawAABB();
    }

    template <typename T>
    void Octree<T>::Clear()
    {
        rootNode.objectList.clear();
        for (auto& child : rootNode.children) {
            if (child) {
                child->objectList.clear();
                child.reset();
            }
        }
        rootNode.isLeaf = true;
    }

    template <typename T>
    std::vector<ObjectContainer<T>> Octree<T>::Query(const Frustum& frustum) const
    {
        std::vector<ObjectContainer<T>> results;
        Query(rootNode, frustum, results);
        return results;
    }

} // namespace Coffee