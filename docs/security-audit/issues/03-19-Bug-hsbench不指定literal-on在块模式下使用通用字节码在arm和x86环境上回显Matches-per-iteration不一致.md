# [Bug]: hsbench不指定literal_on在块模式下使用通用字节码在arm和x86环境上回显Matches per iteration不一致

- Remote: https://gitcode.com/boostkit/hyperscan/issues/19
- Issue Type: 缺陷
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 感谢您的报告！
请认真填写以下信息，否则可能由于无法定位，导致issue无法解决而被取消。

### 测试类型
            
功能
### 测试版本
            
BoostKit 26.1.0
### 严重级别
            
Secondary
### 问题描述
            
hsbench不指定literal_on在块模式下使用通用字节码在arm和x86环境上回显Matches per iteration不一致
### 操作系统及内核信息
            
![image.png](https://raw.gitcode.com/user-images/assets/7669811/ce6066f7-fd7c-4119-aed4-588181296188/image.png 'image.png')
### 硬件及组网信息
            
鲲鹏单节点+x86单节点
### 软件信息
            
Hyperscan：origin/dev_26.1.RC1
### 出现概率
            
有条件必然重现
### 问题复现步骤
            
S1.x86服务器上执行./hsdump -e rule_set -U -N -o ./dump_output, 序列化生成规则集，有预期结果E1
S2.x86服务器上执行./hsbench -c 语料集 -N -U hsdump序列化规则集 -n500 -T 0-127， 记录匹配数，有预期结果E2
S3.arm服务器上执行./hsbench -c 语料集 -N -U hsdump序列化规则集 -n500 -T 0-127， 记录匹配数，有预期结果E3
### 预期结果
            
E1.生成对应raw文件
E2.compile 时间为0，Matches per iteration输出信息与原生相同
E3.compile 时间为0，Matches per iteration输出信息与原生相同
### 实际结果
            
E1.生成对应raw文件
E2.compile 时间为0，Matches per iteration输出信息与原生相同
E3.compile 时间为0，Matches per iteration输出信息与原生不相同
### 日志信息
            
**X86回显：**
Loading FAT database from: /home/hyperscan_test/20260510173822_OPTIMIZED_release_Lily_1_NeoFdr_1/output_snort_literals/dump/db.raw

Loading FAT database from: /home/hyperscan_test/20260510173822_OPTIMIZED_release_Lily_1_NeoFdr_1/output_snort_literals/dump/db.raw

Reading 1570824 bytes

Reading 1570824 bytes

Signature set:        /home/hyperscan_test/20260510173822_OPTIMIZED_release_Lily_1_NeoFdr_1/output_snort_literals/dump/db.raw
Signatures:        db.raw
Hyperscan info:    Version: 5.4.2 Mode: BLOCK
Expression count:  0
Bytecode size:     921,592 bytes
Database CRC:      0x88d383d2
Scratch size:      70,813 bytes
Compile time:      0.000 seconds
Peak heap usage:   0 bytes

Time spent scanning:       316.316 seconds
Corpus size:               177,087,567 bytes (130,957 blocks)
Matches per iteration:     637,380 (3.686 matches/kilobyte)
Overall block rate:        26,496,401.99 blocks/sec
Mean throughput (overall): 286,639.64 Mbit/sec
Max throughput (per core): 3,211.48 Mbit/sec


[?2004h[root@localhost ~]# 
Signature set:        /home/hyperscan_test/20260510173822_OPTIMIZED_release_Lily_1_NeoFdr_1/output_snort_literals/dump/db.raw
Signatures:        db.raw
Hyperscan info:    Version: 5.4.2 Mode: BLOCK
Expression count:  0
Bytecode size:     921,592 bytes
Database CRC:      0x88d383d2
Scratch size:      70,813 bytes
Compile time:      0.000 seconds
Peak heap usage:   0 bytes

Time spent scanning:       324.856 seconds
Corpus size:               177,087,567 bytes (130,957 blocks)
Matches per iteration:     637,380 (3.686 matches/kilobyte)
Overall block rate:        25,799,900.61 blocks/sec
Mean throughput (overall): 279,104.84 Mbit/sec
Max throughput (per core): 3,219.46 Mbit/sec

**ARM回显**
Loading FAT database from: /home/hyperscan_test/20260510174522_OPTIMIZED_release_Lily_1_NeoFdr_1/db.raw
Reading 1570824 bytes

Loading FAT database from: /home/hyperscan_test/20260510174522_OPTIMIZED_release_Lily_1_NeoFdr_1/db.raw

Reading 1570824 bytes

Signature set:        /home/hyperscan_test/20260510174522_OPTIMIZED_release_Lily_1_NeoFdr_1/db.raw
Signatures:        db.raw
Hyperscan info:    Version: 1.0.3 Mode: BLOCK
Expression count:  0
Bytecode size:     649,328 bytes
Database CRC:      0x7962071f
Scratch size:      70,449 bytes
Compile time:      0.000 seconds
Peak heap usage:   0 bytes

Time spent scanning:       232.408 seconds
Corpus size:               177,087,567 bytes (130,957 blocks)
Matches per iteration:     326,986 (1.891 matches/kilobyte)
Overall block rate:        36,062,646.61 blocks/sec
Mean throughput (overall): 390,127.83 Mbit/sec
Max throughput (per core): 4,564.98 Mbit/sec


Signature set:        /home/hyperscan_test/20260510174522_OPTIMIZED_release_Lily_1_NeoFdr_1/db.raw
Signatures:        db.raw
Hyperscan info:    Version: 1.0.3 Mode: BLOCK
Expression count:  0
Bytecode size:     649,328 bytes
Database CRC:      0x7962071f
Scratch size:      70,449 bytes
Compile time:      0.000 seconds
Peak heap usage:   0 bytes

Time spent scanning:       233.152 seconds
Corpus size:               177,087,567 bytes (130,957 blocks)
Matches per iteration:     326,986 (1.891 matches/kilobyte)
Overall block rate:        35,947,611.27 blocks/sec
Mean throughput (overall): 388,883.38 Mbit/sec
Max throughput (per core): 4,577.62 Mbit/sec
### 提单组织
            
内部测试
### 测试代码
