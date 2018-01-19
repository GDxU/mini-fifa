#include "Client.hpp"
#include "Window.hpp"

int main() {
  /* Soccer server_soccer; */
  /* Soccer client_soccer; */
  /* net::Addr server_addr(net::ip_from_ints(127, 0, 0, 1), net::port_t(2345)); */
  /* net::Addr client_addr(net::ip_from_ints(127, 0, 0, 1), net::port_t(2346)); */
  /* { */
  /*   Server iserver(0, server_soccer, server_addr.port, {client_addr}); */
  /*   Remote iclient(0, client_soccer, client_addr.port, server_addr); */
  /*   Window w(client_soccer, iclient); */
  /*   w.run(iserver); */
  /* } */

  Logger::Setup("minififa.log");
  Logger::MirrorLog(stderr);
  Client client(net::Addr(net::ip_from_ints(127, 0, 0, 1), net::port_t(5678)));
  client.start();
  Window w(client);
  w.run();
  client.stop();
  Logger::Close();
}
