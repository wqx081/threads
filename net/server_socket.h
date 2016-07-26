#ifndef NET_SERVER_SOCKET_H_
#define NET_SERVER_SOCKET_H_

#include "net/server_transport.h"
#include "net/socket.h"
#include <functional>
#include <memory>

namespace net {


class ServerSocket : public ServerTransport {
 public:
  typedef std::function<void(Socket::SOCKET_FD)> socket_func_t;
  const static int kDefaultBacklog = 1024;

  ServerSocket(uint16_t port);
  ServerSocket(uint16_t port, int64_t send_timeout, int64_t recv_timeout);
  ServerSocket(const std::string& address, uint16_t port);
  ServerSocket(const std::string& path);
  virtual ~ServerSocket();

  void SetSendTimeout(int64_t send_timeout);
  void SetRecvTimeout(int64_t recv_timeout);

  void SetAcceptTimeout(int64_t accept_timeout);
  void SetAcceptBacklog(int64_t accept_backlog);

  void SetRetryLimit(int64_t retry_limit);
  void SetRetryDelay(int64_t retry_delay);

  void SetKeepAlive(bool keep_alive);

  void SetTcpSendBuffer(int64_t tcp_send_buffer);
  void SetTcpRecvBuffer(int64_t tcp_recv_buffer);

  void SetListenCallback(const socket_func_t& listen_callback);
  void SetAcceptCallback(const socket_func_t& accept_callback);

  void SetInterruptableChidlren(bool enable);

  uint16_t GetPort();

  // virtual
  virtual void Listen() override;
  virtual void Interrupt() override;
  virtual void InterruptChildren() override;
  virtual void Close() override;
  virtual std::shared_ptr<Transport> Accept() override;

 protected:
  virtual std::shared_ptr<Socket> CreateSocket(Socket::SOCKET_FD client);
  bool interruptable_children_;
  std::shared_ptr<Socket::SOCKET_FD> child_interrupt_sock_reader_;

 private:
  void Notify(Socket::SOCKET_FD notify_socket);

  uint16_t port_;
  std::string address_;
  std::string path_;
  Socket::SOCKET_FD server_socket_;
  int64_t accept_backlog_;
  int64_t send_timeout_;
  int64_t recv_timeout_;
  int64_t accept_timeout_;
  int64_t retry_limit_;
  int64_t retry_delay_;
  int64_t tcp_send_buffer_;
  int64_t tcp_recv_buffer_;
  bool keep_alive_;
  bool listening_;

  Socket::SOCKET_FD interrupt_sock_writer_;
  Socket::SOCKET_FD inerrupt_sock_reader_;
  Socket::SOCKET_FD child_interrupt_sock_writer_;

  socket_func_t listen_callback_;
  socket_func_t accept_callback_;
};

} // namespace net
#endif // NET_SERVER_SOCKET_H_
