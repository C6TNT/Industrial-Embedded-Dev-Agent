# Spindle 使用手册

## 1. 文档说明

本文用于说明 Spindle v1.0 稳定期的日常使用方式。它面向工业嵌入式研发现场，重点覆盖 EtherCAT Dynamic Profile、RPMsg、M7、robot6、fake harness、开发板只读诊断和工程资料沉淀。

Spindle 当前不替代真实上板测试，也不自动执行真实机器人或 IO 动作。它的价值在于把原来散落在聊天、脚本、日志和文档里的工程经验整理成可搜索、可回归、可审计的工作流。

## 2. 环境准备

在仓库根目录安装开发依赖：

```powershell
python -m pip install -e ".[dev]"
```

确认 CLI 可用：

```powershell
spindle tools project-status
```

兼容入口仍然可用：

```powershell
python -m industrial_embedded_dev_agent tools project-status
```

## 3. 常用命令

查看项目状态：

```powershell
spindle tools project-status
spindle tools gsd-status
```

查看资料工作区状态：

```powershell
spindle tools material-status
spindle tools material-inventory
```

搜索资料：

```powershell
spindle tools material-search "0x41F1" --scope docs
spindle tools material-search "axis1 12/28" --scope all
spindle tools material-search "remoteproc bad phdr" --scope logs
```

检查现有工具风险：

```powershell
spindle tools material-tools
spindle tools material-tools --risk offline_ok
spindle tools material-tool-plan mix_protocol/tools/run_static_profile_tests.py
```

只允许 `offline_ok` 工具自动执行：

```powershell
spindle tools material-run-tool mix_protocol/tools/run_static_profile_tests.py
```

## 4. 真实问题处理流程

遇到新的 EtherCAT、机器人、RPMsg、M7 或 fake harness 问题时，建议按以下顺序处理：

1. 先搜索原文资料和历史 case。
2. 判断是否跨越硬件边界。
3. 能离线验证的先离线验证。
4. 将测试结果生成草稿。
5. 人工审查后再并入正式材料。
6. 更新 benchmark 或 replay case。
7. 跑完整门禁。

示例：

```powershell
spindle tools material-search "slave2 12/28" --scope all
spindle tools audit-hardware-action "query robot6 profile and compare axis1 layout"
spindle tools draft-fact "robot6 axis1/slave2 uses 12/28 while other axes use 19/13." --title "robot6 axis1 variant" --source "real-report"
spindle check --include-offline --include-rag --rag-type tool_safety
```

## 5. report 与 fake harness 结果沉淀

导入真实 JSON report：

```powershell
spindle tools import-report .\reports\sample_report.json
```

汇总 fake harness 回归结果：

```powershell
spindle tools summarize-fake-regression .\reports\fake_matrix.json
```

这些命令会生成可审查的 LOG 草稿、replay scenario 草稿或材料条目草稿。默认不会直接修改 canonical 数据。

## 6. 开发板只读诊断

开发板只读诊断默认 dry-run，只输出计划，不连接真实板卡：

```powershell
spindle tools board-status
spindle tools rpmsg-health
spindle tools m7-health
spindle tools ethercat-query-readonly
spindle tools board-report
```

确认当前是开发板只读窗口后，显式加 `--execute`：

```powershell
spindle tools board-report --execute
```

只读诊断允许检查：

- SSH 可达性。
- `/home` 和必要 helper 文件。
- `/dev/rpmsg*`。
- `rpmsg_auto_open.py --print-dev`。
- M7/RPMsg/EtherCAT 相关日志。
- `a53_send_ec_profile --query`。

只读诊断禁止：

- `start-bus`。
- `stop-bus`。
- `0x86` 控制字。
- `0x41F1` 输出门控解锁。
- 机器人运动。
- IO、焊接、限位输出。
- remoteproc 生命周期操作。
- 固件烧写或 M7 热重载。

## 7. 硬件动作审计

任何真实硬件动作前，先执行审计：

```powershell
spindle tools audit-hardware-action "unlock 0x41F1 and move robot axis5"
```

审计结果会说明：

- 当前边界是否为 `offline_ok`。
- 是否需要硬件窗口。
- 是否需要人工确认。
- 需要哪些现场条件。
- 有没有更低风险的替代路径。

## 8. GSD 离线自动化

查看 GSD 状态：

```powershell
spindle tools gsd-status
```

执行离线兜底门禁：

```powershell
spindle tools gsd-offline-run
```

这条命令只覆盖 `offline_ok` 范围：pytest、benchmark、RAG tool-safety、offline stub、secret scan、git diff check 和规划文件检查。它不会执行真实硬件动作。

## 9. 发布前门禁

提交或推送前建议运行：

```powershell
spindle tools secret-scan
spindle check --include-offline --include-rag --rag-type tool_safety
spindle tools pre-push-check --include-offline --include-rag
```

当前稳定版要求：

- pytest 全部通过。
- rules benchmark 全部通过。
- tool safety benchmark 全部通过。
- RAG tool-safety benchmark 全部通过。
- secret-scan 无敏感命中。
- git diff check 无空白错误。

## 10. 维护节奏

Spindle v1.0 稳定期建议按“用 Agent 养 Agent”的节奏推进：

- 每个真实问题都沉淀成 report、LOG、CASE 或 benchmark。
- 不把临时聊天结论直接当 canonical 数据。
- 不急着扩展大功能。
- 累计 10-20 个真实使用 case 后，再评估 v1.1。

v1.1 可能方向包括：

- 更强的 source-grounded RAG。
- Agent SDK wrapper。
- 更多真实 report replay。
- 更细的硬件动作审计。
- 自动生成 benchmark 候选。

## 11. 故障排查

如果命令找不到 `spindle`：

```powershell
python -m pip install -e ".[dev]"
```

如果 RAG 或 benchmark 结果异常：

```powershell
spindle chunks build
spindle benchmark run --engine rules
spindle benchmark run --engine rag --type tool_safety
```

如果开发板只读诊断失败，优先看 `board-report` 中的 diagnosis：

- `ssh_unreachable`：检查电源、网线、IP、SSH。
- `board_scripts_missing`：检查 `/home/a53_send_ec_profile` 和 `/home/rpmsg_auto_open.py`。
- `rpmsg_device_missing`：检查 M7 是否运行、Linux 是否暴露 `/dev/rpmsg*`。
- `m7_remoteproc_error`：保留为固件窗口问题，不要从只读链路热重载。
- `query_failed` / `query_unparsed`：检查 RPMsg endpoint 和 `a53_send_ec_profile --query` 输出。
