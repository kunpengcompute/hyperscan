# 【代码质量】hs_scan_stream_internal 中 Lily 模式检查在流模式下恒为 false

- Remote: https://gitcode.com/boostkit/hyperscan/issues/30
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P2 代码质量

## 问题描述
`hs_scan_stream_internal` (runtime.c:962) 在流模式扫描函数中包含 Lily 引擎执行代码，但条件检查恒为 false：

```cpp
// hs_scan_stream_internal (流模式)
if (rose->lilyOffset && !(rose->mode & HS_MODE_STREAM)) {
    lilyResult = KHSEL_LilyRunExec(rose, scratch);
}
```

因为这是流模式函数，`rose->mode` 永远包含 `HS_MODE_STREAM` 标志，`!(rose->mode & HS_MODE_STREAM)` 永远为 false。这意味着 Lily 引擎在流模式下永远不会被调用。

对比 block 模式函数 `hs_scan` (runtime.c:403) 中的同样检查，block 模式下 `rose->mode == HS_MODE_BLOCK`，条件可以正确触发。

此代码可能是从 `hs_scan` 复制粘贴而来，未针对流模式上下文进行调整。如果 Lily 确实不支持流模式（如 README 所述），应添加注释说明并删除此段死代码；如果支持，则需要修正条件。

## 修复建议
若 Lily 仅支持 block 模式：删除此段死代码，添加注释。
若 Lily 应支持流模式：修正条件为 `if (rose->lilyOffset)`。
