#pragma once

#include <cmath>

#include "incgraphics.h"
#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "Shadow.hpp"

#include "Timer.hpp"
#include "Unit.hpp"

struct Player;

struct Ball {
  static constexpr size_t DIM = 10; // number of steps per dimension to perform for tessellation
  static constexpr size_t SIZE = DIM*DIM*4; // number of triangles used in tessellation

  Transformation transform;
  glm::mat4 matrix;

  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::Uniform<gl::UniformType::VEC3> uColor;
  gl::VertexArray vao;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC3> attrVertex;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrTexcoord;
  gl::Texture ballTx;
  Shadow shadow;

  using ShaderAttribVEC3 = decltype(attrVertex);
  using ShaderAttribVEC2 = decltype(attrTexcoord);
  using ShaderProgram = decltype(program);

  Ball():
    uTransform("transform"),
    uColor("color"),
    program({"shaders/ball.vert", "shaders/ball.frag"}),
    ballTx("ballTx"),
    attrVertex("vpos"),
    attrTexcoord("vtex"),
    unit(Unit::vec_t(0, 0, 0), M_PI * 4)
  {
    transform.SetScale(.01, .01, .01);
    transform.SetPosition(unit.pos.x, unit.pos.y, unit.pos.z);
    transform.SetRotation(1, 0, 0, 180.f);
    reset_height();
    shadow.transform.SetScale(.01);
  }

  // tesselate a sphere with triangles
  // this function takes two angles and constructs a triangle with them
  glm::vec3 point_on_sphere(float dyx, float dzx) {
    return glm::vec3(
      std::sin(dyx) * std::cos(dzx),
      std::cos(dyx),
      std::sin(dyx) * std::sin(dzx)
    );
  }

  // set vertices of the tessellation
  void set_vertices(float *verts) {
    float dimf = DIM;
    for(int index = 0; index < SIZE; ++index) {
      int IF = index & 1;

      const float step = M_PI / dimf;
      const float dyx = (float)((index/2) / (DIM*2)) * step;
      const float dzx = (float)((index/2) % (DIM*2)) * step;

      const int offset = index * 9;
      glm::vec3
        a = point_on_sphere(dyx,dzx),
        b = point_on_sphere(dyx + (IF?0:step), dzx+step),
        c = point_on_sphere(dyx + step, dzx + (IF?step:0));
      float *v = &verts[offset];
      v[0]=a.x,v[1]=a.y,v[2]=a.z;
      v[3]=b.x,v[4]=b.y,v[5]=b.z;
      v[6]=c.x,v[7]=c.y,v[8]=c.z;
    }
  }

  // set texture coordinates for triangles used to tessellate
  void set_texcoords(float *coords) {
    float dimf = DIM;
    for(int index = 0; index < SIZE; ++index) {
      int IF = index & 1;

      const int offset = index * 6;
      const float tx0 = (index/2) % (2*DIM) / (2.*dimf);
      const float txstep = .5/dimf;
      const float ty0 = (DIM - 1 - (index/2) / (2*DIM)) / dimf;
      const float tystep = 1./dimf;
      coords[offset+0] = tx0;
      coords[offset+1] = ty0 + tystep;
      coords[offset+2] = tx0 + txstep;
      coords[offset+3] = ty0 + tystep*(IF?1:0);
      coords[offset+4] = tx0 + txstep*(IF?1:0);
      coords[offset+5] = ty0;
    }
  }

  void init() {
    float *vertices = new float[SIZE*9];
    set_vertices(vertices);
    ShaderAttribVEC3::init(attrVertex);
    attrVertex.allocate<GL_STREAM_DRAW>(SIZE*3, vertices);
    delete [] vertices;
    float *txcoords = new float[SIZE*6];
    set_texcoords(txcoords);
    ShaderAttribVEC2::init(attrTexcoord);
    attrTexcoord.allocate<GL_STREAM_DRAW>(SIZE*3, txcoords);
    delete [] txcoords;
    gl::VertexArray::init(vao);
    gl::VertexArray::bind(vao);
    vao.enable(attrVertex);
    vao.set_access(attrVertex, 0, 0);
    vao.enable(attrTexcoord);
    vao.set_access(attrTexcoord, 1, 0);
    glVertexAttribDivisor(0, 0); GLERROR
    glVertexAttribDivisor(1, 0); GLERROR
    gl::VertexArray::unbind();
    ShaderProgram::init(program, vao, {"vpos", "vtex"});
    ballTx.init("assets/ball.png");

    ballTx.uSampler.set_id(program.id());
    uTransform.set_id(program.id());
    uColor.set_id(program.id());
    gl::VertexArray::unbind();

    shadow.init();
    set_timer();
  }

  void keyboard(int key) {
    if(key == GLFW_KEY_X) {
      /* unit.pos = Unit::loc_t(0, 0, .2); */
    } else if(key == GLFW_KEY_T) {
      /* speed = Unit::vec_t(1.25, 0, 1); */
    } else if(key == GLFW_KEY_Y) {
      /* speed = Unit::vec_t(-1.25, 0, 1); */
    }
  }

  void display(Camera &cam) {
    shadow.transform.SetPosition(unit.pos.x, unit.pos.y, .001);
    shadow.display(cam);

    ShaderProgram::use(program);

    if(transform.has_changed || cam.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    {
      glm::vec3 color(1, 1, 1);
      if(owner() != NO_OWNER) {
        color.x = .8;
      }
      if(is_in_air) {
        color.y = .8;
      }
      uColor.set_data(color);
    }

    gl::Texture::set_active(0);
    ballTx.bind();
    ballTx.set_data(0);

    gl::VertexArray::bind(vao);
    glDrawArrays(GL_TRIANGLES, 0, SIZE*3); GLERROR
    gl::VertexArray::unbind();

    gl::Texture::unbind();
    ShaderProgram::unuse();
  }

  void clear() {
    shadow.clear();
    ballTx.clear();
    ShaderAttribVEC3::clear(attrVertex);
    ShaderAttribVEC2::clear(attrTexcoord);
    gl::VertexArray::clear(vao);
    ShaderProgram::clear(program);
  }

// gameplay
  Unit unit;
  Timer timer;
  static constexpr int
    TIME_LOOSE_BALL_BEGINS = 0,
    TIME_ABLE_TO_INTERACT = 1;
  const float loose_ball_cooldown = 0.1;
  static constexpr Timer::time_t CANT_INTERACT_SHOT = .7;
  static constexpr Timer::time_t CANT_INTERACT_SLIDE = .45;
  /* Unit::vec_t speed{0, 0, 0}; */
  const float default_height = Unit::GAUGE * 10;
  static constexpr float GROUND_FRICTION = .05;
  static constexpr float GROUND_HIT_SLOWDOWN = .02;
  const float G = Unit::GAUGE * 2.3;
  /* const float gravity = 2.3; */
  /* const float mass = 1.0f; */
  const float min_speed = Unit::GAUGE;
  float vertical_speed = 0.;
  bool is_in_air = false;
  static constexpr int NO_OWNER = -1;
  int current_owner = NO_OWNER;
  int last_touched = NO_OWNER;

  void set_timer() {
    reset_height();
    timer.set_timeout(TIME_LOOSE_BALL_BEGINS, loose_ball_cooldown);
  }

  Unit::vec_t &position() { return unit.pos; }
  /* Unit::vec_t &velocity() { return speed; } */
  void reset_height() {
    unit.height() = default_height;
  }

  void idle(double curtime) {
    /* printf("ball pos: %f %f %f\n", unit.pos.x,unit.pos.y,unit.pos.z); */
    /* printf("ball speed: %f %f\n", unit.moving_speed, vertical_speed); */
    timer.set_time(curtime);
    Timer::time_t timediff = timer.elapsed();
    if(owner() == NO_OWNER || is_loose()) {
      if(unit.moving_speed < min_speed) {
        unit.stop();
        unit.moving_speed = min_speed;
      } else {
        unit.move(unit.point_offset(1., unit.facing_dest));
      }

      if(is_in_air) {
        if(vertical_speed < 0. && unit.height() <= default_height) {
          // ball hits the ground
          unit.moving_speed -= GROUND_HIT_SLOWDOWN;
          reset_height();
          if(std::abs(vertical_speed) < min_speed) {
            is_in_air = false;
            vertical_speed = 0.;
          } else {
            vertical_speed -= .3 * vertical_speed;
            vertical_speed = std::abs(vertical_speed);
          }
        } else {
          // update height
          unit.height() += 10. * vertical_speed * timediff;
          vertical_speed -= 10. * G * timediff;
        }
      } else {
        unit.moving_speed -= GROUND_FRICTION * timediff;
        reset_height();
      }
    }
    unit.idle(timer);
    transform.SetPosition(unit.pos.x, unit.pos.y, unit.pos.z);
    float angle = unit.facing_dest;
    glm::vec2 dir(std::cos(angle), std::sin(angle));
    glm::vec2 nrm = glm::normalize(dir);
    transform.Rotate(nrm.x, nrm.y, 0, 5*360.f*unit.moving_speed*timediff);
  }

  void face(float angle) {
    unit.facing_dest = angle;
  }

  void face(Unit::loc_t point) {
    unit.facing_dest = atan2(point.y - unit.pos.y, point.x - unit.pos.x);
  }

  void timestamp_set_owner(int new_owner) {
    if(current_owner == new_owner)return;
    current_owner = new_owner;
    if(current_owner != -1) {
      timer.set_event(TIME_LOOSE_BALL_BEGINS);
      last_touched = current_owner;
    }
  }

  int owner() const {
    return current_owner;
  }

  bool is_loose() const {
    return owner() != NO_OWNER && !timer.timed_out(TIME_LOOSE_BALL_BEGINS);
  }

  void disable_interaction(Timer::time_t lock_for=CANT_INTERACT_SHOT) {
    timer.set_event(TIME_ABLE_TO_INTERACT);
    timer.set_timeout(TIME_ABLE_TO_INTERACT, lock_for);
  }

  bool can_interact() const {
    return timer.timed_out(TIME_ABLE_TO_INTERACT);
  }
};
