# 【代码质量】MAX_MQE_LEN 三处重复定义，存在 ABI 不一致风险

- Remote: https://gitcode.com/boostkit/hyperscan/issues/39
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 严重级别
P1 中危

## 问题描述
`MAX_MQE_LEN`（NFA 事件队列最大长度）在三个头文件中独立定义，值均为 10：

- `src/nfa/nfa_api_queue.h:41` — `#define MAX_MQE_LEN 10`
- `src/kunpeng-enhanced/scratch.h:44` — `#define MAX_MQE_LEN 10`
- `src/kunpeng-enhanced/report.h:46` — `#define MAX_MQE_LEN 10`

`nfa_api_queue.h` 中的定义控制队列的 `items` 数组大小（`struct mq_item items[MAX_MQE_LEN]`）。而 `scratch.h` 和 `report.h` 中的定义控制 Scratch 空间分配时的队列大小计算。

如果未来因性能需求调整队列深度（例如支持更复杂的 multi-INFIX 或多重 MPV 链），极有可能只修改一处而遗漏另外两处，导致：
- 队列实际空间 < 代码允许的 push 数量 → 栈缓冲区溢出
- Scratch 分配不足 → 运行时状态损坏

## 修复建议
将 `MAX_MQE_LEN` 统一定义在单一头文件中（如 `nfa_api_queue.h`），`scratch.h` 和 `report.h` 通过 include 引用。
