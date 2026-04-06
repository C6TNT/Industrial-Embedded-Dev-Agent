# Industrial Embedded Dev Agent

一个面向工业嵌入式研发场景的 Agent 项目仓库，目标不是做“会说话的 demo”，而是做一个真正带工业味道的工程助手：

- 能回答工业嵌入式知识问题
- 能看日志、做归因、给出排查建议
- 能结合历史 case 做问题复盘
- 能在风险边界内调用工具、执行脚本、辅助调试

当前仓库以 `i.MX8MP + RPMsg + CANopen / 伺服联调` 场景为核心，已经完成了第一轮资料整理、标签设计、benchmark 种子构建，以及 GitHub 开源前的仓库瘦身与历史清理。

## 项目目标

这个项目希望逐步搭出一套适合工业现场研发与调试的 Agent 基础设施，重点覆盖：

- 工业嵌入式知识检索与问答
- 日志理解、故障归因、风险判断
- 历史问题复用与经验沉淀
- 调试脚本、工具调用与安全约束
- benchmark 驱动的持续评估

一句话理解：这是一个面向真实工业研发链路的 Agent 项目，而不是单纯的大模型问答封装。

## 当前进度

当前仓库已经完成：

- 项目方案整理
- 第一轮真实材料整理
- 标签体系初稿
- benchmark 初稿
- 开源仓库结构清理
- 大文件与 Demo 历史瘦身

当前基线提交之后，仓库已经适合继续往 MVP 工程化方向推进。

## 目录导览

仓库当前主要内容如下：

- `Industrial Embedded Dev Agent_项目方案.md`
  项目方案、目标拆解、能力边界、阶段规划。

- `Industrial Embedded Dev Agent_项目评估.txt`
  外部评估入口，目前主要作为参考链接记录。

- `准备产物/`
  开工前准备好的核心资产。

- `准备产物/真实材料整理.md`
  第一轮真实材料清单，覆盖调试文档、日志样本、历史问题 case、常用脚本。

- `准备产物/标签体系_v1.md`
  标签体系初稿，包含问题大类、原因标签、建议动作标签、风险等级标签。

- `准备产物/benchmark_v1.jsonl`
  benchmark 种子集，包含知识问答、日志归因、工具调用安全测试。

- `资料/`
  当前保留在仓库中的公开工程资料、代码样例和部分项目上下文。

- `仓库整理说明.md`
  仓库瘦身策略、大文件处理边界、开源整理说明。

## 已准备的种子资产

当前已经落好的准备工作包括：

- 10 份调试文档
- 20 份日志样本
- 10 个历史问题 case
- 5 个常用脚本
- 1 套标签体系初稿
- 1 版 benchmark 初稿

这部分内容主要放在：

- `准备产物/真实材料整理.md`
- `准备产物/标签体系_v1.md`
- `准备产物/benchmark_v1.jsonl`

## 使用方式

当前仓库还处于“准备期 + 仓库基建期”，所以最适合的使用方式是：

### 1. 先读方案和准备产物

建议优先阅读：

- `Industrial Embedded Dev Agent_项目方案.md`
- `准备产物/真实材料整理.md`
- `准备产物/标签体系_v1.md`
- `准备产物/benchmark_v1.jsonl`

这样可以最快理解项目目标、数据形态和下一阶段要做什么。

### 2. 用 benchmark 作为第一版评测基线

在后续开始搭 Agent 原型时，可以直接用 `准备产物/benchmark_v1.jsonl` 做最小可用评测：

- 知识问答是否答得对
- 日志归因是否抓得到关键线索
- 工具调用是否满足安全边界

### 3. 用标签体系约束输出结构

无论后面接 RAG、接工具、还是做 case 检索，都建议优先对齐这套标签：

- 问题大类
- 原因标签
- 建议动作标签
- 风险等级标签

这样可以避免 Agent 输出“像回答”，但不利于工程复用。

### 4. 持续扩充真实工业材料

后续每新增一份调试记录、日志样本、历史故障 case，都建议同步做三件事：

- 归档原始材料
- 打标签
- 补 benchmark 或回归样例

这样仓库会越用越像工业项目，而不是越做越像演示项目。

### 5. 真机回工位前可先生成 bench 准备包

如果你准备从离线 stub 回到真实工位，可以先生成一套 ready-to-fill 的真机资料包：

```bash
ieda tools prep-real-bench --session-id bench-am-01 --label "Morning bench"
```

它会在 `reports/real_bench_prep/<session_id>/` 下生成：

- `doctor_snapshot.json`
- `plan_seed.json`
- `00_index.md`
- `01_readiness_checklist.md`
- `02_first_run_record.md`
- `03_issue_capture.md`
- `04_session_review.md`

这样回到工位后可以直接按顺序填，不用再手工整理模板。

如果你想继续把这套准备包直接推进到“第一份只读证据包”，可以再执行：

```bash
ieda tools kickoff-real-bench "reports/real_bench_prep/<session_id>/plan_seed.json"
```

默认会先生成一份 `plan-only` 的 seeded `bench-pack`。  
如果你已经确认环境可读，也可以显式加 `--execute`。

## 本地验证与 CI

当前仓库已经接入了一套最小可用的本地回归与 GitHub Actions CI。

### 本地建议先跑

默认回归：

```bash
ieda check
```

这条命令会统一执行：

- `pytest`
- `rules benchmark`
- `tool_safety benchmark`

如果你想额外检查一部分 RAG 能力，可以再跑：

```bash
ieda check --include-rag --rag-type tool_safety
```

如果你想把固定的离线 stub 样例对比也一起纳入回归，可以再跑：

```bash
ieda check --include-offline
```

如果想做当前最完整的一组本地检查，可以直接跑：

```bash
ieda check --include-offline --include-rag --rag-type tool_safety
```

当前支持的 `--rag-type` 有：

- `knowledge_qa`
- `log_attribution`
- `tool_safety`

### 远端 CI 当前会跑什么

仓库当前已经拆成两层 GitHub Actions：

- `Quick Check`
  在 `push / pull_request` 时默认执行，运行：
  `python -m industrial_embedded_dev_agent check`

- `Full Check`
  在 `main` 分支推送和手动触发时执行，运行：
  `python -m industrial_embedded_dev_agent check`
  `python -m industrial_embedded_dev_agent check --include-offline --include-rag --rag-type tool_safety`

也就是说，日常提交先走轻量兜底，离线样例回归和重点 RAG 回归则放在单独的完整检查里。

## 正式数据并入流程

如果你已经完成了一轮 bench 候选生成，并希望把候选内容逐步推进到正式数据并入准备，可以按这条链走：

```bash
ieda tools review-finish-candidates --session-id <session-id>
ieda tools promote-finish-candidates --session-id <session-id>
ieda tools plan-pending-merge
ieda tools prepare-formal-merge
ieda tools apply-formal-merge --dry-run
ieda tools apply-formal-merge --execute
ieda tools canonical-merge-preflight
ieda tools canonical-patch-helper
ieda tools canonical-merge-preview
ieda tools canonical-merge-report
ieda tools canonical-merge-checklist
```

完整说明见：

- `docs/real_bench_quickstart.md`
- `docs/formal_merge_workflow.md`
- `docs/formal_merge_quickstart.md`
- `docs/manual_canonical_merge_guide.md`
- `docs/real_bench_to_formal_merge_map.md`

当前这条链仍然保持低风险：

- 会生成候选审核、merge plan、append patch 建议
- `--execute` 也只会写到 staging 目录
- `canonical-merge-preflight` 只做只读预检
- `canonical-patch-helper` 只生成 canonical patch bundle
- `canonical-merge-preview` 只生成 merge 后效果预览
- `canonical-merge-report` 只汇总 preflight / patch / preview 结果
- `canonical-merge-checklist` 只生成最终人工审阅清单
- 不会直接修改 `data/materials/`、`data/materials/material_index_v1.md`、`data/benchmark/benchmark_v1.jsonl`

## 路线图

### Phase 0：准备阶段

- 整理真实工业材料
- 定义标签体系
- 写第一版 benchmark
- 清理仓库，建立开源基线

当前状态：已完成基础版本

### Phase 1：MVP 工程骨架

- 定义数据目录结构
- 规范 case / log / doc / script 存储方式
- 搭建最小可运行的 Agent 项目结构
- 接入一版基础检索和问答流程

### Phase 2：日志分析与归因能力

- 加入日志解析和模式提取
- 引入原因标签映射
- 输出结构化归因结论
- 建立错误案例回放与回归集

### Phase 3：工具调用与安全边界

- 为脚本和工具建立调用白名单
- 接入风险等级判定
- 明确可自动执行、需确认执行、禁止执行三类动作
- 建立工具安全 benchmark

### Phase 4：工业化增强

- 引入历史问题检索
- 融合知识问答、日志归因、工具链协同
- 强化现场调试风格输出
- 形成可持续迭代的数据闭环

## 开源边界说明

本仓库已经刻意移出一批不适合进入源码仓库的内容，例如：

- 大型 Demo 资料包
- 超大参考手册
- 板卡上手原始资料大包
- 重复中间工作区

这些内容目前仍可能存在于本地工作区中，但不会继续作为开源仓库历史的一部分维护。

详细说明见：

- `仓库整理说明.md`

## 许可证

本仓库采用 MIT License。

需要注意：

- 仓库中的第三方资料、厂商手册、外部文档，如其版权归属不属于本项目，应以原始版权声明为准。
- 若后续继续公开整理更多第三方材料，建议进一步补一份资料来源与版权边界说明。

## 下一步建议

如果继续推进，这个仓库下一阶段最值得做的事情是：

1. 搭第一版 MVP 工程骨架
2. 把 benchmark 扩成标准评测目录结构
3. 为日志、case、脚本建立统一数据格式
4. 逐步把“问答能力”升级成“可复用的工业调试工作流”
