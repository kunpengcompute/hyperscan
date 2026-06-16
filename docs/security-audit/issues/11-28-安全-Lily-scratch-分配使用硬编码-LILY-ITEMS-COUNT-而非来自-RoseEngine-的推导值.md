# 【安全】Lily scratch 分配使用硬编码 LILY_ITEMS_COUNT 而非来自 RoseEngine 的推导值

- Remote: https://gitcode.com/boostkit/hyperscan/issues/28
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P1 中危

## 问题描述
`hs_alloc_scratch` (scratch.c:394-402) 分配 Lily 匹配项缓存时使用硬编码的 `LILY_ITEMS_COUNT`，而非从 `RoseEngine` 中推导：

```cpp
if (LILY_ITEMS_COUNT > proto->lily_ctx.capacity) { // 理应取自rose属性，依据规则推导出
    resize = 1;
    proto->lily_ctx.capacity = LILY_ITEMS_COUNT;
}

if (LILY_ITEMS_COUNT > proto->lily_for_teddy_ctx.capacity) { // 理应取自rose属性，依据规则推导出
    resize = 1;
    proto->lily_for_teddy_ctx.capacity = LILY_ITEMS_COUNT;
}
```

注释明确说"理应取自rose属性，依据规则推导出"，但目前仍使用硬编码常量。如果实际匹配项数量超过 `LILY_ITEMS_COUNT`（4096），`pushLilyItems` 会返回 `HS_INSUFFICIENT_SPACE` 并终止匹配。这与 README 文档中描述的约束一致（"命中次数合计应不大于 4096"），但硬编码值未与 RoseEngine 中的实际配置关联，未来若增大容量需同步修改多处。

## 修复建议
在 `RoseEngine` 结构体中增加 `lilyItemsCapacity` 字段，编译时根据规则计算并写入，运行时从 RoseEngine 读取。
