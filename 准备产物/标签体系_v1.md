# Industrial Embedded Dev Agent 标签体系 V1

## 1. 问题大类标签

| 标签 | 含义 | 典型信号 |
|---|---|---|
| `bringup_boot` | 上电、启动、镜像加载、启动介质问题 | 无输出、旧镜像仍运行、BOOT 分区异常 |
| `serial_console` | 串口链路与终端工具问题 | UART 无打印、串口切换、CH340/RS232 混淆 |
| `build_config` | 编译配置、工具链、CMake/WSL 环境问题 | `ddr_release` 选错、CMake Warning、WSL 提示 |
| `deploy_runtime` | 部署后未生效、服务未启动、脚本误判 | reboot 后状态不一致、SSH 时序问题 |
| `rpc_rpmsg` | A53/M7 核间通信问题 | OpenRpmsg 失败、任务等待 A 核命令、收发不通 |
| `canopen_link` | CAN/CANopen 总线链路问题 | NMT/PDO/SDO 不通、状态字异常、对象字典不一致 |
| `servo_state_machine` | 驱动器状态机/使能链路问题 | `0x6040/0x6041` 不匹配、无法进运行态 |
| `motion_param_latch` | 速度/加减速/模式参数未锁存 | `0x6083/0x6084/0x60FF` 不变、目标速度无效 |
| `pdo_mapping` | PDO 映射、在线更新、同步链路问题 | RPDO1/RPDO2 行为差异、发包后即异常 |
| `verification_tooling` | 验证脚本、回归脚本、采集手段问题 | COM3 依赖、无串口验证、探针脚本失效 |

## 2. 原因标签

| 标签 | 含义 | 备注 |
|---|---|---|
| `boot_artifact_stale` | BOOT 区文件已更新但实际运行旧镜像 | 常见于未重启/缓存未清理 |
| `serial_connection_drift` | 串口物理连接或串口软件配置漂移 | RS232 拔插、端口号变化 |
| `build_profile_mismatch` | 编译 profile 选错 | 如 DDR/TCM、debug/release 混淆 |
| `toolchain_path_noise` | WSL/Windows 路径映射、工具链路径脏数据 | 构建环境类问题 |
| `rpc_trigger_missing` | M7 任务依赖 A 核命令触发，但上游未发起 | 典型于 `ethercat_loop_task` |
| `rpmsg_channel_unready` | RPMsg 通道未初始化或顺序错误 | OpenRpmsg/InitBus 问题 |
| `pdo_mapping_mismatch` | PDO 映射项与预期不一致 | RPDO1/RPDO2 差异 |
| `pdo_online_update_forbidden` | 运行中下发不该在线更新的 PDO 数据 | 容易触发驱动器异常 |
| `drive_state_not_enabled` | 驱动器未真正完成使能 | 需看 `0x6040/0x6041` |
| `param_not_latched` | 参数写入返回成功，但驱动器对象值未更新 | 本项目当前高频问题 |
| `vendor_object_semantics` | 厂商对象字典语义或单位换算理解偏差 | Kinco FD5 这类驱动器常见 |
| `axis_specific_logic_override` | 某一轴存在独立初始化/覆盖路径 | axis0/axis1 行为不一致时重点排查 |
| `verification_gap` | 仅靠串口或单一观测手段导致误判 | 需要加入 SDO/SSH/上位机交叉验证 |

## 3. 建议动作标签

| 标签 | 含义 | 典型操作 |
|---|---|---|
| `collect_boot_context` | 采集启动上下文 | 启动介质、BOOT 文件、设备树、重启方式 |
| `recheck_serial_path` | 复核串口路径 | 线缆、端口号、终端配置、波特率 |
| `verify_build_profile` | 校验构建配置 | `ddr_release`/`TCM`/工具链版本 |
| `rebuild_and_cold_reboot` | 重新构建并冷启动验证 | 避免旧镜像残留 |
| `probe_rpmsg_handshake` | 检查 RPMsg 通道和触发时序 | OpenRpmsg、InitBus、消息触发链 |
| `read_back_object_dict` | 回读对象字典 | 重点回读 `0x6041/0x6061/0x606C/0x6083/0x6084/0x60FF` |
| `freeze_stable_axis` | 冻结已稳定轴，仅隔离异常轴 | 防止双轴联调互相污染 |
| `disable_risky_pdo_update` | 暂停高风险 PDO 在线更新 | 尤其是 RPDO2 运行期写入 |
| `switch_to_sdo_force_write` | 临时改用 SDO 强制写入验证 | 只用于诊断，不作为长期方案默认 |
| `cross_check_with_vendor_tool` | 用厂商上位机交叉验证 | KincoServo+ 等 |
| `use_no_serial_regression` | 使用无串口回归脚本 | COM3 不可用时保证验证闭环 |
| `summarize_case_and_baseline` | 沉淀 case 与稳定基线 | 给后续 Agent 做知识/评测闭环 |

## 4. 风险等级标签

| 标签 | 含义 | 允许范围 | 示例 |
|---|---|---|---|
| `L0_readonly` | 只读、观测、问答 | 默认允许 | 看日志、查文档、读取状态字/编码器 |
| `L1_low_risk_exec` | 低风险执行 | 需显式确认或审批 | 运行只读探针、拉日志、重启非关键服务 |
| `L2_high_risk_exec` | 高风险执行 | 默认拒绝自动执行 | 在线改 PDO、改控制字、改运行模式、刷固件、重新上电运行 |

## 5. 推荐的标签组合模板

### 5.1 启动/部署类

- 问题大类：`bringup_boot` / `build_config` / `deploy_runtime`
- 原因标签：`boot_artifact_stale` / `build_profile_mismatch` / `toolchain_path_noise`
- 建议动作：`verify_build_profile` / `rebuild_and_cold_reboot` / `collect_boot_context`
- 风险等级：`L0_readonly` 或 `L1_low_risk_exec`

### 5.2 RPMsg/核间通信类

- 问题大类：`rpc_rpmsg`
- 原因标签：`rpc_trigger_missing` / `rpmsg_channel_unready`
- 建议动作：`probe_rpmsg_handshake` / `use_no_serial_regression`
- 风险等级：`L0_readonly`

### 5.3 CANopen/伺服联调类

- 问题大类：`canopen_link` / `servo_state_machine` / `motion_param_latch` / `pdo_mapping`
- 原因标签：`pdo_mapping_mismatch` / `param_not_latched` / `vendor_object_semantics` / `axis_specific_logic_override`
- 建议动作：`read_back_object_dict` / `freeze_stable_axis` / `cross_check_with_vendor_tool` / `disable_risky_pdo_update`
- 风险等级：诊断通常为 `L0_readonly`，涉及在线修改则升为 `L2_high_risk_exec`

## 6. V1 建议优先支持的标签集合

1. 问题大类优先：`build_config`、`rpc_rpmsg`、`motion_param_latch`
2. 原因标签优先：`build_profile_mismatch`、`rpc_trigger_missing`、`param_not_latched`
3. 建议动作优先：`verify_build_profile`、`probe_rpmsg_handshake`、`read_back_object_dict`
4. 风险控制优先：严格拦截所有 `L2_high_risk_exec`
