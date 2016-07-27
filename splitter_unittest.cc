#include "base/macros.h"
#include "splitter/splitter_job.h"
#include "splitter/splitter_job_scheduler.h"

#include <gtest/gtest.h>
#include <glog/logging.h>

using namespace threads;
using namespace splitter;

TEST(SplitterJobScheduler, Basic) {
  std::shared_ptr<SplitterJobScheduler> splitter_job_scheduler = 
      std::make_shared<SplitterJobScheduler>();

  std::string prefixs = "abcdefghijklmnopqrstuvwxyz";
  for (auto prefix : prefixs) {

    std::string input_epub_path = std::string("./test_data/") + prefix + ".epub";
    LOG(INFO) << input_epub_path;
    auto splitter_job = splitter_job_scheduler->CreateSplitterJob();
    splitter_job->SetBookPath(input_epub_path);
    splitter_job_scheduler->AddSplitterJob(splitter_job);
  }

  splitter_job_scheduler->WaitAllJobDone();

  EXPECT_TRUE(true);
}
