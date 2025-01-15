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

// Get方法实现
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
  bool flag = false;  // 判断是否找到对应节点
  for (auto &pair : new_root->children_) {
    if (key.at(0) == pair.first) {
      flag = true;
      if (key.size() > 1) {  //继续递归
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
  if (!flag) {  // 没找到，则新建一个子节点
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

// remove 递归移除方法
bool RemoveCycle(const std::shared_ptr<TrieNode> &new_roottry, std::string_view key) {
  // 在new_root的children找key的第一个元素
  for (auto &pair : new_roottry->children_) {
    // 继续找
    if (key.at(0) != pair.first) {
      continue;
    }
    if (key.size() == 1) {
      // 是键结尾
      if (!pair.second->is_value_node_) {  // 如果不是value节点，返回false
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
    if (root_->is_value_node_) {  // 根节点有value
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
