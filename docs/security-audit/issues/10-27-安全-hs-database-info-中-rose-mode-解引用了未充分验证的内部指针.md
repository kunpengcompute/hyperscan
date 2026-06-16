# 【安全】hs_database_info 中 rose->mode 解引用了未充分验证的内部指针

- Remote: https://gitcode.com/boostkit/hyperscan/issues/27
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P1 中危

## 问题描述
`hs_database_info` (database.c:446-462) 在通过 magic 和 alignment 验证后，通过 `hs_get_bytecode(db)` 获取 `rose` 指针并访问 `rose->mode`：

```cpp
if (!db || !db_correctly_aligned(db) || db->magic != HS_DB_MAGIC) {
    return HS_INVALID;
}
...
const struct RoseEngine *rose = hs_get_bytecode(db);
return print_database_string(info, db->version, plat, rose->mode);  // 未校验 rose 有效性
```

`hs_get_bytecode` 的返回值依赖于 `db->bytecode` 偏移量，该值来自反序列化时的 `db_copy_bytecode`。若调用者构造一个 magic 正确但 `bytecode` 偏移量指向无效地址的伪造 `hs_database_t`，`rose->mode` 将解引用无效指针。

虽 `hs_database_t` 为 opaque 类型，但攻击者可通过内存操作伪造。

## 修复建议
在解引用前增加 RoseEngine 合法性的基本检查，如检查头部 magic 字段或整体 size 字段。
