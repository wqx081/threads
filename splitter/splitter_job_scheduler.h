#ifndef SPLITTER_SPLITTER_JOB_SCHEDULER_H_
#define SPLITTER_SPLITTER_JOB_SCHEDULER_H_
#include "splitter/splitter_job.h"
#include "splitter/splitter_strategy.h"
#include "threads/simple_thread_factory.h"
#include "threads/thread_manager.h"
#include "threads/monitor.h"

#include <list>
#include <memory>

#include <glog/logging.h>

namespace splitter {

class ObservableSplitterJob 
  : public SplitterJob,
    public std::enable_shared_from_this<ObservableSplitterJob> {

 public:

#if 0
  struct JobStats {
    size_t total_tasks;
    size_t completed_tasks;
    size_t pending_tasks;
  };
#endif
 ObservableSplitterJob() {}

  virtual ~ObservableSplitterJob() {
    LOG(INFO) << "ObservableSplitterJob()";
  }

  void SetBookPath(const std::string& book_path) {
    book_path_ = book_path;
  }
 
  std::string GetBookPath() const {
    return book_path_;
  }

  virtual void RegisterObserver(std::weak_ptr<SplitterJobObserver> job_observer) override {
    for (auto& observer : observer_list_) {
      if (job_observer.lock() == observer.lock()) {
        return;
      }
    }
    observer_list_.push_back(job_observer);
  }

  virtual void UnRegisterObserver(std::weak_ptr<SplitterJobObserver> job_observer) override {
    for (auto it = observer_list_.begin();
         it != observer_list_.end();
         ++it) {
      if (it->lock() == job_observer.lock()) {
        observer_list_.erase(it);
      }
    }
  }

  virtual void NotifyAllObservers() override {
    for (auto& observer : observer_list_) {
      std::shared_ptr<SplitterJobObserver> ob = observer.lock();
      ob->OnCompletedJob(shared_from_this());
    }  
  }  

  virtual void Run() override {
    splitter_->Split(book_path_, output_paths_);
    NotifyAllObservers();
  }

#if 0
  JobStats GetJobStats() const {
    return job_stats_;
  }
#endif

  void SetSplitterStrategy(std::shared_ptr<SplitterStrategy> value) {
    splitter_ = value;
  }

  void SetMonitor(threads::Monitor* monitor) {
    monitor_ = monitor;
  }

 private:
  std::list<std::weak_ptr<SplitterJobObserver>> observer_list_;
//  JobStats job_stats_;
  std::shared_ptr<SplitterStrategy> splitter_;
  std::string book_path_;
  std::vector<std::pair<std::string, SplitterStrategy::State>> output_paths_;
  threads::Monitor* monitor_;

};

class EpubSectionSplitterFactory : public SplitterJobFactory {
 public:
  virtual std::shared_ptr<SplitterJob> CreateJob() override {
    std::shared_ptr<ObservableSplitterJob> job = std::make_shared<ObservableSplitterJob>();
    job->SetSplitterStrategy(std::make_shared<EpubSectionSplitter>()); 
    return job;
  }
};

/////////////////////////////////////////////

class SplitterJobScheduler 
  : public SplitterJobObserver,
    public std::enable_shared_from_this<SplitterJobScheduler> {
 public:
  explicit SplitterJobScheduler(size_t num_workers = 4,
                                size_t pending_job_max_count = 1024)
    : num_workers_(num_workers),
      pending_job_max_count_(pending_job_max_count) {
    Init();
  }

  ~SplitterJobScheduler() {
    thread_manager_->Stop();
  }

  void AddSplitterJob(std::shared_ptr<ObservableSplitterJob> job) {
    job_list_.push_back(job);
    job->RegisterObserver(shared_from_this());
    thread_manager_->Add(job);
    {
      threads::Synchronized s(monitor_);
      pending_job_count_++;
      monitor_.NotifyAll();
    }
  }

  virtual void OnCompletedJob(std::shared_ptr<SplitterJob> job) override {
    (void) job;
    {
      threads::Synchronized s(monitor_);
      pending_job_count_--;
      auto observable_job = std::dynamic_pointer_cast<ObservableSplitterJob, SplitterJob>(job);
      LOG(INFO) << "----: " << observable_job->GetBookPath() << " job done -------";
      monitor_.NotifyAll();
    }
    if (pending_job_count_ == 0) {
      LOG(INFO) << "---- last notify ";
    }
  }

  void WaitAllJobDone() {
    {
      while (pending_job_count_ != 0) {
        LOG(INFO) << "will waiiiiiiiiiiiiit";
        monitor_.Wait();
      }
    }
  }

  std::shared_ptr<ObservableSplitterJob> CreateSplitterJob() {
    return std::dynamic_pointer_cast<ObservableSplitterJob, SplitterJob>(splitter_job_factory_->CreateJob());
  }

  void SetSplitterJobFactory(std::shared_ptr<SplitterJobFactory> factory) {
    splitter_job_factory_ = factory;
  }

 private:

  void Init() {
    pending_job_count_ = 0;
    // Default EpubSectionSplitterFactory
    splitter_job_factory_ = std::make_shared<EpubSectionSplitterFactory>();
    //
    thread_factory_ = std::make_shared<threads::SimpleThreadFactory>();
    thread_manager_ = threads::ThreadManager::NewSimpleThreadManager(num_workers_,
                                                                     pending_job_max_count_);
    thread_manager_->SetThreadFactory(thread_factory_);
    thread_manager_->Start();
  }

  std::shared_ptr<SplitterJobFactory> splitter_job_factory_;

  threads::Monitor monitor_;
  size_t pending_job_count_;

  const size_t num_workers_;
  const size_t pending_job_max_count_;
  std::shared_ptr<threads::SimpleThreadFactory> thread_factory_;
  std::shared_ptr<threads::ThreadManager> thread_manager_;
  std::list<std::shared_ptr<ObservableSplitterJob>> job_list_;
  
  DISALLOW_COPY_AND_ASSIGN(SplitterJobScheduler);  
};

} // splitter
#endif // SPLITTER_SPLITTER_JOB_SCHEDULER_H_
