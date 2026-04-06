# Contributing

感谢你关注 `Industrial Embedded Dev Agent`。

这个项目目前仍处于“准备期 + MVP 前夜”阶段，所以我们更欢迎能提升项目工程质量、资料质量和评测质量的贡献，而不是只做表面功能堆叠。

## 当前最欢迎的贡献方向

优先欢迎下面几类贡献：

1. 补充真实工业材料
2. 完善标签体系
3. 扩充 benchmark
4. 补充日志分析与 case 归档规范
5. 搭建 MVP 工程骨架
6. 改善文档、目录结构与仓库可维护性

## 提交前建议

在提交 issue 或 PR 前，建议先了解以下内容：

- `README.md`
- `Industrial Embedded Dev Agent_项目方案.md`
- `准备产物/真实材料整理.md`
- `准备产物/标签体系_v1.md`
- `准备产物/benchmark_v1.jsonl`
- `仓库整理说明.md`

这样可以避免讨论脱离当前项目目标和边界。

## 协作原则

请尽量遵守下面几条原则：

1. 优先贴近真实工业场景，而不是只追求“看起来智能”。
2. 优先做可复用、可评测、可沉淀的改动。
3. 涉及工具调用、脚本执行、安全边界时，要明确风险等级。
4. 涉及厂商资料、第三方文档、外部代码时，要注意版权与来源说明。
5. 不要将超大 Demo 包、镜像、SDK、原始手册大包直接提交进仓库。

## Issue 提交流程

如果你要提 issue，建议尽量写清楚：

1. 问题背景
2. 当前现象
3. 已知输入材料或样本
4. 预期结果
5. 风险或影响范围

对于日志归因、case 复盘、benchmark 设计类问题，最好附：

- 最小复现日志
- 相关标签建议
- 是否可纳入 benchmark

## Pull Request 建议

如果你要提 PR，建议按下面方式组织：

1. 说明改动目的
2. 说明改动范围
3. 说明是否影响现有 benchmark、标签或资料结构
4. 如果有新增材料，说明来源和脱敏情况
5. 如果有工具执行逻辑，说明安全边界

推荐在 PR 描述中附：

- 改动摘要
- 影响目录
- 验证方式
- 后续待办

在提交 PR 前，建议至少先跑：

```bash
ieda check
```

如果你的改动涉及检索、RAG、答案模板或引用排序，建议再额外跑：

```bash
ieda check --include-rag --rag-type tool_safety
```

如果你的改动涉及 bench-pack、compare-pack、session review 或离线 stub 样例，建议再额外跑：

```bash
ieda check --include-offline
```

如果你这次改动同时影响检索和离线 bench 链，建议直接跑：

```bash
ieda check --include-offline --include-rag --rag-type tool_safety
```

如果后续仓库扩展出更重的 RAG 回归集，可以按改动范围选择对应的 `--rag-type`。

仓库远端 CI 当前也分成两层：

- `Quick Check`
  面向日常 `push / pull_request`
- `Full Check`
  面向 `main` 分支和手动触发的更完整回归

所以本地建议至少先把与你改动范围匹配的检查跑干净，再提 PR。

## 数据与资料贡献要求

如果你是在真实工位做首轮 bench 验证，建议优先使用：

```bash
ieda tools prep-real-bench --session-id <session-id> --label "<session label>"
```

先生成一套 bench checklist / first-run / issue / review 资料包，再开始采证，这样后续沉淀为 case、benchmark 或 issue 会更顺。

如果你是在回工位后的首轮真实 bench，会更推荐按下面顺序：

1. `ieda tools prep-real-bench ...`
2. `ieda tools kickoff-real-bench <plan_seed.json>`
3. 确认 plan 结果仍是 `SCRIPT-004 / L0_readonly`
4. 再决定是否显式加 `--execute`

这样可以把“准备包 -> 首次只读采证”这条链尽量标准化。

如果你准备把一次 bench 的候选结果继续推进到正式数据并入准备，建议再阅读：

- `docs/formal_merge_workflow.md`

并优先按下面顺序操作：

1. `ieda tools review-finish-candidates --session-id <session-id>`
2. `ieda tools promote-finish-candidates --session-id <session-id>`
3. `ieda tools plan-pending-merge`
4. `ieda tools prepare-formal-merge`
5. `ieda tools apply-formal-merge --dry-run`
6. 需要预演落地结果时，再执行 `ieda tools apply-formal-merge --execute`
7. 真正考虑 canonical merge 前，再执行 `ieda tools canonical-merge-preflight`
8. 需要整理正式并入 patch 包时，再执行 `ieda tools canonical-patch-helper`
9. 需要预览 merge 后目录形态时，再执行 `ieda tools canonical-merge-preview`
10. 需要把 preflight / patch / preview 汇总成单页总览时，再执行 `ieda tools canonical-merge-report`
11. 需要判断“现在能不能开始人工 canonical merge 审阅”时，再执行 `ieda tools canonical-merge-checklist`

当前这条链即使走到 `--execute`，也只会写入 staging 目录；`canonical-merge-preflight`、`canonical-patch-helper`、`canonical-merge-preview`、`canonical-merge-report` 和 `canonical-merge-checklist` 也都不会直接修改 canonical 数据文件。

如果 checklist 已经通过、准备进入真正的人审并入阶段，再看：

- `docs/real_bench_first_day_plan.md`
- `docs/real_bench_quickstart.md`
- `docs/candidate_quality_check_plan.md`
- `docs/formal_merge_quickstart.md`
- `docs/manual_canonical_merge_guide.md`
- `docs/real_bench_to_formal_merge_map.md`

### 日志样本

建议尽量满足：

- 有最小上下文
- 有时间顺序
- 有问题现象
- 可脱敏
- 可用于归因

### 历史问题 case

建议包含：

- 问题标题
- 背景环境
- 触发条件
- 现象描述
- 根因
- 处理动作
- 风险等级
- 是否适合进入 benchmark

### 调试脚本

建议包含：

- 脚本用途
- 输入输出
- 使用前提
- 风险等级
- 是否允许 Agent 自动调用

## 分支与提交信息

当前没有强制分支命名规范，但建议：

- 功能改动：`feature/...`
- 文档改动：`docs/...`
- benchmark 改动：`benchmark/...`
- 数据整理：`data/...`

提交信息建议尽量简洁明确，例如：

- `Add log attribution benchmark seeds`
- `Refine risk labels for tool execution`
- `Document case ingestion format`

## 暂不建议直接提交的内容

以下内容原则上不建议直接提交到仓库：

- 大型 Demo 资料包
- 超大 PDF / zip / tar 包
- 固件镜像
- 未脱敏的现场日志
- 不明确版权归属的第三方资料

如果确实需要，请先在 issue 中说明原因和处理边界。

## 交流目标

这个项目更希望把讨论拉回到“工业研发是否真的能用”这个核心问题上。

比起：

- 这个 Agent 能不能回答得像

我们更关心：

- 它能不能帮工程师缩短排查时间
- 它能不能沉淀经验
- 它能不能被 benchmark 持续检验
- 它在安全边界内是否可控

## Candidate Gating Note

如果你的改动涉及：

- `candidate-quality-check`
- `review-finish-candidates`
- `promote-finish-candidates`
- `plan-pending-merge`
- `prepare-formal-merge`
- `apply-formal-merge`

请注意当前仓库已经引入了候选自动分流规则：

- `review_recommendation` 是机器建议，不会直接替你拒绝操作
- `promote-finish-candidates` 会把建议落成 `next_step`
- 只有 `next_step = continue_to_pending_merge` 的候选会继续进入 formal merge 链
- `run_manual_edit / stop_and_analyze` 会进入 deferred 路径

这类改动提交前，建议至少确认：

- promotion record 里的 `review_recommendation / next_step / soft_blocked` 是否合理
- merge plan 里的 `eligible` 和 `deferred` 是否符合预期
- `apply-formal-merge` 在没有 eligible 候选时是否能明确提示无需继续推进

## Quality Signal Semantics

如果你的改动会影响 candidate 质量判断，请同时检查下面三层语义是否仍然一致：

- `quality_level`
- `review_recommendation`
- `next_step`

当前推荐理解是：

- `good`：候选质量基本可接受
- `weak`：候选可保留，但需要编辑或补强
- `blocked`：候选当前不应继续推进

并且应继续满足：

- `good` 倾向映射到 `promote_now`
- `weak` 倾向映射到 `edit_before_promote`
- `blocked` 倾向映射到 `hold_for_manual_analysis`

后续在 promote/merge 链里，这些信号还会继续影响：

- 是否进入 eligible 候选
- 是否进入 deferred 路径
- `apply-formal-merge` 是否提示继续推进
## Quality Score Note

If your change touches the candidate-quality or pending-merge flow, please also check whether `quality_score` still behaves reasonably.

Current intent:
- `quality_score` gives a rough sortable signal for candidate quality.
- `quality_level` remains the human-readable class (`good / weak / blocked`).
- `review_recommendation` and `next_step` are downstream actions, not replacements for the score.

Before merging these changes, it is useful to confirm:
- lower-quality candidates really receive lower `quality_score`
- `merge_plan` and `formal_merge_assistant` still surface the score in Markdown output
- the score does not contradict the visible `quality_level`
