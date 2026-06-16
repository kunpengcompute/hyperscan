# 【代码质量】hs_free_database 对 NULL db 调用自定义 free 存在风险

- Remote: https://gitcode.com/boostkit/hyperscan/issues/36
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P2 代码质量

## 问题描述
`hs_free_database` (database.c:52-58) 在 db 为 NULL 时不提前返回，而是调用 `hs_database_free(NULL)`：

```cpp
HS_PUBLIC_API
hs_error_t HS_CDECL hs_free_database(hs_database_t *db) {
    if (db && db->magic != HS_DB_MAGIC) {
        return HS_INVALID;
    }
    hs_database_free(db);  // db==NULL 时调用 free(NULL) 或自定义 free(NULL)
    return HS_SUCCESS;
}
```

`free(NULL)` 在 C 标准中是安全的无操作，但 Hyperscan 允许用户注册自定义 allocator（`hs_set_database_allocator`）。如果用户注册的自定义 free 函数不处理 NULL 输入，将导致未定义行为。

对比 `hs_free_scratch`（scratch.c:443-463）和 `hs_stream_free` 等其他 API 函数，它们都正确处理了 NULL 输入。

## 修复建议
```cpp
if (!db) {
    return HS_SUCCESS;  // 或 HS_INVALID
}
```
