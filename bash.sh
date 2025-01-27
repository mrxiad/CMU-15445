#!/bin/bash

#baby_arithmetic.slt    order_by.slt              p3.01-seqscan.slt     p3.06-empty-table.slt  p3.11-multi-way-join.slt       p3.16-sort-limit.slt       p3.leaderboard-q1-window.slt  update.slt
# hash_join.slt          p0.01-lower-upper.slt     p3.02-insert.slt      p3.07-simple-agg.slt   p3.12-repeat-execute.slt       p3.17-topn.slt             p3.leaderboard-q1.slt         vector.slt
# index.slt              p0.02-function-error.slt  p3.03-update.slt      p3.08-group-agg-1.slt  p3.13-nested-index-join.slt    p3.18-integration-1.slt    p3.leaderboard-q2.slt
# intro.slt              p0.03-string-scan.slt     p3.04-delete.slt      p3.09-group-agg-2.slt  p3.14-hash-join.slt            p3.19-integration-2.slt    p3.leaderboard-q3.slt
# nested_index_join.slt  p3.00-primer.slt          p3.05-index-scan.slt  p3.10-simple-join.slt  p3.15-multi-way-hash-join.slt  p3.20-window-function.slt  subquery.slt

# 获取测试文件路径
TEST_FILE=../test/sql/p3.00-primer.slt
TEST_FILE=../test/sql/p3.01-seqscan.slt
TEST_FILE=../test/sql/p3.02-insert.slt
TEST_FILE=../test/sql/p3.03-update.slt
TEST_FILE=../test/sql/p3.04-delete.slt
TEST_FILE=../test/sql/p3.05-index-scan.slt
TEST_FILE=../test/sql/p3.06-empty-table.slt
TEST_FILE=../test/sql/p3.07-simple-agg.slt
TEST_FILE=../test/sql/p3.08-group-agg-1.slt
TEST_FILE=../test/sql/p3.09-group-agg-2.slt
TEST_FILE=../test/sql/p3.10-simple-join.slt
TEST_FILE=../test/sql/p3.11-multi-way-join.slt
TEST_FILE=../test/sql/p3.12-repeat-execute.slt
TEST_FILE=../test/sql/p3.13-nested-index-join.slt



# 切换到 build 目录
echo "Switching to build directory..."
cd build || { echo "Failed to switch to build directory. Make sure the directory exists."; exit 1; }

# 编译 sqllogictest
echo "Compiling SQLLogicTest..."
make -j$(nproc) sqllogictest || { echo "Failed to compile sqllogictest. Check for errors."; exit 1; }

# 执行测试
echo "Running test: $TEST_FILE"
./bin/bustub-sqllogictest "$TEST_FILE" --verbose || { echo "Test failed. Check for errors."; exit 1; }

echo "Test completed successfully."

