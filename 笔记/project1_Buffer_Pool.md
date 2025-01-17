# 任务链接

https://15445.courses.cs.cmu.edu/fall2023/project1/



在本学期，您将为BusTub DBMS构建一个面向磁盘的存储管理器。在这样的存储管理器中，数据库的主存储位置在磁盘上。
第一个编程项目是在存储管理器中实现缓冲池。缓冲池负责将物理页从主内存来回移动到磁盘。它允许DBMS支持大于系统可用内存量的数据库。缓冲池的操作对系统中的其他部分是透明的。例如，系统使用唯一标识符（page_id_t）向缓冲池请求一个页面，但它不知道该页面是否已经在内存中，或者系统是否必须从磁盘检索它。
您的实现需要是线程安全的。多个线程将并发地访问内部数据结构，并且必须确保它们的临界区受到锁存器（在操作系统中称为“锁”）的保护。
您必须实现以下存储管理器组件：

1. LRU-K替换政策
2. 磁盘调度程序
3. 缓冲池管理器







# Task #1 - LRU-K Replacement Policy

该组件负责跟踪缓冲池中的页面使用情况。您将在src/include/buffer/lru_k_replace .h中实现一个名为LRUKReplacer的新类，并在src/buffer/lru_k_replace .cpp中实现相应的实现文件。请注意，LRUKReplacer是一个独立的类，与任何其他的Replacer类都没有关系。您只需要执行LRU-K替换策略。您不必实现LRU或时钟替换策略，即使有相应的文件



### LRU,LFU,LRU-K参考资料

https://blog.csdn.net/zhanglong_4444/article/details/88344953



LRU-K实现: 

1. 数据第一次被访问，加入到访问历史列表；

2. 如果数据在访问历史列表里后没有达到K次访问，则按照一定规则（FIFO，LRU）淘汰；

3. 当访问历史队列中的数据访问次数达到K次后，将数据索引从历史队列删除，将数据移到缓存队列中，并缓存此数据，缓存队列重新按照时间排序；

4. 缓存数据队列中被再次访问后，重新排序；

5. 需要淘汰数据时，淘汰缓存队列中排在末尾的数据，即：淘汰“倒数第K次访问离现在最久”的数据。

LRU-K具有LRU的优点，同时能够避免LRU的缺点，实际应用中LRU-2是综合各种因素后最优的选择，LRU-3或者更大的K值命中率会高，但适应性差，需要大量的数据访问才能将历史访问记录清除掉。





### LRU-K 算法

- **基本思路**：
  LRU-K 是对 LRU 的改进。它不仅记录页面最近一次的访问时间，而是记录页面最近 **K 次** 的访问时间。然后根据这 K 次访问记录来决定哪个页面更不可能在不久的将来再被访问，从而选择驱逐哪个页面。
- **为什么引入 K 次访问记录**：
  - **减少“抖动”问题**：
    有些页面可能偶尔会被访问一次（偶发的访问），而大部分时间并不频繁使用。如果只依据最近一次访问的时间来判断（如 LRU 算法），这些偶尔被访问的页面可能会频繁地进出内存（先被加载、后被驱逐，再被加载）。这种情况就是所谓的“抖动”（thrashing）。
    而 LRU-K 考虑的是页面最近 K 次访问的情况（例如 K=2 或 3），如果一个页面真正被频繁使用，那么它在过去 K 次内都会有较近的访问记录；如果页面仅仅是偶尔访问一次，那么其历史记录中可能只有一次访问记录，而其它访问记录为空。这样，在驱逐决策时，我们可以优先保留那些被多次访问的页面，从而减少不必要的磁盘 I/O 操作。
- **具体实现思路**：
  1. **数据结构**：
     为了管理这些页面，提出了使用两个自定义排序规则的集合（Set），这两个集合都只存放“可驱逐”的页面。
     - 第一个集合存放的是访问次数少于 K 次的页面。这里根据页面第一次被访问的时间进行排序，访问最早的放在前面。
     - 第二个集合存放的是访问次数大于等于 K 次的页面。这里根据页面第 K 次（倒数第 K 次访问）的时间进行排序，K 次之前访问越早的页面排在前面。
  2. **记录访问**：
     当一个页面被访问时，系统会记录下这个访问的时间，并且将该页面的访问次数增加。如果这个页面是“可驱逐”的（也就是上层调用者允许页面在内存里被替换，比如页面没有被当前的操作锁定），那么：
     - 如果该页面访问次数小于 K，就放入第一个集合中；
     - 如果该页面访问次数大于等于 K，就放入第二个集合中。
  3. **驱逐策略**：
     当缓冲池满，需要驱逐页面时：
     - 首先检查这两个集合是否为空，如果都空，则无法驱逐。
     - 如果第一个集合（访问次数少于 K 次）不为空，则优先从这个集合中选择第一个（也就是第一次被访问时间最早的）页面进行驱逐；
     - 如果第一个集合为空，则从第二个集合（访问次数大于等于 K 次）中选择第一个（即倒数第 K 次访问时间最早的）页面进行驱逐。
- **总体优势**：
  LRU-K 替换策略相比传统的 LRU 算法可以更准确地判断页面未来被访问的可能性，避免频繁地驱逐一些偶尔访问的页面，降低磁盘读取的频率，提高系统的性能。



## 类

### lru_k_replacer.h

```cpp
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.h
//
// Identification: src/include/buffer/lru_k_replacer.h
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <limits>
#include <list>
#include <mutex>  // NOLINT
#include <unordered_map>
#include <vector>

#include "common/config.h"
#include "common/macros.h"

namespace bustub {

enum class AccessType { Unknown = 0, Lookup, Scan, Index };

class LRUKNode {
 public:
  LRUKNode() = default;
  LRUKNode(size_t id, size_t k) : k_(k), fid_(id) {}
  auto GetEvictable() -> bool { return is_evictable_; }
  void SetEvictable(bool evictable) { is_evictable_ = evictable; }
  auto GetHistory() { return history_; }
  void AddHistory(size_t timestamp) { history_.push_front(timestamp); }  // 添加访问历史
  void RemoveHistory() {                                                 // 清空访问历史
    while (!history_.empty()) {
      history_.pop_back();
    }
  }
  auto GetFid() { return fid_; }
  void SetK(size_t k) { k_ = k; }
  auto GetKDistance(size_t cur) -> uint64_t {  // 计算k距离
    // 如果一个帧的历史访问次数少于k次，则将其后向k距离设置为正无穷大（+inf）
    if (history_.size() < k_) {
      return UINT32_MAX;
    }
    // 获取k次前访问的时间戳
    size_t k_distance;
    auto it = history_.begin();
    std::advance(it, k_ - 1);
    k_distance = *it;
    return cur - k_distance;
  }
  auto GetLastAccess() -> size_t {  // 获取最近一次访问的时间戳
    if (history_.empty()) {
      return UINT32_MAX;
    }
    return history_.front();
  }
  auto GetBackAccess() -> size_t {  // 获取最早一次访问的时间戳
    if (history_.empty()) {
      return UINT32_MAX;
    }
    return history_.back();
  }

 private:
  /** History of last seen K timestamps of this page. Least recent timestamp stored in front. */
  // Remove maybe_unused if you start using them. Feel free to change the member variables as you want.

  // 访问历史——时间戳链表，最近的访问时间戳在头部
  [[maybe_unused]] std::list<size_t> history_;
  // k距离
  [[maybe_unused]] size_t k_;
  // 帧id
  [[maybe_unused]] frame_id_t fid_;
  // 是否可驱逐
  [[maybe_unused]] bool is_evictable_{false};
};

/**
 * LRUKReplacer implements the LRU-k replacement policy.
 *
 * The LRU-k algorithm evicts a frame whose backward k-distance is maximum
 * of all frames. Backward k-distance is computed as the difference in time between
 * current timestamp and the timestamp of kth previous access.
 *
 * A frame with less than k historical references is given
 * +inf as its backward k-distance. When multiple frames have +inf backward k-distance,
 * classical LRU algorithm is used to choose victim.
 */
class LRUKReplacer {
 public:
  /**
   *
   * TODO(P1): Add implementation
   *
   * @brief a new LRUKReplacer.
   * @param num_frames the maximum number of frames the LRUReplacer will be required to store
   */
  explicit LRUKReplacer(size_t num_frames, size_t k);

  DISALLOW_COPY_AND_MOVE(LRUKReplacer);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Destroys the LRUReplacer.
   */
  ~LRUKReplacer() = default;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Find the frame with largest backward k-distance and evict that frame. Only frames
   * that are marked as 'evictable' are candidates for eviction.
   *
   * A frame with less than k historical references is given +inf as its backward k-distance.
   * If multiple frames have inf backward k-distance, then evict frame with earliest timestamp
   * based on LRU.
   *
   * Successful eviction of a frame should decrement the size of replacer and remove the frame's
   * access history.
   *
   * @param[out] frame_id id of frame that is evicted.
   * @return true if a frame is evicted successfully, false if no frames can be evicted.
   */
  auto Evict(frame_id_t *frame_id) -> bool;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Record the event that the given frame id is accessed at current timestamp.
   * Create a new entry for access history if frame id has not been seen before.
   *
   * If frame id is invalid (ie. larger than replacer_size_), throw an exception. You can
   * also use BUSTUB_ASSERT to abort the process if frame id is invalid.
   *
   * @param frame_id id of frame that received a new access.
   * @param access_type type of access that was received. This parameter is only needed for
   * leaderboard tests.
   */
  void RecordAccess(frame_id_t frame_id, AccessType access_type = AccessType::Unknown);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Toggle whether a frame is evictable or non-evictable. This function also
   * controls replacer's size. Note that size is equal to number of evictable entries.
   *
   * If a frame was previously evictable and is to be set to non-evictable, then size should
   * decrement. If a frame was previously non-evictable and is to be set to evictable,
   * then size should increment.
   *
   * If frame id is invalid, throw an exception or abort the process.
   *
   * For other scenarios, this function should terminate without modifying anything.
   *
   * @param frame_id id of frame whose 'evictable' status will be modified
   * @param set_evictable whether the given frame is evictable or not
   */
  void SetEvictable(frame_id_t frame_id, bool set_evictable);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Remove an evictable frame from replacer, along with its access history.
   * This function should also decrement replacer's size if removal is successful.
   *
   * Note that this is different from evicting a frame, which always remove the frame
   * with largest backward k-distance. This function removes specified frame id,
   * no matter what its backward k-distance is.
   *
   * If Remove is called on a non-evictable frame, throw an exception or abort the
   * process.
   *
   * If specified frame is not found, directly return from this function.
   *
   * @param frame_id id of frame to be removed
   */
  void Remove(frame_id_t frame_id);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Return replacer's size, which tracks the number of evictable frames.
   *
   * @return size_t
   */
  auto Size() -> size_t;

 private:
  // TODO(student): implement me! You can replace these member variables as you like.
  // Remove maybe_unused if you start using them.
  std::unordered_map<frame_id_t, LRUKNode> node_store_;  // 存放所有的页面
  size_t current_timestamp_{0};                          // 当前时间戳，每次访问都会+1
  size_t curr_size_{0};                                  // 当前存放的可驱逐页面数量
  size_t replacer_size_;                                 // 总共可以存放的页面数量
  size_t k_;                                             // LRU-k中的k
  std::mutex latch_;                                     // 互斥锁
};

}  // namespace bustub
```

### lru_k_replacer.cpp

```cpp
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include "common/exception.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  /**
   *
   * @brief Record the event that the given frame id is accessed at current timestamp.
   * Create a new entry for access history if frame id has not been seen before.
   *
   * If frame id is invalid (ie. larger than replacer_size_), throw an exception. You can
   * also use BUSTUB_ASSERT to abort the process if frame id is invalid.
   *
   * @param frame_id id of frame that received a new access.
   * @param access_type type of access that was received. This parameter is only needed for
   * leaderboard tests.
   */
  std::lock_guard<std::mutex> access_lock(latch_);
  if (curr_size_ == 0) {
    return false;
  }

  bool find = false;
  std::vector<size_t> inf_k_d_frame_id;    // k距离为无穷的页id
  size_t max_k_d = 0;                      // 记录最大的k距离
  frame_id_t max_k_d_id = replacer_size_;  // 记录最大的k距离页id

  //  遍历存储的node,找到最大的就行
  for (auto &node : node_store_) {
    // 该节点是否可淘汰
    if (!node.second.GetEvictable()) {
      continue;
    }
    size_t tmp = node.second.GetKDistance(current_timestamp_);
    // 如果当前节点的k距离更大，更新页id
    if (tmp > max_k_d) {
      find = true;
      max_k_d = tmp;
      max_k_d_id = node.first;
    }

    // 如果k距离为无穷,记录页id
    if (tmp == UINT32_MAX) {
      inf_k_d_frame_id.push_back(node.first);
    }
  }

  *frame_id = max_k_d_id;  //如果无INF,则就是LRUK，找到最大的k距离

  // 如果有inf,则就是普通的LRU，找到最近访问时间最小的
  size_t min_k_d = UINT32_MAX;
  for (size_t &i : inf_k_d_frame_id) {
    // 可淘汰且最近访问时间小
    if (node_store_[i].GetBackAccess() < min_k_d) {
      min_k_d = node_store_[i].GetBackAccess();
      *frame_id = i;
    }
  }
  // 清空历史记录
  if (find) {
    node_store_[*frame_id].RemoveHistory();
    node_store_[*frame_id].SetEvictable(false);
    curr_size_--;
    node_store_.erase(*frame_id);
    return true;
  }

  return false;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  /**
   *
   * @brief Record the event that the given frame id is accessed at current timestamp.
   * Create a new entry for access history if frame id has not been seen before.
   *
   * If frame id is invalid (ie. larger than replacer_size_), throw an exception. You can
   * also use BUSTUB_ASSERT to abort the process if frame id is invalid.
   *
   * @param frame_id id of frame that received a new access.
   * @param access_type type of access that was received. This parameter is only needed for
   * leaderboard tests.
   */
  std::lock_guard<std::mutex> access_lock(latch_);
  // 如果页id不合法
  if (static_cast<size_t>(frame_id) > replacer_size_) {
    throw Exception("LRUKReplacer::RecordAccess: frame_id is invalid");
  }
  if (node_store_.find(frame_id) == node_store_.end()) {
    // 节点已满
    if (node_store_.size() == replacer_size_) {
      return;
    }
    // 如果不存在，则添加
    node_store_[frame_id] = LRUKNode(frame_id, k_);
  }
  // 给当前页添加访问历史
  node_store_[frame_id].AddHistory(current_timestamp_);
  // 时间自增
  current_timestamp_++;
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  /**
   *
   * @brief Toggle whether a frame is evictable or non-evictable. This function also
   * controls replacer's size. Note that size is equal to number of evictable entries.
   *
   * If a frame was previously evictable and is to be set to non-evictable, then size should
   * decrement. If a frame was previously non-evictable and is to be set to evictable,
   * then size should increment.
   *
   * If frame id is invalid, throw an exception or abort the process.
   *
   * For other scenarios, this function should terminate without modifying anything.
   *
   * @param frame_id id of frame whose 'evictable' status will be modified
   * @param set_evictable whether the given frame is evictable or not
   */
  std::lock_guard<std::mutex> access_lock(latch_);
  // 如果页id不合法
  if (node_store_.find(frame_id) == node_store_.end()) {
    throw Exception("LRUKReplacer::RecordAccess: frame_id is invalid");
  }
  if (!set_evictable && node_store_[frame_id].GetEvictable()) {
    curr_size_--;
  } else if (set_evictable && !node_store_[frame_id].GetEvictable()) {
    curr_size_++;
  }
  node_store_[frame_id].SetEvictable(set_evictable);
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  /**
   *
   * @brief Remove an evictable frame from replacer, along with its access history.
   * This function should also decrement replacer's size if removal is successful.
   *
   * Note that this is different from evicting a frame, which always remove the frame
   * with largest backward k-distance. This function removes specified frame id,
   * no matter what its backward k-distance is.
   *
   * If Remove is called on a non-evictable frame, throw an exception or abort the
   * process.
   *
   * If specified frame is not found, directly return from this function.
   *
   * @param frame_id id of frame to be removed
   */

  std::lock_guard<std::mutex> access_lock(latch_);
  if (node_store_.find(frame_id) == node_store_.end()) {
    return;
  }
  // 如果页id不合法或不可驱逐
  if (!node_store_[frame_id].GetEvictable()) {
    throw Exception("LRUKReplacer::Remove: frame_id can not be remove");
  }
  //  移除
  node_store_[frame_id].RemoveHistory();
  node_store_[frame_id].SetEvictable(false);
  node_store_.erase(frame_id);
  curr_size_--;
}

auto LRUKReplacer::Size() -> size_t { return curr_size_; }

}  // namespace bustub
```





## 测试命令

```bash
cd build
make lru_k_replacer_test  -j$(nproc)
./test/lru_k_replacer_test
```



# Task #2 - Disk Scheduler

该组件负责调度对DiskManager的读写操作。您将在src/include/storage/disk/disk_scheduler.h中实现一个名为DiskScheduler的新类，并在src/storage/disk/disk_scheduler.cpp中实现相应的实现文件。

磁盘调度器可以被其他组件（在本例中是任务#3中的BufferPoolManager）用于对磁盘请求进行排队，由DiskRequest结构（已经在src/include/storage/disk/disk_scheduler.h中定义）表示。磁盘调度器将维护一个负责处理调度请求的后台工作线程。


磁盘调度器将利用共享队列来调度和处理diskrequest。一个线程将请求添加到队列中，磁盘调度器的后台工作程序将处理排队的请求。我们已经在src/include/common/ Channel .h中提供了一个Channel类，以促进线程间数据的安全共享，但如果您觉得有必要，可以自由地使用您自己的实现。


DiskScheduler构造函数和析构函数已经实现，它们负责创建和加入后台工作线程。你只需要实现头文件（src/include/storage/disk/disk_scheduler.h）和源文件（src/storage/disk/disk_scheduler.cpp）中定义的以下方法：


Schedule(DiskRequest r)：调度DiskManager执行的请求。DiskRequest结构指定请求是否为读/写，数据应该写入/从哪里写入，以及操作的页ID。DiskRequest还包括一个std::promise，一旦请求被处理，它的值应该被设置为true。
StartWorkerThread()：处理调度请求的后台工作线程的启动方法。工作线程在DiskScheduler构造函数中创建并调用此方法。该方法负责获取排队的请求并将其分配给DiskManager。记住要在DiskRequest的回调上设置值，以向请求发出请求已经完成的信号。在调用DiskScheduler的析构函数之前，这个函数不应该返回。
最后，DiskRequest的一个字段是std::promise。如果您不熟悉c++的承诺和未来，您可以查看它们的文档。出于本项目的目的，它们本质上为线程提供了一种回调机制，以知道它们的调度请求何时完成。要查看如何使用它们的示例，请查看disk_scheduler_test.cpp。


同样，实现细节由您决定，但您必须确保实现是线程安全的。



## 类

### disk_scheduler.cpp

```cpp
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_scheduler.cpp
//
// Identification: src/storage/disk/disk_scheduler.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/disk/disk_scheduler.h"
#include "common/exception.h"
#include "storage/disk/disk_manager.h"

namespace bustub {

DiskScheduler::DiskScheduler(DiskManager *disk_manager) : disk_manager_(disk_manager) {
  // TODO(P1): remove this line after you have implemented the disk scheduler API
  // throw NotImplementedException(
  //     "DiskScheduler is not implemented yet. If you have finished implementing the disk scheduler, please remove the
  //     " "throw exception line in `disk_scheduler.cpp`.");

  // Spawn the background thread
  background_thread_.emplace([&] { StartWorkerThread(); });
}

DiskScheduler::~DiskScheduler() {
  // Put a `std::nullopt` in the queue to signal to exit the loop
  request_queue_.Put(std::nullopt);
  if (background_thread_.has_value()) {
    background_thread_->join();
  }
}

void DiskScheduler::Schedule(DiskRequest r) { request_queue_.Put(std::move(r)); }

void DiskScheduler::StartWorkerThread() {
  /*
    该方法实现了后台工作线程的主循环。工作线程持续不断地从共享队列中获取磁盘请求，并将这些请求发送到 DiskManager 处理。
  */
  while (true) {
    // 获取请求
    auto request = request_queue_.Get();  // 一定要注意这里的Get()方法，它会阻塞直到队列中有元素
    if (!request.has_value()) {
      break;
    }

    // 处理请求
    if (request->is_write_) {
      disk_manager_->WritePage(request->page_id_, request->data_);
    } else {
      disk_manager_->ReadPage(request->page_id_, request->data_);
    }

    // 通知请求者
    request->callback_.set_value(true);
  }
}

}  // namespace bustub
```



## 测试

```bash
cd build
make disk_scheduler_test  -j$(nproc)
./test/disk_scheduler_test
```





# Task #3 - Buffer Pool Manager

接下来，实现缓冲池管理器（BufferPoolManager）。BufferPoolManager负责使用DiskScheduler从磁盘获取数据库页面并将它们存储在内存中。当明确指示BufferPoolManager这样做或当它需要退出一个页面为新页面腾出空间时，BufferPoolManager还可以安排将脏页面写到磁盘。


为了确保您的实现与系统的其余部分正确工作，我们将为您提供一些已经填写的功能。您也不需要实现实际读取和写入数据到磁盘的代码（在我们的实现中称为DiskManager）。我们将提供该功能。但是，您确实需要实现DiskScheduler来处理磁盘请求并将它们分派到DiskManager（这是任务#2）。


系统中的所有内存页都由Page对象表示。BufferPoolManager不需要理解这些页面的内容。但是，作为系统开发人员，理解Page对象只是缓冲池中内存的容器，因此并不特定于唯一的页面是很重要的。也就是说，每个Page对象都包含一个内存块，DiskManager将使用该内存块作为复制从磁盘读取的物理页面内容的位置。当数据来回移动到磁盘时，BufferPoolManager将重用相同的Page对象来存储数据。这意味着在系统的整个生命周期中，**同一个Page对象可能包含不同的物理页面**。Page对象的标识符（**page_id**）跟踪它所包含的物理页面；如果Page对象不包含物理页，那么它的page_id必须设置为**INVALID_PAGE_ID**。


每个Page对象还维护一个计数器，用于“固定”该页的线程数。您的BufferPoolManager**不允许释放固定的页面**。每个Page对象还跟踪它是否脏。您的工作是记录页面在解除固定之前是否被修改。您的BufferPoolManager必须将脏页的内容写回磁盘，然后才能重用该对象。


您的BufferPoolManager实现将使用您在本任务的前面步骤中创建的LRUKReplacer和DiskScheduler类。LRUKReplacer将跟踪何时访问Page对象，以便在必须释放一个帧以腾出空间从磁盘复制一个新的物理页时，它可以决定驱逐哪个对象。当将page_id映射到BufferPoolManager中的frame_id时，再次警告STL容器不是线程安全的。DiskScheduler将在DiskManager上调度对磁盘的写和读操作。


你需要实现头文件（src/include/buffer/buffer_pool_manager.h）和源文件（src/buffer/buffer_pool_manager.cpp）中定义的以下函数：


FetchPage (page_id_t page_id)
UnpinPage（page_id_t page_id, bool is_dirty）
FlushPage (page_id_t page_id)
NewPage (page_id_t * page_id)
DeletePage (page_id_t page_id)
FlushAllPages ()
对于FetchPage，如果空闲列表中没有可用的页面，并且所有其他页面当前都被固定，则应该返回nullptr。FlushPage应该刷新页面，而不管其引脚状态如何。


对于UnpinPage， is_dirty参数跟踪页面在固定时是否被修改。


当您想在NewPage（）中创建一个新页面时，AllocatePage私有方法为BufferPoolManager提供了一个唯一的新页面id。另一方面，DeallocatePage（）方法是一个no-op方法，它模仿释放磁盘上的页面，您应该在DeletePage（）实现中调用它。


您不需要使缓冲池管理器超级高效——在每个面向公共的缓冲池管理器函数中从头到尾持有缓冲池管理器锁应该就足够了。但是，您确实需要确保缓冲池管理器具有合理的性能，否则在将来的项目中会出现问题。您可以将基准测试结果（QPS.1和QPS.2）与其他学生进行比较，看看您的实现是否太慢。


请参考头文件（lru_k_replacer.h、disk_scheduler.h、buffer_pool_manager.h）以获取更详细的规范和文档。





## 类

### buffer_pool_manager.h

```cpp
#pragma once

#include <list>
#include <memory>
#include <mutex>  // NOLINT
#include <unordered_map>

#include "buffer/lru_k_replacer.h"
#include "common/config.h"
#include "recovery/log_manager.h"
#include "storage/disk/disk_scheduler.h"
#include "storage/page/page.h"
#include "storage/page/page_guard.h"

namespace bustub {

/**
 * BufferPoolManager 负责管理缓冲池中页帧的读写操作。
 * 它从磁盘中将页面读入缓冲区，也将修改后的页面写回磁盘，同时利用替换策略进行页面淘汰。
 */
class BufferPoolManager {
 public:
  /**
   * @brief 构造一个新的 BufferPoolManager 对象
   *
   * @param pool_size 缓冲池中页帧的数量
   * @param disk_manager 指向磁盘管理器的指针，用于磁盘读写
   * @param replacer_k LRU-K 算法中的参数 k，决定记录历史访问次数
   * @param log_manager 日志管理器（主要用于测试，如果为 nullptr 则表示禁用日志记录）
   */
  BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k = LRUK_REPLACER_K,
                    LogManager *log_manager = nullptr);

  /**
   * @brief 析构函数，释放 BufferPoolManager 占用的资源。
   */
  ~BufferPoolManager();

  /** @brief 返回缓冲池中页帧的总数。 */
  auto GetPoolSize() -> size_t { return pool_size_; }

  /** @brief 返回指向缓冲池中所有页面的指针。 */
  auto GetPages() -> Page * { return pages_; }

  // （这里省略其它方法的说明，可以参考各个方法的 TODO 注释）

 private:
  /** Number of pages in the buffer pool.  
      缓冲池中页帧的总数，即整个管理器可供使用的内存页数量。 */
  const size_t pool_size_;

  /** The next page id to be allocated  
      用于分配新页面时记录下一个页面的标识符，是一个原子变量，保证多线程环境下分配的正确性。 */
  std::atomic<page_id_t> next_page_id_ = 0;

  /** Array of buffer pool pages.  
      指向缓冲池中所有页面（页帧）的数组，管理器通过该数组维护页在内存中的存储。 */
  Page *pages_;

  /** Pointer to the disk scheduler.  
      指向磁盘调度器的智能指针，磁盘调度器负责管理磁盘读写请求，
      将请求以异步的方式提交给磁盘管理器执行。 */
  std::unique_ptr<DiskScheduler> disk_scheduler_ __attribute__((__unused__));

  /** Pointer to the log manager.  
      指向日志管理器的指针，目前仅在测试或者后续扩展中使用，
      如果传入为 nullptr 则表示禁用日志记录。 */
  LogManager *log_manager_ __attribute__((__unused__));

  /** Page table for keeping track of buffer pool pages.  
      缓冲池中页面的页表，使用无序哈希表将页面的页号（page_id）映射到对应的页帧编号（frame_id）。
      该数据结构用于快速查找某个页面是否已经加载到缓冲池中。 */
  std::unordered_map<page_id_t, frame_id_t> page_table_;

  /** Replacer to find unpinned pages for replacement.  
      替换器（使用 LRU-K 算法）用来选择当前缓冲池中不活跃（即未被 pin）的页面，
      当需要空出一个页帧来加载新页面时，从中选择一个进行淘汰。 */
  std::unique_ptr<LRUKReplacer> replacer_;

  /** List of free frames that don't have any pages on them.  
      空闲页帧列表，当页帧不包含任何页面时，其编号会被加入该列表，在新页分配时首先优先使用。 */
  std::list<frame_id_t> free_list_;

  /** 
   * Mutex to protect shared data structures.
   *
   * 用于保护 BufferPoolManager 内部共享数据结构（例如 page_table_、free_list_、replacer_ 等）
   * 的互斥锁，保证多线程环境下对这些数据的读写不会发生竞态条件。
   */
  std::mutex latch_;

  /**
   * @brief Allocate a page on disk. Caller should acquire the latch before calling this function.
   *
   * 分配一个新的页面标识符，通常在缓冲池中没有空闲页时调用，需要先获取互斥锁，
   * 以便安全地分配一个新的 page_id。
   *
   * @return 分配到的页面 id。
   */
  auto AllocatePage() -> page_id_t;

  /**
   * @brief Deallocate a page on disk. Caller should acquire the latch before calling this function.
   *
   * 回收一个磁盘页面的标识符，目前为一个空操作，因为没有更复杂的数据结构记录回收状态。
   *
   * @param page_id id of the page to deallocate
   */
  void DeallocatePage(__attribute__((unused)) page_id_t page_id) {
    // This is a no-nop right now without a more complex data structure to track deallocated pages
  }

  // TODO(student): 你可以在这里添加额外的私有成员和辅助函数
};

}  // namespace bustub
```

### buffer_pool_manager.cpp

```cpp
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"

#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/page_guard.h"

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_scheduler_(std::make_unique<DiskScheduler>(disk_manager)), log_manager_(log_manager) {
  // TODO(students): remove this line after you have implemented the buffer pool manager
  // throw NotImplementedException(
  //     "BufferPoolManager is not implemented yet. If you have finished implementing BPM, please remove the throw "
  //     "exception line in `buffer_pool_manager.cpp`.");

  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  Page *page;
  frame_id_t frame_id = -1;
  std::scoped_lock lock(latch_);

  //找到对应page
  if (!free_list_.empty()) {
    frame_id = free_list_.front();
    free_list_.pop_front();
    page = pages_ + frame_id;
  } else {
    //如果没有空闲的frame，那么就要从replacer中选择一个frame来替换
    if (!replacer_->Evict(&frame_id)) {
      return nullptr;
    }
    page = pages_ + frame_id;
  }

  //如果page是脏的，那么需要先写回磁盘
  if (page->IsDirty()) {
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
    future.get();
    page->is_dirty_ = false;
  }

  //分配一个新的page id
  *page_id = AllocatePage();

  // ! delete old map
  page_table_.erase(page->GetPageId());
  // ! add new map
  page_table_.emplace(*page_id, frame_id);
  // set page id
  page->page_id_ = *page_id;
  // set pin count
  page->pin_count_ = 1;
  page->ResetMemory();
  // 加入replacer
  replacer_->RecordAccess(frame_id);
  // 设置为不可淘汰
  replacer_->SetEvictable(frame_id, false);
  return page;
}

auto BufferPoolManager::FetchPage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  if (page_id == INVALID_PAGE_ID) {
    return nullptr;
  }
  std::scoped_lock lock(latch_);

  // 如果page已经在buffer pool中，那么直接返回
  if (page_table_.find(page_id) != page_table_.end()) {
    // 获取frame id
    auto frame_id = page_table_[page_id];

    // 找到对应的page
    auto page = pages_ + frame_id;

    // 更新LRU
    replacer_->RecordAccess(frame_id);
    replacer_->SetEvictable(frame_id, false);

    // pin count + 1
    page->pin_count_ += 1;
    return page;
  }

  // 不存在的话，需要从磁盘中读取
  // 不可以直接调用NewPage，因为死锁

  Page *page;
  frame_id_t frame_id = -1;

  //找到对应page
  if (!free_list_.empty()) {
    frame_id = free_list_.front();
    free_list_.pop_front();
    page = pages_ + frame_id;
  } else {
    //如果没有空闲的frame，那么就要从replacer中选择一个frame来替换
    if (!replacer_->Evict(&frame_id)) {
      return nullptr;
    }
    page = pages_ + frame_id;
  }

  //如果page是脏的，那么需要先写回磁盘
  if (page->IsDirty()) {
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
    future.get();
    page->is_dirty_ = false;
  }

  //分配一个新的page id
  *page_id = AllocatePage();

  // ! delete old map
  page_table_.erase(page->GetPageId());
  // ! add new map
  page_table_.emplace(*page_id, frame_id);
  // set page id
  page->page_id_ = *page_id;
  // set pin count
  page->pin_count_ = 1;
  page->ResetMemory();
  // 加入replacer
  replacer_->RecordAccess(frame_id);
  // 设置为不可淘汰
  replacer_->SetEvictable(frame_id, false);
  return page;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
  std::scoped_lock lock(latch_);

  // 如果page在buffer pool中
  if (page_table_.find(page_id) != page_table_.end()) {
    // 获取frame id
    auto frame_id = page_table_[page_id];
    auto page = pages_ + frame_id;
    // 如果要设置为脏页
    if (is_dirty) {
      page->is_dirty_ = is_dirty;
    }
    // 如果pin count为0，那么直接返回
    if (page->GetPinCount() == 0) {
      return false;
    }
    // pin count - 1
    page->pin_count_ -= 1;
    if (page->GetPinCount() == 0) {  //如果pin count为0，那么设置为可淘汰
      replacer_->SetEvictable(frame_id, true);
    }
    return true;
  }
  return false;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
  std::scoped_lock lock(latch_);
  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }
  // 获取对应page
  auto page = pages_ + page_table_[page_id];
  // write
  auto promise = disk_scheduler_->CreatePromise();
  auto future = promise.get_future();
  disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
  future.get();
  // 取消dirty标记
  page->is_dirty_ = false;
  return true;
}

void BufferPoolManager::FlushAllPages() {
  std::scoped_lock lock(latch_);
  for (size_t i = 0; i < pool_size_; i++) {
    auto page = pages_ + i;
    if (page->GetPageId() == INVALID_PAGE_ID) {
      continue;
    }
    // 不要直接调用FlushPage，因为会死锁
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
    future.get();
    // 取消dirty标记
    page->is_dirty_ = false;
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool { return false; }

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard { return {this, nullptr}; }

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard { return {this, nullptr}; }

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard { return {this, nullptr}; }

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard { return {this, nullptr}; }

}  // namespace bustub

```







## 测试

```bash
cd build
make buffer_pool_manager_test  -j$(nproc)
./test/buffer_pool_manager_test
```



# 其他

## Page 的主要参数及作用

在 BusTub 项目中，Page 类（定义在 `storage/page/page.h` 中）通常包含以下几个重要成员变量（在代码中以 `page->xxx` 调用）：

1. **page_id_**
   - **作用**：
     页面标识符，表示当前页面对应的磁盘页号。
     在缓冲池中，一个页面可能刚被分配时值为新生成的 page_id，也可能是从磁盘中读取时已有的有效 page_id。
   - 何时调用/修改
     - 在调用 `NewPage` 时，分配一个新的 page_id，并将其存入 page 的 `page_id_` 字段。
     - 当页面被替换（例如 FetchPage，需要从磁盘读取）时，也需要修改对应的 `page_id_`。
     - 在写回或刷新时，会使用该值通知磁盘管理器写回正确的页。
2. **pin_count_**
   - **作用**：
     表示当前页面的被 “固定”（Pin）的次数。
     当一个页面被使用时，会在缓冲池中被 “pin”（固定），防止在还在使用期间被替换；
     当页面不再使用时，调用 `UnpinPage` 将 `pin_count_` 减 1。
   - 何时调用/修改
     - 当通过 `NewPage` 或 `FetchPage` 获取页面时，**立即将 pin_count_ 设为 1**，表示正在使用中；
     - 每次对页面进行重复请求时，pin_count_ 增加，保证多重访问的计数；
     - 调用 `UnpinPage` 方法时，会将 pin_count_ 递减。当 pin_count_ 变为 0 时，意味着页面不再被使用，可以被替换，此时 BufferPoolManager 会通知替换器将该页面标记为“可淘汰”（evictable）。
3. **is_dirty_**
   - **作用**：
     标志页面是否为脏页。
     如果页面被修改过（例如通过写操作更新了内存中的数据），但尚未写回磁盘，则该标记应被设置为 true。
   - 何时调用/修改
     - 在对页面进行写操作时，会设置 `is_dirty_` 为 true；
     - 当需要写回数据到磁盘（例如在 NewPage 中替换某个页面前，如果该页面为脏，则首先写回磁盘；或者在 FlushPage/FlushAllPages 操作中），写入操作完成后会清除该标志，即设置为 false。
4. **内存数据（data_ 或 GetData() 方法）**
   - **作用**：
     指向页面数据存储区的内存指针（通常作为 Page 内部的一个字节数组）。
     用于保存从磁盘读取的数据或者写回磁盘时数据的缓冲区。
   - 何时调用/修改
     - 在页面从磁盘读取时，通过 DiskScheduler 调用 DiskManager 的 `ReadPage` 方法将磁盘数据填充到该内存区域；
     - 在页面写回（FlushPage、FlushAllPages）时，会从该内存区域取出数据并写入磁盘；
     - 获取新页面时，往往需要通过调用 `ResetMemory()`（或者类似函数）来清空页面内容，为新数据准备。
5. **ResetMemory() 方法**
   - **作用**：
     重置页面内存，为新页面准备干净的内存区域。
     在创建新页面（NewPage）后调用该方法清空旧数据，防止出现脏数据遗留问题。

### 1. 固定 Pin 与解除固定 Unpin

#### 固定页面（Pin）：

- **概念**：
  当客户端（例如 BufferPoolManager 的用户）请求页面后，该页面会进入“固定”状态，也就是“pin”。

- 主要行为

  ：

  - 在 `NewPage()` 或 `FetchPage()` 中获取页面时，都会将页面的 `pin_count_` 设定为大于 0（通常初始值设为 1 或根据已有的引用计数累加），表示当前页面正在使用中。
  - 固定的页面在缓冲池中被视为正在被占用，此时页面不允许被替换（evicted）。

- 对 LRU 替换器的影响

  ：

  - 在获取页面后，会调用类似 `replacer_->SetEvictable(frame_id, false)`。
  - 这一步将页面标记为“不可驱逐”（non-evictable），让替换器忽略该页面，避免在使用期间被错误替换。
  - 同时，还会调用 `replacer_->RecordAccess(frame_id)` 更新访问时间戳，这样可以记录下最近的访问行为，方便后续比较。

#### 解除固定页面（Unpin）：

- **概念**：
  当客户端使用完成页面后，会调用 `UnpinPage()` 来降低页面的引用计数，即解除“固定”，表明该页面不再被直接使用。

- 主要行为

  ：

  - 在 `UnpinPage()` 中，页面的 `pin_count_` 会递减。当 `pin_count_` 变为 0 时，说明页面目前没有被使用，可以被替换。
  - 如果在调用 `UnpinPage()` 时传入 `is_dirty = true`，则页面同时被标记为脏页，需要在后续写回磁盘。

- 对 LRU 替换器的影响

  ：

  - 当 `pin_count_` 变为 0 时，`UnpinPage()` 会调用 `replacer_->SetEvictable(frame_id, true)` 将该页面标记为“可驱逐”（evictable）。
  - 标记为可驱逐后，页面便进入 LRU-K 替换器的淘汰候选队列中，并根据记录的访问历史（LRU-K 算法记录的是最近 K 次访问的时间）进行排序。
  - 如果页面一直不被固定，那么它可能会在替换器中排在较前面（较久未被使用）而被淘汰；反之，如果频繁被固定（即 pin_count 总大于 0），替换器则不会选择这个页面进行替换。

------

### 2. LRU 替换器中页面状态的变化

在 BufferPoolManager 中，页面状态与 LRU 替换器之间存在紧密联系，具体如下：

1. **获取页面时（Pin 操作）：**
   - **更新访问历史**：
     在 `NewPage()` 和 `FetchPage()` 过程中，会调用 `replacer_->RecordAccess(frame_id)` 记录访问时间。
   - **禁用驱逐**：
     紧接着，会调用 `replacer_->SetEvictable(frame_id, false)` 将该页标记为不可驱逐，因为此时页面正被使用中；替换器将不会将其作为淘汰候选。
2. **解除固定页面时（Unpin 操作）：**
   - **减少引用计数**：
     调用 `UnpinPage()` 将页面的 `pin_count_` 递减。当引用计数减少为 0 时，说明页面当前无人使用。
   - **允许驱逐**：
     当 `pin_count_` 为 0 后，会调用 `replacer_->SetEvictable(frame_id, true)` 将页面标记为可驱逐。此时，该页面会根据 LRU-K 策略（也即根据该页面最近 K 次的访问历史与最早的访问时间）在替换器中排定淘汰顺序。
   - **重新记录访问**：
     如果后续再次请求该页面（例如通过 FetchPage），会再次固定（pin）页面，同时更新替换器的访问记录并将状态设为不可驱逐。
3. **页面替换的选择**：
   - 当缓冲池没有空闲帧时，BufferPoolManager 会调用 `replacer_->Evict()` 尝试选择一个可驱逐的页帧。
   - 此时，只有那些已经解除固定（pin_count == 0）并且在替换器中标记为可驱逐的页面才会成为候选者。
   - LRU-K 算法根据这些页面中“倒数第 K 次访问距离”来确定优先淘汰哪个页面。如果一个页面从解除固定开始就一直没有再次被固定，则其淘汰优先级较高。





# 基准测试

```bash
mkdir cmake-build-relwithdebinfo
cd cmake-build-relwithdebinfo
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j`nproc` bpm-bench
./bin/bustub-bpm-bench --duration 5000 --latency 1
```

