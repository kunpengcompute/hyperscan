# 【安全】Rose 字节码 VM 所有偏移访问仅 debug assert 保护，release 无边界检查，恶意数据库可 OOB

- Remote: https://gitcode.com/boostkit/hyperscan/issues/37
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P0 高危

## 问题描述
Rose 程序解释器 `roseRunProgram` (program_runtime.c:2151-2859, 约 1000 行) 的所有字节码偏移访问仅通过 debug assert 进行边界校验：

入口 (line 2157-2159)：
```c
hwlmcb_rv_t roseRunProgram(const struct RoseEngine *t, ..., u32 programOffset, ...) {
    assert(programOffset != ROSE_INVALID_PROG_OFFSET);
    assert(programOffset >= sizeof(struct RoseEngine));
    assert(programOffset < t->size);  // 仅 debug!
```

`getByOffset` 辅助函数 (runtime.h:64)：
```c
static really_inline
const void *getByOffset(const struct RoseEngine *t, u32 offset) {
    assert(offset < t->size);  // 仅 debug!
    return (const u8 *)t + offset;
}
```

程序中大量使用 `getByOffset(t, ri->jump_table)`、`pc += ri->fail_jump`、`pc = pc_base + jumps[idx]` 等方式访问字节码。所有这些值都从字节码中解析，但 release 编译时 assert 被移除，恶意构造/损坏的数据库可：

1. **OOB 读取**：通过精心设计的 `ri->iter_offset`、`ri->jump_table`、`ri->reach_index` 等字段读取 RoseEngine 外的内存
2. **VM 跳转劫持**：通过 `jumps[idx]` 或 `pc += ri->fail_jump` 跳转到任意程序位置
3. 影响约 30+ 处 `getByOffset` 调用和所有 `PROGRAM_NEXT_INSTRUCTION_JUMP` 宏

## 修复建议
在 `getByOffset` 中添加 release 模式的运行时检查：
```c
if (unlikely(offset >= t->size)) {
    return NULL;  // 或触发错误处理
}
return (const u8 *)t + offset;
```
并在关键跳转指令处添加目标地址范围验证。
