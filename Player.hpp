#pragma once

#include "Timer.hpp"
#include "Ball.hpp"
#include "Unit.hpp"

struct Player {
  Unit::loc_t initial_position;
  Player(int id, bool team, std::pair<float, float> pos={0, 0}):
    team(team), playerId(id),
    initial_position(pos.first, pos.second, 0),
    unit(Unit::loc_t(initial_position), 4*M_PI)
  {}

  Unit unit;
  Timer timer;
  bool team;
  int playerId;
  static constexpr int
    TIME_DISPOSSESSED = 0,
    TIME_GOT_BALL = 1,
    TIME_OF_LAST_JUMP = 2,
    TIME_OF_LAST_SLIDE = 3,
    TIME_LAST_SLOWN_DOWN = 4,
    TIME_OF_LAST_PASS = 5;
  static constexpr Timer::time_t CANT_HOLD_BALL_DISPOSSESS = 1.45;
  static constexpr Timer::time_t CANT_HOLD_BALL_SHOT = 0.9;
  float tallness = Unit::GAUGE * 100;
  const float running_speed = Unit::GAUGE * 290;
  float G = Unit::GAUGE * 2.3;
  bool is_in_air = false;
  float default_height = Unit::GAUGE * .01;
  float vertical_speed = .0;
  const Timer::time_t jump_cooldown = 3.;
  bool has_ball = false;
  const float possession_range = Unit::GAUGE * 100;
  const float possession_offset = Unit::GAUGE * 60;
  const float possession_running_speed = Unit::GAUGE * 200;
  const Timer::time_t pass_cooldown = 2.;
  const Timer::time_t slide_duration = .7;
  const Timer::time_t slide_slowdown_duration = 1.8;
  static constexpr Timer::time_t SLOWDOWN_SLID = .95;
  static constexpr Timer::time_t SLOWDOWN_SHOT = 1.;
  const float slide_speed = Unit::GAUGE * 400;
  const float slide_slowdown_speed = .5 * running_speed;
  const float slide_cooldown = slide_duration + slide_slowdown_duration;

  void set_timer() {
    timer.set_event(TIME_GOT_BALL);
    timer.set_event(TIME_DISPOSSESSED);
    timer.set_timeout(TIME_OF_LAST_JUMP, jump_cooldown);
    timer.set_timeout(TIME_OF_LAST_SLIDE, slide_cooldown);
    timer.set_timeout(TIME_LAST_SLOWN_DOWN, SLOWDOWN_SHOT);
    timer.set_timeout(TIME_OF_LAST_PASS, pass_cooldown);
    /* timer.dump_times(); */
  }

  int id() const { return playerId; }
  Unit::vec_t velocity() const { return unit.velocity(); }

  void idle(double curtime) {
    /* printf("\nplayer idle:\n"); */
    timer.set_time(curtime);
    idle_speed();
    idle_jump();
    unit.idle(timer);
    /* printf("rotate to %f\n", unit.facing); */
  }

  void idle_speed() {
    if(is_sliding_fast()) {
      unit.moving_speed = slide_speed;
    } else if(is_sliding_slowndown() || is_slown_down()) {
      unit.moving_speed = slide_slowdown_speed;
    } else if(has_ball) {
      unit.moving_speed = possession_running_speed;
    } else {
      unit.moving_speed = running_speed;
    }
  }

  void idle_jump() {
    double timediff = timer.elapsed();
    if(is_jumping() && id()==0) {
      /* printf("mode: %s [%d %d]\n", is_in_air?"jumping":"running", is_going_up(), is_landing()); */
      /* printf("height: %f\n", unit.height()); */
      /* printf("vertical speed: %f\n", vertical_speed); */
    }
    if(is_jumping()) {
      if(vertical_speed > .0 || unit.height() > default_height) {
        float h = unit.height();
        unit.height() += 10. * vertical_speed * timediff;
        /* if(!id())printf("height: %f -> %f\n", h, unit.height()); */
        vertical_speed -= 10. * G * timediff;
      } else {
        /* if(!id())printf("landing\n"); */
        unit.height() = default_height;
        vertical_speed = .0;
        is_in_air = false;
      }
    } else {
      unit.height() = default_height;
    }
  }

  bool can_jump() const {
    return timer.timed_out(TIME_OF_LAST_JUMP);
  }

  bool is_jumping() const {
    return is_in_air;
  }

  bool is_going_up() const {
    return is_jumping() && vertical_speed > 0.;
  }

  bool is_landing() const {
    return is_jumping() && vertical_speed < 0.;
  }

  void jump(float vspeed) {
    if(!can_jump())return;
    timer.set_event(TIME_OF_LAST_JUMP);
    if(is_sliding() || is_slown_down())return;
    ASSERT(!is_in_air);
    is_in_air = true;
    vertical_speed = vspeed;
  }

  bool is_owner(Ball &ball) {
    bool res = ball.current_owner == id();
    ASSERT(res && has_ball || (!res && !has_ball));
    return res;
  }

  bool can_possess() const {
    return timer.timed_out(TIME_DISPOSSESSED);
  }

  void timestamp_got_ball(Ball &ball) {
    has_ball = true;
    timer.set_event(TIME_GOT_BALL);
    ball.timestamp_set_owner(playerId);
  }

  void timestamp_dispossess(Ball &ball, float lock_for) {
    ASSERT(is_owner(ball));
    has_ball = false;
    timer.set_event(TIME_DISPOSSESSED);
    timer.set_timeout(TIME_DISPOSSESSED, lock_for);
    ball.timestamp_set_owner(Ball::NO_OWNER);
  }

  Unit::loc_t possession_point() const {
    if(is_going_up()) {
      return unit.pos;
    }
    return unit.point_offset(possession_offset);
  }

  float get_control_potential(const Ball &ball) const {
    if(
      !ball.can_interact()
      || !can_possess()
      || ball.unit.height() <= unit.height()
      || unit.height() + tallness <= ball.unit.height()
      || is_sliding_slowndown()
      || is_slown_down()
    )
    {
      if(id() == 0) {
        /* printf("control: elapsed %f, return NAN\n", timer.elapsed(TIME_DISPOSSESSED)); */
        /* printf("control: conditions %d %d %d [%f %f] %d %d\n", */
        /*   !ball.can_interact(), */
        /*   !can_possess(), */
        /*   !( */
        /*     ball.unit.height() >= unit.height() */
        /*     && unit.height() + tallness >= ball.unit.height() */
        /*   ), ball.unit.height(), unit.height(), */
        /*   is_sliding_slowndown(), */
        /*   is_slown_down() */
        /* ); */
      }
      return NAN;
    }
    auto pp = possession_point();
    glm::vec2 bpos(ball.unit.pos.x, ball.unit.pos.y);
    float range = glm::length(bpos - glm::vec2(pp.x, pp.y));
    /* printf("potential %f vs %f\n", range, possession_range); */
    /* if(id()==0)printf("control: ranges %f %f\n", range, possession_range); */
    if(range > possession_range)return NAN;
    /* if(id()==0)printf("control: tackle potential %f\n", range); */
    return range;
  }

  void kick_the_ball(Ball &ball, float speed, float vspeed, float angle) {
    ball.face(angle);
    ball.unit.moving_speed = speed;
    ball.vertical_speed = vspeed;
    timestamp_dispossess(ball, CANT_HOLD_BALL_SHOT);
    ball.disable_interaction(Ball::CANT_INTERACT_SHOT);
  }

  bool can_pass() const {
    return timer.timed_out(TIME_OF_LAST_PASS);
  }

  void timestamp_passed() {
    if(!can_pass())return;
    timer.set_event(TIME_OF_LAST_PASS);
  }

  bool can_slide() {
    return !is_jumping() && timer.timed_out(TIME_OF_LAST_SLIDE);
  }

  bool is_sliding() const {
    return !timer.timed_out(TIME_OF_LAST_SLIDE);
  }

  bool is_sliding_fast() const {
    return timer.elapsed(TIME_OF_LAST_SLIDE) < slide_duration;
  }

  bool is_sliding_slowndown() const {
    return is_sliding() && !is_sliding_fast();
  }

  void timestamp_slide() {
    ASSERT(!has_ball);
    if(!can_slide())return;
    timer.set_event(TIME_OF_LAST_SLIDE);
  }

  void slowdown(float time) {
  }

  bool is_slown_down() const {
    return !timer.timed_out(TIME_LAST_SLOWN_DOWN);
  }

  void timestamp_slowdown(Timer::time_t dur=SLOWDOWN_SLID) {
    timer.set_event(TIME_LAST_SLOWN_DOWN);
    timer.set_timeout(TIME_LAST_SLOWN_DOWN, dur);
  }
};
