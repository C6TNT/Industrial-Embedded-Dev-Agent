from __future__ import annotations

import hashlib
import json
import re
from dataclasses import asdict
from io import BytesIO
from pathlib import Path

from docx import Document
from docx.document import Document as DocxDocument
from docx.table import Table, _Cell
from docx.text.paragraph import Paragraph
from lxml import etree
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
    raw_chunks: list[DocumentChunk] = []
    for source_path, source_id in iter_seed_sources(root):
        title = source_path.stem
        source_type = _classify_source(source_path)
        extracted_blocks = extract_blocks(source_path)
        if not extracted_blocks:
            continue
        for ordinal, block in enumerate(extracted_blocks, start=1):
            raw_chunks.append(
                DocumentChunk(
                    chunk_id=f"{source_id}#chunk-{ordinal:03d}",
                    source_id=source_id,
                    source_type=source_type,
                    title=title,
                    source_path=str(source_path.relative_to(root)),
                    text=block["text"],
                    ordinal=ordinal,
                    section_title=block.get("section_title", ""),
                    content_kind=block.get("content_kind", "text"),
                )
            )
    return _finalize_chunks(raw_chunks)


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


def extract_blocks(path: Path) -> list[dict[str, str]]:
    suffix = path.suffix.lower()
    if suffix in {".md", ".log", ".xml", ".txt"}:
        return _extract_plain_blocks(path)
    if suffix == ".docx":
        return _extract_docx_blocks(path)
    if suffix == ".pdf":
        return _extract_pdf_blocks(path)
    return []


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
    by_content_kind: dict[str, int] = {}
    for chunk in chunks:
        by_source_type[chunk.source_type] = by_source_type.get(chunk.source_type, 0) + 1
        by_source_id[chunk.source_id] = by_source_id.get(chunk.source_id, 0) + 1
        by_content_kind[chunk.content_kind] = by_content_kind.get(chunk.content_kind, 0) + 1
    return {
        "total_chunks": len(chunks),
        "source_type_counts": by_source_type,
        "source_chunk_counts": by_source_id,
        "content_kind_counts": by_content_kind,
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
                section_title=raw.get("section_title", ""),
                content_kind=raw.get("content_kind", "text"),
            )
        )
    return chunks


def _extract_plain_blocks(path: Path) -> list[dict[str, str]]:
    text = _normalize_plain_text(path.read_text(encoding="utf-8", errors="ignore"))
    if not text:
        return []
    blocks = []
    for chunk_text in split_into_chunks(text):
        blocks.append(
            {
                "text": chunk_text,
                "section_title": path.stem,
                "content_kind": "text",
            }
        )
    return blocks


def _extract_docx_blocks(path: Path) -> list[dict[str, str]]:
    doc = _open_docx(path)
    blocks: list[dict[str, str]] = []
    heading_stack: list[str] = [path.stem]
    paragraph_buffer: list[str] = []

    def flush_paragraphs() -> None:
        nonlocal paragraph_buffer
        if not paragraph_buffer:
            return
        combined = _normalize_plain_text("\n\n".join(paragraph_buffer))
        for chunk_text in split_into_chunks(combined):
            blocks.append(
                {
                    "text": chunk_text,
                    "section_title": " / ".join(heading_stack),
                    "content_kind": "text",
                }
            )
        paragraph_buffer = []

    for block in _iter_docx_blocks(doc):
        if isinstance(block, Paragraph):
            text = _normalize_plain_text(block.text)
            if not text:
                continue
            style_name = (block.style.name or "").lower()
            if _is_heading_paragraph(style_name, text):
                flush_paragraphs()
                level = _heading_level(style_name, text)
                heading_stack = _update_heading_stack(heading_stack, text, level)
                blocks.append(
                    {
                        "text": text,
                        "section_title": " / ".join(heading_stack),
                        "content_kind": "heading",
                    }
                )
                continue
            paragraph_buffer.append(text)
            continue

        if isinstance(block, Table):
            flush_paragraphs()
            blocks.extend(_extract_docx_table_blocks(block, heading_stack))

    flush_paragraphs()
    return blocks


def _extract_pdf_blocks(path: Path, *, page_limit: int = 40) -> list[dict[str, str]]:
    reader = PdfReader(str(path))
    blocks: list[dict[str, str]] = []
    current_section = path.stem
    paragraph_buffer: list[str] = []

    def flush_paragraphs() -> None:
        nonlocal paragraph_buffer
        if not paragraph_buffer:
            return
        combined = _normalize_plain_text("\n\n".join(paragraph_buffer))
        for chunk_text in split_into_chunks(combined):
            blocks.append(
                {
                    "text": chunk_text,
                    "section_title": current_section,
                    "content_kind": "text",
                }
            )
        paragraph_buffer = []

    for page_number, page in enumerate(reader.pages[:page_limit], start=1):
        text = page.extract_text() or ""
        lines = [_normalize_plain_text(line) for line in text.splitlines()]
        lines = [line for line in lines if line]
        for line in lines:
            if _is_pdf_heading(line):
                flush_paragraphs()
                current_section = f"{path.stem} / {line}"
                blocks.append(
                    {
                        "text": line,
                        "section_title": current_section,
                        "content_kind": "heading",
                    }
                )
            elif _looks_like_table_row(line):
                flush_paragraphs()
                blocks.append(
                    {
                        "text": f"page: {page_number}；row: {line}",
                        "section_title": current_section,
                        "content_kind": "table_row",
                    }
                )
            else:
                paragraph_buffer.append(f"[page {page_number}] {line}")

    flush_paragraphs()
    return blocks


def _normalize_plain_text(text: str) -> str:
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    text = re.sub(r"[ \t]+", " ", text)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip()


def _finalize_chunks(chunks: list[DocumentChunk]) -> list[DocumentChunk]:
    deduped: list[DocumentChunk] = []
    seen_signatures: set[tuple[str, str, str, str]] = set()

    for chunk in chunks:
        normalized_text = _normalize_for_dedup(chunk.text)
        if not _passes_quality_gate(chunk, normalized_text):
            continue
        signature = (
            chunk.source_id,
            chunk.content_kind,
            chunk.section_title.strip().lower(),
            hashlib.sha1(normalized_text.encode("utf-8")).hexdigest(),
        )
        if signature in seen_signatures:
            continue
        seen_signatures.add(signature)
        deduped.append(chunk)

    finalized: list[DocumentChunk] = []
    by_source: dict[str, list[DocumentChunk]] = {}
    for chunk in deduped:
        by_source.setdefault(chunk.source_id, []).append(chunk)

    for source_id, source_chunks in by_source.items():
        for ordinal, chunk in enumerate(source_chunks, start=1):
            finalized.append(
                DocumentChunk(
                    chunk_id=f"{source_id}#chunk-{ordinal:03d}",
                    source_id=chunk.source_id,
                    source_type=chunk.source_type,
                    title=chunk.title,
                    source_path=chunk.source_path,
                    text=chunk.text,
                    ordinal=ordinal,
                    section_title=chunk.section_title,
                    content_kind=chunk.content_kind,
                )
            )
    return finalized


def _passes_quality_gate(chunk: DocumentChunk, normalized_text: str) -> bool:
    if not normalized_text:
        return False

    length = len(normalized_text)
    if chunk.content_kind == "heading":
        if length < 12:
            return False
        if _is_noise_heading(normalized_text):
            return False
        return True

    if chunk.content_kind == "table_row":
        return length >= 16 and normalized_text.count(":") >= 1

    if length < 40:
        return False
    if _looks_like_noise_paragraph(normalized_text):
        return False
    return True


def _normalize_for_dedup(text: str) -> str:
    text = text.lower().strip()
    text = re.sub(r"\s+", " ", text)
    return text


def _is_noise_heading(text: str) -> bool:
    if text in {"目录", "contents", "附录", "修订记录"}:
        return True
    if re.fullmatch(r"[0-9.]+", text):
        return True
    if re.fullmatch(r"第?[一二三四五六七八九十0-9]+页", text):
        return True
    return False


def _looks_like_noise_paragraph(text: str) -> bool:
    if text.startswith("[page ") and len(text) < 55:
        return True
    if re.fullmatch(r"[0-9a-zA-Z _./:-]+", text) and len(text) < 55:
        return True
    if text.count("->") >= 4 and len(text) < 80:
        return True
    return False


def _open_docx(path: Path) -> DocxDocument:
    data = path.read_bytes()
    return Document(BytesIO(data))


def _iter_docx_blocks(parent):
    if isinstance(parent, DocxDocument):
        parent_elm = parent.element.body
    elif isinstance(parent, _Cell):
        parent_elm = parent._tc
    else:
        raise TypeError(f"Unsupported parent type: {type(parent)!r}")

    for child in parent_elm.iterchildren():
        if child.tag.endswith("}p"):
            yield Paragraph(child, parent)
        elif child.tag.endswith("}tbl"):
            yield Table(child, parent)


def _extract_docx_table_blocks(table: Table, heading_stack: list[str]) -> list[dict[str, str]]:
    rows = [[_normalize_plain_text(cell.text) for cell in row.cells] for row in table.rows]
    rows = [[cell for cell in row if cell] for row in rows if any(row)]
    if not rows:
        return []

    blocks: list[dict[str, str]] = []
    headers = rows[0]
    if headers:
        blocks.append(
            {
                "text": "；".join(headers),
                "section_title": " / ".join(heading_stack),
                "content_kind": "table_header",
            }
        )

    for row in rows[1:] if len(rows) > 1 else []:
        pairs = []
        for index, value in enumerate(row):
            header = headers[index] if index < len(headers) else f"col_{index + 1}"
            pairs.append(f"{header}: {value}")
        blocks.append(
            {
                "text": "；".join(pairs),
                "section_title": " / ".join(heading_stack),
                "content_kind": "table_row",
            }
        )
    return blocks


def _is_heading_paragraph(style_name: str, text: str) -> bool:
    if "heading" in style_name or "标题" in style_name:
        return True
    return _looks_like_structured_heading(text)


def _heading_level(style_name: str, text: str) -> int:
    matched = re.search(r"(\d+)", style_name)
    if matched:
        return max(1, min(6, int(matched.group(1))))
    structured = re.match(r"^([一二三四五六七八九十]+|[0-9]+)[、.．]", text)
    if structured:
        return 1
    nested = re.match(r"^[0-9]+(\.[0-9]+)+", text)
    if nested:
        return min(6, nested.group(0).count(".") + 1)
    return 2


def _update_heading_stack(stack: list[str], text: str, level: int) -> list[str]:
    base = stack[: max(1, level)]
    if len(base) < level:
        base.extend([""] * (level - len(base)))
    if base:
        base = base[: level]
    return [base[0] if base else text, *base[1:], text] if level == 1 else [stack[0], *stack[1 : max(1, level - 1)], text]


def _looks_like_structured_heading(text: str) -> bool:
    return bool(
        re.match(r"^([一二三四五六七八九十]+|[0-9]+)[、.．]", text)
        or re.match(r"^[0-9]+(\.[0-9]+)+", text)
        or re.match(r"^第[一二三四五六七八九十0-9]+[章节部分]", text)
    )


def _is_pdf_heading(line: str) -> bool:
    if len(line) > 80:
        return False
    if _looks_like_structured_heading(line):
        return True
    if re.match(r"^(chapter|section)\s+\d+", line, flags=re.IGNORECASE):
        return True
    if re.match(r"^[A-Z][A-Z0-9 /_-]{4,}$", line):
        return True
    return False


def _looks_like_table_row(line: str) -> bool:
    if "|" in line or "\t" in line:
        return True
    if re.search(r"\S+\s{2,}\S+", line):
        return True
    return False


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
