#include "LuaMath.h"
#include <glm/fwd.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

void Coffee::RegisterMathBindings(sol::state& luaState)
{
    luaState.new_usertype<glm::vec2>("Vector2",
        sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
        "x", &glm::vec2::x,
        "y", &glm::vec2::y,
        "normalize", [](const glm::vec2& a) { return glm::normalize(a); },
        "length", [](const glm::vec2& a) { return glm::length(a); },
        "length_squared", [](const glm::vec2& a) { return glm::length2(a); },
        "distance_to", [](const glm::vec2& a, const glm::vec2& b) { return glm::distance(a, b); },
        "distance_squared_to", [](const glm::vec2& a, const glm::vec2& b) { return glm::distance2(a, b); },
        "lerp", [](const glm::vec2& a, const glm::vec2& b, float t) { return glm::mix(a, b, t); },
        "dot", [](const glm::vec2& a, const glm::vec2& b) { return glm::dot(a, b); },
        "angle_to", [](const glm::vec2& a, const glm::vec2& b) { return glm::degrees(glm::acos(glm::dot(glm::normalize(a), glm::normalize(b)))); },
        "max", [](const glm::vec2& a, const glm::vec2& b) { return (glm::max)(a, b); },
        "min", [](const glm::vec2& a, const glm::vec2& b) { return (glm::min)(a, b); },
        "abs", [](const glm::vec2& a) { return glm::abs(a); },
        // Improve this to not have to specify the operation for each type
        sol::meta_function::addition, [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
        sol::meta_function::multiplication, [](const glm::vec2& vec, sol::object obj) -> glm::vec2 {
            if (obj.is<float>()) {
                return vec * obj.as<float>(); // Scalar multiplication
            } else if (obj.is<glm::vec2>()) {
                return vec * obj.as<glm::vec2>(); // Component-wise multiplication
            }
            throw std::invalid_argument("Invalid multiplication operand for Vector2");
        },
        sol::meta_function::division, [](const glm::vec2& a, sol::object b) -> glm::vec2 {
            if (b.is<float>()) {
                return a / b.as<float>();
            } else if (b.is<glm::vec2>()) {
                return a / b.as<glm::vec2>();
            }
            throw std::invalid_argument("Invalid division operand for Vector2");
        },
        sol::meta_function::equal_to, [](const glm::vec2& a, const glm::vec2& b) { return a == b; }
        //TODO: Add more functions
    );

    luaState.new_usertype<glm::vec3>("Vector3",
        sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z,

        "cross", [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); },
        "dot", [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },
        "normalize", [](const glm::vec3& a) { return glm::normalize(a); },
        "length", [](const glm::vec3& a) { return glm::length(a); },
        "length_squared", [](const glm::vec3& a) { return glm::length2(a); },
        "distance_to", [](const glm::vec3& a, const glm::vec3& b) { return glm::distance(a, b); },
        "distance_squared_to", [](const glm::vec3& a, const glm::vec3& b) { return glm::distance2(a, b); },
        "lerp", [](const glm::vec3& a, const glm::vec3& b, float t) { return glm::mix(a, b, t); },
        "dot", [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },
        "angle_to", [](const glm::vec3& a, const glm::vec3& b) { return glm::degrees(glm::acos(glm::dot(glm::normalize(a), glm::normalize(b)))); },
        "max", [](const glm::vec3& a, const glm::vec3& b) { return (glm::max)(a, b); },
        "min", [](const glm::vec3& a, const glm::vec3& b) { return (glm::min)(a, b); },
        "abs", [](const glm::vec3& a) { return glm::abs(a); },
        //Improve this to not have to specify the operation for each type
        sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
        sol::meta_function::multiplication, [](const glm::vec3& vec, sol::object obj) -> glm::vec3 {
            if (obj.is<glm::quat>()) {
                const glm::quat& quat = obj.as<glm::quat>();
                return quat * vec; // Rotate the vector by the quaternion
            } else if (obj.is<float>()) {
                return vec * obj.as<float>(); // Scalar multiplication
            } else if (obj.is<glm::vec3>()) {
                return vec * obj.as<glm::vec3>(); // Component-wise multiplication
            }
            throw std::invalid_argument("Invalid multiplication operand for Vector3");
        },
        sol::meta_function::division, [](const glm::vec3& a, sol::object b) -> glm::vec3 {
            if (b.is<float>()) {
                return a / b.as<float>();
            } else if (b.is<glm::vec3>()) {
                return a / b.as<glm::vec3>();
            }
            throw std::invalid_argument("Invalid division operand for Vector3");
        },
        sol::meta_function::equal_to, [](const glm::vec3& a, const glm::vec3& b) { return a == b; },
        //sol::meta_function::greater_than, [](const glm::vec3& a, const glm::vec3& b) { return a > b; },
        //sol::meta_function::greater_than_equal_to, [](const glm::vec3& a, const glm::vec3& b) { return a >= b; }
        //sol::meta_function::not_equal_to, [](const glm::vec3& a, const glm::vec3& b) { return a != b; }
        sol::meta_function::unary_minus, [](const glm::vec3& a) { return -a; }
        //TODO: Add more functions
    );

    luaState.new_usertype<glm::vec4>("Vector4",
        sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float)>(),
        "x", &glm::vec4::x,
        "y", &glm::vec4::y,
        "z", &glm::vec4::z,
        "w", &glm::vec4::w,
        "normalize", [](const glm::vec4& a) { return glm::normalize(a); },
        "length", [](const glm::vec4& a) { return glm::length(a); },
        "length_squared", [](const glm::vec4& a) { return glm::length2(a); },
        "distance_to", [](const glm::vec4& a, const glm::vec4& b) { return glm::distance(a, b); },
        "distance_squared_to", [](const glm::vec4& a, const glm::vec4& b) { return glm::distance2(a, b); },
        "lerp", [](const glm::vec4& a, const glm::vec4& b, float t) { return glm::mix(a, b, t); },
        "dot", [](const glm::vec4& a, const glm::vec4& b) { return glm::dot(a, b); },
        "angle_to", [](const glm::vec4& a, const glm::vec4& b) { return glm::degrees(glm::acos(glm::dot(glm::normalize(a), glm::normalize(b)))); },
        "max", [](const glm::vec4& a, const glm::vec4& b) { return (glm::max)(a, b); },
        "min", [](const glm::vec4& a, const glm::vec4& b) { return (glm::min)(a, b); },
        "abs", [](const glm::vec4& a) { return glm::abs(a); },
        // Improve this to not have to specify the operation for each type
        sol::meta_function::addition, [](const glm::vec4& a, const glm::vec4& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec4& a, const glm::vec4& b) { return a - b; },
        sol::meta_function::multiplication, [](const glm::vec4& a, sol::object b) -> glm::vec4 {
            if (b.is<float>()) {
                return a * b.as<float>();
            } else if (b.is<glm::vec4>()) {
                return a * b.as<glm::vec4>();
            }
            throw std::invalid_argument("Invalid multiplication operand for Vector4");
        },
        sol::meta_function::division, [](const glm::vec4& a, sol::object b) -> glm::vec4 {
            if (b.is<float>()) {
                return a / b.as<float>();
            } else if (b.is<glm::vec4>()) {
                return a / b.as<glm::vec4>();
            }
            throw std::invalid_argument("Invalid division operand for Vector4");
        },
        sol::meta_function::equal_to, [](const glm::vec4& a, const glm::vec4& b) { return a == b; }
        //TODO: Add more functions
    );

    luaState.new_usertype<glm::mat4>("Mat4",
        sol::constructors<glm::mat4(), glm::mat4(float)>(),
        "identity", []() { return glm::mat4(1.0f); },
        "inverse", [](const glm::mat4& mat) { return glm::inverse(mat); },
        "transpose", [](const glm::mat4& mat) { return glm::transpose(mat); },
        "translate", [](const glm::mat4& mat, const glm::vec3& vec) { return glm::translate(mat, vec); },
        "rotate", [](const glm::mat4& mat, float angle, const glm::vec3& axis) { return glm::rotate(mat, angle, axis); },
        "scale", [](const glm::mat4& mat, const glm::vec3& vec) { return glm::scale(mat, vec); },
        "perspective", [](float fovy, float aspect, float nearPlane, float farPlane) { return glm::perspective(fovy, aspect, nearPlane, farPlane); },
        "ortho", [](float left, float right, float bottom, float top, float zNear, float zFar) { return glm::ortho(left, right, bottom, top, zNear, zFar); },
        "forward", [](const glm::mat4& mat) { return glm::vec3(-mat[2]); },
        "right", [](const glm::mat4& mat) { return glm::vec3(mat[0]); },
        "up", [](const glm::mat4& mat) { return glm::vec3(mat[1]); },
        // Improve this to not have to specify the operation for each type
        sol::meta_function::addition, [](const glm::mat4& a, const glm::mat4& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::mat4& a, const glm::mat4& b) { return a - b; },
        sol::meta_function::multiplication, [](const glm::mat4& a, const glm::mat4& b) { return a * b; },
        sol::meta_function::multiplication, [](const glm::mat4& mat, const glm::vec3& vec) { return mat * glm::vec4(vec, 1.0f); },
        sol::meta_function::multiplication, [](const glm::mat4& mat, const glm::vec4& vec) { return mat * vec; },
        sol::meta_function::multiplication, [](const glm::mat4& mat, const glm::quat& quat) { return mat * glm::toMat4(quat); },
        sol::meta_function::division, [](const glm::mat4& a, const glm::mat4& b) { return a / b; },
        sol::meta_function::equal_to, [](const glm::mat4& a, const glm::mat4& b) { return a == b; }
    );

    luaState.new_usertype<glm::quat>("Quaternion",
        sol::constructors<glm::quat(), glm::quat(float, float, float, float), glm::quat(const glm::vec3&), glm::quat(float, const glm::vec3&)>(),
        "x", &glm::quat::x,
        "y", &glm::quat::y,
        "z", &glm::quat::z,
        "w", &glm::quat::w,
        "from_euler", [](const glm::vec3& euler) { return glm::quat(glm::radians(euler)); },
        "to_euler_angles", [](const glm::quat& q) { return glm::eulerAngles(q); },
        "to_mat4", [](const glm::quat& q) { return glm::toMat4(q); },
        "normalize", [](const glm::quat& q) { return glm::normalize(q); },
        "slerp", [](const glm::quat& a, const glm::quat& b, float t) { return glm::slerp(a, b, t); },
        "dot", [](const glm::quat& a, const glm::quat& b) { return glm::dot(a, b); },

        // Improve this to not have to specify the operation for each type
        sol::meta_function::addition, [](const glm::quat& a, const glm::quat& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::quat& a, const glm::quat& b) { return a - b; },
        sol::meta_function::multiplication, [](const glm::quat& a, const glm::quat& b) { return a * b; },
        sol::meta_function::multiplication, [](const glm::quat& quat, const glm::vec3& vec) { return quat * vec; },
        sol::meta_function::multiplication, [](const glm::quat& quat, const glm::vec4& vec) { return quat * vec; },
        sol::meta_function::equal_to, [](const glm::quat& a, const glm::quat& b) { return a == b; },
        sol::meta_function::unary_minus, [](const glm::quat& a) { return -a; }
    );
}