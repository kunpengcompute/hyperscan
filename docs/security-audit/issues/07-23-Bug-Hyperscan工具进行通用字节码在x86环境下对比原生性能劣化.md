# [Bug]: Hyperscan工具进行通用字节码在x86环境下对比原生性能劣化

- Remote: https://gitcode.com/boostkit/hyperscan/issues/23
- Issue Type: 缺陷
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 感谢您的报告！
请认真填写以下信息，否则可能由于无法定位，导致issue无法解决而被取消。

### 测试类型
            
性能
### 测试版本
            
BoostKit 26.1.0
### 严重级别
            
Secondary
### 问题描述
            
[Bug]: Hyperscan工具进行通用字节码在x86环境下对比原生性能劣化
### 操作系统及内核信息
            
![image.png](https://raw.gitcode.com/user-images/assets/7669811/18bcba12-896e-4d8a-b4f0-849c3486d827/image.png 'image.png')
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
S4.使用同名规则集在不同平台生成的通用字节码规则集，观察hsbench 输出的性能结果，有预期结果E4
### 预期结果
            
E1.生成对应raw文件
E2.compile 时间为0，Matches per iteration输出信息与原生相同
E3.compile 时间为0，Matches per iteration输出信息与原生相同
E4.性能与开源5.4.2版本在对应环境上编译的字节码相比不劣化
### 实际结果
            
E1.生成对应raw文件
E2.compile 时间为0，Matches per iteration输出信息与原生相同
E3.compile 时间为0，Matches per iteration输出信息与原生不相同
E4.性能与开源5.4.2版本在对应环境上编译的字节码相比劣化
### 日志信息
            
[2026-05-13 02:03:23,991] [INFO] [Ultrascan_universal_database_performance_0034.py:161] [step] ARM服务器上性能校验
[2026-05-13 02:03:23,992] [INFO] [hyperscan_common.py:2130] [assert_performance_data] =====================================实测性能 VS 历史性能：1308817.21 / 1325630.24 = 98.73 %=====================================
[2026-05-13 02:03:23,992] [INFO] [Ultrascan_universal_database_performance_0034.py:163] [step] X86服务器上性能校验
[2026-05-13 02:03:23,992] [INFO] [hyperscan_common.py:2130] [assert_performance_data] ======================================实测性能 VS 历史性能：752945.86 / 821962.92 = 91.6 %=======================================
### 提单组织
            
内部测试
### 测试代码
