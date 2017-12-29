#pragma once

#include <string>
#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "incgraphics.h"
#include "ShaderUniform.hpp"
#include "Debug.hpp"

namespace gl {
enum class UniformType {
  INTEGER, FLOAT,
  VEC2, VEC3, VEC4,
  MAT2, MAT3, MAT4,
  SAMPLER2D
};

#define using_sc static constexpr GLenum
template <UniformType U> struct u_cast_type { using type = void; };
template <> struct u_cast_type <UniformType::INTEGER> { using type = GLint; using_sc gltype = GL_INT; };
template <> struct u_cast_type <UniformType::FLOAT> { using type = GLfloat; using_sc gltype = GL_FLOAT; };
template <> struct u_cast_type <UniformType::VEC2> { using type = glm::vec2; using_sc gltype = GL_FLOAT_VEC2; };
template <> struct u_cast_type <UniformType::VEC3> { using type = glm::vec3; using_sc gltype = GL_FLOAT_VEC3; };
template <> struct u_cast_type <UniformType::VEC4> { using type = glm::vec4; using_sc gltype = GL_FLOAT_VEC4; };
template <> struct u_cast_type <UniformType::MAT2> { using type = glm::mat2; using_sc gltype = GL_FLOAT_MAT2; };
template <> struct u_cast_type <UniformType::MAT3> { using type = glm::mat3; using_sc gltype = GL_FLOAT_MAT3; };
template <> struct u_cast_type <UniformType::MAT4> { using type = glm::mat4; using_sc gltype = GL_FLOAT_MAT4; };
template <> struct u_cast_type <UniformType::SAMPLER2D> { using type = GLuint; using_sc gltype = GL_SAMPLER_2D; };
#undef using_sc

template <UniformType U>
struct Uniform {
  using type = typename u_cast_type<U>::type;
  static constexpr GLenum gltype = u_cast_type<U>::gltype;
  using dtype = std::conditional_t<
    std::is_fundamental<type>::value,
      std::remove_reference_t<type>,
      std::add_const_t<std::add_lvalue_reference_t<type>
    >
  >;
  GLuint uniformId = 0;
  GLuint progId = 0;
  std::string location;
  Uniform(const char *loc):
    location(loc)
  {}
  GLuint id() const {
    return uniformId;
  }
  GLuint loc() const {
    ASSERT(location != "");
    GLuint lc = glGetUniformLocation(progId, location.c_str()); GLERROR
    return lc;
  }
  void set_id(GLuint program_id) {
    ASSERT(program_id != 0);
    if(progId == program_id) {
      return;
    }
    progId = program_id;
    uniformId = loc();
  }
  bool is_active() {
    char name[81];
    GLint size;
    GLenum t;
    glGetActiveUniform(progId, uniformId, 80, &size, &t, name); GLERROR
    return t == gltype && location == name;
  }
  void unset_id() {
    progId = 0;
  }
  void set_data(dtype data);
};

template <>
void gl::Uniform<gl::UniformType::INTEGER>::set_data(Uniform<gl::UniformType::INTEGER>::dtype data) {
  ASSERT(progId != 0);
  glUniform1i(uniformId, data); GLERROR
}

template <>
void gl::Uniform<gl::UniformType::FLOAT>::set_data(Uniform<gl::UniformType::FLOAT>::dtype data) {
  ASSERT(progId != 0);
  glUniform1f(uniformId, data); GLERROR
}

template <>
void gl::Uniform<gl::UniformType::VEC2>::set_data(Uniform<gl::UniformType::VEC2>::dtype data) {
  ASSERT(progId != 0);
  glUniform2f(uniformId, data.x, data.y); GLERROR
}

template <>
void gl::Uniform<gl::UniformType::VEC3>::set_data(Uniform<gl::UniformType::VEC3>::dtype data) {
  ASSERT(progId != 0);
  glUniform3f(uniformId, data.x, data.y, data.z); GLERROR
}

template <>
void Uniform<UniformType::VEC4>::set_data(Uniform<UniformType::VEC4>::dtype data) {
  ASSERT(progId != 0);
  glUniform4f(uniformId, data.x, data.y, data.z, data.t); GLERROR
}

template <>
void Uniform<UniformType::MAT2>::set_data(Uniform<UniformType::MAT2>::dtype data) {
  ASSERT(progId != 0);
  glUniformMatrix2fvARB(uniformId, 1 , GL_FALSE, glm::value_ptr(data)); GLERROR
}

template <>
void Uniform<UniformType::MAT3>::set_data(Uniform<UniformType::MAT3>::dtype data) {
  ASSERT(progId != 0);
  glUniformMatrix3fvARB(uniformId, 1 , GL_FALSE, glm::value_ptr(data)); GLERROR
}

template <>
void Uniform<UniformType::MAT4>::set_data(Uniform<UniformType::MAT4>::dtype data) {
  ASSERT(progId != 0);
  glUniformMatrix4fvARB(uniformId, 1 , GL_FALSE, glm::value_ptr(data)); GLERROR
}

template <>
void Uniform<UniformType::SAMPLER2D>::set_data(Uniform<UniformType::SAMPLER2D>::dtype data) {
  ASSERT(progId != 0);
  glUniform1iARB(uniformId, data); GLERROR
}
}
