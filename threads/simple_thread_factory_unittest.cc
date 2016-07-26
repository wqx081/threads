#include "threads/simple_thread_factory.h"
#include "base/time_util.h"
#include "threads/monitor.h"
#include "threads/exception.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace threads;
using namespace base;

class HelloWorldTask : public Runnable {
 public:
  HelloWorldTask() {}

  void Run() override {
    LOG(INFO) << "\t\t\tHello, World";
  }
};

bool HelloWorldTaskTest() {
  SimpleThreadFactory thread_factory;
  std::shared_ptr<HelloWorldTask> task = std::make_shared<HelloWorldTask>();
  std::shared_ptr<Thread> thread = thread_factory.NewThread(task);
  thread->Start();
  thread->Join();
  return true;
}

TEST(SimpleThreadFactory, HelloWorldTaskTest) {

  EXPECT_TRUE(HelloWorldTaskTest());

}

class ReapNTask : public Runnable {
 public:
  ReapNTask(Monitor& monitor, int& active_count)
    : monitor_(monitor), count_(active_count) {}

  void Run() override {
    Synchronized s(monitor_);
    count_--;
    if (count_ == 0) {
      monitor_.Notify();
    }
  }

  Monitor& monitor_;
  int& count_;
};

bool ReapNTaskTest(int loop = 1, int count = 10) {
  SimpleThreadFactory thread_factory;
  std::shared_ptr<Monitor> monitor = std::make_shared<Monitor>();
  
  for (int i = 0; i < loop; ++i) {
    int* active_count = new int(count);
    std::set<std::shared_ptr<Thread>> threads;
    int t;
    for (t = 0; t < count; ++t) {
      try {
        threads.insert(thread_factory.NewThread(std::shared_ptr<Runnable>(new ReapNTask(*monitor, *active_count))));
      } catch (SystemResourceException& e) {
        LOG(ERROR) << "\t\t\tfailed to create" << i * count + t << " thread " << e.what();
        throw e;
      }
    }

    t = 0;
    for (std::set<std::shared_ptr<Thread>>::const_iterator thread = threads.begin();
         thread != threads.end();
         ++t, ++thread) {
      try {
        (*thread)->Start();
      } catch (SystemResourceException& e) {
        LOG(ERROR) << "\t\t\tFailed to start " << i * count + t << " thread " << e.what();
        throw e;
      }
    }

    {
      Synchronized s(*monitor);
      while (*active_count > 0) {
        try {
          monitor->Wait(1000);
        } catch (TimedOutException& e) {
        }
      }
    }
    delete active_count;
    LOG(INFO) << "\t\t\treaped " << i * count << " threads";
  }
  LOG(INFO) << "\t\t\tSuccess!";
  return true;
}

TEST(SimpleThreadFactory, ReapNTaskTest) {
  ReapNTaskTest(ReapNTaskTest(10));
}

class SyncStartTask : public Runnable {
 public:
  enum STATE {UNINITIALIZED, STARTING, STARTED, STOPPING, STOPPED };
  SyncStartTask(Monitor& monitor, volatile STATE& state): monitor_(monitor), state_(state) {}

  void Run() override {
    {
      Synchronized s(monitor_);
      if (state_ == SyncStartTask::STARTING) {
        state_ = SyncStartTask::STARTED;
        monitor_.Notify();
      }
    }

    {
      Synchronized s(monitor_);
      while (state_ == SyncStartTask::STARTED) {
        monitor_.Wait();
      }

      if (state_ == SyncStartTask::STOPPING) {
        state_ = SyncStartTask::STOPPED;
        monitor_.NotifyAll();
      }
    }
  }

 private:
  Monitor& monitor_;
  volatile STATE& state_;
};

bool SyncStartTaskTest() {
  Monitor monitor;
  SyncStartTask::STATE state = SyncStartTask::UNINITIALIZED;
  std::shared_ptr<SyncStartTask> task = std::make_shared<SyncStartTask>(monitor, state);
  SimpleThreadFactory thread_factory;
  std::shared_ptr<Thread> thread = thread_factory.NewThread(task);

  if (state == SyncStartTask::UNINITIALIZED) {
    state = SyncStartTask::STARTING;
    thread->Start();
  }
  
  {
    Synchronized s(monitor);
    while (state == SyncStartTask::STARTING) {
      monitor.Wait();
    }
  }

  assert(state != SyncStartTask::STARTING);

  {
    Synchronized s(monitor);

    try {
      monitor.Wait(100);
    } catch (TimedOutException&) {
    }

    if (state == SyncStartTask::STARTED) {
      state = SyncStartTask::STOPPING;
      monitor.Notify();
    }

    while (state == SyncStartTask::STOPPING) {
      monitor.Wait();
    }
  }
  assert(state == SyncStartTask::STOPPED);
  LOG(INFO) << "\t\t\tSuccess";
  return true;
}

TEST(SimpleThreadFactory, SyncStartTaskTest) {
  EXPECT_TRUE(SyncStartTaskTest());
}

bool MonitorTimeoutTest(size_t count=1000, int64_t timeout=10) {
  Monitor monitor;
  int64_t start_time = TimeUtil::CurrentTime();
  for (size_t ix = 0; ix < count; ++ix) {
    Synchronized s(monitor);
    try {
      monitor.Wait(timeout);
    } catch (TimedOutException& e){
    }
  }
  int64_t end_time = TimeUtil::CurrentTime();
  double error = ((end_time - start_time) - (count * timeout)) / (double)(count *timeout);
  if (error < 0.0) {
    error *= 1.0;
  }
  bool success = error < .20;
  LOG(INFO) << "\t\t\t" << (success ? "Success" : "Failure") << "! expected time: " << count * timeout
  << "ms elapsed time: " << end_time - start_time << "ms error%: " << error * 100.0;
  return success;
}

TEST(SimpleThreadFactory, MonitorTimeoutTest) {
  EXPECT_TRUE(MonitorTimeoutTest());
}
