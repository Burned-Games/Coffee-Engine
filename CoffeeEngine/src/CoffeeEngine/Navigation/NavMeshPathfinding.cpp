#include "NavMeshPathfinding.h"

#include "CoffeeEngine/Renderer/Renderer2D.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <queue>

namespace Coffee
{
    std::vector<glm::vec3> NavMeshPathfinding::FindPath(const glm::vec3& start, const glm::vec3& end) const
    {
        std::vector<glm::vec3> path;

        const std::vector<NavMeshTriangle>& triangles = m_NavMesh->GetTriangles();
        if (triangles.empty())
            return path;

        int startTriangle = FindTriangleContaining(start);
        int endTriangle = FindTriangleContaining(end);

        if (startTriangle == -1 || endTriangle == -1)
            return path;

        glm::vec3 projectedStart = ProjectPointToNavMesh(start, startTriangle);
        glm::vec3 projectedEnd = ProjectPointToNavMesh(end, endTriangle);

        if (startTriangle == endTriangle)
        {
            path.push_back(projectedStart);
            path.push_back(projectedEnd);
            return path;
        }

        std::unordered_map<int, float> gScore;
        std::unordered_map<int, float> fScore;
        std::unordered_map<int, int> cameFrom;

        auto compare = [&fScore](int a, int b) { return fScore[a] > fScore[b]; };
        std::priority_queue<int, std::vector<int>, decltype(compare)> openSet(compare);

        for (int i = 0; i < triangles.size(); i++)
        {
            gScore[i] = std::numeric_limits<float>::infinity();
            fScore[i] = std::numeric_limits<float>::infinity();
        }

        gScore[startTriangle] = 0.0f;
        fScore[startTriangle] = Heuristic(startTriangle, endTriangle, triangles);
        openSet.push(startTriangle);

        while (!openSet.empty())
        {
            int current = openSet.top();
            openSet.pop();

            if (current == endTriangle)
                break;

            for (int neighbor : triangles[current].neighbors)
            {
                float tentativeGScore = gScore[current] + glm::length(
                    triangles[current].center - triangles[neighbor].center);

                if (tentativeGScore < gScore[neighbor])
                {
                    cameFrom[neighbor] = current;
                    gScore[neighbor] = tentativeGScore;
                    fScore[neighbor] = tentativeGScore + Heuristic(neighbor, endTriangle, triangles);

                    openSet.push(neighbor);
                }
            }
        }

        if (cameFrom.find(endTriangle) != cameFrom.end() || startTriangle == endTriangle)
        {
            std::vector<int> trianglePath;
            int current = endTriangle;

            trianglePath.push_back(current);

            while (current != startTriangle)
            {
                current = cameFrom[current];
                trianglePath.push_back(current);
            }

            std::reverse(trianglePath.begin(), trianglePath.end());

            std::vector<NavMeshPortal> portals;

            NavMeshPortal startPortal;
            startPortal.left = projectedStart;
            startPortal.right = projectedStart;
            portals.push_back(startPortal);

            for (size_t i = 0; i < trianglePath.size() - 1; ++i)
            {
                int fromTri = trianglePath[i];
                int toTri = trianglePath[i + 1];

                portals.push_back(GetPortal(triangles[fromTri], triangles[toTri]));
            }

            NavMeshPortal endPortal;
            endPortal.left = projectedEnd;
            endPortal.right = projectedEnd;
            portals.push_back(endPortal);

            path = StringPull(portals, projectedStart, projectedEnd);
        }

        return path;
    }

    glm::vec3 NavMeshPathfinding::ProjectPointToNavMesh(const glm::vec3& point, int triangleIndex) const
    {
        const std::vector<NavMeshTriangle>& triangles = m_NavMesh->GetTriangles();
        if (triangleIndex < 0 || triangleIndex >= triangles.size())
            return point;

        const NavMeshTriangle& triangle = triangles[triangleIndex];
        return FindClosestPointOnTriangle(point, triangle);
    }

    void NavMeshPathfinding::RenderPath(const std::vector<glm::vec3>& path) const
    {
        constexpr glm::vec4 pathColor(1.0f, 0.0f, 0.0f, 1.0f);

        if (path.size() < 2)
            return;

        for (size_t i = 0; i < path.size() - 1; i++)
        {
            Renderer2D::DrawLine(path[i], path[i + 1], pathColor, 30.0f);

            Renderer2D::DrawSphere(path[i], 0.1f, glm::identity<glm::quat>(), pathColor);
        }

        Renderer2D::DrawSphere(path.back(), 0.1f, glm::identity<glm::quat>(), pathColor);
    }

    int NavMeshPathfinding::FindTriangleContaining(const glm::vec3& point) const
    {
        const std::vector<NavMeshTriangle>& triangles = m_NavMesh->GetTriangles();
        if (triangles.empty())
            return -1;

        for (int i = 0; i < triangles.size(); i++)
        {
            if (IsPointInTriangle(point, triangles[i]))
                return i;
        }

        float closestDist = std::numeric_limits<float>::max();
        int closestTriangle = -1;

        for (int i = 0; i < triangles.size(); i++)
        {
            float dist = DistanceToTriangle(point, triangles[i]);
            if (dist < closestDist)
            {
                closestDist = dist;
                closestTriangle = i;
            }
        }

        return closestTriangle;
    }

    bool NavMeshPathfinding::IsPointInTriangle(const glm::vec3& point, const NavMeshTriangle& triangle) const
    {
        glm::vec3 projected = ProjectPointOnPlane(point, triangle);

        const glm::vec3& a = triangle.vertices[0];
        const glm::vec3& b = triangle.vertices[1];
        const glm::vec3& c = triangle.vertices[2];

        glm::vec3 v0 = c - a;
        glm::vec3 v1 = b - a;
        glm::vec3 v2 = projected - a;

        float dot00 = glm::dot(v0, v0);
        float dot01 = glm::dot(v0, v1);
        float dot02 = glm::dot(v0, v2);
        float dot11 = glm::dot(v1, v1);
        float dot12 = glm::dot(v1, v2);

        float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
        float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
        float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

        return (u >= 0) && (v >= 0) && (u + v <= 1);
    }

    glm::vec3 NavMeshPathfinding::ProjectPointOnPlane(const glm::vec3& point, const NavMeshTriangle& triangle) const
    {
        const glm::vec3& normal = triangle.normal;
        const glm::vec3& planePoint = triangle.vertices[0];

        float distance = glm::dot(normal, point - planePoint);
        return point - distance * normal;
    }

    float NavMeshPathfinding::Heuristic(int current, int goal, const std::vector<NavMeshTriangle>& triangles) const
    {
        return glm::length(triangles[current].center - triangles[goal].center);
    }

    NavMeshPortal NavMeshPathfinding::GetPortal(const NavMeshTriangle& fromTri, const NavMeshTriangle& toTri) const
    {
        NavMeshPortal portal;

        std::vector<glm::vec3> sharedVerts;

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                if (glm::distance2(fromTri.vertices[i], toTri.vertices[j]) < 0.001f)
                {
                    sharedVerts.push_back(fromTri.vertices[i]);
                    if (sharedVerts.size() == 2)
                        break;
                }
            }
            if (sharedVerts.size() == 2)
                break;
        }

        if (sharedVerts.size() != 2)
        {
            portal.left = fromTri.center;
            portal.right = toTri.center;
            return portal;
        }

        glm::vec3 triCenter = fromTri.center;
        glm::vec3 toCenter = toTri.center;
        glm::vec3 dir = glm::normalize(toCenter - triCenter);

        glm::vec3 normal = fromTri.normal;
        glm::vec3 cross = glm::cross(dir, normal);

        float dot0 = glm::dot(cross, sharedVerts[0] - triCenter);

        if (dot0 > 0)
        {
            portal.left = sharedVerts[0];
            portal.right = sharedVerts[1];
        }
        else
        {
            portal.left = sharedVerts[1];
            portal.right = sharedVerts[0];
        }

        return portal;
    }

    std::vector<glm::vec3> NavMeshPathfinding::StringPull(const std::vector<NavMeshPortal>& portals, const glm::vec3& start, const glm::vec3& end) const
    {
        std::vector<glm::vec3> path;
        if (portals.empty())
        {
            path.push_back(start);
            path.push_back(end);
            return path;
        }

        path.push_back(start);

        glm::vec3 apex = start;
        glm::vec3 leftLeg = portals[0].left;
        glm::vec3 rightLeg = portals[0].right;

        int apexIndex = 0;
        int leftIndex = 0;
        int rightIndex = 0;

        for (size_t i = 1; i < portals.size(); ++i)
        {
            const glm::vec3& left = portals[i].left;
            const glm::vec3& right = portals[i].right;

            if (TriArea2D(apex, rightLeg, right) <= 0.0f)
            {
                if (apex == rightLeg || TriArea2D(apex, leftLeg, right) > 0.0f)
                {
                    rightLeg = right;
                    rightIndex = i;
                }
                else
                {
                    path.push_back(leftLeg);

                    apex = leftLeg;
                    apexIndex = leftIndex;

                    leftLeg = apex;
                    rightLeg = apex;

                    leftIndex = apexIndex;
                    rightIndex = apexIndex;

                    i = apexIndex;
                    continue;
                }
            }

            if (TriArea2D(apex, leftLeg, left) >= 0.0f)
            {
                if (apex == leftLeg || TriArea2D(apex, rightLeg, left) < 0.0f)
                {
                    leftLeg = left;
                    leftIndex = i;
                }
                else
                {
                    path.push_back(rightLeg);

                    apex = rightLeg;
                    apexIndex = rightIndex;

                    leftLeg = apex;
                    rightLeg = apex;

                    leftIndex = apexIndex;
                    rightIndex = apexIndex;

                    i = apexIndex;
                }
            }
        }

        path.push_back(end);

        return path;
    }

    float NavMeshPathfinding::TriArea2D(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) const
    {
        float ax = b.x - a.x;
        float az = b.z - a.z;
        float bx = c.x - a.x;
        float bz = c.z - a.z;
        return bx * az - ax * bz;
    }

    glm::vec3 NavMeshPathfinding::FindClosestPointOnTriangle(const glm::vec3& point, const NavMeshTriangle& triangle) const
    {
        glm::vec3 projected = ProjectPointOnPlane(point, triangle);

        if (IsPointInTriangle(projected, triangle))
            return projected;

        glm::vec3 closestPoint;
        float minDistSq = std::numeric_limits<float>::max();

        for (int i = 0; i < 3; i++)
        {
            int next = (i + 1) % 3;

            glm::vec3 edge = triangle.vertices[next] - triangle.vertices[i];
            float edgeLengthSq = glm::length2(edge);

            float t = glm::dot(projected - triangle.vertices[i], edge) / edgeLengthSq;
            t = glm::clamp(t, 0.0f, 1.0f);

            glm::vec3 pointOnEdge = triangle.vertices[i] + t * edge;
            float distSq = glm::distance2(projected, pointOnEdge);

            if (distSq < minDistSq)
            {
                minDistSq = distSq;
                closestPoint = pointOnEdge;
            }
        }

        return closestPoint;
    }

    float NavMeshPathfinding::DistanceToTriangle(const glm::vec3& point, const NavMeshTriangle& triangle) const
    {
        glm::vec3 closestPoint = FindClosestPointOnTriangle(point, triangle);
        return glm::length(point - closestPoint);
    }
}