# 【代码质量】hs_scan_vector 未对 count=0 做早期返回

- Remote: https://gitcode.com/boostkit/hyperscan/issues/35
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P2 代码质量

## 问题描述
`hs_scan_vector` (runtime.c:1152-1220) 在 `count=0` 时仍执行了完整的初始化流程：

```cpp
hs_error_t HS_CDECL hs_scan_vector(..., unsigned int count, ...) {
    if (unlikely(!scratch || !data || !length)) {
        return HS_INVALID;  // 检查了 data/length 但不检查 count
    }
    ...
    hs_stream_t *id = (hs_stream_t *)(scratch->bstate);
    init_stream(id, rose, 1); /* open stream */  // 不必要的 stream 初始化

    for (u32 i = 0; i < count; i++) { ... }  // count=0 时跳过

    if (onEvent) {
        report_eod_matches(id, scratch, onEvent, context);  // 对空流报告 EOD
    }
```

虽然循环不执行，但 `init_stream` + `report_eod_matches` 对空流执行了多余的 work。`count=0` 应作为合法边界条件直接返回 `HS_SUCCESS`。

## 修复建议
```cpp
if (count == 0) {
    return HS_SUCCESS;
}
```
