# Spindle Roadmap

Spindle 当前进入 v1.0 稳定期。下一阶段的重点不是继续堆功能，而是把已有能力稳定用于真实工程项目：搜索资料、分析日志、生成草稿、更新 benchmark，再通过真实 case 反哺 Agent。

## v1.0 稳定目标

- README、roadmap、使用手册保持清晰可读。
- GitHub 仓库可以作为可展示工程。
- 所有真实硬件动作保持明确边界。
- 本地门禁稳定通过：pytest、benchmark、RAG tool-safety、secret-scan、pre-push、GSD offline。
- 后续真实问题进入统一沉淀流程，而不是零散记录在聊天或临时文件里。

## 当前能力基线

- 资料检索：`material-status`、`material-inventory`、`material-search`。
- 安全工具层：`material-tools`、`material-tool-plan`、`material-run-tool`。
- 项目状态：`project-status`、`gsd-status`、`gsd-offline-run`。
- 问题沉淀：`draft-fact`、`material-draft-fact`、`import-report`、`summarize-fake-regression`。
- 硬件审计：`audit-hardware-action`。
- 开发板只读诊断：`board-status`、`rpmsg-health`、`m7-health`、`ethercat-query-readonly`、`board-report`。
- 回归门禁：pytest、rules benchmark、tool safety benchmark、RAG tool-safety benchmark、offline stub samples、secret scan。

## 使用闭环

每次真实 EtherCAT/机器人项目有新问题，按以下顺序推进：

1. 搜索资料和历史 case：`spindle tools material-search "<keyword>" --scope all`。
2. 判断风险边界：`spindle tools audit-hardware-action "<request>"`。
3. 优先离线验证：fake harness、report import、board dry-run、benchmark。
4. 生成草稿：`draft-fact`、`material-draft-fact`、`import-report` 或 `summarize-fake-regression`。
5. 审查后再更新 canonical 材料、benchmark 和 chunks。
6. 跑 `spindle tools pre-push-check --include-offline --include-rag`。

## 安全边界

默认自动化范围为 `offline_ok`。

允许自动执行：

- 文档整理、README/roadmap/manual 更新。
- raw search、material inventory、curated memory 草稿。
- pytest、benchmark、secret scan、pre-push。
- fake harness/replay/report summary。
- 开发板诊断 dry-run。

需要真实窗口或人工确认：

- 带 `--execute` 的开发板只读诊断。
- 真实 `ssh/scp/reboot`。
- `start-bus` / `stop-bus`。
- `0x86` 控制字。
- `0x41F1` 输出门控解锁。
- 机器人运动。
- IO、焊接、限位输出。
- remoteproc 生命周期、固件烧写、M7 热重载。

## 近期不做

- 不重写 harness loop。
- 不搭重型知识库。
- 不做完整机器人动力学仿真。
- 不让 GSD 自动执行真实硬件动作。
- 不删除旧链路或真实工程脚本。

## 进入 v1.1 的条件

累计 10-20 个真实使用 case 后，再评估 v1.1。进入 v1.1 前需要回答：

- 哪些问题是搜索和规则已经解决不了的？
- RAG 引用质量是否仍是瓶颈？
- 是否需要更强 Agent SDK wrapper？
- 是否真的需要自研 runtime，还是现有 CLI wrapper 足够？
- 哪些硬件流程可以进一步拆成“只读、低风险、人工确认、高风险”四层？

## v1.1 候选方向

- 更强的 source-grounded 问答和引用排序。
- Agent SDK wrapper 原型。
- 更多真实 report replay case。
- 更细的硬件动作审计策略。
- 基于真实使用记录的 benchmark 自动扩充。
- GitHub 展示页、示例视频或 demo transcript。
