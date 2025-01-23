//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_extendible_hash_table.cpp
//
// Identification: src/container/disk/hash/disk_extendible_hash_table.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/config.h"
#include "common/exception.h"
#include "common/logger.h"
#include "common/macros.h"
#include "common/rid.h"
#include "common/util/hash_util.h"
#include "container/disk/hash/disk_extendible_hash_table.h"
#include "storage/index/hash_comparator.h"
#include "storage/page/extendible_htable_bucket_page.h"
#include "storage/page/extendible_htable_directory_page.h"
#include "storage/page/extendible_htable_header_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

template <typename K, typename V, typename KC>
DiskExtendibleHashTable<K, V, KC>::DiskExtendibleHashTable(const std::string &name, BufferPoolManager *bpm,
                                                           const KC &cmp, const HashFunction<K> &hash_fn,
                                                           uint32_t header_max_depth, uint32_t directory_max_depth,
                                                           uint32_t bucket_max_size)
    : bpm_(bpm),
      cmp_(cmp),
      hash_fn_(std::move(hash_fn)),
      header_max_depth_(header_max_depth),
      directory_max_depth_(directory_max_depth),
      bucket_max_size_(bucket_max_size) {
  this->index_name_ = name;
  // 初始化header页
  header_page_id_ = INVALID_PAGE_ID;
  auto header_guard = bpm->NewPageGuarded(&header_page_id_);
  auto header_page = header_guard.AsMut<ExtendibleHTableHeaderPage>();
  header_page->Init(header_max_depth_);
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
template <typename K, typename V, typename KC>
// 在哈希表中查找与给定键相关联的值
// key:要查找的键;result:与给定键相关联的值; transaction 当前事务
auto DiskExtendibleHashTable<K, V, KC>::GetValue(const K &key, std::vector<V> *result, Transaction *transaction) const
    -> bool {
  // 获取header page
  ReadPageGuard header_guard = bpm_->FetchPageRead(header_page_id_);
  auto header_page = header_guard.As<ExtendibleHTableHeaderPage>();
  // 通过hash值获取dir_page_id。若dir_page_id为非法id则未找到
  auto hash = Hash(key);
  auto dir_index = header_page->HashToDirectoryIndex(hash);
  page_id_t directory_page_id = header_page->GetDirectoryPageId(dir_index);
  if (directory_page_id == INVALID_PAGE_ID) {
    return false;
  }
  // 获取dir_page
  header_guard.Drop();
  ReadPageGuard directory_guard = bpm_->FetchPageRead(directory_page_id);
  auto directory_page = directory_guard.As<ExtendibleHTableDirectoryPage>();
  // 通过hash值获取bucket_page_id。若bucket_page_id为非法id则未找到
  auto bucket_index = directory_page->HashToBucketIndex(hash);
  auto bucket_page_id = directory_page->GetBucketPageId(bucket_index);
  if (bucket_page_id == INVALID_PAGE_ID) {
    return false;
  }
  ReadPageGuard bucket_guard = bpm_->FetchPageRead(bucket_page_id);
  // 获取bucket_page
  directory_guard.Drop();
  auto bucket_page = bucket_guard.As<ExtendibleHTableBucketPage<K, V, KC>>();
  // 在bucket_page上查找
  V value;
  if (bucket_page->Lookup(key, value, cmp_)) {
    result->push_back(value);
    return true;
  }
  return false;
}

// 分裂单个bucket
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::SplitBucket(ExtendibleHTableDirectoryPage *directory,
                                                    ExtendibleHTableBucketPage<K, V, KC> *bucket, uint32_t bucket_idx)
    -> bool {
  // 创建新bucket_page
  page_id_t split_page_id;
  WritePageGuard split_bucket_guard = bpm_->NewPageGuarded(&split_page_id).UpgradeWrite();
  if (split_page_id == INVALID_PAGE_ID) {
    return false;
  }
  auto split_bucket = split_bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  split_bucket->Init(bucket_max_size_);
  uint32_t split_idx = directory->GetSplitImageIndex(bucket_idx);
  uint32_t local_depth = directory->GetLocalDepth(bucket_idx);
  directory->SetBucketPageId(split_idx, split_page_id);
  directory->SetLocalDepth(split_idx, local_depth);
  // 将原来满的bucket_page拆分到两个page页中
  page_id_t bucket_page_id = directory->GetBucketPageId(bucket_idx);
  if (bucket_page_id == INVALID_PAGE_ID) {
    return false;
  }

  // 先将原来的数据取出，放置在entries容器中
  int size = bucket->Size();
  std::list<std::pair<K, V>> entries;
  for (int i = 0; i < size; i++) {
    entries.emplace_back(bucket->EntryAt(i));
  }
  // 清空bucket:size_ = 0
  bucket->Clear();

  // 分到两个bucket_page中
  for (const auto &entry : entries) {
    uint32_t target_idx = directory->HashToBucketIndex(Hash(entry.first));
    page_id_t target_page_id = directory->GetBucketPageId(target_idx);
    if (target_page_id == bucket_page_id) {
      bucket->Insert(entry.first, entry.second, cmp_);
    } else if (target_page_id == split_page_id) {
      split_bucket->Insert(entry.first, entry.second, cmp_);
    }
  }
  return true;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::Insert(const K &key, const V &value, Transaction *transaction) -> bool {
  std::vector<V> values_found;
  bool key_exists = GetValue(key, &values_found, transaction);
  if (key_exists) {  // 已存在直接返回false表示不插入重复键
    return false;
  }
  auto hash_key = Hash(key);
  // 获取header page
  WritePageGuard header_guard = bpm_->FetchPageWrite(header_page_id_);
  auto header_page = header_guard.AsMut<ExtendibleHTableHeaderPage>();
  // 使用header_page来获取目录索引
  auto directory_index = header_page->HashToDirectoryIndex(hash_key);
  //用目录索引获取目录页，然后找到頁的目录ID
  page_id_t directory_page_id = header_page->GetDirectoryPageId(directory_index);
  // 若dir_page_id为非法id则在新的dir_page添加
  if (directory_page_id == INVALID_PAGE_ID) {
    return InsertToNewDirectory(header_page, directory_index, hash_key, key, value);
  }
  // 对directory加锁
  header_guard.Drop();
  WritePageGuard directory_guard = bpm_->FetchPageWrite(directory_page_id);
  // 获取 dir page
  auto directory_page = directory_guard.AsMut<ExtendibleHTableDirectoryPage>();
  // 通过hash值获取bucket_page_id。若bucket_page_id为非法id则在新的bucket_page添加
  auto bucket_index = directory_page->HashToBucketIndex(hash_key);
  auto bucket_page_id = directory_page->GetBucketPageId(bucket_index);
  if (bucket_page_id == INVALID_PAGE_ID) {
    return InsertToNewBucket(directory_page, bucket_index, key, value);
  }

  // 对bucket加锁
  WritePageGuard bucket_guard = bpm_->FetchPageWrite(bucket_page_id);
  auto bucket_page = bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  // 获取bucket_page插入元素，如果插入失败则代表该bucket_page满了
  if (bucket_page->Insert(key, value, cmp_)) {
    return true;
  }
  auto h = 1U << directory_page->GetGlobalDepth();
  // 判断是否能添加度，不能则返回
  if (directory_page->GetLocalDepth(bucket_index) == directory_page->GetGlobalDepth()) {
    if (directory_page->GetGlobalDepth() >= directory_page->GetMaxDepth()) {  // 如果全局深度大于等于最大深度，则返回
      return false;
    }
    directory_page->IncrGlobalDepth();  // 全局深度加1
  }
  directory_page->IncrLocalDepth(bucket_index);
  directory_page->IncrLocalDepth(bucket_index + h);
  // 拆份bucket
  if (!SplitBucket(directory_page, bucket_page, bucket_index)) {
    return false;
  }
  bucket_guard.Drop();
  directory_guard.Drop();
  return Insert(key, value, transaction);
}

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::InsertToNewDirectory(ExtendibleHTableHeaderPage *header, uint32_t directory_idx,
                                                             uint32_t hash, const K &key, const V &value) -> bool {
  page_id_t directory_page_id = INVALID_PAGE_ID;
  WritePageGuard directory_guard = bpm_->NewPageGuarded(&directory_page_id).UpgradeWrite();
  auto directory_page = directory_guard.AsMut<ExtendibleHTableDirectoryPage>();
  directory_page->Init(directory_max_depth_);
  header->SetDirectoryPageId(directory_idx, directory_page_id);
  uint32_t bucket_idx = directory_page->HashToBucketIndex(hash);
  return InsertToNewBucket(directory_page, bucket_idx, key, value);
}

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::InsertToNewBucket(ExtendibleHTableDirectoryPage *directory, uint32_t bucket_idx,
                                                          const K &key, const V &value) -> bool {
  page_id_t bucket_page_id = INVALID_PAGE_ID;
  WritePageGuard bucket_guard = bpm_->NewPageGuarded(&bucket_page_id).UpgradeWrite();
  auto bucket_page = bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  bucket_page->Init(bucket_max_size_);
  directory->SetBucketPageId(bucket_idx, bucket_page_id);
  return bucket_page->Insert(key, value, cmp_);
}

template <typename K, typename V, typename KC>
void DiskExtendibleHashTable<K, V, KC>::UpdateDirectoryMapping(ExtendibleHTableDirectoryPage *directory,
                                                               uint32_t new_bucket_idx, page_id_t new_bucket_page_id,
                                                               uint32_t new_local_depth, uint32_t local_depth_mask) {
  for (uint32_t i = 0; i < (1U << directory->GetGlobalDepth()); ++i) {
    // 检查目录条目是否需要更新为指向新桶
    // 如果目录项对应的是原桶
    if (directory->GetBucketPageId(i) == directory->GetBucketPageId(new_bucket_idx)) {
      if ((i & local_depth_mask) != 0U) {
        // 如果这个目录项的在新局部深度位上的值为1，应该指向新桶
        directory->SetBucketPageId(i, new_bucket_page_id);
        directory->SetLocalDepth(i, new_local_depth);
      } else {
        // 否则，它仍然指向原桶，但其局部深度需要更新
        directory->SetLocalDepth(i, new_local_depth);
      }
    }
  }
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::Remove(const K &key, Transaction *transaction) -> bool {
  uint32_t hash = Hash(key);
  WritePageGuard header_guard = bpm_->FetchPageWrite(header_page_id_);
  auto header_page = header_guard.AsMut<ExtendibleHTableHeaderPage>();
  uint32_t directory_index = header_page->HashToDirectoryIndex(hash);
  page_id_t directory_page_id = header_page->GetDirectoryPageId(directory_index);
  if (directory_page_id == INVALID_PAGE_ID) {
    return false;
  }
  header_guard.Drop();
  WritePageGuard directory_guard = bpm_->FetchPageWrite(directory_page_id);
  auto directory_page = directory_guard.AsMut<ExtendibleHTableDirectoryPage>();
  uint32_t bucket_index = directory_page->HashToBucketIndex(hash);
  page_id_t bucket_page_id = directory_page->GetBucketPageId(bucket_index);
  if (bucket_page_id == INVALID_PAGE_ID) {
    return false;
  }
  WritePageGuard bucket_guard = bpm_->FetchPageWrite(bucket_page_id);
  auto bucket_page = bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  bool res = bucket_page->Remove(key, cmp_);
  bucket_guard.Drop();
  if (!res) {
    return false;
  }

  // 检查合并桶
  auto check_page_id = bucket_page_id;
  ReadPageGuard check_guard = bpm_->FetchPageRead(check_page_id);
  auto check_page = check_guard.As<ExtendibleHTableBucketPage<K, V, KC>>();
  uint32_t local_depth = directory_page->GetLocalDepth(bucket_index);
  uint32_t global_depth = directory_page->GetGlobalDepth();
  while (local_depth > 0) {
    // 获取要合并的桶的索引
    uint32_t convert_mask = 1 << (local_depth - 1);
    uint32_t merge_bucket_index = bucket_index ^ convert_mask;
    uint32_t merge_local_depth = directory_page->GetLocalDepth(merge_bucket_index);
    page_id_t merge_page_id = directory_page->GetBucketPageId(merge_bucket_index);
    ReadPageGuard merge_guard = bpm_->FetchPageRead(merge_page_id);
    auto merge_page = merge_guard.As<ExtendibleHTableBucketPage<K, V, KC>>();
    if (merge_local_depth != local_depth || (!check_page->IsEmpty() && !merge_page->IsEmpty())) {
      break;
    }
    if (check_page->IsEmpty()) {
      bpm_->DeletePage(check_page_id);
      check_page = merge_page;
      check_page_id = merge_page_id;
      check_guard = std::move(merge_guard);
    } else {
      bpm_->DeletePage(merge_page_id);
    }
    directory_page->DecrLocalDepth(bucket_index);
    local_depth = directory_page->GetLocalDepth(bucket_index);
    uint32_t local_depth_mask = directory_page->GetLocalDepthMask(bucket_index);
    uint32_t mask_idx = bucket_index & local_depth_mask;
    uint32_t update_count = 1 << (global_depth - local_depth);
    for (uint32_t i = 0; i < update_count; ++i) {
      uint32_t tmp_idx = (i << local_depth) + mask_idx;
      UpdateDirectoryMapping(directory_page, tmp_idx, check_page_id, local_depth, 0);
    }
  }
  while (directory_page->CanShrink()) {
    directory_page->DecrGlobalDepth();
  }
  return true;
}

template class DiskExtendibleHashTable<int, int, IntComparator>;
template class DiskExtendibleHashTable<GenericKey<4>, RID, GenericComparator<4>>;
template class DiskExtendibleHashTable<GenericKey<8>, RID, GenericComparator<8>>;
template class DiskExtendibleHashTable<GenericKey<16>, RID, GenericComparator<16>>;
template class DiskExtendibleHashTable<GenericKey<32>, RID, GenericComparator<32>>;
template class DiskExtendibleHashTable<GenericKey<64>, RID, GenericComparator<64>>;
}  // namespace bustub
