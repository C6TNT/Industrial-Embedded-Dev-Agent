# Test Double and Verification Layers - 2026-04-29

Project: Industrial Embedded Dev Agent / EtherCAT Dynamic Profile

Boundary: `offline_ok`

This public-safe material defines how the platform uses stubs, fakes, mocks,
and verification layers before a new EtherCAT driver or IO module is allowed to
enter real Huichuan testing.

## Definitions

Stub:

- a fixed or lightly parameterized response;
- used when the caller only needs a known answer;
- examples: fixed query JSON, fixed IO input sample, fixed status snapshot.

Fake:

- a runnable replacement with simplified but meaningful domain behavior;
- used when the contract needs to execute through a small offline world;
- examples: `ethercat-fake-harness`, XML regression, SOEM trace regression,
  replay regression, profile/schema drift checks.

Mock:

- an assertion about interaction, permission, or call shape;
- used when the important question is whether the system planned, refused, or
  escalated the correct action;
- examples: Agent tool-safety benchmark refusing `start-bus`, `0x41F1`, IO
  output, remoteproc reload, or robot motion.

## Verification Layers

L0 unit/static:

- parser, validator, pure PDO codec, profile schema, and static checks;
- boundary: `offline_ok`.

L1 stub/mock:

- fixed snapshots and Agent safety/tool-gate assertions;
- boundary: `offline_ok`.

L2 fake integration:

- XML/SOEM/replay through the fake harness;
- boundary: `offline_ok`.

L3 offline acceptance:

- one-command evidence bundle and latest summary;
- boundary: `offline_ok`.

L4 board read-only:

- real board status/profile/snapshot reads only;
- boundary: `board_required`.

L5 IO hardware:

- real IO input/output validation;
- boundary: `io_required`.

L6 robot motion:

- drive enable, `0x86`, `0x41F1`, takeover, homing, welding, or robot movement;
- boundary: `robot_motion_required`.

## Required Gate Before Real Testing

A new EtherCAT driver or IO module must pass this sequence before Huichuan
runtime testing is requested:

1. sanitize the evidence;
2. create a trace, replay, XML/ESI, profile, or IO layout fixture;
3. validate the profile candidate and run schema drift checks;
4. pass relevant fake-harness regression;
5. add Spindle Agent material plus safety Q&A or tool-gate coverage;
6. classify the Huichuan entry path as `board_required`, `io_required`, or
   `robot_motion_required`;
7. use an approved run sheet with stop conditions.

The gate order is:

```text
sanitized trace/replay fixture
  -> fake regression
  -> Agent safety Q&A and tool gate
  -> approved Huichuan read-only or IO boundary
```

Passing the offline gates does not authorize board, bus, output gate, IO,
firmware, or robot-motion actions. It only means the candidate is ready to ask
for the next hardware boundary.

## Project Roles

`huichuan-robot-runtime` owns real hardware truth and approved run sheets.

`ethercat-fake-harness` owns the executable offline fake world and regression
summaries.

`spindle-agent` owns public-safe knowledge, chunks, benchmark coverage, and
hardware-action refusal/escalation behavior.

## Reuse Rule

Every new device should make the next device cheaper:

- keep sanitized fixture provenance;
- preserve fake regression evidence;
- add at least one benchmark question when a new risk or device class appears;
- promote only verified facts into public-facing material;
- never let an Agent answer or fake report directly execute hardware actions.

