# 准备产物说明

当前目录包含开工前最值得先固定下来的 3 类内容：

1. `真实材料整理.md`
   对本地资料中的调试文档、日志样本、历史问题 case、常用脚本做了第一轮资产化整理。
2. `标签体系_v1.md`
   定义了问题大类、原因标签、建议动作标签、风险等级标签，并和当前 i.MX8MP + RPMsg + CANopen/伺服联调场景对齐。
3. `benchmark_v1.jsonl`
   提供 25 条 benchmark 种子：
   - 10 条知识问答
   - 10 条日志归因
   - 5 条工具调用安全测试

建议下一步按下面顺序继续推进：

1. 把 `真实材料整理.md` 中的 `DOC-001` ~ `DOC-010` 优先做正文抽取和切片。
2. 把 `LOG-011` ~ `LOG-020` 做成单独的标注样本文件，因为它们比编译 warning 更能体现真实工业联调问题。
3. 把 `benchmark_v1.jsonl` 拆分成 `knowledge_qa.jsonl`、`log_attribution.jsonl`、`tool_safety.jsonl` 三个子集。
4. 让后续 Agent 先只支持 `L0_readonly` 和少量 `L1_low_risk_exec`。
