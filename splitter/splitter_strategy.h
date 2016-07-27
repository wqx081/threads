#ifndef SPLITTER_SPLITER_STRATEGY_H_
#define SPLITTER_SPLITER_STRATEGY_H_
#include "base/macros.h"
#include <string>
#include <vector>

#include "third_party/epub_spliter/EpubSplitter.h"

namespace splitter {

class SplitterStrategy {
 public:
  enum State { OK, ERROR };

  virtual std::string MakePartialName(const std::string& book_path,
                                      int index,
                                      const std::string& book_suffix) {
    return book_path + "_" + std::to_string(index) + book_suffix;
  }
  virtual ~SplitterStrategy() {}
  // 拆分，意味着由一个变为多个
  // @参数
  // @book_path 指明要拆分的图书路径
  // @output_paths 拆分后的文件
  virtual bool Split(const std::string& book_path, 
                     std::vector<std::pair<std::string, State>>& output_paths) = 0;
};

class EpubSectionSplitter : public SplitterStrategy {
 public:
  
  virtual bool Split(const std::string& book_path, 
                     std::vector<std::pair<std::string, State>>& output_paths) override {
    EpubSplitter epub_splitter(book_path);  
    int section_count = epub_splitter.GetSectionCount();
    State state;

    if (section_count <= 0) {
      return false;
    }

    for (int i = 0; i < section_count; ++i) {
      std::string output_name = SplitterStrategy::MakePartialName(book_path,
                                                                  i,
                                                                  "_.epub");
      state = epub_splitter.Split(i, output_name) ? OK : ERROR;
      output_paths.push_back(std::make_pair(output_name, state));
      if (state == ERROR) {
        return false;
      }
    }
    return true;
  }     
};

} // namespace splitter
#endif // SPLITTER_SPLITER_STRATEGY_H_
