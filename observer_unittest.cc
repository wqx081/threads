#include "base/macros.h"
#include "threads/simple_thread_factory.h"
#include "threads/thread_manager.h"

#include <memory>
#include <string>
#include <list>
#include <iostream>

#include <gtest/gtest.h>
#include <glog/logging.h>

using namespace std;
using namespace threads;

class Scheduler;
class Job;

class JobObserver {
 public:
  virtual ~JobObserver() {}
  virtual void Update(std::shared_ptr<Job> job) = 0;
};

class Job : public Runnable ,
            public std::enable_shared_from_this<Job> {
 public:
  explicit Job(const std::string& book_path) : book_path_(book_path) {
    LOG(INFO) << "Job() constructor";
  }
  
  virtual ~Job() {
    LOG(INFO) << "~Job( " << book_path_ << " )";
  }  

  void SetBookPath(const std::string& book_path) {
    book_path_ = book_path;
  }
  std::string GetBookPath() const {
    return book_path_;
  }

  void RegisterObserver(std::weak_ptr<JobObserver> job_observer) {
    for (auto& observer : observer_list_) {
      if (job_observer.lock() == observer.lock()) {
        cout << "Already Register Job\n";
      }
    }
    observer_list_.push_back(job_observer);  
  }
  void NotifyAllObservers() {
    for (auto& observer : observer_list_) {
      std::shared_ptr<JobObserver> ob = observer.lock();
      ob->Update(shared_from_this());
    }
  }

  void Run() override {
    LOG(INFO) << "JOB::Run()";
    NotifyAllObservers();
  }

 private:
  std::list<std::weak_ptr<JobObserver>> observer_list_;
  std::string book_path_;

};

class Scheduler : public JobObserver, 
                  public std::enable_shared_from_this<Scheduler> {
 public:
  explicit Scheduler(size_t num_workers=4, size_t pending_job_max_count=1024) 
      : num_workers_(num_workers),
        pending_job_max_count_(pending_job_max_count) {
    DoInit();
  }

  ~Scheduler() {
    LOG(INFO) << "~Scheduler() " << endl;  
    thread_manager_->Stop();
  }

  static std::shared_ptr<Job> NewJob(const std::string& book_path) {
    return std::make_shared<Job>(book_path);
  }

  void AddJob(std::shared_ptr<Job> job) {
    job_list_.push_back(job);
    job->RegisterObserver(shared_from_this());
    job_count_++;
    thread_manager_->Add(job);
  }

  void Update(std::shared_ptr<Job> job) override {
  (void) job;
    if (job == nullptr) {
      LOG(ERROR) << "-----------Error: job delete before";
    } else {
      LOG(INFO) << "------Get notify";
    }
  }

 private:
  std::list<std::shared_ptr<Job>> job_list_;
  size_t job_count_;
  const size_t num_workers_;
  const size_t pending_job_max_count_;
  std::shared_ptr<SimpleThreadFactory> thread_factory_;
  std::shared_ptr<ThreadManager> thread_manager_;

  DISALLOW_COPY_AND_ASSIGN(Scheduler);

  void DoInit() {
    job_count_ = 0;
    thread_factory_ = std::make_shared<SimpleThreadFactory>();
    thread_manager_ = ThreadManager::NewSimpleThreadManager(num_workers_, pending_job_max_count_);
    thread_manager_->SetThreadFactory(thread_factory_);
    thread_manager_->Start();
  }
};

shared_ptr<Scheduler> g_scheduler;

TEST(ObserverTest, Base) {

  //shared_ptr<Scheduler> scheduler = make_shared<Scheduler>();
  g_scheduler = make_shared<Scheduler>();
  for (size_t i = 0; i < 5; ++i) {
    auto job = Scheduler::NewJob(std::to_string(i) + "_job");
    g_scheduler->AddJob(job);
  }

  LOG(INFO) << "------ DONE -------";
  EXPECT_TRUE(true);
}
