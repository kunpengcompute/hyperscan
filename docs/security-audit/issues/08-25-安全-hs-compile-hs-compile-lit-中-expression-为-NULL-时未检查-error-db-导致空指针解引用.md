# 【安全】hs_compile/hs_compile_lit 中 expression 为 NULL 时未检查 error/db 导致空指针解引用

- Remote: https://gitcode.com/boostkit/hyperscan/issues/25
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P0 高危

## 问题描述
`hs_compile` 和 `hs_compile_lit` 在 early return 路径中直接对 `*db` 和 `*error` 解引用，未检查这两个指针是否为 NULL：

`hs_compile` (hs.cpp:451-460)：
```cpp
hs_error_t HS_CDECL hs_compile(const char *expression, ... hs_compile_error_t **error) {
    if (expression == nullptr) {
        *db = nullptr;       // 若 db 为 NULL，崩溃
        *error = generateCompileError(...);  // 若 error 为 NULL，崩溃
        return HS_COMPILER_ERROR;
    }
```

对比 `hs_compile_multi_int` 正确地在开头检查了 `!comp_error` 和 `!db`。同样的 bug 也存在于 `hs_compile_lit` (hs.cpp:499-503)。

## 触发条件
用户调用 `hs_compile(NULL, 0, 0, NULL, NULL, NULL)` 直接段错误。

## 修复建议
```cpp
if (expression == nullptr) {
    if (db) {
        *db = nullptr;
    }
    if (error) {
        *error = generateCompileError("Invalid parameter: expression is NULL", -1);
    }
    return HS_COMPILER_ERROR;
}
```
