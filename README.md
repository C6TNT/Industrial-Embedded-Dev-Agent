# Spindle

## Public Offline Acceptance Snapshot

- Standalone fake harness: acceptance PASS `5/5`, pytest `22 passed`, fixture dry-run `40 planned / 0 copied`, schema drift `5 docs / 10 profiles / 0 errors`, XML batch `3/3 PASS`, replay batch `15/15 PASS`.
- Huichuan runtime mirror: static profile `16/16 PASS`, acceptance PASS `5/5`, pytest inside acceptance `29 passed`, fixture dry-run `40 noop / 0 copied`, schema drift `5 docs / 10 profiles / 0 errors`, XML batch `3/3 PASS`, replay batch `15/15 PASS`.
- Public material source: `data/materials/offline_acceptance_evidence_2026_04_28.md`.
- Boundary: this evidence is `offline_ok` only and does not approve board, bus, output gate, IO, firmware, or robot-motion actions.

Spindle 是一个面向工业嵌入式研发现场的工程 Agent 项目。名字取自主轴，强调它服务的核心场景：机器人、伺服、电机、运动控制、工业总线和真实调试链路。

这个仓库当前围绕 `i.MX8MP A53 + RPMsg + M7 + EtherCAT Dynamic Profile / robot6 位置模式 / Fake Harness` 建立一套可落地的工程助手能力：资料检索、日志归因、工具安全边界、离线回归、报告沉淀和开发板只读诊断。

## GitHub 展示快照

- 当前主线：EtherCAT Dynamic Profile、robot6 位置模式、fake harness 离线回归和硬件动作安全审计。
- 最新离线证据：standalone fake harness `17 passed`，XML batch `3/3 PASS`，replay batch `15/15 PASS`。
- 真实工程离线同步：Huichuan runtime 本地 pytest `24 passed`，static profile `16/16 PASS`，XML batch `3/3 PASS`，replay batch `15/15 PASS`。
- 安全护栏：`ssh/scp/reboot`、`start-bus`、`stop-bus`、`0x86`、`0x41F1`、IO output、remoteproc、robot motion 会先被硬件边界拦截，不能被 fake harness/replay 等低风险关键词降级。
- 公开仓库只沉淀可审查材料、benchmark、RAG chunks 和安全规则，不导入真实硬件私有源码、凭据或原始现场日志。

## 当前定位

Spindle 不是完整自治硬件控制系统，也不是重型知识库。它当前定位为 EtherCAT/机器人调试项目的研发副驾：

- 用 raw search、curated memory 和 benchmark 管理真实工程资料。
- 用规则归因和 RAG 辅助分析 profile、PDO、RPMsg、M7、robot6、fake harness 等问题。
- 用安全工具层区分 `offline_ok`、`board_required`、`robot_motion_required`、`io_required`、`firmware_required`。
- 用 pre-push、secret-scan、pytest、benchmark 和 GSD offline gate 保持仓库可交付。

## 能力概览

- `material-status` / `material-search`：检索西厂工程资料、源码、文档、XML、profile、日志和工具脚本。
- `project-status`：查看当前项目基线、benchmark、chunks、git 状态和硬件边界。
- `draft-fact` / `material-draft-fact`：把新测试结论生成可审查材料草稿，避免直接污染 canonical 数据。
- `import-report` / `summarize-fake-regression`：把真实 report 或 fake harness 回归结果沉淀成 LOG/replay/材料草稿。
- `audit-hardware-action`：审计真实板卡、机器人、IO、固件相关动作，给出人工确认条件和低风险替代路径。
- `board-status` / `rpmsg-health` / `m7-health` / `ethercat-query-readonly` / `board-report`：开发板只读诊断，默认 dry-run，`--execute` 才会 SSH。
- `gsd-status` / `gsd-offline-run`：按 `.planning` 边界执行离线自动化检查。

## 快速开始

安装开发依赖：

```powershell
python -m pip install -e ".[dev]"
```

查看项目状态：

```powershell
spindle tools project-status
spindle tools material-status
spindle tools gsd-status
```

搜索工程资料：

```powershell
spindle tools material-search "0x41F1" --scope docs
spindle tools material-search "axis1 12/28" --scope all
```

运行本地门禁：

```powershell
spindle check --include-offline --include-rag --rag-type tool_safety
spindle tools pre-push-check --include-offline --include-rag
```

发布前检查敏感信息：

```powershell
spindle tools secret-scan
```

## 开发板只读诊断

开发板诊断默认只生成计划，不连接板卡：

```powershell
spindle tools board-status
spindle tools rpmsg-health
spindle tools m7-health
spindle tools ethercat-query-readonly
spindle tools board-report
```

确认当前只有开发板只读窗口、不会影响机器人/IO/固件后，再显式加 `--execute`：

```powershell
spindle tools board-report --execute
```

这条链路只允许 SSH 只读检查、RPMsg 设备/日志检查、M7 日志检查和 `a53_send_ec_profile --query`。它禁止 `start-bus`、`stop-bus`、`0x86`、`0x41F1`、机器人运动、IO 输出、remoteproc 生命周期和固件热重载。

## 真实问题沉淀流程

后续每次 EtherCAT/机器人项目出现新问题，按这个闭环处理：

1. 用 `material-search` 或 `ask` 查原始资料和历史 case。
2. 用 `audit-hardware-action` 判断是否跨越硬件边界。
3. 能离线验证的先跑 fake harness、report import 或 board dry-run。
4. 把真实 report 或测试结论用 `import-report`、`summarize-fake-regression`、`draft-fact` 生成草稿。
5. 审查后再更新材料索引、benchmark、chunks 和回归样例。

当前策略是先累计 10-20 个真实使用 case，再决定是否进入 v1.1，例如更强的知识库、Agent SDK wrapper 或更复杂的自动执行层。

## 安全边界

Spindle 默认只自动执行 `offline_ok` 工作。以下动作不能自动执行，只能生成计划、审计或清单：

- 真实 `ssh/scp/reboot`。
- `start-bus` / `stop-bus`。
- `0x86` 控制字。
- `0x41F1` 输出门控解锁。
- 机器人运动。
- IO、焊接、限位输出。
- remoteproc 生命周期、固件烧写、M7 热重载。

硬件动作统一先跑：

```powershell
spindle tools audit-hardware-action "unlock 0x41F1 and move robot axis5"
```

## 文档

- [Spindle 使用手册](docs/spindle_user_manual.md)
- [项目路线图](docs/roadmap.md)
- [GSD 自动化边界](.planning/GSD_BOUNDARY.md)

## 当前状态

Spindle 当前进入 v1.0 稳定期。短期目标不是继续堆功能，而是作为可展示工程交付到 GitHub，并在真实 EtherCAT/机器人项目中持续使用、记录和回归。

推荐提交前门禁：

```powershell
spindle tools pre-push-check --include-offline --include-rag
```
