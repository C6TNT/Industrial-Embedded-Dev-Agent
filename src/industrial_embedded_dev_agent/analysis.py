from __future__ import annotations

import re

from .models import StructuredDiagnosis


LOG_RULES = [
    {
        "issue_category": "verification_tooling",
        "keywords": ["secret_scan", "secret-scan", "pre-push", "pre_push", "sensitive_filename_hits", "content_secret_hits"],
        "cause_labels": ["verification_gap"],
        "action_labels": ["run_pre_push_check"],
        "risk_level": "L0_readonly",
        "summary": "当前指向发布前工程门禁，应先运行 secret-scan、git diff check 和本地回归，确认没有敏感信息或回归失败再推送。",
    },
    {
        "issue_category": "verification_tooling",
        "keywords": ["import-report", "import_real_report", "replay_scenario_draft", "log_entry_draft", "real report"],
        "cause_labels": ["verification_gap"],
        "action_labels": ["import_real_report"],
        "risk_level": "L0_readonly",
        "summary": "当前指向真实 report 沉淀流程，应先生成 LOG 草稿和 replay scenario 草稿，审查后再并入正式材料。",
    },
    {
        "issue_category": "safety",
        "keywords": ["hardware_scope", "offline_ok", "board_required", "robot_motion_required", "io_required", "firmware_required"],
        "cause_labels": ["verification_gap"],
        "action_labels": ["audit_hardware_action"],
        "risk_level": "L0_readonly",
        "summary": "当前指向工具硬件边界审计，应按 offline_ok、board_required、robot_motion_required、io_required、firmware_required 区分执行权限。",
    },
    {
        "issue_category": "verification_tooling",
        "keywords": ["fake harness", "pytest", "replay", "xml batch", "离线", "scenario", "report.json"],
        "cause_labels": ["verification_gap"],
        "action_labels": ["run_fake_harness_regression"],
        "risk_level": "L1_low_risk_exec",
        "summary": "当前更像离线回归或 fake harness 证据链问题，建议先跑 fake matrix、XML batch 和 replay 回归。",
    },
    {
        "issue_category": "ethercat_profile",
        "keywords": ["profile", "loaded", "applied", "slave_id", "logical_axis", "driver_type", "strategy"],
        "cause_labels": ["profile_schema_mismatch"],
        "action_labels": ["query_dynamic_profile"],
        "risk_level": "L0_readonly",
        "summary": "当前更像动态 profile 结构或应用状态问题，先用 query 和 profile 校验确认 loaded/applied/slave/axis 映射。",
    },
    {
        "issue_category": "pdo_layout",
        "keywords": ["ob=19", "ib=13", "19/13", "12/28", "pdo", "offset", "0x6040", "0x60ff", "0x6064"],
        "cause_labels": ["pdo_ob_ib_mismatch"],
        "action_labels": ["compare_pdo_layout"],
        "risk_level": "L0_readonly",
        "summary": "当前更像 PDO 字节布局或 ob/ib 不一致问题，先做旧链路与动态 profile 的字节级对照。",
    },
    {
        "issue_category": "build_config",
        "keywords": ["ddr_release", "build", "编译", ".bin", "reboot"],
        "cause_labels": ["build_profile_mismatch"],
        "action_labels": ["keep_boot_partition_reboot"],
        "risk_level": "L0_readonly",
        "summary": "优先确认当前 M7 构建产物和 BOOT 分区部署路径，当前安全基线仍是 .bin + board reboot。",
    },
    {
        "issue_category": "rpc_rpmsg",
        "keywords": ["rpmsg", "endpoint", "openrpmsg", "start-bus", "stop-bus", "query", "ssh"],
        "cause_labels": ["rpmsg_endpoint_stale"],
        "action_labels": ["stop_start_stability_check"],
        "risk_level": "L0_readonly",
        "summary": "当前更像 RPMsg endpoint 或 EtherCAT 任务生命周期残留，先跑 start/query/stop/query 稳定性检查。",
    },
    {
        "issue_category": "robot6_position",
        "keywords": ["axis5", "axis4", "axis3", "axis2", "axis1", "axis0", "六轴", "位置模式", "target_delta"],
        "cause_labels": ["axis_mapping_mismatch"],
        "action_labels": ["capture_report"],
        "risk_level": "L0_readonly",
        "summary": "当前更像 robot6 位置模式轴映射或回归证据问题，先按单轴、双轴、三轴、六轴顺序保存报告。",
    },
    {
        "issue_category": "ethercat_profile",
        "keywords": ["0x4100", "runtime_axis", "parameter_set", "parameter_get", "动态参数"],
        "cause_labels": ["profile_schema_mismatch"],
        "action_labels": ["query_dynamic_profile"],
        "risk_level": "L0_readonly",
        "summary": "动态参数区应按 0x4100 + logical_axis 访问，先核对 logical_axis 与 runtime_axis 是否一致。",
    },
    {
        "issue_category": "runtime_takeover",
        "keywords": ["旧 memcpy", "动态接管", "只读对照", "并行读", "只切机器人关节输出"],
        "cause_labels": ["axis_mapping_mismatch"],
        "action_labels": ["readonly_compare_before_write"],
        "risk_level": "L0_readonly",
        "summary": "动态链路完全接管前应先做旧 memcpy 与动态 runtime 只读对照，确认六轴位置、状态字和错误码一致。",
    },
    {
        "issue_category": "m7_deploy",
        "keywords": ["remoteproc", "bad phdr", "boot failed", "-22", "hot reload", "热重载"],
        "cause_labels": ["remoteproc_elf_incompatible"],
        "action_labels": ["keep_boot_partition_reboot"],
        "risk_level": "L2_high_risk_exec",
        "summary": "当前 DDR ELF 与 Linux remoteproc 热重载路径不兼容，继续采用 BOOT 分区 .bin + reboot 安全路径。",
    },
    {
        "issue_category": "io_profile",
        "keywords": ["stm32f767", "geliio", "限位", "焊接", "只读 profile"],
        "cause_labels": ["io_hardware_unavailable"],
        "action_labels": ["readonly_compare_before_write"],
        "risk_level": "L0_readonly",
        "summary": "IO 节点当前适合先纳入只读 profile 描述和旁路对照，真实输出切换需要等设备窗口。",
    },
]

SAFETY_HIGH_RISK = [
    "刷",
    "固件",
    "上电",
    "控制字",
    "0x86",
    "0x41f1",
    "使能",
    "运动",
    "写输出",
    "start-bus",
    "stop-bus",
    "热重载",
    "remoteproc",
    "motion",
    "move",
    "enable",
    "unlock",
    "write output",
    "io output",
    "firmware",
    "flash",
    "homing",
    "limit switch",
]

SAFETY_REQUEST_HINTS = [
    "帮我",
    "直接",
    "自动",
    "先别管审批",
    "不用确认",
    "不需要确认",
    "解锁",
    "跑一下",
    "now",
    "immediately",
    "运行",
    "下发",
    "改掉",
    "刷",
]

READONLY_HINTS = [
    "为什么",
    "作用是什么",
    "职责",
    "区别",
    "至少",
    "首次",
    "回读",
    "是什么",
]


def analyze_text(text: str, *, mode: str = "auto") -> StructuredDiagnosis:
    normalized = text.lower()
    effective_mode = _infer_mode(normalized, mode)
    if effective_mode == "safety":
        return _analyze_safety(text, normalized)
    if effective_mode == "log":
        return _analyze_log(text, normalized)
    return _analyze_general(text, normalized)


def _infer_mode(normalized: str, requested_mode: str) -> str:
    if requested_mode != "auto":
        return requested_mode
    has_log_signal = any(keyword in normalized for rule in LOG_RULES for keyword in rule["keywords"])
    has_safety_risk = any(token in normalized for token in SAFETY_HIGH_RISK)
    has_safety_request = any(token in normalized for token in SAFETY_REQUEST_HINTS)
    has_readonly_hint = any(token in normalized for token in READONLY_HINTS)

    if has_safety_risk and has_safety_request and not has_readonly_hint:
        return "safety"
    if has_log_signal:
        return "log"
    if has_safety_risk and not has_readonly_hint:
        return "safety"
    return "general"


def _analyze_log(text: str, normalized: str) -> StructuredDiagnosis:
    # Some messages mention profile/PDO while the real intent is the takeover or
    # IO boundary. Give those safety-critical lifecycle categories precedence.
    if any(keyword in normalized for keyword in ["旧 memcpy", "动态 runtime", "动态接管", "只读对照", "并行读"]):
        return StructuredDiagnosis(
            summary="动态链路完全接管前应先做旧 memcpy 与动态 runtime 只读对照，确认六轴位置、状态字和错误码一致。",
            issue_category="runtime_takeover",
            cause_labels=["axis_mapping_mismatch"],
            action_labels=["readonly_compare_before_write"],
            risk_level="L0_readonly",
            should_refuse=False,
            evidence=_extract_evidence(text, ["旧 memcpy", "动态 runtime", "只读对照", "并行读"]),
        )
    if any(keyword in normalized for keyword in ["stm32f767", "geliio", "限位", "焊接", "只读 profile"]):
        return StructuredDiagnosis(
            summary="IO 节点当前适合先纳入只读 profile 描述和旁路对照，真实输出切换需要等设备窗口。",
            issue_category="io_profile",
            cause_labels=["io_hardware_unavailable"],
            action_labels=["readonly_compare_before_write"],
            risk_level="L0_readonly",
            should_refuse=False,
            evidence=_extract_evidence(text, ["STM32F767", "GELIIO", "IO", "限位", "焊接", "只读 profile"]),
        )
    for rule in LOG_RULES:
        if any(keyword in normalized for keyword in rule["keywords"]):
            return StructuredDiagnosis(
                summary=rule["summary"],
                issue_category=rule["issue_category"],
                cause_labels=list(rule["cause_labels"]),
                action_labels=list(rule["action_labels"]),
                risk_level=rule["risk_level"],
                should_refuse=rule["risk_level"] == "L2_high_risk_exec",
                evidence=_extract_evidence(text, rule["keywords"]),
            )
    return _analyze_general(text, normalized)


def _analyze_safety(text: str, normalized: str) -> StructuredDiagnosis:
    evidence = _extract_evidence(text, SAFETY_HIGH_RISK)
    if any(token in normalized for token in SAFETY_HIGH_RISK):
        summary = "高风险请求，涉及控制字、门控解锁、机器人运动、固件切换或总线启停，默认禁止自动执行，并且需要人工确认。"
        if "0x41f1" in normalized:
            summary = "高风险请求，涉及 0x41F1 输出门控解锁，默认禁止自动执行，并且需要人工确认。"
        elif "0x86" in normalized or "控制字" in text or "使能" in text:
            summary = "高风险请求，涉及 0x86/控制字/使能动作，默认禁止自动执行，并且需要人工确认。"
        elif "运动" in text or "motion" in normalized or "move" in normalized:
            summary = "高风险请求，涉及机器人运动，默认禁止自动执行，并且需要人工确认。"
        elif "写输出" in text or "io output" in normalized:
            summary = "高风险请求，涉及 IO 或现场输出写入，默认禁止自动执行，并且需要人工确认。"
        elif "固件" in text or "刷" in text or "remoteproc" in normalized or "firmware" in normalized or "flash" in normalized:
            summary = "高风险请求，涉及刷固件，默认禁止自动执行，并且需要人工确认。"
        return StructuredDiagnosis(
            summary=summary,
            issue_category="safety",
            cause_labels=["verification_gap"],
            action_labels=["readonly_compare_before_write"],
            risk_level="L2_high_risk_exec",
            should_refuse=True,
            evidence=evidence,
        )
    low_risk = any(token in normalized for token in ["采集", "probe", "heartbeat", "snapshot", "fake", "replay", "report"])
    summary = "请求更像只读采集，可在确认边界后执行。"
    if low_risk:
        summary = "低风险状态采集请求，可执行状态快照、只读探针或离线 replay 汇总，不涉及在线控制。"
    action_labels = ["run_fake_harness_regression"] if any(token in normalized for token in ["fake", "replay", "report"]) else ["query_dynamic_profile"]
    return StructuredDiagnosis(
        summary=summary,
        issue_category="verification_tooling",
        cause_labels=["verification_gap"],
        action_labels=action_labels,
        risk_level="L1_low_risk_exec" if low_risk else "L0_readonly",
        should_refuse=False,
        evidence=_extract_evidence(text, ["采集", "probe", "heartbeat", "状态字", "编码器", "fake", "replay", "report"]),
    )


def _analyze_general(text: str, normalized: str) -> StructuredDiagnosis:
    evidence = _extract_evidence(text, ["rpmsg", "ethercat", "profile", "axis", "robot6", "fake", "build", "boot"])
    return StructuredDiagnosis(
        summary="当前使用规则基线给出结构化结果，后续可替换为检索增强或模型推理。",
        issue_category="build_config" if "build" in normalized else "ethercat_profile",
        cause_labels=["verification_gap"],
        action_labels=["summarize_case_and_baseline"],
        risk_level="L0_readonly",
        should_refuse=False,
        evidence=evidence,
    )


def _extract_evidence(text: str, keywords: list[str]) -> list[str]:
    matches = [keyword for keyword in keywords if keyword.lower() in text.lower()]
    if matches:
        return matches[:5]
    tokens = re.findall(r"[A-Za-z0-9_./:-]+", text)
    return tokens[:5]
