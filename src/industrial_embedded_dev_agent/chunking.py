from __future__ import annotations

import json
import re
from dataclasses import asdict
from pathlib import Path

from docx import Document
from pypdf import PdfReader

from .models import DocumentChunk


TARGET_FILENAMES = {
    "Industrial Embedded Dev Agent_项目方案.md": "PROJECT-SOLUTION",
    "material_index_v1.md": "MATERIAL-INDEX",
    "labels_v1.md": "LABELS-V1",
    "TLIMX8MP-EVM_上手指南.docx": "DOC-001",
    "TLIMX8MP-EVM_3月学习总结.docx": "DOC-002",
    "3月学习过程.docx": "DOC-003",
    "1-1-调试工具安装.pdf": "DOC-004",
    "1-2-Linux开发环境搭建.pdf": "DOC-005",
    "2-1-评估板测试手册.pdf": "DOC-006",
    "2-4-GDB程序调试方法说明.pdf": "DOC-007",
    "2-11-Linux-RT系统测试手册.pdf": "DOC-008",
    "2-12-IgH EtherCAT主站开发案例.pdf": "DOC-009",
    "3-1-Linux系统使用手册.pdf": "DOC-010",
    "build_latest.log": "LOG-BUILD-LATEST",
    "worklog_2026-03-31.docx": "WORKLOG-2026-03-31",
    "worklog_2026-04-01.docx": "WORKLOG-2026-04-01",
    "worklog_2026-04-03.docx": "WORKLOG-2026-04-03",
}

SKIP_SEGMENTS = {
    "资料\\Demo",
    "资料\\imxSoem-motion-control_first",
    "资料\\imxSoem-motion-control_second",
    ".git",
    "__pycache__",
    "site-packages",
}


def build_chunks(root: Path, *, output_path: Path | None = None) -> list[DocumentChunk]:
    chunks = collect_chunks(root)
    destination = output_path or root / "data" / "chunks" / "doc_chunks_v1.jsonl"
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open("w", encoding="utf-8", newline="\n") as handle:
        for chunk in chunks:
            handle.write(json.dumps(asdict(chunk), ensure_ascii=False) + "\n")
    return chunks


def collect_chunks(root: Path) -> list[DocumentChunk]:
    chunks: list[DocumentChunk] = []
    for source_path, source_id in iter_seed_sources(root):
        text = extract_text(source_path)
        if not text.strip():
            continue
        title = source_path.stem
        source_type = _classify_source(source_path)
        for ordinal, chunk_text in enumerate(split_into_chunks(text), start=1):
            chunks.append(
                DocumentChunk(
                    chunk_id=f"{source_id}#chunk-{ordinal:03d}",
                    source_id=source_id,
                    source_type=source_type,
                    title=title,
                    source_path=str(source_path.relative_to(root)),
                    text=chunk_text,
                    ordinal=ordinal,
                )
            )
    return chunks


def iter_seed_sources(root: Path) -> list[tuple[Path, str]]:
    sources: list[tuple[Path, str]] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(root)
        rel_text = str(rel)
        if any(segment in rel_text for segment in SKIP_SEGMENTS):
            continue
        source_id = TARGET_FILENAMES.get(path.name)
        if not source_id:
            continue
        sources.append((path, source_id))
    return sorted(sources, key=lambda item: (item[1], str(item[0])))


def extract_text(path: Path) -> str:
    suffix = path.suffix.lower()
    if suffix in {".md", ".log", ".xml", ".txt"}:
        return _normalize_plain_text(path.read_text(encoding="utf-8", errors="ignore"))
    if suffix == ".docx":
        return _extract_docx(path)
    if suffix == ".pdf":
        return _extract_pdf(path)
    return ""


def split_into_chunks(text: str, *, target_chars: int = 700, max_chars: int = 950) -> list[str]:
    paragraphs = [part.strip() for part in re.split(r"\n{2,}", text) if part.strip()]
    if not paragraphs:
        paragraphs = [text.strip()]

    chunks: list[str] = []
    current: list[str] = []
    current_len = 0
    for paragraph in paragraphs:
        paragraph_len = len(paragraph)
        if current and (current_len + paragraph_len > max_chars or current_len >= target_chars):
            chunks.append("\n\n".join(current))
            current = [paragraph]
            current_len = paragraph_len
            continue
        current.append(paragraph)
        current_len += paragraph_len

    if current:
        chunks.append("\n\n".join(current))
    return chunks


def summarize_chunks(chunks: list[DocumentChunk]) -> dict[str, object]:
    by_source_type: dict[str, int] = {}
    by_source_id: dict[str, int] = {}
    for chunk in chunks:
        by_source_type[chunk.source_type] = by_source_type.get(chunk.source_type, 0) + 1
        by_source_id[chunk.source_id] = by_source_id.get(chunk.source_id, 0) + 1
    return {
        "total_chunks": len(chunks),
        "source_type_counts": by_source_type,
        "source_chunk_counts": by_source_id,
    }


def load_chunk_documents(path: Path) -> list[DocumentChunk]:
    chunks: list[DocumentChunk] = []
    if not path.exists():
        return chunks
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        raw = json.loads(line)
        chunks.append(
            DocumentChunk(
                chunk_id=raw["chunk_id"],
                source_id=raw["source_id"],
                source_type=raw["source_type"],
                title=raw["title"],
                source_path=raw["source_path"],
                text=raw["text"],
                ordinal=raw["ordinal"],
            )
        )
    return chunks


def _extract_docx(path: Path) -> str:
    doc = Document(str(path))
    paragraphs = [para.text.strip() for para in doc.paragraphs if para.text and para.text.strip()]
    return _normalize_plain_text("\n\n".join(paragraphs))


def _extract_pdf(path: Path, *, page_limit: int = 40) -> str:
    reader = PdfReader(str(path))
    pages: list[str] = []
    for page in reader.pages[:page_limit]:
        text = page.extract_text() or ""
        text = text.strip()
        if text:
            pages.append(text)
    return _normalize_plain_text("\n\n".join(pages))


def _normalize_plain_text(text: str) -> str:
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    text = re.sub(r"[ \t]+", " ", text)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip()


def _classify_source(path: Path) -> str:
    suffix = path.suffix.lower()
    if suffix == ".docx":
        return "docx"
    if suffix == ".pdf":
        return "pdf"
    if suffix == ".log":
        return "log"
    if suffix == ".xml":
        return "xml"
    return "text"
