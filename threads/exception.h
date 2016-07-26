#ifndef THREADS_EXCEPTION_H_
#define THREADS_EXCEPTION_H_

#include <exception>
#include <string>

namespace threads {

class Exception : public std::exception {
 public:
  Exception(): message_() {}
  Exception(const std::string& message) : message_(message) {}

  virtual ~Exception() throw() {}
  virtual const char* what() const throw() {
    if (message_.empty()) {
      return "Default Exception";
    } else {
      return message_.c_str();
    }
  }

 protected:
  std::string message_;  
};


////////////////
class NoSuchTaskException : public Exception {};
class UncancellableTaskException : public Exception {};
class InvalidArgumentException : public Exception {};

class IllegalStateException : public Exception {
 public:
  IllegalStateException() {}
  IllegalStateException(const std::string& message) : Exception(message) {}
};

class TimedOutException : public Exception {
 public:
  TimedOutException() : Exception("TimedOutException") {}
  TimedOutException(const std::string& message): Exception(message) {}
};

class SystemResourceException : public Exception {
 public:
  SystemResourceException() {}
  SystemResourceException(const std::string& message) : Exception(message) {}
};

class TooManyPendingTasksException : public Exception {
 public:
  TooManyPendingTasksException() : Exception("TooManyPendingTasksException") {}
  TooManyPendingTasksException(const std::string& message) : Exception(message) {}
};

} // namespace threads
#endif // THREADS_EXCEPTION_H_
