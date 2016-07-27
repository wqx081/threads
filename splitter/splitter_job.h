#ifndef SPLITTER_SPLITTER_JOB_H_
#define SPLITTER_SPLITTER_JOB_H_
#include "base/macros.h"
#include "threads/simple_thread_factory.h"

#include <memory>

namespace splitter {

class SplitterJob;

class SplitterJobObserver {
 public:
  virtual ~SplitterJobObserver() {}
  virtual void OnCompletedJob(std::shared_ptr<SplitterJob> job) = 0;
};

class SplitterJob : public threads::Runnable {
 public:
  virtual ~SplitterJob() {}

  virtual void RegisterObserver(std::weak_ptr<SplitterJobObserver> job_observer) = 0;
  virtual void UnRegisterObserver(std::weak_ptr<SplitterJobObserver> job_observer) = 0;
  virtual void NotifyAllObservers() = 0;

  // From threads::Runnable
  virtual void Run() = 0;
};

class SplitterJobFactory {
 public:
  virtual ~SplitterJobFactory() {}
  virtual std::shared_ptr<SplitterJob> CreateJob() = 0;
};

} // namespace splitter
#endif // SPLITTER_SPLITTER_JOB_H_
