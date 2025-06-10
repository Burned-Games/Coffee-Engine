#pragma once

#include "NavMesh.h"

#include <vector>

namespace Coffee
{
    /**
     * @brief Structure representing a portal between two triangles in the navigation mesh.
     */
    struct NavMeshPortal
    {
        glm::vec3 left; ///< Left vertex of the portal
        glm::vec3 right; ///< Right vertex of the portal
    };

    /**
     * @brief Class for pathfinding on a navigation mesh.
     */
    class NavMeshPathfinding
    {
    public:
        /**
         * @brief Constructor.
         * @param navMesh The navigation mesh to use for pathfinding.
         */
        NavMeshPathfinding(const Ref<NavMesh>& navMesh)
            : m_NavMesh(navMesh) {}

        /**
         * @brief Finds a path from the start point to the end point.
         * @param start The starting point of the path.
         * @param end The ending point of the path.
         * @return A vector of points representing the path.
         */
        std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& end) const;

        /**
         * @brief Renders the path for debugging purposes.
         * @param path The path to render.
         */
        void RenderPath(const std::vector<glm::vec3>& path) const;

        void SetNavMesh(const Ref<NavMesh>& navMesh) { m_NavMesh = navMesh; }

    private:
        /**
         * @brief Finds the triangle containing the given point.
         * @param point The point to check.
         * @return The index of the triangle containing the point, or -1 if no triangle contains the point.
         */
        int FindTriangleContaining(const glm::vec3& point) const;

        /**
         * @brief Calculates the heuristic cost from the current triangle to the goal triangle.
         * @param current The index of the current triangle.
         * @param goal The index of the goal triangle.
         * @param triangles The list of triangles in the navigation mesh.
         * @return The heuristic cost.
         */
        float Heuristic(int current, int goal, const std::vector<NavMeshTriangle>& triangles) const;

        /**
         * @brief Checks if a point is inside a triangle.
         * @param point The point to check.
         * @param triangle The triangle to check against.
         * @return True if the point is inside the triangle, false otherwise.
         */
        bool IsPointInTriangle(const glm::vec3& point, const NavMeshTriangle& triangle) const;

        /**
         * @brief Projects a point onto the plane of a triangle.
         * @param point The point to project.
         * @param triangle The triangle whose plane to project onto.
         * @return The projected point.
         */
        glm::vec3 ProjectPointOnPlane(const glm::vec3& point, const NavMeshTriangle& triangle) const;

        /**
         * @brief Gets the portal between two triangles.
         * @param fromTri The starting triangle.
         * @param toTri The ending triangle.
         * @return The portal between the two triangles.
         */
        NavMeshPortal GetPortal(const NavMeshTriangle& fromTri, const NavMeshTriangle& toTri) const;

        /**
         * @brief Performs string pulling on the portals to generate a smooth path.
         * @param portals The list of portals.
         * @param start The starting point of the path.
         * @param end The ending point of the path.
         * @return A vector of points representing the smoothed path.
         */
        std::vector<glm::vec3> StringPull(const std::vector<NavMeshPortal>& portals, const glm::vec3& start, const glm::vec3& end) const;

        /**
         * @brief Finds the closest point on a triangle to a given point.
         * @param point The point to check.
         * @param triangle The triangle to check against.
         * @return The closest point on the triangle.
         */
        glm::vec3 FindClosestPointOnTriangle(const glm::vec3& point, const NavMeshTriangle& triangle) const;

        /**
         * @brief Calculates the distance from a point to a triangle.
         * @param point The point to check.
         * @param triangle The triangle to check against.
         * @return The distance to the triangle.
         */
        float DistanceToTriangle(const glm::vec3& point, const NavMeshTriangle& triangle) const;

        /**
         * @brief Calculates the signed area of a triangle in 2D.
         * @param a The first vertex of the triangle.
         * @param b The second vertex of the triangle.
         * @param c The third vertex of the triangle.
         * @return The signed area of the triangle.
         */
        float TriArea2D(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) const;

        /**
         * @brief Projects a point to the closest point on the navigation mesh.
         * @param point The point to project.
         * @param triangleIndex The index of the starting triangle to check.
         * @return The projected point on the navigation mesh.
         */
        glm::vec3 ProjectPointToNavMesh(const glm::vec3& point, int triangleIndex) const;

    private:
        Ref<NavMesh> m_NavMesh; ///< The navigation mesh used for pathfinding
    };
}