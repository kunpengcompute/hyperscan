# Security Audit Issues

This directory tracks 28 locally scanned issues for boostkit/hyperscan. Each entry mirrors a GitCode issue and records the issue type and domain used for creation.

- Issue type: 代码检视 for code review findings where applicable
- Domain: boost-core
- Repository: boostkit/hyperscan

| # | Remote | Title | Link |
|---|---:|---|---|
| 01 | 17 | [需求]: vermicelli 2字节序列预过滤算法优化 | https://gitcode.com/boostkit/hyperscan/issues/17 |
| 02 | 18 | [Bug]: 使用hscollider工具测试时回显FAILED | https://gitcode.com/boostkit/hyperscan/issues/18 |
| 03 | 19 | [Bug]: hsbench不指定literal_on在块模式下使用通用字节码在arm和x86环境上回显Matches per iteration不一致 | https://gitcode.com/boostkit/hyperscan/issues/19 |
| 04 | 20 | [需求]: arm上生成通用字节码，在x86和arm平台能够成功部署运行 | https://gitcode.com/boostkit/hyperscan/issues/20 |
| 05 | 21 | [需求]: x86上生成通用字节码，在x86和arm平台能够成功部署运行 | https://gitcode.com/boostkit/hyperscan/issues/21 |
| 06 | 22 | [需求]: Hyperscan工具进行通用字节码功能适配 | https://gitcode.com/boostkit/hyperscan/issues/22 |
| 07 | 23 | [Bug]: Hyperscan工具进行通用字节码在x86环境下对比原生性能劣化 | https://gitcode.com/boostkit/hyperscan/issues/23 |
| 08 | 25 | 【安全】hs_compile/hs_compile_lit 中 expression 为 NULL 时未检查 error/db 导致空指针解引用 | https://gitcode.com/boostkit/hyperscan/issues/25 |
| 09 | 26 | 【安全】hs_serialized_database_info 从截断的序列化数据中越界读取 RoseEngine::mode | https://gitcode.com/boostkit/hyperscan/issues/26 |
| 10 | 27 | 【安全】hs_database_info 中 rose->mode 解引用了未充分验证的内部指针 | https://gitcode.com/boostkit/hyperscan/issues/27 |
| 11 | 28 | 【安全】Lily scratch 分配使用硬编码 LILY_ITEMS_COUNT 而非来自 RoseEngine 的推导值 | https://gitcode.com/boostkit/hyperscan/issues/28 |
| 12 | 29 | 【代码风格】存在两套 khsel_runtime.h 头文件，内容重叠易混淆 | https://gitcode.com/boostkit/hyperscan/issues/29 |
| 13 | 30 | 【代码质量】hs_scan_stream_internal 中 Lily 模式检查在流模式下恒为 false | https://gitcode.com/boostkit/hyperscan/issues/30 |
| 14 | 31 | 【安全】buildLily/buildLilyForTeddy 中 reportVec/ekeyVec 无边界检查，可导致 OOB 写入 | https://gitcode.com/boostkit/hyperscan/issues/31 |
| 15 | 32 | 【安全】KHSEL_DoConfirmFdr 中 confLoc + byteNum - 7 可能越界读取 zone buffer | https://gitcode.com/boostkit/hyperscan/issues/32 |
| 16 | 33 | 【安全】roseCatchUpMPV_i 中 next_pos_match_loc 在部分路径下未被初始化 | https://gitcode.com/boostkit/hyperscan/issues/33 |
| 17 | 34 | 【代码质量】buildLily 中 u8 idx 第 8 次移位后溢出为零，规则静默丢失 | https://gitcode.com/boostkit/hyperscan/issues/34 |
| 18 | 35 | 【代码质量】hs_scan_vector 未对 count=0 做早期返回 | https://gitcode.com/boostkit/hyperscan/issues/35 |
| 19 | 36 | 【代码质量】hs_free_database 对 NULL db 调用自定义 free 存在风险 | https://gitcode.com/boostkit/hyperscan/issues/36 |
| 20 | 37 | 【安全】Rose 字节码 VM 所有偏移访问仅 debug assert 保护，release 无边界检查，恶意数据库可 OOB | https://gitcode.com/boostkit/hyperscan/issues/37 |
| 21 | 38 | 【安全】hs_serialize_database 手动指针序列化无写入边界校验 | https://gitcode.com/boostkit/hyperscan/issues/38 |
| 22 | 39 | 【代码质量】MAX_MQE_LEN 三处重复定义，存在 ABI 不一致风险 | https://gitcode.com/boostkit/hyperscan/issues/39 |
| 23 | 40 | 【安全】som_stream.c 裸指针强转依赖 debug assert 对齐保护，release 可能 ARM SIGBUS | https://gitcode.com/boostkit/hyperscan/issues/40 |
| 24 | 41 | 【代码质量】block.c 中无符号减法可能回绕（损坏字节码场景） | https://gitcode.com/boostkit/hyperscan/issues/41 |
| 25 | 42 | 【安全】pcapscan/patbench 未校验 caplen，截断 PCAP 可触发越界读取 | https://gitcode.com/boostkit/hyperscan/issues/42 |
| 26 | 43 | 【安全】PythonGenerator 将 generatorType 拼接进 popen shell 命令，缺少白名单校验 | https://gitcode.com/boostkit/hyperscan/issues/43 |
| 27 | 44 | 【代码质量】loadDatabase 对空文件 mmap 长度 0 路径处理不完整 | https://gitcode.com/boostkit/hyperscan/issues/44 |
| 28 | 45 | 【代码质量】initDB 建表失败路径未 finalize sqlite3_stmt | https://gitcode.com/boostkit/hyperscan/issues/45 |
