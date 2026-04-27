# 当前训练主线：EtherCAT Dynamic Profile 与机器人位置模式联调

## 1. 项目背景

当前训练资料已经从早期双驱 CANopen 联调，切换为 i.MX8MP A53 + M7 异构架构下的 EtherCAT Dynamic Profile 项目。

核心链路为：A53 负责 XML/ESI 解析、生成 JSON profile、通过 RPMsg 下发 profile；M7 负责 EtherCAT 主站运行、profile apply、PDO mapping、PDO pack/unpack 和 runtime_axis 发布。

## 2. 动态 profile 关键事实

- 动态参数区按 `0x4100 + logical_axis` 访问。
- 动态输出门控使用 `0x41F1`，未解锁前动态控制写入应被拦截。
- 单驱链路已覆盖汇川 SV660N、步科 Kinco FD、YAKO MS。
- 汇川 SV660N 单驱使用 `fixed_layout`，`ob=7`，`ib=13`。
- 步科 Kinco FD 和 YAKO MS 使用 `remap`，`ob=7`，`ib=13`。
- 汇川六轴 robot6 拓扑包含 6 个 SV660N EtherCAT 从站。
- robot6 中 axis1/slave2 是 `12/28` 位置型变体，其余 axis0、axis2、axis3、axis4、axis5 使用 `19/13` 基线布局。

## 3. 已完成真实联调成果

机器人位置模式已形成安全基线：

- 单轴位置模式：axis5、axis4、axis3、axis2、axis1、axis0 均通过。
- 双轴位置联调：axis5+axis4、axis3+axis2、axis1+axis0 均通过。
- 三轴位置联调：axis5+axis4+axis3、axis2+axis1+axis0 均通过。
- 六轴同时位置联调：六轴 `+200 dec` 小步长验证通过。

正式机器人联调坚持位置模式优先。速度模式只作为调试探针，不作为机器人正式联调基线。

## 4. 动态接管策略

动态链路完全覆盖旧链路前，不能直接删除旧 memcpy 路径。

当前推进顺序：

1. 梳理旧机器人 memcpy 路径中的输入输出打包点、状态回填点、轴数据镜像点。
2. 做动态 runtime 与旧 memcpy 的只读并行对照，不并行写。
3. 确认六轴位置、状态字、错误码、关键运行态持续一致。
4. 先切机器人关节 PDO 输出，不碰 IO、焊接、回零、限位等周边逻辑。
5. 回归单轴、双轴、三轴、六轴位置模式矩阵。
6. 最后再把 STM32F767、GELIIO、焊接相关节点纳入统一 runtime 描述。

## 5. Fake Harness 离线回归

EtherCAT Dynamic Profile Fake Harness 是当前项目的离线上板前门禁。

它不连接真实板卡，不 SSH，不 reboot，不 start-bus/stop-bus 真实总线。它只读取 profile/topology/scenario，模拟 fake M7 apply、fake EtherCAT slave、CiA402 状态机、PDO 字节镜像和 query/report 输出。

当前能力：

- `fake matrix`：14 个场景 PASS。
- `XML batch`：汇川、步科、YAKO 真实 XML 样本 3 个 PASS。
- `replay batch`：15 个真实 report replay case PASS。
- `pytest`：9 个 fake harness 单元测试 PASS。

fake harness 适合提前发现 profile 格式、PDO 字节布局、axis/slave 映射、query 状态、fault_mode 分支等软件问题，但不能替代真实机器人安全验证。

## 6. M7 固件部署和热重载结论

当前 DDR ELF 走 Linux remoteproc 热重载时出现 `bad phdr da 0x80000000` 和 `Boot Failed: -22`。因此当前安全部署基线仍是 boot-partition `.bin + board reboot`。

如果后续要恢复热重载，需要先创建 remoteproc 兼容的 ELF/linker 布局，再重新验证 M7、RPMsg、EtherCAT 生命周期。

## 7. IO 与焊接节点边界

在没有 IO 真实设备窗口时，可以继续做准备工作：

- 把 STM32F767、GELIIO 输入输出布局写进 topology/profile。
- 做 IO 只读 profile 描述和旁路对照。
- 做 IO 输出 dry-run 对照和安全分类。

真实 IO 输出、焊接、回零、限位等高风险逻辑必须等真实设备窗口，并且先从只读对照开始。

## 8. 当前 Agent 训练重点

当前 Agent 的训练目标是服务这条 EtherCAT 动态 profile 主线：

- 回答 A53/RPMsg/M7/EtherCAT 动态链路问题。
- 判断 profile、PDO、axis/slave 映射、query 状态和 report 字段。
- 对 start/query/stop、RPMsg endpoint、M7 部署、fake harness 回归做故障归因。
- 识别 0x86、0x41F1、机器人运动、IO 输出、固件切换等高风险动作。
- 将真实 report replay 和离线 fake harness 用作上板前回归门禁。
