# Formal Merge Workflow

## 目的

这份文档说明如何把 bench 结束后生成的候选内容，从 `reports/` 和 `data/pending/` 一路推进到“可审阅的正式数据并入建议”，同时保持低风险边界，不直接污染 canonical 数据。

这条链的设计原则是：

- 先生成候选
- 再做人审
- 再放进 pending 区
- 再生成 merge plan
- 再生成 formal merge assistant
- 再做 apply dry-run / staging execute
- 最后在考虑 canonical merge 之前先跑 preflight
- 预检通过后，再生成 canonical patch bundle
- 再看 canonical merge preview
- 最后收成 canonical merge report
- 最后再看 canonical merge checklist

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

### 8. 执行 canonical merge preflight

```bash
ieda tools canonical-merge-preflight
```

这一步仍然是只读检查，不会写 canonical 数据。

它当前会检查：

- `data/benchmark/benchmark_v1.jsonl` 是否存在
- `data/materials/material_index_v1.md` 是否存在
- benchmark append 候选是否有重复 `id`
- material index 拟追加条目是否与现有内容冲突
- staging 包是否完整
- 当前 pending 区是否真的存在候选内容

输出：

- `data/pending/formal_merge_assistant/canonical_merge_preflight.json`
- `data/pending/formal_merge_assistant/canonical_merge_preflight.md`

如果当前仓库里还没有待并入的 pending/staging 数据，那么这一步返回 `passed=false` 是正常的。
这代表“当前并入前提不满足”，不是工具本身出了故障。

### 9. 生成 canonical patch bundle

```bash
ieda tools canonical-patch-helper
```

这一步仍然不会写 canonical 文件，但会生成一套更接近正式并入动作的 patch 包：

- `data/pending/formal_merge_assistant/canonical_patch_bundle/canonical_patch_manifest.json`
- `data/pending/formal_merge_assistant/canonical_patch_bundle/canonical_patch_manifest.md`
- `data/pending/formal_merge_assistant/canonical_patch_bundle/data/benchmark/benchmark_v1.append.jsonl`
- `data/pending/formal_merge_assistant/canonical_patch_bundle/data/materials/material_index_v1.append.md`
- `data/pending/formal_merge_assistant/canonical_patch_bundle/data/materials/materials_case_merge_candidates.md`
- `data/pending/formal_merge_assistant/canonical_patch_bundle/recommended_commit_split.md`

这一步的用途是：

- 把 canonical merge 前真正需要审阅的 patch 文件集中打包
- 给 benchmark append、material index append、materials 候选文件一个更清晰的落地点
- 让后续人工 canonical merge 不必再从 staging 或 dry-run 输出里手工翻找文件

### 10. 生成 canonical merge preview

```bash
ieda tools canonical-merge-preview
```

这一步仍然不会写 canonical 文件，但会生成一套“merge 后效果预览”：

- `data/pending/formal_merge_assistant/canonical_merge_preview/canonical_merge_preview_manifest.json`
- `data/pending/formal_merge_assistant/canonical_merge_preview/canonical_merge_preview_manifest.md`
- `data/pending/formal_merge_assistant/canonical_merge_preview/data/benchmark/benchmark_v1.preview.jsonl`
- `data/pending/formal_merge_assistant/canonical_merge_preview/data/materials/material_index_v1.preview.md`
- `data/pending/formal_merge_assistant/canonical_merge_preview/data/materials/materials_case_merge_candidates.preview.md`
- `data/pending/formal_merge_assistant/canonical_merge_preview/recommended_commit_split.preview.md`

这一步的用途是：

- 直接预览 benchmark append 之后的整体文件形态
- 直接预览 material index 追加后会长什么样
- 让人工审阅不只看 patch，还能看“合并后大概长什么样”

### 11. 生成 canonical merge report

```bash
ieda tools canonical-merge-report
```

这一步会把 `preflight / patch / preview` 三层结果收成单个总览：

- `data/pending/formal_merge_assistant/canonical_merge_report/canonical_merge_report.json`
- `data/pending/formal_merge_assistant/canonical_merge_report/canonical_merge_report.md`

它当前会汇总：

- preflight 是否通过
- canonical patch bundle 的关键文件
- canonical merge preview 的关键文件
- 推荐人工审阅顺序

这一步依然不会改 canonical 数据，它只是把 formal merge 前最后几层审阅结果收成一页。

### 12. 生成 canonical merge checklist

```bash
ieda tools canonical-merge-checklist
```

这一步会把前面的 `preflight / patch / preview / report` 再收成一份“最终是否可以开始人工审阅”的清单：

- `data/pending/formal_merge_assistant/canonical_merge_checklist/canonical_merge_checklist.json`
- `data/pending/formal_merge_assistant/canonical_merge_checklist/canonical_merge_checklist.md`

它当前会回答：

- 现在是否 `ready_for_manual_canonical_merge_review`
- 哪些关键前提已经满足
- 当前 blocker 是什么
- 下一步先看哪些文件

这一步依然不会改 canonical 数据，它只是把“正式 merge 之前最后该看的信息”压成一份最终清单。

### 13. 进入人工 canonical merge 指南

当下面条件都满足后：

- checklist 显示 `ready_for_manual_canonical_merge_review = true`
- preflight 没有未处理 blocker
- patch / preview / report 都已人工看过

再进入：

- `docs/formal_merge_quickstart.md`
- `docs/manual_canonical_merge_guide.md`
- `docs/real_bench_to_formal_merge_map.md`

这份 guide 不是自动执行命令，而是告诉你：

- 先改什么
- 后改什么
- 怎么拆 commit
- 哪些情况下应该停止，不要直接写 canonical 数据

---

## 推荐人工审阅顺序

建议按下面顺序看：

1. `finish_outputs/candidate_review/review_summary.md`
2. `data/pending/merge_plan/merge_plan.md`
3. `data/pending/formal_merge_assistant/formal_merge_assistant.md`
4. `data/pending/formal_merge_assistant/apply_formal_merge_dry_run.md`
5. `data/pending/formal_merge_assistant/recommended_commit_split.md`
6. `data/pending/formal_merge_assistant/staging/staging_summary.json`
7. `data/pending/formal_merge_assistant/canonical_merge_preflight.md`
8. `data/pending/formal_merge_assistant/canonical_patch_bundle/canonical_patch_manifest.md`
9. `data/pending/formal_merge_assistant/canonical_merge_preview/canonical_merge_preview_manifest.md`
10. `data/pending/formal_merge_assistant/canonical_merge_report/canonical_merge_report.md`
11. `data/pending/formal_merge_assistant/canonical_merge_checklist/canonical_merge_checklist.md`
12. `docs/manual_canonical_merge_guide.md`

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
- `canonical-merge-preflight` 只做只读预检
- `canonical-patch-helper` 只生成 patch bundle，不会写 canonical 数据
- `canonical-merge-preview` 只生成 merge 效果预览，不会写 canonical 数据
- `canonical-merge-report` 只做汇总，不会写 canonical 数据
- `canonical-merge-checklist` 只做最终审阅清单，不会写 canonical 数据

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

1. 在人工确认过 staging、preflight、canonical patch bundle、preview、report 和 checklist 之后，再决定是否真的写 canonical 数据
2. 继续保持 benchmark 变更和 materials 变更分 commit 审阅

这样既能提高 formal merge 的自动化程度，又不会过早突破当前项目的数据安全边界。

---

## New Gating Rule

从当前版本开始，formal merge 链已经不再把所有 pending 候选默认一路往后推，而是先做自动分流：

1. `candidate-quality-check`
2. `review-finish-candidates`
3. `promote-finish-candidates`
4. `plan-pending-merge`
5. `prepare-formal-merge`
6. `apply-formal-merge`

关键约束是：

- `review-finish-candidates` 产出 `review_recommendation`
- `promote-finish-candidates` 把它映射成 `next_step`
- `plan-pending-merge` 只保留 `next_step = continue_to_pending_merge` 的 eligible 候选
- 其余候选进入 `deferred_candidates`
- `prepare-formal-merge` 和 `apply-formal-merge` 都只消费 eligible 候选

### `next_step` 语义

- `continue_to_pending_merge`
  候选可以继续进入 formal merge 准备链
- `run_manual_edit`
  候选需要先手工润色或补材料，再重新进入 review/promote
- `stop_and_analyze`
  候选当前不适合继续推进，应该先回到 bench 证据或人工分析

### 对 apply 阶段的影响

当没有 eligible 候选时：

- `apply-formal-merge --dry-run` 会返回 `status = no_eligible_candidates`
- `apply-formal-merge --execute` 的 staging summary 也会保留同样状态
- 这不是错误，而是明确的“当前无需推进 formal merge”信号

---

## Quality Signals Across The Merge Chain

当前 formal merge 链里，候选会沿着三层信号继续往后传：

- `quality_level`
- `review_recommendation`
- `next_step`

### 1. quality_level

由 `candidate-quality-check` 产出，当前分为：

- `good`
- `weak`
- `blocked`

### 2. review_recommendation

由 `review-finish-candidates` 结合质量结果产出，当前分为：

- `promote_now`
- `edit_before_promote`
- `hold_for_manual_analysis`

### 3. next_step

由 `promote-finish-candidates` 根据 review 建议产出，当前分为：

- `continue_to_pending_merge`
- `run_manual_edit`
- `stop_and_analyze`

### 当前传播路径

这三层信号现在会继续出现在：

- `review_summary.md`
- `promotion_record.json / promotion_record.md`
- `merge_plan.json / merge_plan.md`
- `formal_merge_assistant.md`

因此在人工审阅时，你已经可以直接从文档里看到：

- 候选质量属于 `good / weak / blocked`
- 机器建议是继续 promote、先编辑，还是先停下分析
- 为什么某条候选会被放进 deferred，而不是继续进入 formal merge bundle
## quality_score in the Formal Merge Flow

`quality_score` is now propagated together with the existing merge-gating signals.

Purpose:
- give a lightweight numeric cue for ranking and triage
- help compare multiple `weak` candidates
- make `eligible` and `deferred` lists easier to prioritize during manual review

Current flow:
1. `candidate-quality-check` generates `quality_score`
2. `review-finish-candidates` carries it into review summaries
3. `promote-finish-candidates` preserves the review outcome
4. `plan-pending-merge` shows `quality_score` for eligible and deferred candidates
5. `prepare-formal-merge` surfaces the score in the assistant bundle

Interpretation guidance:
- treat the score as a ranking helper, not a canonical truth value
- rely on `quality_level` for the main class boundary
- rely on `review_recommendation` and `next_step` for workflow decisions
## Top Candidates To Review First

The formal merge flow now includes a short review-focused summary in:
- `merge_plan.md`
- `formal_merge_assistant.md`

Purpose:
- reduce scan time for human reviewers
- show the most relevant candidates immediately
- preserve the full eligible/deferred lists below for complete context

Rules:
- the summary is built from the already-sorted candidate lists
- candidates are ordered by `quality_score` descending
- both `eligible` and `deferred` items can appear in the summary
- the summary does not override `eligible/deferred` routing; it only improves review ergonomics
## review_now / watch / blocked

The top review summary now carries a lightweight review-priority bucket in addition to the existing merge signals.

Purpose:
- help human reviewers decide what to inspect first
- separate immediate review targets from items that should only be monitored
- make blocked items obvious without reading the full deferred section first

Current interpretation:
- `review_now`: review first
- `watch`: keep visible, but not first priority
- `blocked`: do not advance until the blocker is addressed

This bucket is derived from the existing merge state and remains a presentation-layer shortcut.
It does not override `eligible/deferred`, `review_recommendation`, or `next_step`.
## Guidance Propagation

The formal merge chain now carries lightweight actionable guidance together with the existing quality and routing signals.

Current interpretation:
- warning categories classify the issue type
- guidance explains the next human action
- review recommendation decides whether promotion should continue
- next step decides whether the candidate remains eligible or deferred
- deferred guidance in merge planning helps reviewers decide what to fix first

This keeps the merge workflow more explainable without changing the canonical safety boundary.
## Deferred Candidates By Warning Category

The formal merge workflow now renders deferred candidates in two complementary ways:
- a ranked deferred list
- a grouped deferred-by-warning-category list

Why both views exist:
- ranking helps reviewers see the most important deferred items first
- grouping helps reviewers fix one class of issue in batches

This improves review ergonomics without changing the canonical merge boundary.
