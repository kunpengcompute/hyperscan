# 【安全】hs_serialize_database 手动指针序列化无写入边界校验

- Remote: https://gitcode.com/boostkit/hyperscan/issues/38
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P1 中危

## 问题描述
`hs_serialize_database` (database.c:86-107) 通过手动 `buf++` 指针递增进行序列化，全程无越界检查：

```c
u32 *buf = (u32 *)out;
*buf = db->magic;        // 无检查
buf++;
*buf = db->version;      // 无检查
buf++;
*buf = db->length;       // 无检查
buf++;
memcpy(buf, &db->platform, sizeof(u64a)); buf += 2;
*buf = db->crc32;        buf++;
*buf = db->reserved0;    buf++;
*buf = db->reserved1;    buf++;

const char *bytecode = hs_get_bytecode(db);
memcpy(buf, bytecode, db->length);  // 最终 memcpy
```

`out` 的分配大小由 `sizeof(struct hs_database) + db->length` 计算（line 77），但如果 `db->length` 字段被损坏或 `sizeof(struct hs_database)` 与序列化格式不匹配，`buf` 可能写穿 `out` 缓冲区。

## 修复建议
在每次指针递增后添加边界检查：
```c
u32 *buf = (u32 *)out;
u32 *buf_end = (u32 *)(out + length);
if (buf >= buf_end) goto error;
```
或使用结构化序列化以避免手动指针操作。
