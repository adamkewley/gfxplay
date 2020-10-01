#pragma once

#include "gl.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace glglm {
    void Uniform(gl::UniformMatrix4fv& u, glm::mat4 const& mat) {
        gl::Uniform(u, glm::value_ptr(mat));
    }

    void Uniform(gl::UniformVec4f& u, glm::vec4 const& v) {
        glUniform4f(u, v.x, v.y, v.z, v.w);
    }

    void Uniform(gl::UniformVec3f& u, glm::vec3 const& v) {
        glUniform3f(u, v.x, v.y, v.z);
    }
}
