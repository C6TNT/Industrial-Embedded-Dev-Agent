# Legacy Materials Ingestion Policy - 2026-04-28

This material records how legacy project folders can feed Spindle Agent without
turning the public GitHub project into a dump of private source, raw logs, or
hardware execution scripts.

## Source Groups Reviewed

Private source group A is a personal documentation folder containing Word/PDF
manuals, summaries, and an internship log around i.MX8MP, RPMsg, CANopen,
single-drive EtherCAT, multi-drive EtherCAT, robot position mode, and fake
harness work.

Private source group B is a legacy `imxSoem-motion-control` engineering tree
containing historical M7 firmware, FlexCAN/CANopen code, RPMsg/M7 notes, VS Code
configuration, build outputs, logs, binary artifacts, and deployment scripts.

## Import Decision

Source group A is high-value for Spindle, but only after deduplication and
sanitization. Prefer the latest `.docx` source over duplicate exported PDFs, and
extract conclusions instead of copying full documents.

Source group B is high-value historical context, but it must not be imported as
raw source. Use it as a private reference to write summaries about lessons,
interfaces, failure modes, and migration history.

## Allowed Public Material

- Sanitized summaries of verified engineering lessons.
- Timeline of documentation and project evolution.
- Interface-level notes such as RPDO/SDO roles, feedback fields, and validation
  order.
- Historical problem patterns after removing board addresses, credentials,
  deployment paths, and raw logs.
- Benchmark questions that test safety boundaries and engineering recall.

## Excluded From Public Spindle

- `.git` content or repository metadata.
- `.vscode` configuration.
- build directories, CMake cache/output, `debug`, `release`, `ddr_release`, and
  `CMakeFiles`.
- binary or generated artifacts such as `.elf`, `.bin`, `.o`, `.obj`, `.map`,
  and `.so`.
- raw source-code trees from the legacy hardware project.
- raw private logs and operator notes.
- deployment scripts that contain board login strings, board addresses, local
  absolute paths, SSH/SCP/reboot flows, or serial-port assumptions.
- vendor manuals copied verbatim.

## Boundary

This ingestion work is `offline_ok`. It does not authorize board access,
SSH/SCP, reboot, deployment, remoteproc reload, bus control, `0x86`, `0x41F1`,
takeover, IO output, or robot motion.

## Promotion Rule

Every legacy fact promoted into Spindle should answer three questions:

1. Is this a verified engineering conclusion rather than a raw private detail?
2. Can the fact be stated without board addresses, credentials, private paths,
   or operator-only notes?
3. Does the fact improve search, benchmark coverage, safety audit, or future
   fixture/replay design?

If any answer is no, keep the material private and do not add it to the public
Agent knowledge base.
