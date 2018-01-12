#pragma once

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/intersect.hpp>

#include "Transformation.hpp"
#include "Camera.hpp"
#include "PlayerObject.hpp"
#include "BallObject.hpp"
#include "Pitch.hpp"
#include "Post.hpp"
#include "Cursor.hpp"

#include "Soccer.hpp"

struct SoccerObject {
  Soccer &soccer;

  std::vector<PlayerObject> playerObjs;
  Pitch pitch;
  Post post_red, post_blue;
  BallObject ballObj;

  SoccerObject(Soccer &soccer):
    soccer(soccer),
    playerObjs(),
    post_red(Soccer::Team::RED_TEAM),
    post_blue(Soccer::Team::BLUE_TEAM)
  {
    for(auto &p : soccer.players) {
      playerObjs.push_back(PlayerObject({p.unit.pos.x, p.unit.pos.y}));
    }
  }

  void init() {
    pitch.init();
    post_red.init();
    post_blue.init();
    ballObj.init();
    for(auto &p : playerObjs) {
      p.init();
    }
  }

  enum class CursorState {
    DEFAULT,
    X_AIM,
    C_AIM,
    F_AIM
  };
  CursorState cursorState = CursorState::DEFAULT;
  glm::vec2 cursorPoint;
  void keyboard(int key, int playerId=0) {
    if(key == GLFW_KEY_Z) {
      soccer.z_action(soccer.ball.owner());
    } else if(key == GLFW_KEY_X) {
      cursorState = CursorState::X_AIM;
    } else if(key == GLFW_KEY_C) {
      cursorState = CursorState::C_AIM;
    } else if(key == GLFW_KEY_V) {
      soccer.v_action(playerId);
    } else if(key == GLFW_KEY_F) {
      cursorState = CursorState::F_AIM;
    } else if(key == GLFW_KEY_S) {
      soccer.s_action(playerId);
    } else if(key == GLFW_KEY_ESCAPE) {
      if(cursorState != CursorState::DEFAULT) {
        cursorState = CursorState::DEFAULT;
      }
    }
  }

  void set_cursor(Cursor &cursor, float m_x, float m_y, float width, float height, Camera &cam) {
    float s_x = m_x * 2 - 1, s_y = m_y * 2 - 1;
    glm::vec4 screenPos = glm::vec4(s_x, -s_y, 1.f, 1.f);
    glm::vec4 worldPos = glm::inverse(cam.get_matrix()) * screenPos;
    glm::vec3 dir = glm::normalize(glm::vec3(worldPos));
    float dist;
    bool intersects = glm::intersectRayPlane(
      cam.cameraPos,
      dir,
      glm::vec3(0, 0, 0),
      glm::vec3(0, 0, 1),
      dist
    );
    glm::vec3 pos = cam.cameraPos + dir * dist;
    cursorPoint = glm::vec2(pos.x, pos.y);
    /* printf("cursor (%f %f) -> (%f %f %f)\n", s_x, s_y, pos.x,pos.y, pos.z); */
    if(cursorState == CursorState::DEFAULT) {
      cursor.state = Cursor::State::POINTER;
    } else {
      cursor.state = Cursor::State::SELECTOR;
    }
  }

  void mouse_click(int button, int action, int playerId=0) {
    auto &p = soccer.get_player(playerId);
    glm::vec3 cpos(cursorPoint.x, cursorPoint.y, 0);
    if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
      if(cursorState == CursorState::DEFAULT) {
        soccer.m_action(playerId, cpos);
      } else {
        cursorState = CursorState::DEFAULT;
      }
    } else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
      switch(cursorState) {
        case CursorState::DEFAULT:
        break;
        case CursorState::X_AIM:
          soccer.x_action(playerId, p.unit.facing_angle(cpos));
          cursorState = CursorState::DEFAULT;
        break;
        case CursorState::C_AIM:
          soccer.c_action(playerId, cpos);
          cursorState = CursorState::DEFAULT;
        break;
        case CursorState::F_AIM:
          soccer.f_action(playerId, p.unit.facing_angle(cpos));
          cursorState = CursorState::DEFAULT;
        break;
      }
    }
  }

  void display(Camera &cam) {
    pitch.display(cam);
    post_red.display(cam);
    post_blue.display(cam);
    ballObj.display(soccer.ball, cam);

    std::vector<int> indices(soccer.players.size());
    for(int i = 0; i < soccer.players.size(); ++i) {
      indices[i] = i;
    }
    // sort by Y coordinate as we want to see the closest.
    for(int i = 0; i < soccer.players.size() - 1; ++i) {
      for(int j = i + 1; j < soccer.players.size(); ++j) {
        if(soccer.get_player(indices[i]).unit.pos.y > soccer.get_player(indices[j]).unit.pos.y) {
          std::swap(indices[i], indices[j]);
        }
      }
    }
    for(auto &ind: indices) {
      playerObjs[ind].display(soccer.get_player(ind), cam);
    }
  }

  void clear() {
    for(auto &p: playerObjs) {
      p.clear();
    }
    ballObj.clear();
    post_red.clear();
    post_blue.clear();
    pitch.clear();
  }
};