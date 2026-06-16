# 【安全】PythonGenerator 将 generatorType 拼接进 popen shell 命令，缺少白名单校验

- Remote: https://gitcode.com/boostkit/hyperscan/issues/43
- Issue Type: 代码检视
- Domain: boost-core
- Source: local audit / GitCode issue sync

## Details

## 问题

unit/fuzz/generator/python_generator.cpp 使用字符串拼接构造 shell 命令并传给 popen()：

- unit/fuzz/generator/python_generator.cpp:20-31：cmd << "python ../../../tools/fuzz/" << generatorType << ".py"; 后执行 popen(cmd.str().c_str(), "r")。
- unit/fuzz/fuzz_test.h:18-22 注释显示 generatorType 预期只允许 aristocrats、completocrats、heuristocrats。
- unit/fuzz/fuzz_test.cpp:21-30 通过 generator->configure(params.generatorType, ...) 将该字符串传入生成器。

当前实现没有对白名单做运行时校验，也没有避免 shell 解释。如果将来测试参数来自命令行、环境、配置文件、CI matrix 或被第三方测试扩展复用，generatorType 中的 shell 元字符会被 popen() 执行。

## 影响

这是测试工具链中的命令注入风险。攻击者如果能影响 fuzz 参数，可以在运行 fuzz 测试的开发机或 CI 环境中执行任意命令。即使当前内置参数是固定数组，该接口形态也容易在后续扩展时被误用。

## 建议

- 在 configure() 或 generate() 中对 generatorType 做白名单校验，仅允许三个已知生成器名。
- 避免通过 shell 执行，优先使用 fork/exec 或等价 API，并把参数作为 argv 传递。
- 如果继续使用 popen()，至少拒绝包含路径分隔符、空白、分号、&、管道、反引号、命令替换等 shell 元字符的值。
- 增加单元测试覆盖非法 generatorType，确认不会执行外部命令。
