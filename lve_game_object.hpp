#pragma once

#include "lve_model.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

namespace lve {
    // @todo: maybe implement as quaternion in the future
    struct TransformComponent {
        glm::vec3 translation{};
        glm::vec3 scale{1.f, 1.f, 1.f};
        glm::vec3 rotation{};

        /*
        matrix corresponds to translate * Ry * Rx * Rz * scale
        using tait-bryan angles (Y1X2Z3) not inherently intrinsic or extrinsic, depends on what direction its read in
        */
        // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
        glm::mat4 mat4();
        glm::mat3 normalMatrix();
    };


    class LveGameObject {
    public:
        using id_t = unsigned int;
        static LveGameObject createGameObject() {
            static id_t currentId = 0;
            return LveGameObject{currentId++};
        }
        struct RigidBody2dComponent {
            glm::vec2 velocity;
            float mass{1.0f};
        };

        LveGameObject(const LveGameObject &) = delete;
        LveGameObject &operator=(const LveGameObject &) = delete;
        LveGameObject(LveGameObject &&) = default;
        LveGameObject &operator=(LveGameObject &&) = default;

        const id_t getId() { return id; }

        std::shared_ptr<LveModel> model{};
        glm::vec3 color{};
        TransformComponent transform{};
        RigidBody2dComponent rigidBody2d;

    private:
        LveGameObject(id_t objId) : id(objId) {}

        id_t id;
    };
} // namespace lve
