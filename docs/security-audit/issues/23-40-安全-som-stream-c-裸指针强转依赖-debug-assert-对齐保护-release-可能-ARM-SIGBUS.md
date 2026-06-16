# 【安全】som_stream.c 裸指针强转依赖 debug assert 对齐保护，release 可能 ARM SIGBUS

- Remote: https://gitcode.com/boostkit/hyperscan/issues/40
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P2 低危

## 问题描述
`storeSomValue` 和 `loadSomValue` (som_stream.c:50-90) 通过裸指针强转写入/读取多字节值，对齐仅依赖 debug assert：

```c
void storeSomValue(void *stream_som_store, u64a som_value, u64a offset, ...) {
    ...
    assert(ISALIGNED_N(stream_som_store, alignof(u16)));
    *(u16 *)stream_som_store = rel_offset;  // ARM 上若未对齐可能 SIGBUS
    ...
    assert(ISALIGNED_N(stream_som_store, alignof(u32)));
    *(u32 *)stream_som_store = rel_offset;
    ...
    assert(ISALIGNED_N(stream_som_store, alignof(u64a)));
    *(u64a *)stream_som_store = rel_offset;
}
```

目标平台鲲鹏 920 (ARM64) 在默认配置下允许非对齐访问，但在以下场景可能异常：
- `/proc/cpu/alignment` 设置为严格模式
- 某些虚拟化环境
- Device memory 访问

## 修复建议
使用 `memcpy` 或 `unaligned_store_u16/u32/u64a` 替代裸指针强转：
```c
unaligned_store_u16(stream_som_store, rel_offset);
```
