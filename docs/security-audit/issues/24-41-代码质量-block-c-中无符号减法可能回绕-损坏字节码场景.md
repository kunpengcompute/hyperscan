# 【代码质量】block.c 中无符号减法可能回绕（损坏字节码场景）

- Remote: https://gitcode.com/boostkit/hyperscan/issues/41
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P2 代码质量

## 问题描述
`runAnchoredTableBlock` (block.c:46-57) 中的无符号减法缺少保护：

```c
size_t alen = MIN(length, t->anchoredDistance);
...
if (length >= curr->anchoredMinDistance) {  // 检查 length
    size_t local_alen = alen - curr->anchoredMinDistance;  // 但使用 alen 做减法！
    const u8 *local_buffer = buffer + curr->anchoredMinDistance;
```

如果 `t->anchoredDistance < curr->anchoredMinDistance`（依赖 assert `assert(t->anchoredDistance > curr->anchoredMinDistance)` 在 release 中被移除），则 `alen = t->anchoredDistance`，`local_alen = alen - curr->anchoredMinDistance` 产生巨大的回绕值。后续 `local_buffer + local_alen` 可能越界访问。

虽然正常编译的数据库保证此约束，但损坏或恶意构造的数据库可触发。

## 修复建议
```c
if (unlikely(curr->anchoredMinDistance > alen)) {
    goto next_nfa;  // 安全跳过
}
size_t local_alen = alen - curr->anchoredMinDistance;
```
