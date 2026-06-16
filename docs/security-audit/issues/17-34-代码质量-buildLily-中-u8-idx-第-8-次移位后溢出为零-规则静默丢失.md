# 【代码质量】buildLily 中 u8 idx 第 8 次移位后溢出为零，规则静默丢失

- Remote: https://gitcode.com/boostkit/hyperscan/issues/34
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P2 代码质量

## 问题描述
`buildLily` (lily.cpp:46-74) 使用 `u8 idx` 作为位掩码标识每个规则：

```cpp
u8 idx = 1;       // 00000001
for (auto it : lily) {
    ...
    hi_a[hi] |= idx;
    lo_a[lo] |= idx;
    ...
    idx = idx << 1;  // 1→2→4→8→16→32→64→128→0 (第8次溢出!)
}
```

`u8` 最大值为 255。第 8 次循环后 `idx = 128`，第 9 次 `idx << 1 = 256`，在 u8 中溢出为 0。后续所有规则的 mask 位均为 0，匹配时这些规则永远无法命中，即**静默丢失**。

虽然 Lily 应有 8 条规则上限的保护，但该 guard 可能在编译器路径上失效（如重复规则合并后的计数 bug），此溢出没有运行时报错，难以排查。

## 修复建议
```cpp
if (idx == 0 || litCount >= 8) break;  // 显式上限保护
```
