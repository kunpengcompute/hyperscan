# 【安全】buildLily/buildLilyForTeddy 中 reportVec/ekeyVec 无边界检查，可导致 OOB 写入

- Remote: https://gitcode.com/boostkit/hyperscan/issues/31
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P0 高危

## 问题描述
`buildLily` 和 `buildLilyForTeddy` (lily.cpp:46-74, 84-168) 通过 index 访问 `reportVec`、`ekeyVec`、`lenVec`，但没有边界检查：

### buildLily (line 71-73)
```cpp
for (auto it : lily) {
    ...
    reportVec[litCount] = it.second.external_report;  // 无边界检查！
    ekeyVec[litCount] = it.second.ekey;
    litCount++;
}
```

### buildLilyForTeddy (line 101-103)
```cpp
for (size_t i = 0; i < maxRules; i++) {
    ...
    reportVec[i] = (u32)p.second.external_report;  // 无边界检查！
    ekeyVec[i] = (u32)p.second.ekey;
    lenVec[i] = (u32)p.first.length();
}
```

Lily 引擎硬编码最大 8 条规则，但循环的次数由 `lily` map 的实际大小决定。若因编译器上游逻辑 bug 导致 map 超过 8 条，将写穿 `std::vector<u32>` 的堆内存，造成堆破坏。

## 修复建议
在循环中添加 `litCount < 8` 的上限检查：
```cpp
if (litCount >= 8) break;
reportVec[litCount] = ...;
```
或使用 `std::vector::at()` 替代 `operator[]` 以获得异常保护。
