# 【安全】hs_serialized_database_info 从截断的序列化数据中越界读取 RoseEngine::mode

- Remote: https://gitcode.com/boostkit/hyperscan/issues/26
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P1 中危

## 问题描述
`hs_serialized_database_info` (database.c:426-443) 在未校验 bytecode 大小的情况下直接读取 `RoseEngine::mode`：

```cpp
hs_error_t ret = db_decode_header(&bytes, length, &header);  // 验证了 header
...
u32 mode = unaligned_load_u32(bytes + offsetof(struct RoseEngine, mode));
```

`db_decode_header` 只验证了 `header.length` 与传入的 `length` 匹配（`sizeof(struct hs_database) + header.length`），但不验证 `header.length >= sizeof(struct RoseEngine)`。如果传入截断的数据库数据（例如合法的 header 但 bytecode 极短），此处越界读取。

## 修复建议
在读取 `RoseEngine::mode` 前增加长度校验：
```cpp
if (header.length < sizeof(struct RoseEngine)) {
    return HS_INVALID;
}
```
