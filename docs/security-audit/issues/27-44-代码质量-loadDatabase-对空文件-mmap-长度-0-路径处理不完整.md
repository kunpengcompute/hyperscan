# 【代码质量】loadDatabase 对空文件 mmap 长度 0 路径处理不完整

- Remote: https://gitcode.com/boostkit/hyperscan/issues/44
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 问题

util/database_util.cpp 在 HAVE_MMAP 路径中读取数据库文件时，fstat 后直接将 st.st_size 转为 size_t len，并调用 mmap(nullptr, len, PROT_READ, MAP_SHARED, fd, 0)。代码没有对 len == 0 做早期返回。

相关位置：

- util/database_util.cpp:90-98：fstat 后未检查 st.st_size 是否为 0，直接 mmap。
- util/database_util.cpp:121-138：后续假设 bytes 可用，并可能调用 hs_serialized_database_info / hs_deserialize_database。

在 POSIX 系统上，mmap 长度为 0 会失败并返回 EINVAL。当前代码会打印 mmap failed 并返回 nullptr，但错误原因不清晰；非 mmap 分支则会 new char[0] 并继续把 0 长度传给反序列化接口，两条路径行为不一致。

## 影响

空数据库文件或被截断为 0 字节的输入会走不同错误路径，影响工具诊断和自动化测试稳定性。

## 建议

在 fstat 后显式检查 st.st_size <= 0，统一返回 nullptr 并输出明确错误；非 mmap 分支也应对 len == 0 做同样处理。
