#include "storage/page/page_guard.h"
#include "buffer/buffer_pool_manager.h"

namespace bustub {

BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept {
  this->bpm_ = that.bpm_;
  this->page_ = that.page_;
  that.page_ = nullptr;
  that.bpm_ = nullptr;
  this->is_dirty_ = that.is_dirty_;
}

// 释放page
void BasicPageGuard::Drop() {
  if (bpm_ != nullptr && page_ != nullptr) {
    bpm_->UnpinPage(page_->GetPageId(), is_dirty_);
  }
  bpm_ = nullptr;
  page_ = nullptr;
}

auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard & {
  Drop();  // 不保护之前的page
  bpm_ = that.bpm_;
  page_ = that.page_;
  that.page_ = nullptr;
  that.bpm_ = nullptr;
  this->is_dirty_ = that.is_dirty_;
  return *this;
}
auto BasicPageGuard::UpgradeRead() -> ReadPageGuard {
  if (page_ != nullptr) {
    page_->RLatch();
  }
  auto read_page_guard = ReadPageGuard(bpm_, page_);
  bpm_ = nullptr;
  page_ = nullptr;
  return read_page_guard;
}

auto BasicPageGuard::UpgradeWrite() -> WritePageGuard {
  if (page_ != nullptr) {
    page_->WLatch();
  }
  auto write_page_guard = WritePageGuard(bpm_, page_);
  bpm_ = nullptr;
  page_ = nullptr;
  return write_page_guard;
};  // NOLINT

BasicPageGuard::~BasicPageGuard() { Drop(); };  // NOLINT

ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept { guard_ = std::move(that.guard_); }

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
  Drop();
  guard_ = std::move(that.guard_);
  return *this;
}

void ReadPageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
  }
  guard_.Drop();
}

ReadPageGuard::~ReadPageGuard() { Drop(); }  // NOLINT

WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept { guard_ = std::move(that.guard_); }

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
  Drop();
  guard_ = std::move(that.guard_);
  return *this;
}

void WritePageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->WUnlatch();
  }
  guard_.is_dirty_ = true;
  guard_.Drop();
}

WritePageGuard::~WritePageGuard() { Drop(); }  // NOLINT

}  // namespace bustub
