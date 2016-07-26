#ifndef NET_SERVER_TRANSPORT_H_
#define NET_SERVER_TRANSPORT_H_
#include "base/macros.h"
#include "net/transport.h"
#include <memory>

namespace net {

class ServerTransport {
 public:
  virtual ~ServerTransport() {}

  virtual void Listen() = 0;
  virtual std::shared_ptr<Transport> Accept() = 0;
  virtual void Interrupt();
  virtual void InterruptChildren();
  virtual void Close() = 0;

 protected:
  ServerTransport() {}
//  virtual std::shared_ptr<Transport> AcceptImpl() = 0;
};

} // namespace net
#endif // NET_SERVER_TRANSPORT_H_
