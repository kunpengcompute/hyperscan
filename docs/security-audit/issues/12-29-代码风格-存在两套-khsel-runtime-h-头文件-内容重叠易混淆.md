# 【代码风格】存在两套 khsel_runtime.h 头文件，内容重叠易混淆

- Remote: https://gitcode.com/boostkit/hyperscan/issues/29
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P2 代码风格

## 问题描述
项目中存在两个 `khsel_runtime.h` 文件：

- `src/khsel_runtime.h` — 定义了 `KHSEL_LilyRunExec` 等函数声明和 `HsEngine` 枚举
- `src/kunpeng-enhanced/khsel_runtime.h` — 内容几乎完全相同

两文件包含重复的类型定义（`HsEngine` 枚举、`LILY_TO_OFFSET_MAX` 宏、`ALL_LILY_MATCH_ITEMS` 宏）和函数声明。`runtime.c:60` 通过 `#include "khsel_runtime.h"` 引用，实际编译时由 include path 决定使用哪个版本。

这种重复定义违反 DRY 原则，未来修改时可能只更新其中一个，导致不一致。

## 修复建议
保留 `src/kunpeng-enhanced/khsel_runtime.h` 作为权威定义，`src/khsel_runtime.h` 仅做转发 include 或直接删除并以 include path 确保正确引用。
