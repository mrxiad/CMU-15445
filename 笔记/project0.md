# 任务链接

https://15445.courses.cs.cmu.edu/fall2023/project0/





# 任务 #1 - 写时复制 Trie

在此任务中，您需要修改`trie.h`并`trie.cpp`实现写时复制 trie。在写时复制 trie 中，操作不会直接修改原始 trie 的节点。相反，会为修改后的数据创建新节点，并为新修改的 trie 返回新的根。写时复制使我们能够在每次操作后随时以最小的开销访问 trie。考虑`("ad", 2)`上述示例中的插入。我们`Node2`通过重用原始树中的两个子节点并创建新的值节点 2 来创建一个新节点。



你的 trie 必须支持三种操作：

- `Get(key)`：获取key对应的value。
- `Put(key, value)`：将相应值设置为键。如果键已存在，则覆盖现有值。请注意，值的类型可能是不可复制的（即`std::unique_ptr<int>`）。此方法返回一个新的 trie。
- `Delete(key)`：删除键的值。此方法返回一个新的 trie。

这些操作都不应该直接在 trie 本身上执行。您应该创建新的 trie 节点并尽可能重复使用现有的节点。

要创建一个全新的节点（即没有子节点的新叶节点），您只需使用`TrieNodeWithValue`构造函数构造对象，然后将其变成智能指针即可。要通过写时复制创建新节点，您应该使用类`Clone`上的函数`TrieNode`。要在新的 trie 中重用现有节点，您可以复制`std::shared_ptr<TrieNode>`：复制共享指针不会复制底层数据。您不应该在本项目中使用`new`和手动分配内存。当没有人引用底层对象时，将释放对象。`delete``std::shared_ptr`

有关这些操作的完整规范，请参阅起始代码中的注释。**您的实现应像上述示例一样存储数据。**不要将 C 字符串终止符存储`\0`在您的 trie 中。

## 类

### trie.h

```cpp
namespace bustub {

/// 一个特殊类型，用于阻止移动构造函数和移动赋值操作。用于 TrieStore 的测试中。
class MoveBlocked {
 public:
  explicit MoveBlocked(std::future<int> wait) : wait_(std::move(wait)) {}

  MoveBlocked(const MoveBlocked &) = delete;
  MoveBlocked(MoveBlocked &&that) noexcept {
    if (!that.waited_) {
      that.wait_.get();
    }
    that.waited_ = waited_ = true;
  }

  auto operator=(const MoveBlocked &) -> MoveBlocked & = delete;
  auto operator=(MoveBlocked &&that) noexcept -> MoveBlocked & {
    if (!that.waited_) {
      that.wait_.get();
    }
    that.waited_ = waited_ = true;
    return *this;
  }

  bool waited_{false};
  std::future<int> wait_;
};

// TrieNode 是 Trie（前缀树）中的一个节点。
class TrieNode {
 public:
  // 创建一个没有子节点的 TrieNode。
  TrieNode() = default;

   // 创建一个带有子节点的 TrieNode。
  explicit TrieNode(std::map<char, std::shared_ptr<const TrieNode>> children) : children_(std::move(children)) {}

  virtual ~TrieNode() = default;

  // Clone 返回当前 TrieNode 的一个拷贝。如果 TrieNode 包含一个值，那么这个值也会被拷贝。返回类型是
  // std::unique_ptr 指向 TrieNode。
  //
  // 注意：不能使用拷贝构造函数来克隆节点，因为它无法确定 `TrieNode` 中是否包含一个值。
  //
  // 注：如果你想将 `unique_ptr` 转换为 `shared_ptr` ，可以使用 `std::shared_ptr<T>(std::move(ptr))`。
  virtual auto Clone() const -> std::unique_ptr<TrieNode> { return std::make_unique<TrieNode>(children_); }

  // 子节点的映射，key 是字符串中的下一个字符，value 是下一个 TrieNode。
  // 必须使用这个结构来存储子节点信息。你不被允许移除该成员变量中的 const 限定。
  std::map<char, std::shared_ptr<const TrieNode>> children_;

  // 指示该节点是否为终结节点（即是否为键的末尾）。
  bool is_value_node_{false};

   // 你可以在此添加其他字段和方法，除了存储子节点的相关字段。总的来说，为完成本项目无需额外添加字段。
};

// TrieNodeWithValue 是一个扩展的 TrieNode，它除了拥有 TrieNode 的所有功能外，还关联了一个类型为 T 的值。
template <class T>
class TrieNodeWithValue : public TrieNode {
 public:
  // 创建一个没有子节点但带有值的节点。
  explicit TrieNodeWithValue(std::shared_ptr<T> value) : value_(std::move(value)) { this->is_value_node_ = true; }

   // 创建一个带有子节点和对应值的节点。
  TrieNodeWithValue(std::map<char, std::shared_ptr<const TrieNode>> children, std::shared_ptr<T> value)
      : TrieNode(std::move(children)), value_(std::move(value)) {
    this->is_value_node_ = true;
  }

 // 重写 Clone 方法以同时克隆节点的值。
  //
  // 注：如果你想将 `unique_ptr` 转换为 `shared_ptr`，可以使用 `std::shared_ptr<T>(std::move(ptr))`。
  auto Clone() const -> std::unique_ptr<TrieNode> override {
    return std::make_unique<TrieNodeWithValue<T>>(children_, value_);
  }

  // 关联于此 Trie 节点的值。
  std::shared_ptr<T> value_;
};

// Trie 是一种数据结构，将字符串映射到类型 T 的值。所有对 Trie 的操作都不应修改 Trie 本身。它应
// 尽可能重用现有的节点，并仅为新的 Trie 构建新节点。
//
// 在本项目中，你不被允许移除任何 `const` 限定符，或者使用 `mutable` 来绕过 const 检查。
class Trie {
 private:
  // Trie 的根节点。
  std::shared_ptr<const TrieNode> root_{nullptr};

  // Create a new trie with the given root.
  explicit Trie(std::shared_ptr<const TrieNode> root) : root_(std::move(root)) {}

 public:
  // Create an empty trie.
  Trie() = default;

  template <class T>
  auto Get(std::string_view key) const -> const T *;

  template <class T>
  auto Put(std::string_view key, T value) const -> Trie;

  auto Remove(std::string_view key) const -> Trie;

 // 获取 Trie 的根节点，仅用于测试用例。
  auto GetRoot() const -> std::shared_ptr<const TrieNode> { return root_; }
};

}  // namespace bustub
```



### tire.cpp

```cpp
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// trie.cpp
//
// Identification: src/primer/trie.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/trie.h"
#include <string_view>
#include "common/exception.h"

namespace bustub {

//Get方法实现
template <class T>
auto Trie::Get(std::string_view key) const -> const T * {
  auto node = root_;
  // 按输入的key值遍历Trie树
  for (char ch : key) {
    // 如果当前节点为nullptr，或者没有对应字符的子节点，返回nullptr，程序结束
    if (node == nullptr || node->children_.find(ch) == node->children_.end()) {
      return nullptr;
    }
    // 当前字符存在，接着往后面找
    node = node->children_.at(ch);
  }
  // 路径存在，检查类型是否匹配
  // 将节点转换为TrieNodeWithValue<T>类型
  const auto *value_node = dynamic_cast<const TrieNodeWithValue<T> *>(node.get());
  // 如果转换成功并且节点包含正确类型的值，则返回该值的指针
  if (value_node != nullptr) {
    return value_node->value_.get();
  }
  // 若类型不匹配，返回nullptr
  return nullptr;
}

// Put：递归方法具体实现
template <class T>
void PutCycle(const std::shared_ptr<bustub::TrieNode> &new_root, std::string_view key, T value) {
  bool flag = false;// 判断是否找到对应节点
  for (auto &pair : new_root->children_) {
    if (key.at(0) == pair.first) {
      flag = true;
      if (key.size() > 1) {//继续递归
        std::shared_ptr<TrieNode> ptr = pair.second->Clone();
        PutCycle<T>(ptr, key.substr(1), std::move(value));
        pair.second = std::shared_ptr<const TrieNode>(ptr);
      } else {
        //插入值
        std::shared_ptr<T> val_p = std::make_shared<T>(std::move(value));
        TrieNodeWithValue node_with_val(pair.second->children_, val_p);
        pair.second = std::make_shared<const TrieNodeWithValue<T>>(node_with_val);
      }
      return;
    }
  }
  if (!flag) {// 没找到，则新建一个子节点
    char c = key.at(0);
    if (key.size() == 1) {
      // 直接插入children
      std::shared_ptr<T> val_p = std::make_shared<T>(std::move(value));
      new_root->children_.insert({c, std::make_shared<const TrieNodeWithValue<T>>(val_p)});
    } else {
      // 创建一个空的children节点
      auto ptr = std::make_shared<TrieNode>();
      // 递归
      PutCycle<T>(ptr, key.substr(1), std::move(value));
      // 插入
      new_root->children_.insert({c, std::move(ptr)});
    }
  }
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  if (key.empty()) {
    std::shared_ptr<T> val_p = std::make_shared<T>(std::move(value));
    // 建立新根
    std::unique_ptr<TrieNodeWithValue<T>> new_root = nullptr;
    // 如果原根节点无子节点
    if (root_->children_.empty()) {
      new_root = std::make_unique<TrieNodeWithValue<T>>(std::move(val_p));
    } else {
      new_root = std::make_unique<TrieNodeWithValue<T>>(root_->children_, std::move(val_p));
    }
    // 返回新的Trie
    return Trie(std::move(new_root));
  }
  // 第二种情况：值不放在根节点中
  // 2.1 根节点如果为空，新建一个空的TrieNode;
  // 2.2 如果不为空，调用clone方法复制根节点
  std::shared_ptr<TrieNode> new_root = nullptr;
  if (root_ == nullptr) {
    new_root = std::make_unique<TrieNode>();
  } else {
    new_root = root_->Clone();
  }
  PutCycle<T>(new_root, key, std::move(value));
  return Trie(std::move(new_root));
}


//remove 递归移除方法
bool RemoveCycle(const std::shared_ptr<TrieNode> &new_roottry, std::string_view key) {
  // 在new_root的children找key的第一个元素
  for (auto &pair : new_roottry->children_) {
    // 继续找
    if (key.at(0) != pair.first) {
      continue;
    }
    if (key.size() == 1) {
      // 是键结尾
      if (!pair.second->is_value_node_) {// 如果不是value节点，返回false
        return false;
      }
      // 如果子节点为空，直接删除
      if (pair.second->children_.empty()) {
        new_roottry->children_.erase(pair.first);
      } else {
        // 否则转为TireNode
        pair.second = std::make_shared<const TrieNode>(pair.second->children_);
      }
      return true;
    }
    // 拷贝一份当前节点
    std::shared_ptr<TrieNode> ptr = pair.second->Clone();
    // 递归删除
    bool flag = RemoveCycle(ptr, key.substr(1, key.size() - 1));
    // 如果没有可删除的键
    if (!flag) {
      return false;
    }
    // 如果删除后当前节点无value且子节点为空，则删除
    if (ptr->children_.empty() && !ptr->is_value_node_) {
      new_roottry->children_.erase(pair.first);
    } else {
      // 否则将删除的子树覆盖原来的子树
      pair.second = std::shared_ptr<const TrieNode>(ptr);
    }
    return true;
  }
  return false;
}

auto Trie::Remove(std::string_view key) const -> Trie {
  if (this->root_ == nullptr) {
    return *this;
  }
  // 键为空
  if (key.empty()) {
    if (root_->is_value_node_) {// 根节点有value
      // 根节点无子节点
      if (root_->children_.empty()) {
        // 直接返回一个空的trie
        return Trie(nullptr);
      }
      // 根节点有子节点,把子结点转给新根
      std::shared_ptr<TrieNode> new_root = std::make_shared<TrieNode>(root_->children_);
      return Trie(new_root);
    }
    return *this;
  }
  // 创建一个当前根节点的副本作为新的根节点
  std::shared_ptr<TrieNode> newroot = root_->Clone();
  // 递归删除
  bool flag = RemoveCycle(newroot, key);
  if (!flag) {
    return *this;
  }
  if (newroot->children_.empty() && !newroot->is_value_node_) {
    newroot = nullptr;
  }
  return Trie(std::move(newroot));

  // You should walk through the trie and remove nodes if necessary. If the node doesn't contain a value any more,
  // you should convert it to `TrieNode`. If a node doesn't have children any more, you should remove it.
}



// Below are explicit instantiation of template functions.
//
// Generally people would write the implementation of template classes and functions in the header file. However, we
// separate the implementation into a .cpp file to make things clearer. In order to make the compiler know the
// implementation of the template functions, we need to explicitly instantiate them here, so that they can be picked up
// by the linker.

template auto Trie::Put(std::string_view key, uint32_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint32_t *;

template auto Trie::Put(std::string_view key, uint64_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint64_t *;

template auto Trie::Put(std::string_view key, std::string value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const std::string *;

// If your solution cannot compile for non-copy tests, you can remove the below lines to get partial score.

using Integer = std::unique_ptr<uint32_t>;

template auto Trie::Put(std::string_view key, Integer value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const Integer *;

template auto Trie::Put(std::string_view key, MoveBlocked value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const MoveBlocked *;

}  // namespace bustub

```



## 三个类的说明

1. **TrieNode**

   - **作用**：这是 trie 的基本节点，用来存储子节点的映射以及指示当前节点是否是“值节点”（即是否表示一个键的结束）。
   - **使用时机**：当创建内部节点或叶子节点时，如果该节点**不**需要存储具体的值，则使用 `TrieNode`。在部分操作中，比如删除后变成普通节点或者在递归复制节点的时候，都会使用 `TrieNode`。

2. **TrieNodeWithValue**

   - **作用**：这是对 `TrieNode` 的扩展，除了包含子节点信息、是否为值节点等信息之外，还存储具体的值。
   - **使用时机**：当某个节点表示一个完整键时，即该节点存储了键对应的值，就需要用 `TrieNodeWithValue`。例如在 `Put` 操作中新插入一个键值对，或者更新现有键时，都需要创建一个新的 `TrieNodeWithValue` 节点。

3. **Trie**

   - 作用

     ：Trie 类封装了整个 trie 树的操作，提供了三个操作接口：

     - `Get(key)`：查询键对应的值
     - `Put(key, value)`：插入或更新一个键的值
     - `Remove(key)`：删除一个键

   - **使用时机**：用户只需使用 Trie 提供的接口来进行查找、插入、删除操作。由于 Trie 实现了写时复制（即持久化数据结构），每一次操作都会返回一个新的 Trie，原 Trie 保持不变。



## 测试命令

```bash
cd build
make trie_test trie_store_test -j$(nproc)
./test/trie_test

#测试某个具体的case
 ./test/trie_test --gtest_filter=TrieTest.RemoveFreeTest
```





# 任务 #2 - 并发键值存储

拥有可以在单线程环境中使用的写时复制 trie 后，为多线程环境实现并发键值存储。在此任务中，您需要修改`trie_store.h`和`trie_store.cpp`。此键值存储还支持 3 个操作：

- `Get(key)`返回值。
- `Put(key, value)`. 无返回值。
- `Delete(key)`. 无返回值。

对于原始的 Trie 类，每次我们修改 trie 时，我们都需要获取新的根来访问新内容。但对于并发键值存储，`put`和`delete`方法没有返回值。这要求您使用并发原语来同步读取和写入，以便在此过程中不会丢失任何数据。

您的并发键值存储应该同时为多个读取器*和*单个写入器提供服务。也就是说，当有人修改 trie 时，仍可以在旧根上执行读取。当有人正在读取时，仍可以在不等待读取的情况下执行写入。

1. **操作接口不同：**

   - **Get(key)**：返回一个值（具体来看，我们需要返回一个 `ValueGuard` 对象）。
   - **Put(key, value)**：无返回值（写操作）。
   - **Delete(key)**：无返回值（写操作）。

2. **并发要求：**

   - **多个读取器（Readers）可以同时访问存储中的值。**
     换句话说，当有人在修改 trie 时，不会阻塞其他读取操作。读取者总是能够在某个“稳定版本”（旧根）上进行查询。
   - **写操作（Put 和 Delete）允许并发，但同时只允许一个写入者执行操作。**
     因为写操作需要构造新的 trie，并更新全局的数据结构，所以必须使用互斥（mutex）或其它同步原语确保同时只进行一个写入操作。
   - **写者不应该干扰正在进行的读取。**
     当一个线程执行写入时，其他线程依然可以在旧版本的 trie 上进行查询。写入后，会更新一个全局指针，从而让新来的查询操作获取新版本。

3. **数据一致性与引用安全：**

   - 由于多线程环境下，写操作不会返回新的根来让调用者使用，而是内部更新全局数据。

   - 如果我们直接返回 trie 节点中存储的指针，那么一旦写操作修改了 trie 并释放了一部分节点，读取者获取的指针就会**悬空**。

   - 为了解决这个问题，引入了 `ValueGuard`。

     > **ValueGuard**：封装了指向具体值的指针以及与该值所在 trie 节点有关的共享指针（例如指向对应 trie 节点的 `shared_ptr`）。
     > 这样，只要 `ValueGuard` 存在，对应的 trie 节点仍然会被引用计数管理，不会被释放，从而保证了对值的持续访问。



## 类

### tire_store.h

```cpp
#pragma once

#include <optional>
#include <shared_mutex>
#include <utility>
#include <mutex>

#include "primer/trie.h"

namespace bustub {

/**
 * @brief ValueGuard 模板类用于保护从 Trie 中获取到的值。
 *
 * 该类封装了：
 *  - 一个 Trie 对象（对应于值所在的 Trie 版本），保证了只要 ValueGuard 存在，
 *    对应版本的节点不会因更新操作而被析构，确保对值的引用不失效。
 *  - 对值本身的引用。
 *
 * 使用时，读取操作在获取值时返回 ValueGuard<T>，从而确保即使全局 Trie 更新，
 * 对应值依然有效。
 *
 * @tparam T 存储在 Trie 中的值的类型
 */
template <class T>
class ValueGuard {
 public:
  /**
   * @brief 构造一个 ValueGuard 对象
   * @param root 当前 Trie 的一个快照（包含值所在的节点）
   * @param value 对值的引用
   */
  ValueGuard(Trie root, const T &value) : root_(std::move(root)), value_(value) {}

  /**
   * @brief 重载解引用运算符，用于访问被保护的值。
   * @return 对值的常量引用
   */
  auto operator*() const -> const T & { return value_; }

 private:
  // 持有 Trie 的一个快照，保证值所在的 Trie 节点引用计数不减，从而防止值悬空
  Trie root_;
  // 对值本身的引用
  const T &value_;
};

/**
 * @brief TrieStore 是对写时复制 Trie 的线程安全包装类。
 *
 * 该类提供简单的接口用于访问 Trie，并支持多线程环境：
 *  - 允许多个线程同时执行读操作（Get）
 *  - 允许单个线程执行写操作（Put 和 Remove），写操作不会阻塞读操作。
 *
 * 内部通过两个互斥锁来保证并发安全：
 * 1. root_lock_：保护 Trie 根（root_）的访问或修改。每次读写全局 Trie 时需要加锁。
 * 2. write_lock_：保证每次只允许一个写操作同时执行。写操作中创建新的 Trie 版本后，
 *    更新全局 Trie 的过程需要加写锁。
 */
class TrieStore {
 public:
  /**
   * @brief 根据给定 key 获取对应的值。返回值封装在 ValueGuard 中，该对象持有 Trie 根的引用，
   *        以确保对值的引用在 Trie 更新时不失效。
   *
   * @tparam T 值的类型
   * @param key 要查询的键
   * @return 如果键存在则返回包含值的 ValueGuard<T>，否则返回 std::nullopt
   */
  template <class T>
  auto Get(std::string_view key) -> std::optional<ValueGuard<T>> {
    // 这里先加锁保护全局 Trie 根的访问
    std::lock_guard<std::mutex> guard(root_lock_);
    // 使用当前 Trie 根执行查询
    const T *value = root_.Get<T>(key);
    if (value == nullptr) {
      return std::nullopt;
    }
    // 返回 ValueGuard，构造时传入当前 Trie 的一个快照和查询到的值的引用
    return ValueGuard<T>(root_, *value);
  }

  /**
   * @brief 将键值对插入 Trie 中。如果该键已存在，则覆盖原有值。
   *        写操作不返回新的 Trie 版本，而是在内部更新全局 Trie 根。
   *
   * @tparam T 值的类型
   * @param key 要插入或更新的键
   * @param value 要存储的值
   */
  template <class T>
  void Put(std::string_view key, T value) {
    // 写操作需要独占地进行，因此先获取 write_lock_ 进行串行化写操作
    std::lock_guard<std::mutex> write_guard(write_lock_);
    // 获取 root_lock_ 来操作全局 Trie 根，并更新为新的 Trie 版本
    std::lock_guard<std::mutex> root_guard(root_lock_);
    // 调用 Trie::Put 方法产生新的 Trie 版本，并更新全局根
    root_ = root_.Put<T>(key, std::move(value));
  }

  /**
   * @brief 从 Trie 中删除指定的键。删除操作不返回任何值。
   *
   * 删除操作同样必须序列化，因为需要生成新的 Trie 版本并更新全局 Trie 根。
   *
   * @param key 要删除的键
   */
  void Remove(std::string_view key) {
    // 写操作序列化
    std::lock_guard<std::mutex> write_guard(write_lock_);
    // 更新全局 Trie 根
    std::lock_guard<std::mutex> root_guard(root_lock_);
    root_ = root_.Remove(key);
  }

 private:
  // 保护全局 Trie 根的互斥锁，每次访问或修改 root_ 都需要持有该锁
  std::mutex root_lock_;

  // 用于序列化写操作的互斥锁，一次仅允许一个写操作进行
  std::mutex write_lock_;

  // 存储当前 Trie 的根。由于 Trie 是写时复制的，每次写操作都会产生一个新的 Trie 版本。
  Trie root_;
};

}  // namespace bustub
```

### tire_store.cpp

```cpp
#include "primer/trie_store.h"
#include "common/exception.h"

namespace bustub {

template <class T>
auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<T>> {
  // Pseudo-code:
  // (1) Take the root lock, get the root, and release the root lock. Don't lookup the value in the
  //     trie while holding the root lock.
  // (2) Lookup the value in the trie.
  // (3) If the value is found, return a ValueGuard object that holds a reference to the value and the
  //     root. Otherwise, return std::nullopt.
  auto trie = Trie();
  {
    std::lock_guard<std::mutex> lock(root_lock_);
    trie = root_;
  }
  const T *value = trie.Get<T>(key);
  if (value) {
    return ValueGuard<T>(trie, *value);
  }
  return std::nullopt;
}

template <class T>
void TrieStore::Put(std::string_view key, T value) {
  // You will need to ensure there is only one writer at a time. Think of how you can achieve this.
  // The logic should be somehow similar to `TrieStore::Get`.

  std::lock_guard<std::mutex> lock(write_lock_);
  Trie new_trie = root_.Put<T>(key, std::move(value));
  {
    std::lock_guard<std::mutex> lock(root_lock_);
    root_ = new_trie;
  }  
}

/** @brief This function will remove the key-value pair from the trie. */
void TrieStore::Remove(std::string_view key) {
  // You will need to ensure there is only one writer at a time. Think of how you can achieve this.
  // The logic should be somehow similar to `TrieStore::Get`.

  std::lock_guard<std::mutex> lock(write_lock_);
  Trie new_trie = root_.Remove(key);
  {
    std::lock_guard<std::mutex> lock(root_lock_);
    root_ = new_trie;
  }
}

// Below are explicit instantiation of template functions.

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<uint32_t>>;
template void TrieStore::Put(std::string_view key, uint32_t value);

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<std::string>>;
template void TrieStore::Put(std::string_view key, std::string value);

// If your solution cannot compile for non-copy tests, you can remove the below lines to get partial score.

using Integer = std::unique_ptr<uint32_t>;

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<Integer>>;
template void TrieStore::Put(std::string_view key, Integer value);

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<MoveBlocked>>;
template void TrieStore::Put(std::string_view key, MoveBlocked value);

}  // namespace bustub
```

## 测试命令

```bash
cd build
make trie_store_test -j$(nproc)
./test/trie_store_test
```



# 任务 #3 - 调试

在这个任务中，你将学习调试的基本技巧。你可以选择任何你喜欢的方式进行调试，包括但不限于：使用`cout`和`printf`、使用 CLion / VSCode 调试器、使用 gdb 等。

请参阅 了解`trie_debug_test.cpp`说明。您需要在生成 trie 结构后设置断点并回答几个问题。您需要在 中填写答案`trie_answer.h`。



## 类

### trie_debug_test.cpp

```cpp
#include <fmt/format.h>
#include <zipfian_int_distribution.h>
#include <bitset>
#include <functional>
#include <numeric>
#include <optional>
#include <random>
#include <thread>  // NOLINT

#include "common/exception.h"
#include "gtest/gtest.h"
#include "primer/trie.h"
#include "primer/trie_answer.h"
#include "trie_debug_answer.h"  // NOLINT

namespace bustub {

TEST(TrieDebugger, TestCase) {
  // 使用种子 23333 初始化一个 64 位随机数生成器
  std::mt19937_64 gen(23333);
  // 创建一个 Zipf 分布，范围为 [0, 1000]
  zipfian_int_distribution<uint32_t> dis(0, 1000);

  // 创建一个空的 Trie 对象
  auto trie = Trie();
  // 循环插入 100 个键值对到 Trie 中
  for (uint32_t i = 0; i < 100; i++) {
    // 格式化生成的随机数作为 key（转换为字符串）
    std::string key = fmt::format("{}", dis(gen));
    // 生成一个随机值
    auto value = dis(gen);
    // 根据循环的迭代次数，针对前3个插入的值进行断言检验
    switch (i) {
      // 测试随机数生成器前3次生成的值是否符合预期
      case 0:
        ASSERT_EQ(value, 128) << "随机数生成器在不同平台可能有差异，请在 Piazza 上发帖寻求帮助。";
        break;
      case 1:
        ASSERT_EQ(value, 16) << "随机数生成器在不同平台可能有差异，请在 Piazza 上发帖寻求帮助。";
        break;
      case 2:
        ASSERT_EQ(value, 41) << "随机数生成器在不同平台可能有差异，请在 Piazza 上发帖寻求帮助。";
        break;
      default:
        break;
    }
    // 更新 Trie：写时复制，每次 Put 操作返回新的 Trie 版本
    trie = trie.Put<uint32_t>(key, value);
  }

  // 在此处设置断点，便于调试观察 Trie 内部结构

  // (1) 根节点上有多少个子节点？
  // 请将 trie_answer.h 中的 `CASE_1_YOUR_ANSWER` 替换为正确答案。
  if (CASE_1_YOUR_ANSWER != Case1CorrectAnswer()) {
    ASSERT_TRUE(false) << "case 1 不正确";
  }

  // (2) 以前缀 '9' 为根的节点上有多少个子节点？
  // 请将 trie_answer.h 中的 `CASE_2_YOUR_ANSWER` 替换为正确答案。
  if (CASE_2_YOUR_ANSWER != Case2CorrectAnswer()) {
    ASSERT_TRUE(false) << "case 2 不正确";
  }

  // (3) 键 "969" 对应的值是多少？
  // 请将 trie_answer.h 中的 `CASE_3_YOUR_ANSWER` 替换为正确答案。
  if (CASE_3_YOUR_ANSWER != Case3CorrectAnswer()) {
    ASSERT_TRUE(false) << "case 3 不正确";
  }
}

}  // namespace bustub
```

### trie_debug_answer.h

```cpp
#include "common/exception.h"

/**
 * @brief 返回问题 (1) 中根节点的子节点数量的正确答案。
 *
 * 当你提交到 Gradescope 后，可以知道自己的答案是否正确，
 * 如果不正确请检查你的 Trie 实现或者重新理解题目要求。
 */
auto Case1CorrectAnswer() -> int {
  throw bustub::NotImplementedException(
      "正确答案已隐藏。提交到 Gradescope 后可查看你的答案是否正确。");
}

/**
 * @brief 返回问题 (2) 中，对于前缀 '9' 的节点，子节点的数量的正确答案。
 */
auto Case2CorrectAnswer() -> int {
  throw bustub::NotImplementedException(
      "正确答案已隐藏。提交到 Gradescope 后可查看你的答案是否正确。");
}

/**
 * @brief 返回问题 (3) 中，键 "969" 对应的正确值。
 */
auto Case3CorrectAnswer() -> int {
  throw bustub::NotImplementedException(
      "正确答案已隐藏。提交到 Gradescope 后可查看你的答案是否正确。");
}
```



## vscode配置

### launch.json

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "lldb",
            "request": "launch",
            "name": "trie_test",
            "program": "${workspaceFolder}/build/test/trie_test",
            "args": [],
            "cwd": "${workspaceFolder}",
            "preLaunchTask": "build trie_test"
        },
        {
            "type": "lldb",
            "request": "launch",
            "name": "trie_debug_test",
            "program": "${workspaceFolder}/build/test/trie_debug_test",
            "args": [],
            "cwd": "${workspaceFolder}",
            "preLaunchTask": "build trie_debug_test"
        },
    ]
}
```



### settings.json

```json
{
    "files.associations": {
        "functional": "cpp",
        "bitset": "cpp",
        "string": "cpp",
        "any": "cpp",
        "array": "cpp",
        "atomic": "cpp",
        "*.tcc": "cpp",
        "cctype": "cpp",
        "chrono": "cpp",
        "clocale": "cpp",
        "cmath": "cpp",
        "condition_variable": "cpp",
        "cstdarg": "cpp",
        "cstddef": "cpp",
        "cstdint": "cpp",
        "cstdio": "cpp",
        "cstdlib": "cpp",
        "cstring": "cpp",
        "ctime": "cpp",
        "cwchar": "cpp",
        "cwctype": "cpp",
        "deque": "cpp",
        "list": "cpp",
        "unordered_map": "cpp",
        "unordered_set": "cpp",
        "vector": "cpp",
        "exception": "cpp",
        "algorithm": "cpp",
        "iterator": "cpp",
        "map": "cpp",
        "memory": "cpp",
        "memory_resource": "cpp",
        "numeric": "cpp",
        "optional": "cpp",
        "random": "cpp",
        "ratio": "cpp",
        "set": "cpp",
        "string_view": "cpp",
        "system_error": "cpp",
        "tuple": "cpp",
        "type_traits": "cpp",
        "utility": "cpp",
        "fstream": "cpp",
        "future": "cpp",
        "initializer_list": "cpp",
        "iomanip": "cpp",
        "iosfwd": "cpp",
        "iostream": "cpp",
        "istream": "cpp",
        "limits": "cpp",
        "mutex": "cpp",
        "new": "cpp",
        "ostream": "cpp",
        "shared_mutex": "cpp",
        "sstream": "cpp",
        "stdexcept": "cpp",
        "streambuf": "cpp",
        "thread": "cpp",
        "cinttypes": "cpp",
        "typeindex": "cpp",
        "typeinfo": "cpp",
        "variant": "cpp",
        "bit": "cpp"
    },
    "C_Cpp.default.compilerPath": "/bin/clang-14",
    "cmake.debugConfig": {
        "type": "lldb",
        "request": "launch",
        "name": "trie_test",
        "program": "${workspaceFolder}/build/test/trie_test",
        "args": [],
        "cwd": "${workspaceFolder}"
    },
    "cmake.options.statusBarVisibility": "visible",
    "makefile.makefilePath": "/home/mrxiad/workspace/CMU-15445/build",
}
```

### tasks.json

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build trie_test",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--build",
                "${workspaceFolder}/build",
                "--target",
                "trie_test"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": []
        },
        {
            "label": "build trie_debug_test",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--build",
                "${workspaceFolder}/build",
                "--target",
                "trie_debug_test"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": []
        }
    ]
}
```

### c_cpp_properties.json

```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "/usr/include/**"
            ],
            "defines": [],
            "cStandard": "c17",
            "cppStandard": "c++14",
            "intelliSenseMode": "linux-clang-x64",
            "compilerPath": ""
        }
    ],
    "version": 4
}
```

直接debug即可





# 任务 #4 - SQL 字符串函数

现在是时候深入研究 BusTub 本身了！您需要实现`upper`和`lower`SQL 函数。这可以通过 2 个步骤完成：(1) 在 中实现函数逻辑`string_expression.h`。(2) 在 BusTub 中注册该函数，以便 SQL 框架可以在用户执行 SQL 时调用您的函数（在 中`plan_func_call.cpp`）。



## 类【todo】





## 客户端shell

```shell
cd build
make -j`nproc` shell
./bin/bustub-shell
bustub> select upper('AbCd'), lower('AbCd');
ABCD abcd
```





## 测试命令

```bash
cd build
make -j`nproc` sqllogictest
./bin/bustub-sqllogictest ../test/sql/p0.01-lower-upper.slt --verbose
./bin/bustub-sqllogictest ../test/sql/p0.02-function-error.slt --verbose
./bin/bustub-sqllogictest ../test/sql/p0.03-string-scan.slt --verbose
```



# 格式化【todo】