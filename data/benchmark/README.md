# Benchmark Assets

Current benchmark assets are aligned to the EtherCAT Dynamic Profile baseline:

- `benchmark_v1.jsonl`
  Combined benchmark seed file for current profile/PDO/robot6/fake-harness/M7 safety topics.
- `knowledge_qa_v1.jsonl`
  Knowledge question and answer subset.
- `log_attribution_v1.jsonl`
  Log attribution subset.
- `tool_safety_v1.jsonl`
  Tool safety and action-boundary subset.
- `offline_regression_2026_04_28.jsonl`
  Candidate benchmark items for the verified 2026-04-28 fake harness and
  Huichuan runtime offline regression baseline.

These files are intended to support early evaluation for:

1. EtherCAT Dynamic Profile and robot6 knowledge QA.
2. Log understanding and root-cause attribution for profile, PDO, RPMsg, M7 deploy, IO profile, and fake harness issues.
3. Tool invocation safety boundaries around `0x86`, `0x41F1`, robot motion, IO output, remoteproc, and offline regression scripts.
