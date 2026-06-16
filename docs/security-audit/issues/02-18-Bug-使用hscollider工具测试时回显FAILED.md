# [Bug]: 使用hscollider工具测试时回显FAILED

- Remote: https://gitcode.com/boostkit/hyperscan/issues/18
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
            
使用hscollider工具测试时回显FAILED
### 操作系统及内核信息
            
![image.png](https://raw.gitcode.com/user-images/assets/7669811/92780e3c-05d5-4a64-b2bb-0bbe87236c0e/image.png 'image.png')
### 硬件及组网信息
            
鲲鹏单节点
### 软件信息
            
Hyperscan：origin/dev_26.1.RC1
### 出现概率
            
有条件必然重现
### 问题复现步骤
            
S1.hscheck 指定 流模式, 执行./bin/hscollider-e 规则集 -t 10，有预期结果E1
### 预期结果
            
E1.工具执行通过，无error,匹配差异为0
### 实际结果
            
FAILED: id 200000391, alignment 0-63, corpus 206144, results differ
  Expression: '/([^A-Z0-9](JNK[ABCDHN][AFJGPRYV][0-36][14-7][ACDEFPY][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})|([11435]N[1-468KR][0A-HJMNPSTV][A-DFJMNR-WYZ][A-HJ-NPR-Z0-9][ABDEFYZNTF1-8][12356A-FHJPSYUW][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})[^A-Z0-9])/'
  Corpus data: '4N4EWJDW5NYZ4N242l'
  PCRE matches: (0,18)
  UE2 (0 - 63) matches: 
  PCRE only: match (0,18)
FAILED: id 200000391, alignment 0-63, corpus 209054, results differ
  Expression: '/([^A-Z0-9](JNK[ABCDHN][AFJGPRYV][0-36][14-7][ACDEFPY][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})|([11435]N[1-468KR][0A-HJMNPSTV][A-DFJMNR-WYZ][A-HJ-NPR-Z0-9][ABDEFYZNTF1-8][12356A-FHJPSYUW][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})[^A-Z0-9])/'
  Corpus data: '4N4NWTN35NW4L4635y'
  PCRE matches: (0,18)
  UE2 (0 - 63) matches: 
  PCRE only: match (0,18)
FAILED: id 200000391, alignment 0-63, corpus 252843, results differ
  Expression: '/([^A-Z0-9](JNK[ABCDHN][AFJGPRYV][0-36][14-7][ACDEFPY][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})|([11435]N[1-468KR][0A-HJMNPSTV][A-DFJMNR-WYZ][A-HJ-NPR-Z0-9][ABDEFYZNTF1-8][12356A-FHJPSYUW][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})[^A-Z0-9])/'
  Corpus data: '5N2EN5134N4NHW687\xe3'
  PCRE matches: (0,18)
  UE2 (0 - 63) matches: 
  PCRE only: match (0,18)
FAILED: id 200000391, alignment 0-63, corpus 271372, results differ
  Expression: '/([^A-Z0-9](JNK[ABCDHN][AFJGPRYV][0-36][14-7][ACDEFPY][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})|([11435]N[1-468KR][0A-HJMNPSTV][A-DFJMNR-WYZ][A-HJ-NPR-Z0-9][ABDEFYZNTF1-8][12356A-FHJPSYUW][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})[^A-Z0-9])/'
  Corpus data: '5N4NM3ZW4NPXXC676\x8d'
  PCRE matches: (0,18)
  UE2 (0 - 63) matches: 
  PCRE only: match (0,18)
FAILED: id 200000391, alignment 0-63, corpus 271415, results differ
  Expression: '/([^A-Z0-9](JNK[ABCDHN][AFJGPRYV][0-36][14-7][ACDEFPY][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})|([11435]N[1-468KR][0A-HJMNPSTV][A-DFJMNR-WYZ][A-HJ-NPR-Z0-9][ABDEFYZNTF1-8][12356A-FHJPSYUW][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})[^A-Z0-9])/'
  Corpus data: '5N4NNHYY2RUS4N509u'
  PCRE matches: (0,18)

  UE2 (0 - 63) matches: 
  PCRE only: match (0,18)
FAILED: id 200000391, alignment 0-63, corpus 271612, results differ
  Expression: '/([^A-Z0-9](JNK[ABCDHN][AFJGPRYV][0-36][14-7][ACDEFPY][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})|([11435]N[1-468KR][0A-HJMNPSTV][A-DFJMNR-WYZ][A-HJ-NPR-Z0-9][ABDEFYZNTF1-8][12356A-FHJPSYUW][X0-9][A-HJ-NPR-Z0-9]{5}[0-9]{3})[^A-Z0-9])/'
  Corpus data: '5N4NWSF61S4N9U948\xb4'
  PCRE matches: (0,18)
  UE2 (0 - 63) matches: 
  PCRE only: match (0,18)


### 日志信息
            
Summary:
Mode:                           Single/Streaming-10
=========
Expressions processed:          1190
Corpora processed:              241652268
Expressions with failures:      1
  Corpora generation failures:  0
  Compilation failures:         pcre:0, ng:0, ue2:0
  Matching failures:            pcre:1000000, ng:0, ue2:0
  Match differences:            1
  No ground truth:              0
Total match differences:        384
### 提单组织
            
内部测试
### 测试代码
