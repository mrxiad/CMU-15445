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
