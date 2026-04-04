from __future__ import annotations

import re

from .models import StructuredDiagnosis


LOG_RULES = [
    {
        "issue_category": "verification_tooling",
        "keywords": ["com3", "ssh", "heartbeat", "verify_robot_motion", "无串口"],
        "cause_labels": ["verification_gap"],
        "action_labels": ["use_no_serial_regression"],
        "risk_level": "L1_low_risk_exec",
        "summary": "当前更像验证链路问题，建议切到低风险探针或无串口回归脚本。",
    },
    {
        "issue_category": "serial_console",
        "keywords": ["rs232", "uart", "串口", "moba"],
        "cause_labels": ["serial_connection_drift"],
        "action_labels": ["recheck_serial_path"],
        "risk_level": "L0_readonly",
        "summary": "串口链路或终端配置漂移，优先复核物理连接与串口路径。",
    },
    {
        "issue_category": "bringup_boot",
        "keywords": ["boot", "m7_app.bin", "旧镜像", "冷启动"],
        "cause_labels": ["boot_artifact_stale"],
        "action_labels": ["rebuild_and_cold_reboot", "collect_boot_context"],
        "risk_level": "L1_low_risk_exec",
        "summary": "部署产物与实际运行镜像不一致，建议冷启动并补齐启动上下文。",
    },
    {
        "issue_category": "build_config",
        "keywords": ["localhost", "nat", "cmake"],
        "cause_labels": ["toolchain_path_noise"],
        "action_labels": ["verify_build_profile"],
        "risk_level": "L0_readonly",
        "summary": "更像工具链路径噪声或 WSL/Windows 映射问题，先核验构建链路。",
    },
    {
        "issue_category": "build_config",
        "keywords": ["ddr_release", "build", "编译"],
        "cause_labels": ["build_profile_mismatch"],
        "action_labels": ["verify_build_profile"],
        "risk_level": "L0_readonly",
        "summary": "优先怀疑构建 profile 或工具链路径问题。",
    },
    {
        "issue_category": "rpc_rpmsg",
        "keywords": ["rpmsg", "openrpmsg", "ethercat_loop_task", "a53", "m7", "核间"],
        "cause_labels": ["rpc_trigger_missing"],
        "action_labels": ["probe_rpmsg_handshake"],
        "risk_level": "L0_readonly",
        "summary": "A53/M7 之间的触发或握手链路未打通。",
    },
    {
        "issue_category": "motion_param_latch",
        "keywords": ["0x6083", "0x6084", "0x60ff"],
        "cause_labels": ["param_not_latched"],
        "action_labels": ["read_back_object_dict", "freeze_stable_axis"],
        "risk_level": "L0_readonly",
        "summary": "参数写入成功但未在驱动器侧锁存，先回读对象字典并隔离稳定轴。",
    },
    {
        "issue_category": "canopen_link",
        "keywords": ["0x6041", "0x6064", "0x606c", "0x6061", "sdo"],
        "cause_labels": ["verification_gap"],
        "action_labels": ["cross_check_with_vendor_tool", "read_back_object_dict"],
        "risk_level": "L0_readonly",
        "summary": "对象字典回读已形成有效诊断证据，建议继续做上位机与 SDO 交叉验证。",
    },
    {
        "issue_category": "motion_param_latch",
        "keywords": ["axis1/node2", "axis0/node1", "双轴"],
        "cause_labels": ["axis_specific_logic_override"],
        "action_labels": ["freeze_stable_axis"],
        "risk_level": "L0_readonly",
        "summary": "稳定轴与异常轴行为不一致，优先冻结稳定轴并排查单轴覆盖逻辑。",
    },
    {
        "issue_category": "pdo_mapping",
        "keywords": ["rpdo2", "0x5437", "21559", "pdo", "在线更新"],
        "cause_labels": ["pdo_online_update_forbidden"],
        "action_labels": ["disable_risky_pdo_update"],
        "risk_level": "L2_high_risk_exec",
        "summary": "PDO 在线更新触发驱动异常，默认按高风险处理。",
    },
    {
        "issue_category": "motion_param_latch",
        "keywords": ["0x6083", "0x6084", "0x60ff", "axis0", "axis1", "速度模式"],
        "cause_labels": ["param_not_latched"],
        "action_labels": ["read_back_object_dict", "freeze_stable_axis"],
        "risk_level": "L0_readonly",
        "summary": "参数写入成功但未在驱动器侧锁存，先回读对象字典并隔离稳定轴。",
    },
]

SAFETY_HIGH_RISK = [
    "刷",
    "固件",
    "上电",
    "控制字",
    "在线",
    "rpdo2",
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
    if "帮我" in normalized or any(token in normalized for token in SAFETY_HIGH_RISK):
        return "safety"
    if any(keyword in normalized for rule in LOG_RULES for keyword in rule["keywords"]):
        return "log"
    return "general"


def _analyze_log(text: str, normalized: str) -> StructuredDiagnosis:
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
        summary = "高风险请求，涉及在线更新、控制字、刷固件或重新上电，默认禁止自动执行，并且需要人工确认。"
        if "rpdo2" in normalized:
            summary = "高风险请求，涉及 RPDO2 在线更新，默认禁止自动执行，并且需要人工确认。"
        elif "控制字" in text or "上电" in text:
            summary = "高风险请求，涉及控制字修改和重新上电，默认禁止自动执行，并且需要人工确认。"
        elif "固件" in text or "刷" in text:
            summary = "高风险请求，涉及刷固件，默认禁止自动执行，并且需要人工确认。"
        return StructuredDiagnosis(
            summary=summary,
            issue_category="safety",
            cause_labels=["verification_gap"],
            action_labels=["disable_risky_pdo_update"],
            risk_level="L2_high_risk_exec",
            should_refuse=True,
            evidence=evidence,
        )
    low_risk = any(token in normalized for token in ["采集", "probe", "heartbeat", "snapshot"])
    summary = "请求更像只读采集，可在确认边界后执行。"
    if low_risk:
        summary = "低风险采集请求，可执行状态快照或只读探针，不涉及在线控制。"
    return StructuredDiagnosis(
        summary=summary,
        issue_category="verification_tooling",
        cause_labels=["verification_gap"],
        action_labels=["use_no_serial_regression"],
        risk_level="L1_low_risk_exec" if low_risk else "L0_readonly",
        should_refuse=False,
        evidence=_extract_evidence(text, ["采集", "probe", "heartbeat", "状态字", "编码器"]),
    )


def _analyze_general(text: str, normalized: str) -> StructuredDiagnosis:
    evidence = _extract_evidence(text, ["rpmsg", "canopen", "servo", "axis", "build", "boot"])
    return StructuredDiagnosis(
        summary="当前使用规则基线给出结构化结果，后续可替换为检索增强或模型推理。",
        issue_category="build_config" if "build" in normalized else "canopen_link",
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
