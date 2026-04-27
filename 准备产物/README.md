# 准备产物说明

当前目录包含当前训练基线最值得固定下来的 3 类内容：

1. `真实材料整理.md`
   对本地资料中的调试文档、日志样本、历史问题 case、常用脚本做了第一轮资产化整理。
2. `标签体系_v1.md`
   定义了问题大类、原因标签、建议动作标签、风险等级标签，并和当前 i.MX8MP A53 + RPMsg + M7 + EtherCAT Dynamic Profile / robot6 / Fake Harness 场景对齐。
3. `benchmark_v1.jsonl`
   提供 25 条 benchmark 种子：
   - 10 条知识问答
   - 10 条日志归因
   - 5 条工具调用安全测试

当前训练资料已经从早期双驱 CANopen 切换到 EtherCAT Dynamic Profile 主线。建议下一步按下面顺序继续推进：

1. 把 `真实材料整理.md` 中的 `CUR-001` ~ `CUR-007` 优先做正文抽取和切片。
2. 把 robot6 位置模式 report、fake harness matrix、XML batch、replay batch 继续沉淀成标注样本。
3. 保持 `benchmark_v1.jsonl` 与 `data/benchmark/` 下三个子集同步。
4. 让后续 Agent 先只支持 `L0_readonly` 和少量 `L1_low_risk_exec`。
