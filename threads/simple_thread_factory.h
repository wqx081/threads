#ifndef THREADS_SIMPLE_THREAD_FACTORY_H_
#define THREADS_SIMPLE_THREAD_FACTORY_H_
#include "threads/thread.h"
#include <memory>

namespace threads {

class SimpleThreadFactory : public ThreadFactory {
 public:
  enum PolicyEnum { OTHER, FIFO, ROUND_ROBIN };
  enum PriorityEnum { LOWEST = 0, LOWER, LOW, NORMAL, HIGH, HIGHER, HIGHEST, INCREMENT, DECREMENT };

  SimpleThreadFactory(PolicyEnum policy = ROUND_ROBIN,
                      PriorityEnum priority = NORMAL,
                      int stack_size = 1,
                      bool detached = true);

  std::shared_ptr<Thread> NewThread(std::shared_ptr<Runnable> runnable) const override;
  Thread::id_t GetCurrentThreadId() const override;
  
  virtual int GetStackSize() const;
  virtual void SetStackSize(int value);
  virtual PriorityEnum GetPriority() const;
  virtual void SetPriority(PriorityEnum priority);
  virtual void SetDetached(bool detached);
  virtual bool IsDetached() const;

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

} // namespace threads
#endif // THREADS_SIMPLE_THREAD_FACTORY_H_
