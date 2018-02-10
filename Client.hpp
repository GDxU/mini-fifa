#pragma once

#include "MetaServer.hpp"
#include "Lobby.hpp"
#include "Soccer.hpp"
#include "Intelligence.hpp"

struct Client {
  MetaServerClient mclient;
  LobbyActor *l_actor = nullptr;
  Soccer *soccer = nullptr;
  Intelligence<IntelligenceType::ABSTRACT> *intelligence = nullptr;

  template <typename... ArgTs>
  Client(ArgTs... args):
    mclient(std::forward<ArgTs>(args)...)
  {}

  bool is_active_mclient() {
    return !mclient.should_stop();
  }

  bool is_active_lobby() {
    return l_actor != nullptr;
  }

  bool is_active_game() {
    ASSERT(((soccer == nullptr) ? 1 : 0) == ((intelligence == nullptr) ? 1 : 0));
    return soccer != nullptr && intelligence != nullptr;
  }

  void start_mclient() {
    ASSERT(!is_active_mclient());
    mclient.start();
    Logger::Info("started mclient\n");
  }
  void stop_mclient() {
    ASSERT(is_active_mclient());
    mclient.stop();
  }

  void start_lobby() {
    ASSERT(!is_active_lobby());
    l_actor = mclient.make_lobby();
    l_actor->set_state(LobbyActor::State::DEFAULT);
    l_actor->start();
  }
  void stop_lobby() {
    ASSERT(is_active_lobby());
    l_actor->stop();
    delete l_actor;
    l_actor = nullptr;
  }

  void start_game() {
    ASSERT(!is_active_game());
    soccer = new Soccer(l_actor->get_soccer());
    intelligence = l_actor->make_intelligence(*soccer);
  }
  void stop_game() {
    ASSERT(is_active_game());
    intelligence->stop();
    delete intelligence;
    intelligence = nullptr;
    delete soccer;
    soccer = nullptr;
  }

  enum class State {
    DEFAULT, QUIT
  };
  State state = State::DEFAULT;
  bool has_quit() {
    return state == State::QUIT;
  }

// actions
  void action_quit() {
    Logger::Info("client: action quit\n");
    state = State::QUIT;
  }
  void action_host_game() {
    Logger::Info("client: action host game\n");
    stop_mclient();
    start_lobby();
  }
  void action_join_game() {
    Logger::Info("client: action join game\n");
    stop_mclient();
    start_lobby();
  }

  void action_start_game() {
    Logger::Info("client: action start game\n");
    start_game();
    stop_lobby();
    intelligence->start();
  }

  void action_quit_lobby() {
    Logger::Info("client: action quit lobby\n");
    stop_lobby();
    start_mclient();
  }

  void action_quit_game() {
    Logger::Info("client: action leave game\n");
    stop_game();
    start_mclient();
    mclient.set_state(MetaServerClient::State::DEFAULT);
  }

  // scope functions
  void start() {
    Logger::Info("client: start\n");
    state = State::DEFAULT;
    start_mclient();
  }
  void stop() {
    if(is_active_game()) {
      stop_game();
    }
    if(is_active_lobby()) {
      stop_lobby();
    }
    if(is_active_mclient()) {
      mclient.stop();
    }
    Logger::Info("client: stopped\n");
  }
};
