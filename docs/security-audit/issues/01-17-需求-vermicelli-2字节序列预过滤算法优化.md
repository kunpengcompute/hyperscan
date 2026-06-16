# [需求]: vermicelli 2字节序列预过滤算法优化

- Remote: https://gitcode.com/boostkit/hyperscan/issues/17
- Issue Type: 需求
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

感谢提出新需求。

### 【需求背景】
            
鲲鹏平台部分DPI应用场景中，预过滤模块占据端到端10%热点，需要针对预过滤模块中vermicelli 2字节短序列匹配算法进行性能优化
### 【需求描述】
            
利用平台支持的neon指令集，对Hyperscan double-vermicelli 2字节短序列预过滤算法进行性能优化，提升x%性能。
### 【其他说明】
