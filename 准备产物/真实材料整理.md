# Industrial Embedded Dev Agent 准备产物：真实材料整理

## 0. 说明

- `Industrial Embedded Dev Agent_项目评估.txt` 中的外部链接为 ChatGPT 分享页：`https://chatgpt.com/s/t_69d069c95b748191bffafd2f50b5d425`
- 当前环境下只能读取到页面标题“工业嵌入式开发Agent”，无法直接取回完整正文，因此本次准备工作主要基于本地方案文档和资料目录完成。
- 本清单优先选择与 V1 目标最相关的三类问题域：启动/上板、构建/部署、CANopen/伺服联调。

## 1. 已整理的 10 份调试文档

| ID | 文档 | 类型 | 主题 | 价值 |
|---|---|---|---|---|
| DOC-001 | `资料/TLIMX8MP-EVM 评估板学习过程/TLIMX8MP-EVM_上手指南.docx` | docx | 平台上手、A53/M7 分工、RPMsg、EtherCAT、CANopen | 适合作为 V1 总体知识入口 |
| DOC-002 | `资料/TLIMX8MP-EVM 评估板学习过程/TLIMX8MP-EVM_3月学习总结.docx` | docx | 3 月阶段成果、链路打通情况、未决问题 | 适合作为阶段性复盘与 case 来源 |
| DOC-003 | `资料/TLIMX8MP-EVM 评估板学习过程/3月学习过程.docx` | docx | 具体踩坑、解决动作、环境问题 | 适合作为“历史问题 case”种子 |
| DOC-004 | `资料/3-用户手册/1-1-调试工具安装.pdf` | pdf | 调试工具、串口/环境准备 | 适合作为上手与环境排查知识 |
| DOC-005 | `资料/3-用户手册/1-2-Linux开发环境搭建.pdf` | pdf | Linux 开发环境搭建 | 适合作为构建链路排查知识 |
| DOC-006 | `资料/3-用户手册/2-1-评估板测试手册.pdf` | pdf | 板级验证与测试流程 | 适合作为 bring-up 检查项来源 |
| DOC-007 | `资料/3-用户手册/2-4-GDB程序调试方法说明.pdf` | pdf | GDB 调试流程 | 适合作为建议动作知识库 |
| DOC-008 | `资料/3-用户手册/2-11-Linux-RT系统测试手册.pdf` | pdf | RT 场景测试 | 适合作为实时性问题背景文档 |
| DOC-009 | `资料/3-用户手册/2-12-IgH EtherCAT主站开发案例.pdf` | pdf | EtherCAT 主站开发 | 适合作为总线联调知识源 |
| DOC-010 | `资料/3-用户手册/3-1-Linux系统使用手册.pdf` | pdf | Linux 系统使用与运维 | 适合作为部署和系统服务类问题背景 |

## 2. 已整理的 20 份日志样本

> 说明：这里的“日志样本”按后续 benchmark 和分类训练的最小单元整理，不等同于 20 个原始文件。一个真实文件可以切分出多个可标注样本。

| ID | 来源 | 场景 | 样本摘要 |
|---|---|---|---|
| LOG-001 | `资料/imxSoem-motion-control/build_latest.log` | WSL 构建 | WSL `localhost`/NAT 提示异常，说明 Windows/WSL 互通存在噪声 |
| LOG-002 | `资料/imxSoem-motion-control/build_latest.log` | CMake 配置 | `project()` 应先于 `enable_language()` 的 CMake Warning |
| LOG-003 | `资料/imxSoem-motion-control/build_latest.log` | 编译质量 | `nicdrv.c: warning: variable 'lp' set but not used` |
| LOG-004 | `资料/imxSoem-motion-control/build_latest.log` | 编译质量 | `weld_current.c: warning: unused variable 'down_time'` |
| LOG-005 | `资料/imxSoem-motion-control/build_latest.log` | 编译质量 | `weld_parameter.c: warning: function defined but not used` |
| LOG-006 | `资料/imxSoem-motion-control/build_latest.log` | 编译质量 | `libtpr20pro.c: warning: unused variable 'd5'` |
| LOG-007 | `资料/imxSoem-motion-control/build_latest.log` | RPMsg 链路 | `rpmsg_loop.c: variable 'result' set but not used` |
| LOG-008 | `资料/imxSoem-motion-control/build_latest.log` | EtherCAT 主循环 | `ethercat_loop.c: warning: 'wkc' set but not used` |
| LOG-009 | `资料/imxSoem-motion-control/build_latest.log` | EtherCAT 主循环 | `simpletest_1`/`csp_setup_1` 未使用，提示实验代码未清理 |
| LOG-010 | `资料/imxSoem-motion-control/build_latest.log` | 构建上下文 | `Building ddr_release`，可作为 DDR 版编译成功上下文样本 |
| LOG-011 | `资料/TLIMX8MP-EVM 评估板学习过程/3月学习过程.docx` | 串口调试 | 同一份代码第二天 RS232 UART3 不打印信息 |
| LOG-012 | `资料/TLIMX8MP-EVM 评估板学习过程/3月学习过程.docx` | 镜像部署 | BOOT 区放入新 `m7_app.bin` 后仍运行旧镜像 |
| LOG-013 | `资料/TLIMX8MP-EVM 评估板学习过程/3月学习过程.docx` | DDR/TCM 选择 | TCM 版本 boot 区空间不足导致代码溢出，改走 DDR |
| LOG-014 | `资料/TLIMX8MP-EVM 评估板学习过程/3月学习过程.docx` | RPC 触发链路 | `ethercat_loop_task` 因等待 A 核命令无法进入 |
| LOG-015 | `资料/TLIMX8MP-EVM 评估板学习过程/3月学习过程.docx` | 新工程构建 | 新 SDK 工程 DDR 版本无法跑通，原因是未选 `ddr_release Default` |
| LOG-016 | `资料/imxSoem-motion-control/tmp_worklog_2026_04_01/word/document.xml` | CANopen 参数验证 | 通过 SDO 读取 `0x6041/0x6064/0x606C/0x6061` 做对象字典诊断 |
| LOG-017 | `资料/imxSoem-motion-control/tmp_worklog_2026_04_01/word/document.xml` | 无串口回归 | 增加 `verify_robot_motion.py` + `build_deploy_verify_robot_no_serial.ps1` 做 SSH 验证 |
| LOG-018 | `资料/imxSoem-motion-control/tmp_worklog_2026_04_01/word/document.xml` | RPDO2 联调 | 发送 `RPDO2` 数据后稳定复现 `statusCode=21559 (0x5437)` |
| LOG-019 | `资料/imxSoem-motion-control/tmp_worklog_2026_04_03/word/document.xml` | 双轴联调 | `axis1/node2` 可恢复到可运行状态，编码器持续变化 |
| LOG-020 | `资料/imxSoem-motion-control/tmp_worklog_2026_04_03/word/document.xml` | 参数未锁存 | `axis0/node1` 中 `0x6083/0x6084` 保持旧值、`0x60FF` 保持 0，目标速度未真正写入 |

## 3. 已整理的 10 个历史问题 case

| ID | 问题现象 | 初步归因 | 已验证动作 | 建议标签 |
|---|---|---|---|---|
| CASE-001 | RS232 UART3 第二天不打印信息 | 物理连接/串口工具配置漂移 | 重新插拔 RS232，重新设置 MobaXterm | `serial_no_output` |
| CASE-002 | 新 `m7_app.bin` 放入 BOOT 区后仍运行旧程序 | 部署缓存/未完全重启 | 重启终端软件，重新拷贝，开关机重启板卡 | `stale_boot_artifact` |
| CASE-003 | TCM 版本工程容易溢出 | TCM 启动区空间不足 | 改用 DDR 版本镜像 | `memory_layout_mismatch` |
| CASE-004 | `ethercat_loop_task` 无法进入 | 依赖 A 核命令触发，链路未就绪 | 将函数提到判断前直接运行调试 | `rpc_trigger_missing` |
| CASE-005 | 新 SDK 工程 DDR 跑不通 | 构建配置选错 | 勾选 `ddr_release Default` | `build_profile_mismatch` |
| CASE-006 | `axis0/node1` 进入速度模式但电机不转 | `0x6083/0x6084/0x60FF` 未真正锁入驱动器 | SDO 强制写入、最小化隔离测试 | `canopen_param_not_latched` |
| CASE-007 | `axis1/node2` 可以正常运行，`axis0/node1` 不稳定 | 双轴行为差异，说明问题更像业务链路或参数来源差异 | 单轴/双轴对比、对象字典回读 | `axis_asymmetry` |
| CASE-008 | `RPDO2` 一旦发数据就复现 `0x5437` | PDO 在线更新触发驱动器异常 | 阶段性只保留 RPDO2 映射，不在线下发该数据 | `pdo_online_update_risk` |
| CASE-009 | COM3 不可用时难以持续联调 | 调试链路过度依赖串口 | 引入 SSH 无串口验证脚本 | `serial_dependency` |
| CASE-010 | 编译链路出现大量 warning | 实验代码未清理、变量未使用、链路脏数据 | 用 build log 做静态样本清洗与质量门禁 | `build_hygiene_issue` |

## 4. 已整理的 5 个常用脚本

| ID | 脚本 | 路径 | 作用 |
|---|---|---|---|
| SCRIPT-001 | `tmp_axis1_delay_probe.py` | `资料/imxSoem-motion-control/tmp_axis1_delay_probe.py` | 对 axis1 执行速度模式下发并轮询编码器/状态/错误码 |
| SCRIPT-002 | `tmp_axis_command_and_probe.py` | `资料/imxSoem-motion-control/tmp_axis_command_and_probe.py` | 同时给双轴下发命令并读取 AType、使能、状态、编码器 |
| SCRIPT-003 | `tmp_axis_probe.py` | `资料/imxSoem-motion-control/tmp_axis_probe.py` | 单轴探针，验证 `SetTargetVel/SetAccel/SetDecel` 后反馈变化 |
| SCRIPT-004 | `tmp_probe_can_heartbeat.py` | `资料/imxSoem-motion-control/tmp_probe_can_heartbeat.py` | 只读式 CAN/轴状态快照采集，适合低风险巡检 |
| SCRIPT-005 | `tmp_verify_acc_dec.py` | `资料/imxSoem-motion-control/tmp_verify_acc_dec.py` | 验证 Acc/Dec/状态码/编码器变化，适合定位参数是否锁存 |

## 5. 建议的后续入库方式

1. 文档优先入库 `DOC-001` ~ `DOC-010`，因为它们最接近知识问答需求。
2. 日志优先按 `LOG-011` ~ `LOG-020` 做高价值标注，这些更像真实联调问题，不只是编译 warning。
3. case 优先按 `CASE-006` ~ `CASE-009` 做标准化复盘，因为它们最能体现“工业味道”。
4. 脚本优先对白名单化 `SCRIPT-002` 和 `SCRIPT-004`，它们更接近后续 L0/L1 工具调用能力。
