// Exports marvin::main() to run the Thrift server.  Example usage:

/*
#include "../marvin/main.h"

#include "gen-cpp/API.h"

class APIImpl : virtual public API::APIIf {
 public:
  void ariadne_status(API::Status& status) {
    ...
  }
  void ariadne_keepalive(API::Void&, const API::Void&) {
    watchdog_reset();
  }
};

int main(int argc, char** argv) {
  if (!google::ParseCommandLineFlags(&argc, &argv, true)) {
    return -1; 
  }
  marvin::main<APIImpl, API::APIProcessor>();
}
*/

#ifndef MARVIN_MAIN_H
#define MARVIN_MAIN_H

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "../felicity/with_gflags/watchdog.h"

DEFINE_int32(port, 9090, "Port to use.");

namespace marvin {

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

template<typename TImpl, typename TProcessor> struct runMain {
  struct impl_with_healthz : TImpl {
    int32_t healthz() {
      felicity::watchdog_reset();
      return 1;
    }
  };
  struct ReadyPrinter : TServerEventHandler {
    virtual void preServe() {
      felicity::safe_cout << "READY\n";
    }
  };
  static void run() {
    felicity::watchdog_reset();
    boost::shared_ptr<impl_with_healthz> handler(new impl_with_healthz());
    boost::shared_ptr<TProcessor> processor(new TProcessor(handler));
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(FLAGS_port));
    boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    boost::shared_ptr<TServerEventHandler> readyPrinter(new ReadyPrinter());
    server.setServerEventHandler(readyPrinter);
    server.serve();
  }
};

template<typename TImpl, typename TProcessor> void main() {
  runMain<TImpl, TProcessor>::run();
}

}  // namespace marvin

#endif
