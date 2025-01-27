//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.cpp
//
// Identification: src/execution/delete_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/delete_executor.h"

namespace bustub {

DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void DeleteExecutor::Init() {
  // 初始化子执行器
  child_executor_->Init();
  // 初始化删除标志位
  has_deleted_ = false;
}

auto DeleteExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  // 如果已经删除过所有元组，直接返回 false
  if (has_deleted_) {
    return false;
  }

  // 标记为已删除
  has_deleted_ = true;

  int deleted_count = 0;  // 记录删除的元组数量
  Tuple child_tuple{};
  RID child_rid{};

  // 获取表信息
  auto table_info = exec_ctx_->GetCatalog()->GetTable(plan_->GetTableOid());
  // 获取表的所有索引
  auto indexes = exec_ctx_->GetCatalog()->GetTableIndexes(table_info->name_);

  // 遍历子执行器获取的元组
  while (child_executor_->Next(&child_tuple, &child_rid)) {
    // 标记元组为已删除
    table_info->table_->UpdateTupleMeta({0, true}, child_rid);
    deleted_count++;

    // 更新索引：删除旧元组的索引条目
    for (auto &index_info : indexes) {
      auto index = index_info->index_.get();
      auto key_schema = index->GetKeySchema();
      auto key_attrs = index->GetKeyAttrs();
      auto key = child_tuple.KeyFromTuple(table_info->schema_, *key_schema, key_attrs);
      index->DeleteEntry(key, child_rid, exec_ctx_->GetTransaction());
    }
  }

  // 返回删除的元组数量
  std::vector<Value> result = {{TypeId::INTEGER, deleted_count}};
  *tuple = Tuple(result, &GetOutputSchema());

  return true;
}

}  // namespace bustub
