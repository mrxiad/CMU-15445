# 任务连接

https://15445.courses.cs.cmu.edu/fall2023/project2/





在这个编程项目中，您将在数据库系统中实现磁盘支持的哈希索引。您将使用[可扩展哈希](https://en.wikipedia.org/wiki/Extendible_hashing)的变体作为哈希方案。与课堂上教授的两级方案不同，我们在目录页顶部添加了一个不可调整大小的标题页，以便哈希表可以容纳更多值并可能实现更好的多线程性能。

下图显示了一个可扩展哈希表，其标题页的最大深度为 2，目录页的最大深度为 2，存储桶页最多可容纳两个条目。省略了值，存储桶页中显示的是键的哈希值，而不是键本身。





# 任务 #1 - 读/写页面保护

在缓冲池管理器中，`FetchPage`和`NewPage`函数返回已锁定页面的指针。锁定机制确保页面不会被逐出，直到不再对该页面进行读写操作为止。为了指示内存中不再需要该页面，程序员必须手动调用`UnpinPage`。

另一方面，如果程序员忘记调用`UnpinPage`，页面将永远不会被逐出缓冲池。由于缓冲池以较少的帧数运行，因此将有更多的页面进出磁盘。不仅性能受到影响，而且错误也很难被发现。

您将实现`BasicPageGuard`存储指向`BufferPoolManager`和`Page`对象的指针。页面保护确保在相应对象超出范围时立即`UnpinPage`调用。请注意，它仍应公开一种方法，供程序员手动取消固定页面。`Page`

由于`BasicPageGuard`隐藏了底层`Page`指针，它还可以提供只读/写数据 API，提供编译时检查以确保`is_dirty`为每个用例正确设置标志。

在这个和未来的项目中，多个线程将从相同的页面读取和写入，因此需要读写闩锁来确保数据的正确性。请注意，在类中`Page`，有相关的闩锁方法用于此目的。与页面的取消锁定类似，程序员可能会在使用后忘记解锁页面。为了缓解这个问题，您将实现`ReadPageGuard`和，`WritePageGuard`一旦页面超出范围，就会自动解锁页面。

您需要为所有`BasicPageGuard`、`ReadPageGuard`和实现以下功能`WritePageGuard`。

- `PageGuard(PageGuard &&that)`：移动构造函数。
- `operator=(PageGuard &&that)`：移动运算符。
- `Drop()`：解开和/或解锁。
- `~PageGuard()`：析构函数。

您还需要为 实现以下升级函数`BasicPageGuard`。这些函数需要保证受保护的页面在升级期间不会被从缓冲池中逐出。

- `UpgradeRead()`：升级至`ReadPageGuard`
- `UpgradeWrite()`：升级至`WritePageGuard`

使用新的页面保护，在中实现以下包装器`BufferPoolManager`。

- `FetchPageBasic(page_id_t page_id)`
- `FetchPageRead(page_id_t page_id)`
- `FetchPageWrite(page_id_t page_id)`
- `NewPageGuarded(page_id_t *page_id)`

请参阅头文件（`buffer_pool_manager.h`和`page_guard.h`）了解更详细的规格和文档。





## 类

### page_guard.h

```cpp
#pragma once

#include "storage/page/page.h"

namespace bustub {

class BufferPoolManager;
class ReadPageGuard;
class WritePageGuard;

class BasicPageGuard {
 public:
  BasicPageGuard() = default;

  BasicPageGuard(BufferPoolManager *bpm, Page *page) : bpm_(bpm), page_(page) {}

  BasicPageGuard(const BasicPageGuard &) = delete;
  auto operator=(const BasicPageGuard &) -> BasicPageGuard & = delete;

  /** TODO(P2): 添加实现
   *
   * @brief BasicPageGuard 的移动构造函数
   *
   * 当调用 BasicPageGuard(std::move(other_guard)) 时，
   * 期望新 guard 的行为与原来的完全相同。此外，旧的 page guard 不应可用。
   * 例如，不能对两个 page guard 都调用 .Drop()，并使 pin 计数减少 2。
   */
  BasicPageGuard(BasicPageGuard &&that) noexcept;

  /** TODO(P2): 添加实现
   *
   * @brief 释放 page guard
   *
   * 释放 page guard 应清除所有内容
   * （使 page guard 不再有用），并
   * 告诉 BPM 我们已经完成了对这个页面的使用，
   * 根据文档中的规格。
   */
  void Drop();

  /** TODO(P2): 添加实现
   *
   * @brief BasicPageGuard 的移动赋值操作符
   *
   * 类似于移动构造函数，但移动赋值操作符假设 BasicPageGuard 已经拥有了一个被保护的页面。
   * 仔细考虑当 guard 用一个不同的页面替换它持有的页面时，应该发生什么，
   * 考虑到 page guard 的目的。
   */
  auto operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard &;

  /** TODO(P1): 添加实现
   *
   * @brief BasicPageGuard 的析构函数
   *
   * 当 page guard 超出作用域时，它的行为应该像释放掉该 guard 一样。
   */
  ~BasicPageGuard();

  /** TODO(P2): 添加实现
   *
   * @brief 将 BasicPageGuard 升级为 ReadPageGuard
   *
   * 在升级过程中，受保护的页面不会从缓冲池中被驱逐，
   * 并且在调用此函数后，basic page guard 应被标记为无效。
   *
   * @return 升级后的 ReadPageGuard
   */
  auto UpgradeRead() -> ReadPageGuard;

  /** TODO(P2): 添加实现
   *
   * @brief 将 BasicPageGuard 升级为 WritePageGuard
   *
   * 在升级过程中，受保护的页面不会从缓冲池中被驱逐，
   * 并且在调用此函数后，basic page guard 应被标记为无效。
   *
   * @return 升级后的 WritePageGuard
   */
  auto UpgradeWrite() -> WritePageGuard;

  auto PageId() -> page_id_t { return page_->GetPageId(); }

  auto GetData() -> const char * { return page_->GetData(); }

  template <class T>
  auto As() -> const T * {
    return reinterpret_cast<const T *>(GetData());
  }

  auto GetDataMut() -> char * {
    is_dirty_ = true;
    return page_->GetData();
  }

  template <class T>
  auto AsMut() -> T * {
    return reinterpret_cast<T *>(GetDataMut());
  }

 private:
  friend class ReadPageGuard;
  friend class WritePageGuard;

  [[maybe_unused]] BufferPoolManager *bpm_{nullptr};
  Page *page_{nullptr};
  bool is_dirty_{false};
};

class ReadPageGuard {
 public:
  ReadPageGuard() = default;
  ReadPageGuard(BufferPoolManager *bpm, Page *page) : guard_(bpm, page) {}
  ReadPageGuard(const ReadPageGuard &) = delete;
  auto operator=(const ReadPageGuard &) -> ReadPageGuard & = delete;

  /** TODO(P2): 添加实现
   *
   * @brief ReadPageGuard 的移动构造函数
   *
   * 与 BasicPageGuard 非常相似。需要使用另一个 ReadPageGuard 创建一个新的 ReadPageGuard。
   * 考虑是否有方法可以使这个过程更简单...
   */
  ReadPageGuard(ReadPageGuard &&that) noexcept;

  /** TODO(P2): 添加实现
   *
   * @brief ReadPageGuard 的移动赋值操作符
   *
   * 与 BasicPageGuard 非常相似。给定另一个 ReadPageGuard，
   * 用它的内容替换当前的内容。
   */
  auto operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard &;

  /** TODO(P2): 添加实现
   *
   * @brief 释放 ReadPageGuard
   *
   * ReadPageGuard 的释放行为应与 BasicPageGuard 类似，
   * 但 ReadPageGuard 有一个额外的资源——锁定！
   * 需要非常仔细地考虑释放这些资源的顺序。
   */
  void Drop();

  /** TODO(P2): 添加实现
   *
   * @brief ReadPageGuard 的析构函数
   *
   * 与 BasicPageGuard 类似，其行为应像释放 guard 一样。
   */
  ~ReadPageGuard();

  auto PageId() -> page_id_t { return guard_.PageId(); }

  auto GetData() -> const char * { return guard_.GetData(); }

  template <class T>
  auto As() -> const T * {
    return guard_.As<T>();
  }

 private:
  // 可以选择删除这个变量并添加自己的私有变量。
  BasicPageGuard guard_;
};

class WritePageGuard {
 public:
  WritePageGuard() = default;
  WritePageGuard(BufferPoolManager *bpm, Page *page) : guard_(bpm, page) {}
  WritePageGuard(const WritePageGuard &) = delete;
  auto operator=(const WritePageGuard &) -> WritePageGuard & = delete;

  /** TODO(P2): 添加实现
   *
   * @brief WritePageGuard 的移动构造函数
   *
   * 与 BasicPageGuard 非常相似。需要使用另一个 WritePageGuard 创建一个新的 WritePageGuard。
   * 考虑是否有方法可以使这个过程更简单...
   */
  WritePageGuard(WritePageGuard &&that) noexcept;

  /** TODO(P2): 添加实现
   *
   * @brief WritePageGuard 的移动赋值操作符
   *
   * 与 BasicPageGuard 非常相似。给定另一个 WritePageGuard，
   * 用它的内容替换当前的内容。
   */
  auto operator=(WritePageGuard &&that) noexcept -> WritePageGuard &;

  /** TODO(P2): 添加实现
   *
   * @brief 释放 WritePageGuard
   *
   * WritePageGuard 的释放行为应与 BasicPageGuard 类似，
   * 但 WritePageGuard 有一个额外的资源——锁定！
   * 需要非常仔细地考虑释放这些资源的顺序。
   */
  void Drop();

  /** TODO(P2): 添加实现
   *
   * @brief WritePageGuard 的析构函数
   *
   * 与 BasicPageGuard 类似，其行为应像释放 guard 一样。
   */
  ~WritePageGuard();

  auto PageId() -> page_id_t { return guard_.PageId(); }

  auto GetData() -> const char * { return guard_.GetData(); }

  template <class T>
  auto As() -> const T * {
    return guard_.As<T>();
  }

  auto GetDataMut() -> char * { return guard_.GetDataMut(); }

  template <class T>
  auto AsMut() -> T * {
    return guard_.AsMut<T>();
  }

 private:
  // 可以选择删除这个变量并添加自己的私有变量。
  BasicPageGuard guard_;
};

}  // namespace bustub
```



### page_guard.cpp

```cpp
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
```



### buffer_pool_manager.cpp

```cpp
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
```



## 测试

```bash
cd build
make page_guard_test  -j$(nproc)
./test/page_guard_test
```





# 任务 #2 - 可扩展哈希表页面

讲解：https://www.cnblogs.com/wevolf/p/18302985
详解：https://zhuanlan.zhihu.com/p/569306929

您必须实现三个 Page 类来存储可扩展哈希表的数据。

- [**哈希表标题页**](https://15445.courses.cs.cmu.edu/fall2023/project2/#htable-header-page)
- [**哈希表目录页**](https://15445.courses.cs.cmu.edu/fall2023/project2/#htable-directory-page)
- [**哈希表桶页**](https://15445.courses.cs.cmu.edu/fall2023/project2/#htable-bucket-page)



## Hash

### **1. Hash Table Header Page**

#### **作用**

Header Page 是哈希表的第一层目录页，负责存储指向二级目录页的逻辑子指针（page IDs）。在整个哈希表中，只有一个 Header Page。

#### **数据结构**

| **字段名**            | **大小**  | **描述**                                                     |
| --------------------- | --------- | ------------------------------------------------------------ |
| `directory_page_ids_` | 2048 字节 | 一个数组，存储指向目录页的 page IDs，每个元素是一个整数。    |
| `max_depth_`          | 4 字节    | Header Page 能处理的最大深度，用于限制 `directory_page_ids_` 的大小。 |

- 逻辑
  - `directory_page_ids_` 是一个固定大小的数组，但逻辑限制由 `max_depth_` 决定。这个字段确保目录结构的深度在合理范围内。
  - `max_depth_` 决定了哈希表的层级深度和能存储的子指针数量。

------

### **2. Hash Table Directory Page**

#### **作用**

Directory Page 位于哈希表的第二层。它负责映射桶（bucket）的结构，存储桶的 page IDs 及其元数据，并处理动态目录扩展和收缩。

#### **数据结构**

| **字段名**         | **大小**  | **描述**                                               |
| ------------------ | --------- | ------------------------------------------------------ |
| `max_depth_`       | 4 字节    | Header Page 可支持的最大深度（继承自 Header Page）。   |
| `global_depth_`    | 4 字节    | 当前目录的全局深度，表示当前目录的层级深度。           |
| `local_depths_`    | 512 字节  | 一个数组，存储每个桶的本地深度，表示每个桶的分裂深度。 |
| `bucket_page_ids_` | 2048 字节 | 一个数组，存储桶的 page IDs，每个元素对应一个桶。      |

- **逻辑**：
  - `global_depth_` 控制目录的全局范围，决定了哈希值映射的前缀长度（`global_depth_` 位）。
  - `local_depths_` 表示每个桶的分裂状态，用于优化动态扩展和收缩。
  - `bucket_page_ids_` 存储实际桶页的指针，是目录页的主要作用。
- **目录扩展**：
  - 当某个桶满了并需要分裂时，目录可能需要扩展（全局深度增加），从而创建更多映射。
  - 动态调整 `global_depth_` 和 `local_depths_` 以支持更大范围的哈希值。

------

### **3. Hash Table Bucket Page**

#### **作用**

Bucket Page 位于哈希表的第三层，是真正存储键值对的页。每个桶（bucket）存储一部分键值对。

#### **数据结构**

| **字段名**  | **大小**           | **描述**                                  |
| ----------- | ------------------ | ----------------------------------------- |
| `size_`     | 4 字节             | 当前桶中存储的键值对数量。                |
| `max_size_` | 4 字节             | 桶能容纳的最大键值对数量。                |
| `array_`    | 小于等于 4088 字节 | 键值对数组，实际大小由 `max_size_` 决定。 |

- **逻辑**：
  - `size_` 用于动态追踪当前桶中存储的键值对数量。
  - `max_size_` 限制桶的容量，确保不会超出磁盘页的大小。
  - `array_` 存储键值对，通常实现为一个定长数组，具体键值对结构视实现而定。
- **溢出与分裂**：
  - 当桶中的键值对数量达到 `max_size_` 时，需要触发桶的分裂。
  - 桶分裂可能导致目录扩展，依赖 Directory Page 的逻辑处理。

------

### **三者的层级关系**

1. **Header Page**：
   - 位于顶层，存储指向多个 Directory Page 的指针。
   - 每个指针指向一个二级目录页。
2. **Directory Page**：
   - 位于中间层，管理桶的元信息。
   - 通过 `bucket_page_ids_` 数组，指向实际存储数据的 Bucket Page。
3. **Bucket Page**：
   - 位于底层，存储实际的键值对。
   - 桶满时，触发分裂并可能导致上层结构的调整。



以下是扩展哈希表的三个页面类及其变量的详细说明和具体示例：

------

- [ ] 哈希表头页面 (Hash Table Header Page)

| 变量名                | 大小 | 作用描述                                                     | 示例                                                         |
| --------------------- | ---- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| `directory_page_ids_` | 2048 | 存储目录页面的页ID数组，每个页ID指向一个目录页面。用于管理哈希表的目录层。 | 假设哈希表有两个目录页面，`directory_page_ids_[0] = 1001`，`directory_page_ids_[1] = 1002`。 |
| `max_depth_`          | 4    | 表示头页面可以处理的最大深度，限制目录页面ID数组的大小。     | 如果 `max_depth_ = 4`，则最多支持深度为4的目录结构。         |

------

- [ ] 哈希表目录页面 (Hash Table Directory Page)

| 变量名             | 大小 | 作用描述                                                     | 示例                                                       |
| ------------------ | ---- | ------------------------------------------------------------ | ---------------------------------------------------------- |
| `max_depth_`       | 4    | 表示目录页面能够处理的最大深度，通常与头页面的 `max_depth_` 相同。 | 如果 `max_depth_ = 4`，则目录页面的深度不会超过4。         |
| `global_depth_`    | 4    | 当前目录的全局深度，决定了哈希表的整体分割程度。             | 初始时 `global_depth_ = 2`，表示使用2位哈希值进行索引。    |
| `local_depths_`    | 512  | 存储每个桶页面的局部深度，用于处理特定桶的分裂。             | `local_depths_[5] = 3` 表示第5个桶的局部深度为3。          |
| `bucket_page_ids_` | 2048 | 存储桶页面的页ID数组，每个页ID指向一个实际存储键值对的桶页面。 | `bucket_page_ids_[5] = 2005` 表示第5个桶页面的页ID为2005。 |

------

- [ ] 哈希表桶页面 (Hash Table Bucket Page)

| 变量名      | 大小  | 作用描述                                     | 示例                                                      |
| ----------- | ----- | -------------------------------------------- | --------------------------------------------------------- |
| `size_`     | 4     | 当前桶中存储的键值对数量。                   | `size_ = 10` 表示桶中有10对键值对。                       |
| `max_size_` | 4     | 桶能够容纳的最大键值对数量，超过时需要分裂。 | `max_size_ = 20` 表示桶最多可以存储20对键值对。           |
| `array_`    | ≤4088 | 存储实际的键值对数组，包含多个键和值。       | `array_` 中可能包含 `{key1: value1, key2: value2, ...}`。 |

------

### 示例说明

假设我们有一个扩展哈希表，其头页面的 `max_depth_` 为4，目录页面的 `global_depth_` 初始为2，并且有两个目录页面指向不同的桶页面。

1. **头页面示例**：
   - `directory_page_ids_[0] = 1001`
   - `directory_page_ids_[1] = 1002`
   - `max_depth_ = 4`
2. **目录页面示例**（目录页面ID为1001）：
   - `max_depth_ = 4`
   - `global_depth_ = 2`
   - `local_depths_[0] = 2`
   - `local_depths_[1] = 2`
   - `bucket_page_ids_[0] = 2001`
   - `bucket_page_ids_[1] = 2002`
3. **桶页面示例**（桶页面ID为2001）：
   - `size_ = 3`
   - `max_size_ = 5`
   - `array_ = { (key1, value1), (key2, value2), (key3, value3) }`

通过以上结构，当插入新的键值对时，如果某个桶页面的 `size_` 达到 `max_size_`，则需要根据 `local_depths_` 和 `global_depth_` 进行相应的分裂和目录扩展操作。





## 类

### extendible_htable_header_page.h

```cpp
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_header_page.h
//
// Identification: src/include/storage/page/extendible_htable_header_page.h
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * Header 页格式:
 *  ---------------------------------------------------
 * | DirectoryPageIds(2048) | MaxDepth (4) | Free(2044)
 *  ---------------------------------------------------
 */

#pragma once

#include <cstdlib>
#include "common/config.h"
#include "common/macros.h"

namespace bustub {

static constexpr uint64_t HTABLE_HEADER_PAGE_METADATA_SIZE = sizeof(uint32_t);  // Header 页元数据大小
static constexpr uint64_t HTABLE_HEADER_MAX_DEPTH = 9;  // Header 页支持的最大深度
static constexpr uint64_t HTABLE_HEADER_ARRAY_SIZE = 1 << HTABLE_HEADER_MAX_DEPTH;  // Header 页中目录数组的最大大小

class ExtendibleHTableHeaderPage {
 public:
  // 删除所有的构造函数和析构函数以确保内存安全
  ExtendibleHTableHeaderPage() = delete;
  DISALLOW_COPY_AND_MOVE(ExtendibleHTableHeaderPage);

  /**
   * 在从缓冲池创建新的 Header 页后，必须调用此方法以设置默认值
   * @param max_depth Header 页中的最大深度
   */
  void Init(uint32_t max_depth = HTABLE_HEADER_MAX_DEPTH);

  /**
   * 获取哈希值对应的目录索引
   *
   * @param hash 键的哈希值
   * @return 哈希值对应的目录索引
   */
  auto HashToDirectoryIndex(uint32_t hash) const -> uint32_t;

  /**
   * 获取指定索引处的目录页 ID
   *
   * @param directory_idx 目录页 ID 数组中的索引
   * @return 指定索引处的目录页 ID
   */
  auto GetDirectoryPageId(uint32_t directory_idx) const -> uint32_t;

  /**
   * @brief 设置指定索引处的目录页 ID
   *
   * @param directory_idx 目录页 ID 数组中的索引
   * @param directory_page_id 要设置的目录页 ID
   */
  void SetDirectoryPageId(uint32_t directory_idx, page_id_t directory_page_id);

  /**
   * @brief 获取 Header 页可处理的最大目录页 ID 数量
   */
  auto MaxSize() const -> uint32_t;

  /**
   * 打印 Header 页的占用信息
   */
  void PrintHeader() const;

 private:
  page_id_t directory_page_ids_[HTABLE_HEADER_ARRAY_SIZE];  // 目录页 ID 数组
  uint32_t max_depth_;  // Header 页的最大深度
};

static_assert(sizeof(page_id_t) == 4);  // 确保 page_id_t 的大小为 4 字节

static_assert(sizeof(ExtendibleHTableHeaderPage) ==
              sizeof(page_id_t) * HTABLE_HEADER_ARRAY_SIZE + HTABLE_HEADER_PAGE_METADATA_SIZE);  // 确保 Header 页的大小符合预期

static_assert(sizeof(ExtendibleHTableHeaderPage) <= BUSTUB_PAGE_SIZE);  // 确保 Header 页不会超过页面大小限制

}  // namespace bustub
```



### extendible_htable_directory_page.h

```cpp
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_directory_page.h
//
// Identification: src/include/storage/page/extendible_htable_directory_page.h
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * 目录页格式:
 *  --------------------------------------------------------------------------------------
 * | 最大深度 (4 字节) | 全局深度 (4 字节) | 本地深度 (512 字节) | 桶页 ID (2048 字节) | 空闲空间 (1528 字节)
 *  --------------------------------------------------------------------------------------
 */

#pragma once

#include <cassert>
#include <climits>
#include <cstdlib>
#include <string>

#include "common/config.h"
#include "storage/index/generic_key.h"

namespace bustub {

static constexpr uint64_t HTABLE_DIRECTORY_PAGE_METADATA_SIZE = sizeof(uint32_t) * 2;  // 目录页元数据大小

/**
 * HTABLE_DIRECTORY_ARRAY_SIZE 是可扩展哈希索引的目录页中可以容纳的页面 ID 数量。
 * 这个值为 512，因为目录数组必须按 2 的幂次增长，而 1024 个页面 ID 会导致没有空间存储其他成员变量。
 */
static constexpr uint64_t HTABLE_DIRECTORY_MAX_DEPTH = 9;  // 最大深度为 9
static constexpr uint64_t HTABLE_DIRECTORY_ARRAY_SIZE = 1 << HTABLE_DIRECTORY_MAX_DEPTH;  // 目录数组大小

/**
 * 可扩展哈希表的目录页。
 */
class ExtendibleHTableDirectoryPage {
 public:
  // 删除所有的构造函数和析构函数以确保内存安全
  ExtendibleHTableDirectoryPage() = delete;
  DISALLOW_COPY_AND_MOVE(ExtendibleHTableDirectoryPage);

  /**
   * 在从缓冲池创建新的目录页后，必须调用此方法以设置默认值。
   * @param max_depth 目录页的最大深度。
   */
  void Init(uint32_t max_depth = HTABLE_DIRECTORY_MAX_DEPTH);

  /**
   * 获取键的哈希值对应的桶索引。
   *
   * @param hash 键的哈希值。
   * @return 当前键的桶索引。
   */
  auto HashToBucketIndex(uint32_t hash) const -> uint32_t;

  /**
   * 使用目录索引查找桶页面。
   *
   * @param bucket_idx 要查找的目录索引。
   * @return 对应桶索引的页面 ID。
   */
  auto GetBucketPageId(uint32_t bucket_idx) const -> page_id_t;

  /**
   * 使用桶索引和页面 ID 更新目录索引。
   *
   * @param bucket_idx 要插入页面 ID 的目录索引。
   * @param bucket_page_id 要插入的页面 ID。
   */
  void SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id);

  /**
   * 获取一个索引的分裂镜像。
   *
   * @param bucket_idx 要查找分裂镜像的目录索引。
   * @return 分裂镜像的目录索引。
   **/
  auto GetSplitImageIndex(uint32_t bucket_idx) const -> uint32_t;

  /**
   * GetGlobalDepthMask - 返回全局深度的掩码，其中从最低有效位 (LSB) 开始是全局深度个 1，其余为 0。
   *
   * 在可扩展哈希中，我们使用以下哈希和掩码函数将键映射到目录索引：
   *
   * DirectoryIndex = Hash(key) & GLOBAL_DEPTH_MASK
   *
   * 其中 GLOBAL_DEPTH_MASK 是一个掩码，从最低有效位开始有 GLOBAL_DEPTH 个 1。例如，全局深度 3 对应于 32 位表示的 0x00000007。
   *
   * @return 全局深度的掩码（从最低有效位开始有 GLOBAL_DEPTH 个 1，其余为 0）。
   */
  auto GetGlobalDepthMask() const -> uint32_t;

  /**
   * GetLocalDepthMask - 与全局深度掩码类似，但使用位于 bucket_idx 的桶的本地深度。
   *
   * @param bucket_idx 用于查找本地深度的索引。
   * @return 本地深度的掩码（从最低有效位开始有本地深度个 1，其余为 0）。
   */
  auto GetLocalDepthMask(uint32_t bucket_idx) const -> uint32_t;

  /**
   * 获取哈希表目录的全局深度。
   *
   * @return 目录的全局深度。
   */
  auto GetGlobalDepth() const -> uint32_t;

  auto GetMaxDepth() const -> uint32_t;

  /**
   * 增加目录的全局深度。
   */
  void IncrGlobalDepth();

  /**
   * 减少目录的全局深度。
   */
  void DecrGlobalDepth();

  /**
   * @return 如果目录可以收缩，则返回 true。
   */
  auto CanShrink() -> bool;

  /**
   * @return 当前目录的大小。
   */
  auto Size() const -> uint32_t;

  /**
   * @return 目录的最大大小。
   */
  auto MaxSize() const -> uint32_t;

  /**
   * 获取位于 bucket_idx 的桶的本地深度。
   *
   * @param bucket_idx 要查找的桶索引。
   * @return 位于 bucket_idx 的桶的本地深度。
   */
  auto GetLocalDepth(uint32_t bucket_idx) const -> uint32_t;

  /**
   * 设置位于 bucket_idx 的桶的本地深度。
   *
   * @param bucket_idx 要更新的桶索引。
   * @param local_depth 新的本地深度。
   */
  void SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth);

  /**
   * 增加位于 bucket_idx 的桶的本地深度。
   * @param bucket_idx 要增加的桶索引。
   */
  void IncrLocalDepth(uint32_t bucket_idx);

  /**
   * 减少位于 bucket_idx 的桶的本地深度。
   * @param bucket_idx 要减少的桶索引。
   */
  void DecrLocalDepth(uint32_t bucket_idx);

  /**
   * VerifyIntegrity
   *
   * 验证以下不变量：
   * (1) 所有本地深度 (LD) <= 全局深度 (GD)。
   * (2) 每个桶恰好有 2^(GD - LD) 个指针指向它。
   * (3) 每个具有相同 bucket_page_id 的索引处的 LD 相同。
   */
  void VerifyIntegrity() const;

  /**
   * 打印当前目录。
   */
  void PrintDirectory() const;

 private:
  uint32_t max_depth_;  // 最大深度。
  uint32_t global_depth_;  // 全局深度。
  uint8_t local_depths_[HTABLE_DIRECTORY_ARRAY_SIZE];  // 本地深度数组。
  page_id_t bucket_page_ids_[HTABLE_DIRECTORY_ARRAY_SIZE];  // 桶页面 ID 数组。
};

static_assert(sizeof(page_id_t) == 4);  // 确保 page_id_t 的大小为 4 字节。

static_assert(sizeof(ExtendibleHTableDirectoryPage) == HTABLE_DIRECTORY_PAGE_METADATA_SIZE +
                                                           HTABLE_DIRECTORY_ARRAY_SIZE +
                                                           sizeof(page_id_t) * HTABLE_DIRECTORY_ARRAY_SIZE);  // 确保目录页大小正确。

static_assert(sizeof(ExtendibleHTableDirectoryPage) <= BUSTUB_PAGE_SIZE);  // 确保目录页不会超过页面大小限制。

}  // namespace bustub

```







# 任务 #3 - 可扩展哈希实现

您的实现需要支持插入、点搜索和删除。可扩展哈希表的头文件和 cpp 文件中已实现或记录了许多辅助函数。您唯一严格的 API 要求是遵守`Insert`、`GetValue`和`Remove`。您还必须保留该`VerifyIntegrity`函数的原样。请随意设计和实现您认为合适的其他函数。

本学期，哈希表仅支持**唯一键**。这意味着如果用户尝试插入重复键，哈希表应返回 false。

**注意：您应该使用在**[**任务 #2**](https://15445.courses.cs.cmu.edu/fall2023/project2/#hash-table-pages)中实现的页面类 来存储键值对以及维护哈希表的元数据（页面 ID、全局/本地深度）。例如，您不应使用诸如 之类的内存数据结构`std::unordered_map`来模拟哈希表。

可扩展哈希表可根据任意键、值和键比较器类型进行参数化。



`您必须通过仅修改其头文件（ src/include/container/disk/hash/disk_extendible_hash_table.cpp`）和相应的源文件（`src/container/disk/hash/disk_extendible_hash_table.cpp` ）来实现可扩展哈希表存储桶页面。



## 类

### extendible_hash_table

```cpp
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_extendible_hash_table.h
//
// Identification: src/include/container/disk/hash/extendible_hash_table.h
//
// 版权所有 (c) 2015-2023, 卡内基梅隆大学数据库组
//
//===----------------------------------------------------------------------===//

#pragma once

#include <deque>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "concurrency/transaction.h"
#include "container/hash/hash_function.h"
#include "storage/page/extendible_htable_bucket_page.h"
#include "storage/page/extendible_htable_directory_page.h"
#include "storage/page/extendible_htable_header_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

/**
 * 基于缓冲池管理器的扩展哈希表实现。支持非唯一键。支持插入和删除操作。
 * 表会随着桶的满溢或空闲而动态扩展/收缩。
 */
template <typename K, typename V, typename KC>
class DiskExtendibleHashTable {
 public:
  /**
   * @brief 创建一个新的 DiskExtendibleHashTable。
   *
   * @param name 哈希表的名称
   * @param bpm 使用的缓冲池管理器
   * @param cmp 键的比较器
   * @param hash_fn 哈希函数
   * @param header_max_depth 头页面允许的最大深度
   * @param directory_max_depth 目录页面允许的最大深度
   * @param bucket_max_size 桶页面数组允许的最大大小
   */
  explicit DiskExtendibleHashTable(const std::string &name, BufferPoolManager *bpm, const KC &cmp,
                                   const HashFunction<K> &hash_fn, uint32_t header_max_depth = HTABLE_HEADER_MAX_DEPTH,
                                   uint32_t directory_max_depth = HTABLE_DIRECTORY_MAX_DEPTH,
                                   uint32_t bucket_max_size = HTableBucketArraySize(sizeof(std::pair<K, V>)));

  /** TODO(P2): 添加实现
   * 将一个键值对插入到哈希表中。
   *
   * @param key 要插入的键
   * @param value 与键关联的值
   * @param transaction 当前事务
   * @return 如果插入成功，返回 true，否则返回 false
   */
  auto Insert(const K &key, const V &value, Transaction *transaction = nullptr) -> bool;

  /** TODO(P2): 添加实现
   * 从哈希表中移除一个键值对。
   *
   * @param key 要删除的键
   * @param transaction 当前事务
   * @return 如果删除成功，返回 true，否则返回 false
   */
  auto Remove(const K &key, Transaction *transaction = nullptr) -> bool;

  /** TODO(P2): 添加实现
   * 获取与给定键关联的值。
   *
   * 注意（fall2023）：本学期你只需要支持唯一键值对。
   *
   * @param key 要查找的键
   * @param[out] result 与给定键关联的值
   * @param transaction 当前事务
   * @return 如果找到关联的值，返回 true，否则返回 false
   */
  auto GetValue(const K &key, std::vector<V> *result, Transaction *transaction = nullptr) const -> bool;

  /**
   * 辅助函数，用于验证扩展哈希表目录的完整性。
   */
  void VerifyIntegrity() const;

  /**
   * 辅助函数，用于获取头页面的页ID。
   */
  auto GetHeaderPageId() const -> page_id_t;

  /**
   * 辅助函数，用于打印哈希表的内容。
   */
  void PrintHT() const;

 private:
  /**
   * 哈希 - 简单辅助函数，将 MurmurHash 的 64 位哈希值下转为 32 位，用于扩展哈希。
   *
   * @param key 要哈希的键
   * @return 下转后的 32 位哈希值
   */
  auto Hash(K key) const -> uint32_t;

  auto InsertToNewDirectory(ExtendibleHTableHeaderPage *header, uint32_t directory_idx, uint32_t hash, const K &key,
                            const V &value) -> bool;

  auto InsertToNewBucket(ExtendibleHTableDirectoryPage *directory, uint32_t bucket_idx, const K &key, const V &value)
      -> bool;

  void UpdateDirectoryMapping(ExtendibleHTableDirectoryPage *directory, uint32_t new_bucket_idx,
                              page_id_t new_bucket_page_id, uint32_t new_local_depth, uint32_t local_depth_mask);

  void MigrateEntries(ExtendibleHTableBucketPage<K, V, KC> *old_bucket,
                      ExtendibleHTableBucketPage<K, V, KC> *new_bucket, uint32_t new_bucket_idx,
                      uint32_t local_depth_mask);

  // 成员变量
  std::string index_name_;
  BufferPoolManager *bpm_;
  KC cmp_;
  HashFunction<K> hash_fn_;
  uint32_t header_max_depth_;
  uint32_t directory_max_depth_;
  uint32_t bucket_max_size_;
  page_id_t header_page_id_;
};

}  // namespace bustub
```

