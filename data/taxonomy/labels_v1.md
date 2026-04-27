# Industrial Embedded Dev Agent 标签体系 V2

## 1. 问题大类标签

| 标签 | 含义 | 典型信号 |
|---|---|---|
| `bringup_boot` | 上电、启动、镜像加载、启动介质问题 | 旧镜像仍运行、BOOT 分区异常 |
| `build_config` | 编译配置、工具链、CMake/WSL 环境问题 | `ddr_release` 选错、构建 warning、路径噪声 |
| `m7_deploy` | M7 固件部署与热重载问题 | `.bin` 未生效、remoteproc 失败、`bad phdr da` |
| `rpc_rpmsg` | A53/M7 核间通信与 endpoint 生命周期问题 | `OpenRpmsg` 失败、endpoint stale、query 超时 |
| `ethercat_profile` | XML/ESI、JSON profile、profile apply 问题 | `loaded=1 applied=0`、driver/strategy 字段异常 |
| `pdo_layout` | PDO 字节布局、ob/ib、entry offset 问题 | `19/13`、`12/28`、offset 错位、对象缺失 |
| `robot6_position` | 汇川六轴 robot6 位置模式联调问题 | 单轴/双轴/三轴/六轴位置模式 report 异常 |
| `runtime_takeover` | 动态 runtime 接管旧 memcpy 路径问题 | 旧链路和动态链路并行、状态镜像不一致 |
| `fake_harness` | 离线 fake harness 回归问题 | fake report fail、scenario/replay/XML batch 异常 |
| `io_profile` | STM32F767、GELIIO、IO/焊接节点纳入 profile 问题 | IO 只读对照、dry-run 输出、真实设备待验证 |
| `verification_tooling` | 验证脚本、回归脚本、报告沉淀问题 | report 缺字段、replay case 不完整、pytest 失败 |
| `safety` | 工业现场安全边界问题 | 0x86、0x41F1、机器人运动、IO 输出、刷固件 |

## 2. 原因标签

| 标签 | 含义 | 备注 |
|---|---|---|
| `boot_artifact_stale` | BOOT 区文件已更新但实际运行旧镜像 | 常见于未重启或部署路径错误 |
| `build_profile_mismatch` | 编译 profile 选错 | 如 DDR/TCM、debug/release 混淆 |
| `remoteproc_elf_incompatible` | 当前 ELF/linker 与 Linux remoteproc loader 不兼容 | 已见 `bad phdr da 0x80000000` |
| `rpmsg_endpoint_stale` | stop/start 后 RPMsg endpoint 或 M7 task 残留 | 需要清理 endpoint 和任务状态 |
| `profile_schema_mismatch` | profile 字段缺失、类型不一致或兼容层未覆盖 | `slave_id/logical_axis/driver_type/strategy` |
| `pdo_ob_ib_mismatch` | profile entry 总长度与 ob/ib 或旧结构体不一致 | 常见于 `19/13` 与 `12/28` 变体 |
| `axis_mapping_mismatch` | logical_axis、slave_id、role 或实际从站顺序不一致 | robot6 多轴最需要关注 |
| `gate_locked` | `0x41F1` 输出门控未解锁 | 动态控制写入应被拦截 |
| `applied_failed` | profile 已 loaded 但 apply 失败 | fake harness 和 query 应能识别 |
| `slaves_zero` | 从站数为 0 | 总线未识别或扫描失败 |
| `not_in_op` | EtherCAT 未进入 OP 或 task 未运行 | `inOP=0` / `task=0` |
| `no_motion_feedback` | 位置/速度反馈未变化 | 需要确认门控、状态机和实际输出 |
| `io_hardware_unavailable` | IO 或焊接真实设备窗口不可用 | 只能先做只读和 dry-run |
| `runtime_takeover_pending` | 动态 runtime 尚未完全替换旧 memcpy 路径 | 需要先只读对照再切写路径 |
| `verification_gap` | 证据链不完整 | 需要补 report、replay、query 或 XML 检查 |

## 3. 建议动作标签

| 标签 | 含义 | 典型操作 |
|---|---|---|
| `collect_boot_context` | 采集启动上下文 | BOOT 文件、设备树、reboot 方式、串口输出 |
| `verify_build_profile` | 校验构建配置 | `ddr_release`、工具链版本、输出文件路径 |
| `keep_boot_partition_reboot` | 使用 `.bin + board reboot` 安全部署路径 | 当前 remoteproc 热重载不作为默认路径 |
| `probe_rpmsg_handshake` | 检查 RPMsg 通道和 endpoint | `rpmsg_auto_open.py`、`--query` |
| `stop_start_stability_check` | 做 start/query/stop/query 连续稳定性检查 | 目标是减少 reboot 依赖 |
| `query_dynamic_profile` | 查询动态 profile 状态 | `loaded/applied/slaves/inOP/task/ob/ib` |
| `compare_pdo_layout` | 做 PDO 字节级对比 | entry offset、bit length、ob/ib |
| `readonly_compare_before_write` | 写路径切换前先只读对照 | 旧 memcpy 与动态 runtime 并行读 |
| `run_fake_harness_regression` | 跑 fake matrix | 三单驱、robot6、fault、sequence |
| `run_xml_batch_regression` | 跑 XML 真实样本批量回归 | 汇川、步科、YAKO XML |
| `run_replay_batch` | 跑真实 report replay 回归 | 将现场问题沉淀为离线 case |
| `gate_unlock_check` | 检查 0x41F1 门控状态 | 未解锁前禁止动态写入 |
| `capture_report` | 保存测试报告 | XML、profile、query、status、error、position |
| `summarize_case_and_baseline` | 沉淀 case 与稳定基线 | 给后续 Agent 做知识/评测闭环 |

## 4. 风险等级标签

| 标签 | 含义 | 允许范围 | 示例 |
|---|---|---|---|
| `L0_readonly` | 只读、观测、问答、离线分析 | 默认允许 | 查文档、query、profile 静态校验、fake harness |
| `L1_low_risk_exec` | 低风险执行 | 需显式确认或审批 | 离线 pytest、只读采集、日志整理 |
| `L2_high_risk_exec` | 高风险执行 | 默认拒绝自动执行 | 发 0x86、解锁 0x41F1、机器人运动、IO 输出、刷固件、remoteproc 热重载 |

## 5. 推荐的标签组合模板

### 5.1 profile/PDO 类

- 问题大类：`ethercat_profile` / `pdo_layout`
- 原因标签：`profile_schema_mismatch` / `pdo_ob_ib_mismatch` / `axis_mapping_mismatch`
- 建议动作：`query_dynamic_profile` / `compare_pdo_layout` / `run_xml_batch_regression`
- 风险等级：`L0_readonly`

### 5.2 robot6 位置模式类

- 问题大类：`robot6_position`
- 原因标签：`axis_mapping_mismatch` / `gate_locked` / `no_motion_feedback`
- 建议动作：`capture_report` / `gate_unlock_check` / `readonly_compare_before_write`
- 风险等级：只读为 `L0_readonly`，运动或使能为 `L2_high_risk_exec`

### 5.3 fake harness 类

- 问题大类：`fake_harness` / `verification_tooling`
- 原因标签：`verification_gap` / `applied_failed` / `slaves_zero` / `not_in_op`
- 建议动作：`run_fake_harness_regression` / `run_xml_batch_regression` / `run_replay_batch`
- 风险等级：通常为 `L0_readonly` 或 `L1_low_risk_exec`

### 5.4 动态接管类

- 问题大类：`runtime_takeover` / `io_profile`
- 原因标签：`runtime_takeover_pending` / `io_hardware_unavailable`
- 建议动作：`readonly_compare_before_write` / `compare_pdo_layout` / `capture_report`
- 风险等级：只读对照为 `L0_readonly`，输出切换为 `L2_high_risk_exec`

## 6. V2 建议优先支持的标签集合

1. 问题大类优先：`ethercat_profile`、`pdo_layout`、`robot6_position`、`fake_harness`、`rpc_rpmsg`
2. 原因标签优先：`profile_schema_mismatch`、`pdo_ob_ib_mismatch`、`axis_mapping_mismatch`、`rpmsg_endpoint_stale`
3. 建议动作优先：`query_dynamic_profile`、`compare_pdo_layout`、`run_fake_harness_regression`、`readonly_compare_before_write`
4. 风险控制优先：严格拦截所有 `L2_high_risk_exec`
