# 【代码质量】initDB 建表失败路径未 finalize sqlite3_stmt

- Remote: https://gitcode.com/boostkit/hyperscan/issues/45
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 问题

tools/hsbench/sqldb.cpp 的 initDB() 在循环执行多条 CREATE TABLE 语句时，如果 sqlite3_step() 或 sqlite3_finalize() 前后的错误路径跳转到 fail，当前 statement 可能没有被 sqlite3_finalize() 释放。

相关位置：

- tools/hsbench/sqldb.cpp:94-110：sqlite3_prepare 成功后，sqlite3_step 失败会直接 goto fail。
- tools/hsbench/sqldb.cpp:115-120：fail 只关闭 db，没有 finalize 当前 statement。

## 影响

这是 hsbench 工具的错误路径资源泄漏。建表 SQL 当前是静态字符串，正常情况下不触发；但当数据库损坏、schema 冲突、磁盘/权限异常或 SQLite 返回中途错误时，statement 资源可能泄漏，并且 sqlite3_close 可能因为未释放 statement 返回 SQLITE_BUSY。当前代码在 fail 路径 assert(status == SQLITE_OK)，release 下会忽略关闭失败。

## 建议

初始化 sqlite3_stmt *statement = nullptr；所有 fail 路径在 sqlite3_close 前先 if (statement) sqlite3_finalize(statement)。同时避免在错误路径依赖 assert 检查 sqlite3_close 返回值。
