#ifndef NET_TRANSPORT_H_
#define NET_TRANSPORT_H_
#include "base/macros.h"
#include <memory>
#include <string>

namespace net {

class Transport {
 public:
  virtual ~Transport() {}

  virtual bool IsOpen() = 0;
  virtual bool Peek() = 0;
  virtual void Open() = 0;
  virtual void Close() = 0;

  virtual uint32_t Read(uint8_t* buf, uint32_t len) = 0;
  virtual uint32_t ReadAll(uint8_t* buf, uint32_t len) = 0;
  virtual uint32_t ReadEnd() = 0;

  virtual void Write(const uint8_t* buf, uint32_t len) = 0;
  virtual uint32_t WriteEnd() = 0;
  virtual void Flush() = 0;
  virtual const uint8_t* Borrow(uint8_t* buf, uint32_t*) = 0;

  virtual void Consume(uint32_t len) = 0;
  virtual const std::string GetOrigin() = 0;

 protected:
  Transport() {}  
};

class TransportFactory {
 public:
  TransportFactory() {}
  virtual ~TransportFactory() {}

  virtual std::shared_ptr<Transport> NewTransport(std::shared_ptr<Transport> 
		  trans) {
    return trans;
  }
};

} // namespace net
#endif // NET_TRANSPORT_H_
