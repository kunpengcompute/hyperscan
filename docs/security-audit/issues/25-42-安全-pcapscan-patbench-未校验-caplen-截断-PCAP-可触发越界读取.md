# 【安全】pcapscan/patbench 未校验 caplen，截断 PCAP 可触发越界读取

- Remote: https://gitcode.com/boostkit/hyperscan/issues/42
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 问题

examples/pcapscan.cc 和 examples/patbench.cc 在离线读取 PCAP 时调用 pcap_next() 得到 pktHeader，但后续解析完全没有使用 pktHeader.caplen 校验捕获到的实际包长。payloadOffset() 只接收 pkt_data，会直接解引用以太网/IP/TCP 头，并用 IP 头里的 ip_len 计算 payload 长度。

相关位置：

- examples/pcapscan.cc:221-238：读取 pktHeader 后未使用 caplen，直接 packets.push_back(string(payload, length))。
- examples/pcapscan.cc:574-608：payloadOffset() 读取 iph->ip_v、iph->ip_hl、TCP th_off，并用 ntohs(iph->ip_len) 计算 length，没有任何捕获长度边界检查。
- examples/patbench.cc:326-342 与 examples/patbench.cc:792-826 存在同样逻辑。

## 影响

恶意或截断的 PCAP 文件可以声明较大的 IP 总长度或不足的链路/IP/TCP 头长度，使代码在解析头部或构造 string(payload, length) 时从 pktData 后方读取数据。结果可能是进程崩溃、越界读，或把相邻内存拷贝进测试数据。虽然位于 examples/benchmark 工具中，但这些工具通常会处理外部提供的 PCAP 文件。

## 建议

- 将 pktHeader.caplen 传入 payloadOffset()。
- 在读取 ether_header、ip、TCP/UDP header 前分别检查 caplen 是否足够。
- 校验 iph->ip_hl >= 5、TCP th_off >= 5。
- 校验 sizeof(ether_header) + ntohs(iph->ip_len) <= caplen，且 offset <= packet_end 后再计算 payload 长度。
- 为截断以太网帧、截断 IP 头、截断 TCP 头、ip_len > caplen 添加回归测试。
