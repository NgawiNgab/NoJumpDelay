# Apollon Kill Aura

The independent native implementation, build and tests live in
[`fresh-port/`](fresh-port/). The earlier root implementation is not used.

The mod installs combat hooks only when twelve in-memory game prologues match.
**Device behavior and 1:1 parity have not been verified**: function signatures,
object layouts and game allocation/lifetime assumptions may be wrong. See the
[research report](research/KILLAURA_1_26_60_RE.md) and
[port documentation](fresh-port/README.md) before testing.
