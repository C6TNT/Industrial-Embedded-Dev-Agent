# Industrial Embedded Dev Agent 真实材料整理 V2

## 0. 说明

- 当前训练资料主线已经从双驱 CANopen 切换为 EtherCAT Dynamic Profile、汇川六轴 robot6 位置模式联调、动态 runtime 接管和 fake harness 离线回归。
- 早期 CANopen 双驱资料保留在历史资料中，但不再作为 V1 Agent 的主要训练基线。
- 当前资料优先服务四类问题域：profile/PDO/拓扑解释、robot6 位置模式回归、fake harness 离线验证、M7/RPMsg/EtherCAT 生命周期安全边界。

## 1. 已整理的核心文档

| ID | 文档 | 类型 | 主题 | 价值 |
|---|---|---|---|---|
| CUR-001 | `data/materials/current_ethercat_dynamic_profile_project_v1.md` | md | 当前 EtherCAT 动态 profile 项目总览 | 当前训练基线入口 |
| CUR-002 | `D:/桌面/test/mix_protocol/docs/dynamic_profile_test_chain.md` | md | XML/profile/单驱测试链路 | 单驱三厂商回归基线 |
| CUR-003 | `D:/桌面/test/mix_protocol/docs/robot_memcpy_takeover_audit.md` | md | 旧 memcpy 路径和动态接管梳理 | 动态 runtime 覆盖旧链路的审计入口 |
| CUR-004 | `D:/桌面/test/mix_protocol/docs/ethercat_fake_harness_design.md` | md | fake harness 设计 | 离线仿真架构与边界 |
| CUR-005 | `D:/桌面/test/mix_protocol/docs/ethercat_fake_harness_manual.md` | md | fake harness 使用手册 | 回归命令和 report 解读 |
| CUR-006 | `D:/桌面/test/mix_protocol/docs/m7_hot_reload_validation_result_2026-04-27.md` | md | M7 热重载验证结果 | remoteproc 风险结论 |
| CUR-007 | `D:/桌面/test/mix_protocol/docs/io_hardware_return_test_playbook_2026-04-27.md` | md | IO 设备回归方案 | IO/焊接节点后续验证边界 |
| CUR-008 | `docs/current_ethercat_agent_development_guide.md` | md | 当前 Agent 离线开发说明 | 无板开发边界、回归命令和硬件停止条件 |

## 2. 已整理的日志与报告样本

| ID | 来源 | 场景 | 样本摘要 |
|---|---|---|---|
| LOG-001 | `tools/generated/board_reports/20260423_141425_inovance.json` | 汇川单驱 | SV660N 单驱动态 profile 链路通过，`ob=7`，`ib=13` |
| LOG-002 | `tools/generated/board_reports/20260423_142131_kinco.json` | 步科单驱 | Kinco FD remap profile 链路通过，`ob=7`，`ib=13` |
| LOG-003 | `tools/generated/board_reports/20260423_140443_yako.json` | YAKO 单驱 | YAKO MS remap profile 链路通过，`ob=7`，`ib=13` |
| LOG-004 | `tools/generated/inovance_robot6_observed_topology_profile.json` | robot6 topology | 六个 SV660N 从站拓扑，axis1/slave2 为 `12/28` 位置型变体 |
| LOG-005 | `tools/generated/dynamic_axis_probe_axis5_position_cw86_v2.json` | 单轴位置模式 | axis5 低风险轴位置模式回归通过 |
| LOG-006 | `tools/generated/dynamic_axis_probe_axis0_position_cw86_v1.json` | 单轴位置模式 | axis0 位置模式回归通过，低速小步长验证 |
| LOG-007 | `tools/generated/dynamic_dual_axis_probe_axis5_axis4_position_v1.json` | 双轴位置模式 | axis5+axis4 位置联调通过 |
| LOG-008 | `tools/generated/dynamic_dual_axis_probe_axis1_axis0_position_v2.json` | 双轴位置模式 | axis1+axis0 位置联调通过 |
| LOG-009 | `tools/generated/dynamic_triple_axis_probe_axis5_axis4_axis3_position_v2.json` | 三轴位置模式 | axis5+axis4+axis3 位置联调通过 |
| LOG-010 | `tools/generated/dynamic_triple_axis_probe_axis2_axis1_axis0_position_v1.json` | 三轴位置模式 | axis2+axis1+axis0 位置联调通过 |
| LOG-011 | `tools/generated/dynamic_six_axis_position_probe_v1.json` | 六轴位置模式 | 六轴同时 `+200 dec` 位置模式联调通过 |
| LOG-012 | `tools/generated/fake_ecat_harness_regression_latest.json` | fake matrix | 14 个 fake harness 场景通过 |
| LOG-013 | `tools/generated/fake_ecat_harness_xml_regression_latest.json` | XML batch | 汇川、步科、YAKO XML 样本批量回归通过 |
| LOG-014 | `tools/generated/fake_ecat_harness_replay_batch_latest.json` | replay batch | 15 个真实 report replay case 通过 |
| LOG-015 | `tools/generated/dynamic_runtime_acceptance/20260427_140030/dynamic_runtime_acceptance.json` | 接管准备 | stop/start、IO 只读、IO dry-run 对照形成阶段性验收 |

## 3. 已整理的历史问题 case

| ID | 问题现象 | 初步归因 | 已验证动作 | 建议标签 |
|---|---|---|---|---|
| CASE-001 | profile loaded 但 applied=0 | profile apply 失败或字段不完整 | fake harness `applied_failed` 场景覆盖 | `profile_schema_mismatch` |
| CASE-002 | query 显示 slaves=0 | EtherCAT 从站未识别或总线未进入有效扫描 | fake harness `slaves_zero` 场景覆盖 | `slaves_zero` |
| CASE-003 | inOP=0 或 task=0 | 总线未进入 OP 或 M7 EtherCAT task 未运行 | query/stop/start 稳定性检查 | `not_in_op` |
| CASE-004 | actual_position 不变化 | 动作被拦截、未运行或反馈未更新 | `no_motion` 场景和 report 对比 | `no_motion_feedback` |
| CASE-005 | 0x41F1 未解锁时动作被拦截 | 动态输出门控生效 | `gate_locked` 场景通过 | `gate_locked` |
| CASE-006 | axis1/slave2 与其他轴 ob/ib 不同 | robot6 中 axis1 是 `12/28` 位置型变体 | 单独写入真实 topology/profile 并对照 | `pdo_ob_ib_mismatch` |
| CASE-007 | stop-bus -> start-bus 偶发卡死 | M7 EtherCAT task、RPMsg endpoint 或网口状态未清理 | 固化 RPMsg 清理和 stop/start 回归 | `rpmsg_endpoint_stale` |
| CASE-008 | remoteproc 热重载 DDR ELF 失败 | ELF/linker 不兼容 Linux remoteproc loader | 记录 `bad phdr da 0x80000000`，回到 .bin + reboot | `remoteproc_elf_incompatible` |
| CASE-009 | IO 节点无法真实输出验证 | 机器人/IO 设备被同事占用或缺少真实设备窗口 | 先做只读 profile 和 dry-run 对照 | `io_hardware_unavailable` |
| CASE-010 | 旧 memcpy 与动态 runtime 并存 | 动态链路尚未完全接管旧链路 | 先只读并行对照，再逐项切换输出 | `runtime_takeover_pending` |

## 4. 已整理的工具与脚本

| ID | 脚本 | 路径 | 作用 |
|---|---|---|---|
| SCRIPT-001 | `xml_to_ec_profile.py` | `D:/桌面/test/mix_protocol/tools/xml_to_ec_profile.py` | 从 XML/ESI 生成 JSON profile |
| SCRIPT-002 | `a53_send_ec_profile` | `D:/桌面/test/mix_protocol/tools/a53_send_ec_profile.cpp` | A53 侧 profile 下发和 query/start/stop 入口 |
| SCRIPT-003 | `robot_single_axis_dynamic_position_probe.py` | `D:/桌面/test/mix_protocol/tools/robot_single_axis_dynamic_position_probe.py` | robot6 单轴位置模式验证 |
| SCRIPT-004 | `robot_dual_axis_dynamic_position_probe.py` | `D:/桌面/test/mix_protocol/tools/robot_dual_axis_dynamic_position_probe.py` | robot6 双轴位置模式验证 |
| SCRIPT-005 | `robot_triple_axis_dynamic_position_probe.py` | `D:/桌面/test/mix_protocol/tools/robot_triple_axis_dynamic_position_probe.py` | robot6 三轴位置模式验证 |
| SCRIPT-006 | `robot_six_axis_dynamic_position_probe.py` | `D:/桌面/test/mix_protocol/tools/robot_six_axis_dynamic_position_probe.py` | robot6 六轴位置模式验证 |
| SCRIPT-007 | `fake_ecat_harness.py` | `D:/桌面/test/mix_protocol/tools/fake_ecat_harness/fake_ecat_harness.py` | 离线 fake M7/fake EtherCAT report 生成 |
| SCRIPT-008 | `run_fake_harness_regression.py` | `D:/桌面/test/mix_protocol/tools/fake_ecat_harness/run_fake_harness_regression.py` | fake matrix 一键回归 |
| SCRIPT-009 | `run_xml_profile_batch_regression.py` | `D:/桌面/test/mix_protocol/tools/fake_ecat_harness/run_xml_profile_batch_regression.py` | XML 真实样本批量回归 |
| SCRIPT-010 | `run_replay_batch.py` | `D:/桌面/test/mix_protocol/tools/fake_ecat_harness/run_replay_batch.py` | 真实 report replay 批量回归 |

## 5. 建议的后续入库方式

1. 新增真实机器人测试结果时，优先追加到 `LOG-*` 和 replay case。
2. 新增 XML/ESI 样本时，必须补 XML batch regression 和 fake harness scenario。
3. 新增 IO/焊接节点时，先补只读 profile 描述，再补 dry-run 对照，最后等设备窗口做真实输出验证。
4. 新增高风险动作样本时，优先补 `tool_safety` benchmark，确保 Agent 默认不会自动执行。

## 6. 2026-04-28 Public Offline Acceptance Evidence

| ID | Document | Type | Topic | Value |
|---|---|---|---|---|
| CUR-011 | `data/materials/offline_regression_baseline_2026_04_28.md` | md | Offline regression baseline | Fake harness and Huichuan regression counts |
| CUR-012 | `data/materials/offline_acceptance_evidence_2026_04_28.md` | md | Offline acceptance evidence | Public latest acceptance status and hardware boundary |
| SCRIPT-011 | `run_offline_acceptance.py` | py | One-command offline acceptance | Runs pytest, fixture dry-run, schema drift, XML batch, and replay batch |

CUR-012 is the preferred public-facing source for the latest green offline
acceptance state. It records standalone fake harness acceptance PASS 6/6 with
24 pytest cases, SOEM trace batch 3/3, and Huichuan runtime mirror acceptance
PASS 5/5 with 29 pytest cases, static profile 16/16, 40 noop fixture refresh
entries, schema drift 5 documents / 10 profiles / 0 errors, XML batch 3/3, and
replay batch 15/15.

## 7. Public Demo Q&A

| ID | Document | Type | Topic | Value |
|---|---|---|---|---|
| CUR-013 | `docs/demo_offline_acceptance_qa.md` | md | Public demo Q&A | Try-these-questions script for offline acceptance and safety-boundary answers |

CUR-013 is the public demo companion for CUR-012. It lists expected questions,
answer shape, preferred sources, and the `offline_ok` boundary so the GitHub
project can show the current acceptance state without exposing private hardware
details.

## 8. Public Frozen Baseline

| ID | Document | Type | Topic | Value |
|---|---|---|---|---|
| CUR-014 | `docs/frozen_baseline_2026_04_28.md` | md | Public frozen baseline | GitHub-facing frozen baseline, verification gate, tag, and safety boundary |

CUR-014 freezes the public Agent demo baseline after the local fake-harness and
Huichuan offline evidence were stabilized. It is an `offline_ok` reference point
and explicitly does not block later `board_required` real-device testing.

## 9. Sanitized Legacy Material Imports

| ID | Document | Type | Topic | Value |
|---|---|---|---|---|
| CUR-015 | `data/materials/legacy_materials_ingestion_policy_2026_04_28.md` | md | Legacy ingestion policy | Public-safe rules for importing old documents and old engineering trees |
| CUR-016 | `data/materials/legacy_canopen_rpmsg_lessons_2026_04_28.md` | md | Legacy CANopen/RPMsg lessons | A53/M7/RPMsg/FlexCAN/CANopen lessons distilled from the old dual-drive chain |
| CUR-017 | `data/materials/ethercat_dynamic_profile_doc_timeline_2026_04_28.md` | md | Documentation timeline | Private-doc timeline from CANopen to EtherCAT Dynamic Profile, robot position mode, and fake harness |

CUR-015 through CUR-017 are sanitized summaries. They intentionally do not copy
raw legacy source, build artifacts, `.git`, `.vscode`, deployment scripts, board
addresses, credentials, raw logs, or vendor manuals into the public Agent
project.

## 10. SOEM Trace Regression

| ID | Document | Type | Topic | Value |
|---|---|---|---|---|
| CUR-018 | `data/materials/soem_trace_regression_2026_04_28.md` | md | SOEM trace regression | Sanitized SOEM text evidence to profile candidate, validator, and fake replay |
| SCRIPT-012 | `soem_trace_parser.py` | py | SOEM trace parser | Parses sanitized SOEM text into profile candidate evidence |
| SCRIPT-013 | `run_soem_trace_batch.py` | py | SOEM trace batch | Runs trace parsing, profile validation, and fake replay for three sample drivers |

CUR-018 records the `offline_ok` SOEM-adjacent evidence bridge. It is useful
when answering how new driver adaptation can use SOEM logs without claiming that
offline evidence authorizes live board, bus, IO, gate, or robot actions.

## 11. Test Double And Verification Layers

| ID | Document | Type | Topic | Value |
|---|---|---|---|---|
| CUR-019 | `data/materials/test_double_verification_layers_2026_04_29.md` | md | Test double and verification layers | Public-safe fake/mock/stub definitions and the pre-real-test gate for new drivers or IO modules |

CUR-019 defines stub, fake, and mock for this platform, then fixes the required
gate before real Huichuan testing: sanitized trace/replay fixture, fake
regression, Agent safety Q&A/tool gate, then approved `board_required` or
`io_required` execution.
