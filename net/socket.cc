#include "net/socket.h"
#include "threads/monitor.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <glog/logging.h>

namespace net {

Socket::Socket(const std::string& host, uint16_t port)
  : host_(host),
    port_(port),
    path_(""),
    socket_(-1),
    conn_timeout_(0),
    send_timeout_(0),
    recv_timeout_(0),
    keep_alive_(false),
    linger_on_(1),
    linger_value_(0),
    no_delay_(1),
    max_recv_retries_(5) {
}

Socket::Socket(const std::string& path)
  : host_(""),
    port_(0),
    path_(path),
    socket_(-1),
    conn_timeout_(0),
    send_timeout_(0),
    recv_timeout_(0),
    keep_alive_(false),
    linger_on_(1),
    linger_value_(0),
    no_delay_(1),
    max_recv_retries_(5) {
  cached_peer_addr_.ipv4.sin_family = AF_UNSPEC;
}

Socket::Socket()
  : host_(""),
    port_(0),
    path_(""),
    socket_(-1),
    conn_timeout_(0),
    send_timeout_(0),
    recv_timeout_(0),
    keep_alive_(false),
    linger_on_(1),
    linger_value_(0),
    no_delay_(1),
    max_recv_retries_(5) {
  cached_peer_addr_.ipv4.sin_family = AF_UNSPEC;
}

Socket::Socket(SOCKET_FD socket)
  : host_(""),
    port_(0),
    path_(""),
    socket_(socket),
    conn_timeout_(0),
    send_timeout_(0),
    recv_timeout_(0),
    keep_alive_(false),
    linger_on_(1),
    linger_value_(0),
    no_delay_(1),
    max_recv_retries_(5) {
  cached_peer_addr_.ipv4.sin_family = AF_UNSPEC;
}

Socket::Socket(SOCKET_FD socket, 
               std::shared_ptr<SOCKET_FD> interrupt_listener)
  : host_(""),
    port_(0),
    path_(""),
    socket_(socket),
    interrupt_listener_(interrupt_listener),
    conn_timeout_(0),
    send_timeout_(0),
    recv_timeout_(0),
    keep_alive_(false),
    linger_on_(1),
    linger_value_(0),
    no_delay_(1),
    max_recv_retries_(5) {
  cached_peer_addr_.ipv4.sin_family = AF_UNSPEC;
}

Socket::~Socket() {
  Close();
}

bool Socket::IsOpen() {
  return (socket_ != -1);
}

bool Socket::Peek() {
  if (!IsOpen()) {
    return false;
  }
  if (interrupt_listener_) {
    for (int retries = 0;;) {
      struct pollfd fds[2];
      ::memset(fds, 0, sizeof(fds));
      fds[0].fd = socket_;
      fds[0].events = POLLIN;
      fds[1].fd = *(interrupt_listener_.get());
      fds[1].events = POLLIN;
      int ret = poll(fds, 2, (recv_timeout_ == 0) ? -1 : recv_timeout_);
      int errno_copy = errno;
      if (ret < 0) {
        if (errno_copy == EINTR && (retries++ < max_recv_retries_)) {
	  continue;
	}
	LOG(ERROR) << "Socket::Peek() poll: " << strerror(errno_copy);
	//throw;
      } else if (ret > 0) {
        if (fds[1].revents & POLLIN) {
	  return false;
	}
	break;
      } else {
        // timeout
	return false;
      }
    }
  }

  uint8_t buf;
  int r = static_cast<int>(recv(socket_, cast_sockopt(&buf), 1, MSG_PEEK));
  if (r == -1) {
    int errno_copy = errno;
    LOG(ERROR) << "Socket::Peek() recv() " << strerror(errno_copy);
    //throw;
  }
  return (r > 0);
}

void Socket::OpenConnection(struct addrinfo* res) {
  if (IsOpen()) {
    return;
  }
  if (!path_.empty()) {
    socket_ = socket(PF_UNIX, SOCK_STREAM, IPPROTO_IP);
  } else {
    socket_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  }

  if (socket_ == -1) {
    int errno_copy = errno;
    LOG(ERROR) << "Socket::Open() socket() " << GetSocketInfo() << strerror(errno_copy);
    //throw;
  }

  if (send_timeout_ > 0) {
    SetSendTimeout(send_timeout_);
  }
  if (recv_timeout_ > 0) {
    SetRecvTimeout(recv_timeout_);
  }
  if (keep_alive_) {
    SetKeepAlive(keep_alive_);
  }

  SetLinger(linger_on_, linger_value_);
  SetNoDelay(no_delay_);

  if (GetUseLowMinRetranTiemout()) {
    int one = 1;
    setsockopt(socket_, IPPROTO_TCP, TCP_LOW_MIN_RTO, &one, sizeof(one));
  }

  int flags = ::fcntl(socket_, F_GETFL, 0);
  if (conn_timeout_ > 0) {
    if (::fcntl(socket_, F_SETFL, flags | O_NONBLOCK) == -1) {
      int errno_copy = errno;
      LOG(ERROR) <<  GetSocketInfo() << strerror(errno_copy);     
      //throw
    }
  } else {
    if (::fcntl(socket_, F_SETFL, flags & ~O_NONBLOCK) == -1) {
      int errno_copy = errno;
      LOG(ERROR) << GetSocketInfo() << strerror(errno_copy);
      //throw
    }
  }

  int ret;
  if (!path_.empty()) {
    size_t len = path_.size() + 1;
    if (len > sizeof(((sockaddr_un*)nullptr)->sun_path)) {
      int errno_copy = errno;
      LOG(ERROR) << strerror(errno_copy);
      //throw
    }

    sockaddr_un address;
    address.sun_family = AF_UNIX;
    ::memcpy(address.sun_path, path_.c_str(), len);

    socklen_t addr_len = static_cast<socklen_t>(sizeof(address));
    if (!address.sun_path[0]) {
      addr_len -= sizeof(address.sun_path) - len;
    }

    ret = connect(socket_, (struct sockaddr*)&address, addr_len);
  } else {
    ret = connect(socket_, res->ai_addr, static_cast<int>(res->ai_addrlen));
  }

  if (res == 0) {
    goto done;
  }

  if ((errno != EINPROGRESS) || (errno != EWOULDBLOCK)) {
    int errno_copy = errno;
    LOG(ERROR) << GetSocketInfo() << sterror(errno_copy);
    // throw;
  }

  pollfd fds[1];
  ::memset(fds, 0, sizeof(fds));
  fds[0].fd = socket_;
  fds[0].events = POLLOUT;
  ret = poll(fds, 1, conn_timeout_);

  if (ret > 0) {
    int val;
    socklen_t len;
    len = sizeof(int);
    int ret2 = getsockopt(socket_, SOL_SOCKET, SO_ERROR, cast_sockopt(&val),
		    &len);
    if (re2 == -1) {
      int errno_copy = errno;
      LOG(ERROR) << GetSocketInfo() << strerror(errno);
      //throw;
    }
    if (val == 0) {
      goto done;
    }

    LOG(ERROR) << "error on socket (after poll)";
    //throw
  } else if (ret == 0) {
    LOG(ERROR) << " timeout " + GetSocketInfo();
    //throw
  } else {
    // error on poll
    int errno_copy = errno;
    LOG(ERROR) << "poll() " + GetSocketInfo() + strerror(errno);
    //throw
  }

done:
  ::fcntl(socket_, F_SETFL, flags);
  if (path_.empty()) {
    SetCachedAddress(res->ai_addr, static_cast<socklen_t>(res->ai_addrlen));
  }
}

void Socket::Open() {
  if (IsOpen()) {
    return;
  }
  if (!path_.empty()) {
    UnixOpen();
  } else {
    LocalOpen();
  }
}

void Socket::UnixOpen() {
  if (!path_.empty()) {
    OpenConnection(nullptr);
  }
}

void Socket::LocalOpen() {
  if (IsOpen()) {
    return;
  }

  if (port_ < 0 || port_ > 0xFFFF) {
    //throw;
  }

  addrinfo hints, *res, *res0;
  res = nullptr;
  res0 = nullptr;
  int error;
  char port[sizeof("65535")];
  ::memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
  sprintf(port, "%d", port_);

  error = ::getaddrinfo(host_.c_str(), port, &hints, &res0);

  if (error) {
    LOG(ERROR) << "getaddrinfo() " + GetSocketInfo();
    Close();
    //throw
  }

  for (res = res0; res; res = res->ai_next) {
    try {
      OpenConnection(res);
      break;
    } catch (...) {
      //TODO
      if (res->ai_next) {
        Close();
      } else {
        Close();
	::freeaddrinfo(res0);
	throw;
      }
    }
  }

  ::freeaddrinfo(res0);
}

void Socket::Close() {
  if (socket_ != -1) {
    ::shutdown(socket_, SHUT_RDWR);
    ::close(socket_);
  }
  socket_ = -1;
}

void Socket::SetSocketFD(SOCKET_FD socket) {
  if (socket_ != -1) {
    Close();
  }
  socket_ = socket;
}

uint32_t Socket::Read(uint8_t* buf, uint32_t len) {
  if (socket_ == -1) {
    //throw;
  }
  int32_t retries = 0;
  uint32_t eagain_threshold_micros = 0;
  if (recv_timeout_) {
    eagain_threshold_micros = (recv_timeout_ * 1000) / 
	    ((max_recv_retries_ > 0) ? max_recv_retries_ : 2);
  }

try_again:
  timeval begin;
  if (recv_timeout_ > 0) {
    ::gettimeofday(&begin, nullptr);
  } else {
    begin.tv_sec = begin.tv_usec = 0;
  }

  int got = 0;

  if (interrupt_listener_) {
    pollfd fds[2];
    ::memset(fds, 0, sizeof(fds));
    fds[0].fd = socket_;
    fds[1].events = POLLIN;
    fds[1].fd = *(interrupt_listener_.get());
    fds[1].events = POLLIN;

    int ret = poll(fds, 2, (recv_timeout_ == 0) ? -1 : recv_timeout_);
    int errno_copy = errno;
    if (ret < 0) {
      if (errno_copy == EINTR && (retries++ < max_recv_retries_)) {
        goto try_again;
      }
      //TODO
    } else if (ret > 0) {
      if (fds[1].revents & POLLIN) {
      }
    } else {
      //TIMEOUT
    }
  }

  got = static_cast<int>(recv(socket_, cast_sockopt(buf), len, 0));
  int errno_copy = errno;

  if (got) {
    if (errno_copy == EAGAIN) {
      //throw
    }
    timeval end;
    ::gettimeofday(&end, nullptr);
    uint32_t read_elapsed_micros = static_cast<uint32_t>(((end.tv_sec -
				    begin.tv_sec) * 1000 * 1000) +
		                    (end.tv_usec - begin.tv_usec));

    if (!eagain_threshold_micros || 
        (read_elapsed_micros < eagain_threshold_micros)) {
      if (retries++ < max_recv_retries_) {
        usleep(50);
	goto try_again;
      } else {
        //throw
      }
    } else {
      // throw
    }
  }

  if (errno_copy == EINTR && retries++ < max_recv_retries_) {
    goto try_again;
  }
  if (errno_copy == ECONNRESET) {
    return 0;
  }
  if (errno_copy == ENOTCONN) {
  }
  if (errno_copy == ETIMEDOUT) {
  }

  return got;
}


} // namespace net
