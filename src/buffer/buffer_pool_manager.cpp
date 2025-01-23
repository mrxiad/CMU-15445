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
  if (!free_list_.empty()) {
    frame_id = free_list_.back();
    free_list_.pop_back();
    page = pages_ + frame_id;
  } else {
    if (!replacer_->Evict(&frame_id)) {
      return nullptr;
    }
    page = pages_ + frame_id;
  }
  // 脏页
  if (page->IsDirty()) {
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    disk_scheduler_->Schedule({true, page->GetData(), page->GetPageId(), std::move(promise)});
    future.get();
    page->is_dirty_ = false;
  }
  // 删掉映射
  page_table_.erase(page->GetPageId());
  // 加入新映射
  page_table_.emplace(page_id, frame_id);
  // 设置page id
  page->page_id_ = page_id;
  page->pin_count_ = 1;
  page->ResetMemory();  // reset memory
  // 更新LRU
  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);
  // 从磁盘中读取
  auto promise = disk_scheduler_->CreatePromise();
  auto future = promise.get_future();
  disk_scheduler_->Schedule({false, page->GetData(), page->GetPageId(), std::move(promise)});
  future.get();
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

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard {
  auto page = FetchPage(page_id);
  return {this, page};
}

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  auto page = FetchPage(page_id);
  if (page != nullptr) {
    page->RLatch();
  }
  return {this, page};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  auto page = FetchPage(page_id);
  if (page != nullptr) {
    page->WLatch();
  }
  return {this, page};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard {
  auto page = NewPage(page_id);
  return {this, page};
}

}  // namespace bustub
