# WSL 无硬件 Stub 模式

这套模式的目标是：在没有伺服驱动器、电机和现场总线的情况下，先把 `ieda tools` 的执行链跑通。

## 适用场景

- 放假或离开公司，身边没有真实硬件
- 想先验证 `SCRIPT-004` 这类只读/低风险工具的执行闭环
- 想先调通 WSL + Python + 工具解析器，而不是直接碰现场设备

## 工作方式

当前仓库不会修改原始探针脚本，而是通过一个 WSL Python wrapper 在运行时接管：

- 原脚本里的 `ctypes.CDLL("/home/librobot.so.1.0.0")`
- 被替换成仓库内置的纯 Python 假总线实现
- 这样不需要真实 `librobot.so`，也不需要编译器或 root 权限

相关文件：

- `scripts/wsl/run_with_stub.py`
- `scripts/wsl/librobot_stub_runtime.py`

## 启用方式

执行：

```powershell
python -m industrial_embedded_dev_agent tools setup-stub
```

这会在仓库根目录生成一个本地标记文件：

```text
.ieda_wsl_stub_enabled
```

它只是本机运行开关，已经被 `.gitignore` 忽略，不会进入仓库历史。

## 环境检查

查看当前 WSL 工具环境：

```powershell
python -m industrial_embedded_dev_agent tools doctor
```

重点字段：

- `wsl_available`
- `python3_available`
- `stub_mode_enabled`
- `stub_library_present`

当 `stub_mode_enabled=true` 时，即使 `/home/librobot.so.1.0.0` 不存在，也可以通过 wrapper 运行探针脚本。

## 验证命令

无硬件场景下，优先验证 `SCRIPT-004`：

```powershell
python -m industrial_embedded_dev_agent tools run "运行 tmp_probe_can_heartbeat.py 采集 8 轮状态快照，然后把结果汇总给我。" --execute
```

预期结果：

- `returncode = 0`
- `parsed_output.status = ok`
- 能看到 `poll_count` 和 `axis0/axis1` 的结构化快照

## 与真机模式的关系

这套 stub 模式只用于离线环境联调，不代表真实硬件状态。

回到公司、接入真实设备后：

1. 删除仓库根目录的 `.ieda_wsl_stub_enabled`
2. 准备真实的 `librobot.so.1.0.0` 和目标运行环境
3. 再次执行 `ieda tools doctor`
4. 用真实链路重新验证 `tools run --execute`

## 当前边界

当前主要面向：

- `SCRIPT-004` 的执行闭环
- 工具层的 plan / execute / parse 流程验证

它不是电机运动仿真器，也不替代真机联调。  
更高风险的 `L2` 脚本仍然保持人工确认或拒绝执行策略。
