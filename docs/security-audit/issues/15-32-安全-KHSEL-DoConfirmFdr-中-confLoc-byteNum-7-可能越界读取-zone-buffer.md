# 【安全】KHSEL_DoConfirmFdr 中 confLoc + byteNum - 7 可能越界读取 zone buffer

- Remote: https://gitcode.com/boostkit/hyperscan/issues/32
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P1 中危

## 问题描述
`KHSEL_DoConfirmFdr` (fdr_enhanced.c:302) 为确认匹配而加载 8 字节数据，偏移量可退到 `itPtr` 之前 7 字节：

```cpp
u64a confVal = UnalignedLoadU64a(confLoc + byteNum - sizeof(u64a) + 1);
// = confLoc + byteNum - 7
```

当 `bit=0` 且 `offset=0` 时，`byteNum = 0 / 8 + 0 = 0`，读取地址为 `confLoc - 7`。

在 MainZone 场景中，zone 直接指向输入 buffer，`itPtr - 7` 可能访问 buffer 前的内存。虽然 StartZone 和 ShortZone 在 zone buffer 中预留了历史数据，但 MainZone 没有历史区，且 ShortZone 的 padding 也仅 16 字节（不足以覆盖 stride=1 时 `itPtr - 7` 到 `itPtr + 15` 的范围）。

## 受影响代码
```cpp
#define KHSEL_FDR_MAIN_LOOP(zz, s, get_conf_fn)
    for (const u8 *itPtr = start_ptr; itPtr <= end_ptr; itPtr += 16) {
        ...
        KHSEL_DoConfirmFdr(&conf[0], 0, ..., itPtr, ...);
        // offset=0: confLoc=itPtr, byteNum由conf中的置位bit决定
        // 若bit=0: UnalignedLoadU64a(itPtr + 0 - 7) = itPtr前7字节
```

## 修复建议
在 `KHSEL_DoConfirmFdr` 中添加 `byteNum >= sizeof(u64a) - 1` 的检查，或在 zone 构建时确保 MainZone 之前也填充足够的历史 padding。
