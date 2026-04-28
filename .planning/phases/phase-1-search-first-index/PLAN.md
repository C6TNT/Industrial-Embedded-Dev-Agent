# Phase 1 Plan: Search-First Engineering Index

## Goal

Make `D:\桌面\test` searchable and understandable before building any heavy knowledge base.

## Tasks

1. Inventory source, docs, tools, logs, XML, JSON profiles, reports, manuals, and generated artifacts.
2. Add a compact file classification output that explains what each important folder is for.
3. Add targeted search wrappers for:
   - C/C++ and headers;
   - Python tools;
   - Markdown docs;
   - XML/ESI files;
   - JSON profiles/reports;
   - logs.
4. Keep raw search results source-backed and easy for an Agent to open.

## Completion Criteria

- A command or report can answer “where should I search for this problem?”
- Search works without pre-ingesting all files into a knowledge base.
- Generated inventory excludes obvious cache/build noise.

## Current Implementation

- `spindle tools material-inventory`
- `spindle tools material-status`
- `spindle tools material-search "<query>" --scope docs|source|tools|xml|profiles|logs|all`

## Safety

Offline only. Do not run board-facing scripts.
