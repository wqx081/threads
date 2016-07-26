#ifndef NET_SOCKET_H_
#define NET_SOCKET_H_

#include "base/macros.h"
#include "net/transport.h"

#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>

#include <string>

namespace net {

class Socket : public Transport {
 public:
  typedef int SOCKET_FD;

  Socket();
  Socket(const std::string& host, uint16_t port);
  Socket(const std::string& path);
  virtual ~Socket();

  virtual bool IsOpen() override;
  virtual bool Peek() override;
  virtual void Open() override;
  virtual void Close() override;
  virtual uint32_t Read(uint8_t* buf, uint32_t len) override;
  virtual uint32_t ReadAll(uint8_t* buf, uint32_t len) override; 
  virtual uint32_t ReadEnd() override;

  virtual void Write(const uint8_t* buf, uint32_t len) override;
  virtual uint32_t WriteEnd() override;
  virtual void Flush() override;

  virtual const uint8_t* Borrow(uint8_t* buf, uint32_t* ) override;
  virtual void Consume(uint32_t len) override;
  virtual const std::string GetOrigin() override;

  uint32_t WritePartial(const uint8_t* buf, uint32_t len); 

  std::string GetHost();
  int GetPort();
  void SetHost(std::string host);
  void SetPort(uint16_t port);

  void SetLinger(bool on, int linger);
  void SetNoDelay(bool no_delay);
  void SetConnTimeout(int64_t ms);
  void SetSendTimeout(int64_t ms);
  void SetRecvTimeout(int64_t ms);
  void SetMaxRecvRetries(int64_t max_recv_retries);
  void SetKeepAlive(bool keep_alive);

  std::string GetSocketInfo();
  std::string GetPeerAddress();
  uint16_t GetPeerPort();
  SOCKET_FD GetSocketFD();
  void SetSocketFD(SOCKET_FD fd);

  static void SetUseLowMinRetranTimeout();
  static bool GetUseLowMinRetranTiemout();

  Socket(SOCKET_FD socket);
  Socket(SOCKET_FD socket, std::shared_ptr<SOCKET_FD> interrupt_listener);
  void SetCachedAddress(const sockaddr* addr, socklen_t len);

 protected:
  std::string host_;
  std::string peer_host_;
  std::string peer_address_;
  uint16_t peer_port_;
  uint16_t port_;
  std::string path_;
  SOCKET_FD socket_;
  std::shared_ptr<SOCKET_FD> interrupt_listener_;

  int64_t conn_timeout_;
  int64_t send_timeout_;
  int64_t recv_timeout_;
  bool keep_alive_;
  bool linger_on_;
  int linger_value_;
  bool no_delay_;
  int64_t max_recv_retries_;

  union {
    sockaddr_in ipv4;
    sockaddr_in6 ipv6;
  } cached_peer_addr_;

  static bool use_low_min_retran_timeout_;

  void OpenConnection(struct addrinfo* res);  

 private:
  void UnixOpen();
  void LocalOpen();
};

} // namespace net
#endif // NET_SOCKET_H_
