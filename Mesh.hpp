#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

#include "Debug.hpp"
#include "Logger.hpp"
#include "ShaderProgram.hpp"
#include "ShaderAttrib.hpp"
#include "Texture.hpp"

struct ModelVertex {
  glm::vec3 pos;
  glm::vec3 nrm;
  glm::vec2 txcoords;
  glm::vec3 tangent;
  glm::vec3 bitan;
};

struct ModelTexture {
  GLuint id;
  std::string type;
  std::string path;
};

struct Mesh {
  std::vector<ModelVertex> vertices;
  std::vector<unsigned> indices;
  std::vector<ModelTexture> textures;

  gl::VertexArray vao;

  Mesh(std::vector<ModelVertex> vertices, std::vector<unsigned> indices, std::vector<ModelTexture> textures):
    vertices(vertices), indices(indices), textures(textures)
  {}

  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC3> vbo;
  gl::Attrib<GL_ELEMENT_ARRAY_BUFFER, gl::AttribType::VEC3> ebo;

  void init() {
    vao.init();
    vbo.init(); ebo.init();
    vao.bind();
    vbo.bind();
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ModelVertex), &vertices[0], GL_STATIC_DRAW); GLERROR
    ebo.bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW); GLERROR
    vao.enable(ebo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), nullptr); GLERROR
    vao.enable(ebo);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, nrm));
    vao.enable(ebo);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, txcoords));
    vao.enable(ebo);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, tangent));
    vao.enable(ebo);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, bitan));
    /* decltype(ebo)::unbind(); */
    decltype(vao)::unbind();
  }

  template <typename... ShaderTs>
  void display(gl::ShaderProgram<ShaderTs...> &program) {
    program.use();
    unsigned
      diffuseNr = 1,
      specNr = 1,
      normalNr = 1,
      heightNr = 1;
    for(unsigned i = 0; i < textures.size(); ++i) {
      int number;
      std::string name = textures[i].type;
      if(name == "texture_diffuse") {
        number = (diffuseNr++);
      } else if(name == "texture_specular") {
        number = (specNr++);
      } else if(name == "texture_normal") {
        number = (normalNr++);
      } else if(name == "texture_height") {
        number = (heightNr++);
      }
      /* Logger::Info("Mesh: field %s\n", (name + number).c_str()); */
      gl::Uniform<gl::UniformType::SAMPLER2D> uSampler((name + std::to_string(number)).c_str());
      uSampler.set_id(program.id());
      uSampler.set_data(i);
      gl::Texture::set_active(i);
      glBindTexture(GL_TEXTURE_2D, textures[i].id); GLERROR
    }
    vao.bind();
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0); GLERROR
    gl::VertexArray::unbind();
    std::remove_reference_t<decltype(program)>::unuse();
  }

  void clear() {
    vbo.clear();
    ebo.clear();
    vao.clear();
  }
};
