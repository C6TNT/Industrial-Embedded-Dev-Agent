# Formal Merge Workflow

## 目的

这份文档说明如何把 bench 结束后生成的候选内容，从 `reports/` 和 `data/pending/` 一路推进到“可审阅的正式数据并入建议”，同时保持低风险边界，不直接污染 canonical 数据。

这条链的设计原则是：

- 先生成候选
- 再做人审
- 再放进 pending 区
- 再生成 merge plan
- 再生成 formal merge assistant
- 再做 apply dry-run

当前版本仍然不会直接改这些正式文件：

- `data/materials/`
- `data/materials/material_index_v1.md`
- `data/benchmark/benchmark_v1.jsonl`

---

## 整体流程

### 1. 结束一轮 bench 并导出候选

```bash
ieda tools finish-real-bench --session-id <session-id>
```

这一步会在准备包目录下生成：

- `finish_outputs/final_summary.json`
- `finish_outputs/final_summary.md`
- `finish_outputs/candidate_exports/case_candidate.md`
- `finish_outputs/candidate_exports/log_candidate.json`
- `finish_outputs/candidate_exports/benchmark_candidate.json`

这一步的意义是：

- 把一次 session 的收尾结果结构化
- 把潜在可复用内容先变成候选草稿

### 2. 审核候选

```bash
ieda tools review-finish-candidates --session-id <session-id>
```

这一步会生成：

- `finish_outputs/candidate_review/review_summary.json`
- `finish_outputs/candidate_review/review_summary.md`

建议在这一步重点确认：

- `suggested_tag` 是否合理
- case 候选是否值得长期保留
- benchmark 候选是否表达清楚、适合回归

### 3. 进入 pending 区

```bash
ieda tools promote-finish-candidates --session-id <session-id>
```

这一步会把通过初筛的内容放到：

- `data/pending/cases/`
- `data/pending/logs/`
- `data/pending/benchmarks/`
- `data/pending/promotion_records/`

注意：

- 这一步依然不是正式入库
- 它只是把内容从“session 私有产物”推进到“仓库级待审候选区”

### 4. 生成 pending merge plan

```bash
ieda tools plan-pending-merge
```

这一步会生成：

- `data/pending/merge_plan/merge_plan.json`
- `data/pending/merge_plan/merge_plan.md`

它回答的是：

- 现在 pending 区里有哪些候选
- 它们大致应该往哪个正式目录去
- 建议动作是什么

### 5. 生成 formal merge assistant

```bash
ieda tools prepare-formal-merge
```

这一步会生成：

- `data/pending/formal_merge_assistant/formal_merge_assistant.json`
- `data/pending/formal_merge_assistant/formal_merge_assistant.md`
- `data/pending/formal_merge_assistant/materials_case_merge_candidates.md`
- `data/pending/formal_merge_assistant/log_merge_candidates.jsonl`
- `data/pending/formal_merge_assistant/benchmark_append_candidates.jsonl`
- `data/pending/formal_merge_assistant/material_index_patch.md`

这一步的作用是：

- 把 pending 区候选进一步整理成“可正式并入前审阅”的建议包
- 提前把 case bundle、benchmark append 草稿、material index 候选条目整理好

### 6. 执行 apply dry-run

```bash
ieda tools apply-formal-merge --dry-run
```

这一步会生成：

- `data/pending/formal_merge_assistant/apply_formal_merge_dry_run.json`
- `data/pending/formal_merge_assistant/apply_formal_merge_dry_run.md`
- `data/pending/formal_merge_assistant/benchmark_append_patch.jsonl`
- `data/pending/formal_merge_assistant/material_index_append_patch.md`
- `data/pending/formal_merge_assistant/recommended_commit_split.md`

这一步回答的是：

- 如果真的要并入正式数据，会改哪些地方
- benchmark 应该如何 append
- material index 应该补哪些行
- 建议如何拆 commit，避免把材料整理、benchmark 变更、索引变更混在一起

### 7. 执行 staging 模式

```bash
ieda tools apply-formal-merge --execute
```

这一步仍然不会修改 canonical 数据，但会把更接近真实并入动作的结果写到 staging 目录：

- `data/pending/formal_merge_assistant/staging/data/materials/materials_case_merge_candidates.md`
- `data/pending/formal_merge_assistant/staging/data/materials/material_index_append_patch.md`
- `data/pending/formal_merge_assistant/staging/data/benchmark/benchmark_append_patch.jsonl`
- `data/pending/formal_merge_assistant/staging/recommended_commit_split.md`
- `data/pending/formal_merge_assistant/staging/staging_summary.json`

这一步适合在下面这种场景使用：

- 你已经完成了 pending 区审核
- 你想先看到一份接近“真实 merge 前形态”的 staging 包
- 但你还不希望工具直接写入 `data/materials/` 或 `data/benchmark/`

---

## 推荐人工审阅顺序

建议按下面顺序看：

1. `finish_outputs/candidate_review/review_summary.md`
2. `data/pending/merge_plan/merge_plan.md`
3. `data/pending/formal_merge_assistant/formal_merge_assistant.md`
4. `data/pending/formal_merge_assistant/apply_formal_merge_dry_run.md`
5. `data/pending/formal_merge_assistant/recommended_commit_split.md`
6. `data/pending/formal_merge_assistant/staging/staging_summary.json`

如果要进一步做实际并入，建议再分别查看：

- `materials_case_merge_candidates.md`
- `benchmark_append_patch.jsonl`
- `material_index_append_patch.md`
- `staging/data/materials/`
- `staging/data/benchmark/`

---

## 当前安全边界

截至当前版本，这条 formal merge 流程仍然保持以下边界：

- 不自动改 `data/materials/`
- 不自动改 `data/materials/material_index_v1.md`
- 不自动改 `data/benchmark/benchmark_v1.jsonl`
- 不自动删除 pending 区候选
- `--execute` 也只会写到 `data/pending/formal_merge_assistant/staging/`

也就是说，当前工具会尽可能把“正式并入前的准备工作”做完，但最后的 canonical 数据修改仍然保留给人工确认。

---

## 推荐提交拆分

当前建议至少拆成两个 commit：

### 1. formal-material-candidates

建议包含：

- `data/materials/` 中的新 case / 说明材料
- `data/materials/material_index_v1.md`

这样做的原因是：

- 材料整理通常偏文档与结构调整
- 更适合单独 review

### 2. formal-benchmark-appends

建议包含：

- `data/benchmark/benchmark_v1.jsonl`

这样做的原因是：

- benchmark 变更本身就是评测基线变更
- 应该单独可审阅、可回滚、可复盘

---

## 下一步

如果后面要继续推进，最自然的方向是：

1. 在人工确认过 staging 结果之后，再决定是否引入“canonical merge helper”
2. 继续保持 benchmark 变更和 materials 变更分 commit 审阅

这样既能提高 formal merge 的自动化程度，又不会过早突破当前项目的数据安全边界。
