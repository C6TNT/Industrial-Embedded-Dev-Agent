Servo CAN 接口接入说明
========================

1. 文件说明

- `servo_can.h`
  对外公开的接口头文件。
- `servo_can.c`
  接口实现文件，内部已经包含当前项目使用的 `RPDO + SDO` 控制逻辑。
- `flexcan_interrupt_transfer.c`
  当前工程里的最小调用示例。

2. 当前接口能力

当前版本默认使用以下方案：
- 主站通过 `RPDO1` 发送工作模式、控制字、目标速度
- 主站通过少量 `SDO` 回读以下反馈
  - `0x6061` 模式显示值
  - `0x6041` 状态字
  - `0x606C` 实际速度
  - `0x6064` 实际位置

当前版本不依赖 `TPDO`。

3. 对外接口

- `ServoCan_LoadDefaultConfig`
  加载默认配置，默认节点号为 `1`，目标速度为 `1000000`。
- `ServoCan_InitHardware`
  完成当前 EVK 工程里的板级初始化和 FlexCAN 初始化。
- `ServoCan_Init`
  保存运行配置，准备内部 RPDO 数据。
- `ServoCan_Start`
  执行一次启动流程：`0x06 -> 0x07 -> 0x0F`。
- `ServoCan_SetTargetSpeed`
  修改后续运行期 RPDO 使用的目标速度。
- `ServoCan_Service`
  周期发送一帧运行期 RPDO，并按配置周期做一次 SDO 回读。
- `ServoCan_ReadFeedback`
  立即通过 SDO 刷新一次反馈。
- `ServoCan_GetCachedFeedback`
  读取最近一次缓存的反馈，不触发新的 SDO。
- `ServoCan_GetServicePeriodUs`
  返回建议的周期调用间隔，当前默认是 `8000us`。

4. 推荐调用顺序

```c
ServoCan_Config config;

ServoCan_LoadDefaultConfig(&config);
ServoCan_InitHardware();
ServoCan_Init(&config);
ServoCan_Start();

while (true)
{
    ServoCan_Service();
    SDK_DelayAtLeastUs(ServoCan_GetServicePeriodUs(),
                       SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
}
```

5. 反馈读取示例

```c
ServoCan_Feedback feedback;

ServoCan_ReadFeedback(&feedback);

PRINTF("mode=%d status=0x%04X speed=%d pos=%d enabled=%d\r\n",
       feedback.modeDisplay,
       feedback.statusWord,
       feedback.actualSpeed,
       feedback.actualPosition,
       feedback.operationEnabled ? 1 : 0);
```

6. 可配置项

`ServoCan_Config` 当前支持以下参数：
- `nodeId`
  从站节点 ID。
- `targetSpeed`
  目标速度。
- `startupPeriodUs`
  启动阶段 RPDO 连发周期。
- `runtimeFeedbackCycles`
  运行期每发送多少次 RPDO 后做一次 SDO 回读。

7. 移植说明

如果要把这套接口移到别的工程里，优先关注下面两点：

- `ServoCan_InitHardware()`
  这里带有当前板子的 RDC、PinMux、时钟、GPIO、DebugConsole、FlexCAN 初始化代码。
  如果换工程或换板卡，这里最可能需要调整。
- `servo_can.c` 内部的对象字典和 PDO 映射
  当前默认使用：
  - RPDO1 映射：`0x6060 + 0x6040 + 0x60FF`
  - SDO 回读：`0x6061 + 0x6041 + 0x606C + 0x6064`

8. 当前验证结果

当前这套封装已经在本工程中完成以下验证：
- WSL 编译通过
- 自动下发成功
- SSH 重启成功
- `COM3` 串口抓到完整日志
- 电机可以正常进入运行态
- 运行期可以稳定回读状态字、实际速度、实际位置
