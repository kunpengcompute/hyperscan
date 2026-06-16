# 【安全】roseCatchUpMPV_i 中 next_pos_match_loc 在部分路径下未被初始化

- Remote: https://gitcode.com/boostkit/hyperscan/issues/33
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P1 中危

## 问题描述
`roseCatchUpMPV_i` (catchup.c:356-441) 中 `next_pos_match_loc` 在以下路径保持初始值 0：

```cpp
u64a next_pos_match_loc = 0;  // 初始化为 0
...
if (roseSuffixInfoIsExhausted(t, info, ...)) {
    deactivateQueue(t, aa, qi, scratch);
    goto done;  // 跳过 nfaExecMpv_QueueExecRaw 调用！
}
...
next_pos_match_loc = nfaExecMpv_QueueExecRaw(q->nfa, q, loc);
...
done:
    ...
    scratch->tctxt.next_mpv_offset
        = MAX(next_pos_match_loc + scratch->core_info.buf_offset,  // 0 + offset
              mpv_exec_end + 1);
```

当 mpv 队列因 exhaustion 被 deactivate 时，`next_pos_match_loc` 保持为 0，导致 `next_mpv_offset` 回退到较小值，可能使后续匹配跳过合法位置。`next_mpv_offset = 0` 还可能在后续 `roseHandleChainMatch`（match.c:179）中触发异常行为。

## 修复建议
在 `goto done` 之前设置：
```cpp
next_pos_match_loc = scratch->core_info.len;
```
