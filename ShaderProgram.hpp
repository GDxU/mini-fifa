#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <tuple>

#include "incgraphics.h"
#include "Tuple.hpp"
#include "Shader.hpp"
#include "ShaderAttrib.hpp"

namespace gl {
template <typename... ShaderTs>
class ShaderProgram {
  static GLuint last_used_program;
  GLuint program = 0;
  std::tuple<ShaderTs...> shaders;

  template <typename... Ts> void unroll(...){}

// each
  template <size_t I> int init_each() { std::get<I>(shaders).init(); }
  template <size_t I> int attach_each() { glAttachShader(program, std::get<I>(shaders).id()); GLERROR }
  template <size_t I> int clear_each() { std::get<I>(shaders).clear(); }
// iter
  template <size_t... Is> void init_iter(std::index_sequence<Is...>) { unroll(init_each<Is>()...); }
  template <size_t... Is> void attach_iter(std::index_sequence<Is...>) { unroll(attach_each<Is>()...); }
  template <size_t... Is> void clear_iter(std::index_sequence<Is...>) { unroll(clear_each<Is>()...); }
//

  void compile_program() {
    init_iter(std::make_index_sequence<sizeof...(ShaderTs)>());
    program = glCreateProgram(); GLERROR
    ASSERT(program != 0);
    attach_iter(std::make_index_sequence<sizeof...(ShaderTs)>());
    glLinkProgram(program); GLERROR
    clear_iter(std::make_index_sequence<sizeof...(ShaderTs)>());
  }
  void bind_attrib(const std::vector <std::string> &locations) {
    for(size_t i = 0; i < locations.size(); ++i) {
      glBindAttribLocation(program, i, locations[i].c_str()); GLERROR
    }
  }
public:
  template <typename... STRINGs>
  ShaderProgram(STRINGs... shader_filenames):
    shaders(shader_filenames...)
  {}
  GLuint id() const {
    return program;
  }
  void init(gl::VertexArray &vao, const std::vector <std::string> &&locations) {
    vao.bind();
    compile_program();
    bind_attrib(locations);
    ASSERT(is_valid());
    gl::VertexArray::unbind();
  }
  void use() {
    if(last_used_program != id()) {
      glUseProgram(id()); GLERROR
      last_used_program = id();
    }
  }
  static void unuse() {
    glUseProgram(0); GLERROR
    last_used_program = 0;
  }
  // clear_each
  template <size_t I> int detach_each() { glDetachShader(program, std::get<I>(shaders).id()); GLERROR }
  template <size_t... Is> void detach_iter(std::index_sequence<Is...>) { unroll(detach_each<Is>()...); }
  void clear() {
    detach_iter(std::make_index_sequence<sizeof...(ShaderTs)>());
    /* Tuple::for_each(shaders, [&](const auto &s) { */
    /*   glDetachShader(program, s.id()); GLERROR */
    /* }); */
    glDeleteProgram(program); GLERROR
  }

  bool is_valid() {
    glValidateProgram(program);
    int params = -1;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &params);
    Logger::Info("program %d GL_VALIDATE_STATUS = %d\n", program, params);
    print_info_log();
    print_all();
    if (GL_TRUE != params) {
      return false;
    }
    return true;
  }
  static const char* GL_type_to_string(GLenum type) {
    switch(type) {
      case GL_BOOL: return "bool";
      case GL_INT: return "int";
      case GL_FLOAT: return "float";
      case GL_FLOAT_VEC2: return "vec2";
      case GL_FLOAT_VEC3: return "vec3";
      case GL_FLOAT_VEC4: return "vec4";
      case GL_FLOAT_MAT2: return "mat2";
      case GL_FLOAT_MAT3: return "mat3";
      case GL_FLOAT_MAT4: return "mat4";
      case GL_SAMPLER_2D: return "sampler2D";
      case GL_SAMPLER_3D: return "sampler3D";
      case GL_SAMPLER_CUBE: return "samplerCube";
      case GL_SAMPLER_2D_SHADOW: return "sampler2DShadow";
      default: break;
    }
    return "other";
  }
  void print_info_log() {
    int max_length = 2048;
    int actual_length = 0;
    char program_log[2048];
    glGetProgramInfoLog(program, max_length, &actual_length, program_log); GLERROR
    Logger::Info("program info log for GL index %u:\n%s", program, program_log);
  }
  void print_all() {
    Logger::Info("--------------------\nshader program %d info:\n", program);
    int params = -1;
    glGetProgramiv(program, GL_LINK_STATUS, &params);
    Logger::Info("GL_LINK_STATUS = %d\n", params);

    glGetProgramiv(program, GL_ATTACHED_SHADERS, &params);
    Logger::Info("GL_ATTACHED_SHADERS = %d\n", params);

    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &params);
    Logger::Info("GL_ACTIVE_ATTRIBUTES = %d\n", params);
    for (int i = 0; i < params; i++) {
      char name[64];
      int max_length = 64;
      int actual_length = 0;
      int size = 0;
      GLenum type;
      glGetActiveAttrib (
        program,
        i,
        max_length,
        &actual_length,
        &size,
        &type,
        name
      );
      if(size > 1) {
        for(int j = 0; j < size; j++) {
          char long_name[64];
          sprintf(long_name, "%s[%d]", name, j);
          int location = glGetAttribLocation(program, long_name);
          Logger::Info("  %d) type:%s name:%s location:%d\n",
                 i, GL_type_to_string(type), long_name, location);
        }
      } else {
        int location = glGetAttribLocation(program, name);
        Logger::Info("  %d) type:%s name:%s location:%d\n",
               i, GL_type_to_string(type), name, location);
      }
    }

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &params);
    Logger::Info("GL_ACTIVE_UNIFORMS = %d\n", params);
    for(int i = 0; i < params; i++) {
      char name[64];
      int max_length = 64;
      int actual_length = 0;
      int size = 0;
      GLenum type;
      glGetActiveUniform(
      program,
      i,
      max_length,
      &actual_length,
      &size,
      &type,
      name
      );
      if(size > 1) {
      for(int j = 0; j < size; j++) {
        char long_name[64];
        sprintf(long_name, "%s[%d]", name, j);
        int location = glGetUniformLocation(program, long_name);
        Logger::Info("  %d) type:%s name:%s location:%d\n",
           i, GL_type_to_string(type), long_name, location);
      }
      } else {
      int location = glGetUniformLocation(program, name);
      Logger::Info("  %d) type:%s name:%s location:%d\n",
           i, GL_type_to_string(type), name, location);
      }
    }
  }
};
template <typename... Ss>
GLuint gl::ShaderProgram<Ss...>::last_used_program = 0;
}
