#!/usr/bin/env python3
"""Write job lists for run_one.sh.

Logical inputs (same for both designs): OUT = S ? D1 : D0
  SEN (Sen et al. 2015, Fig. 3a):          S -> S, D1 -> I1, D0 -> I0
  USE (Sill Torres et al. 2018, Fig. 4):   S -> s, D1 -> a,  D0 -> b
Combination code "S D1 D0", e.g. "101" = S=1, D1=0, D0=1.
"""
import sys

DESIGNS = {
    "SEN": ("MUX_SEN.qca", {"S": "S", "D1": "I1", "D0": "I0"}),
    "USE": ("MUX_USE.qca", {"S": "s", "D1": "a", "D0": "b"}),
}
PERIODS = [10, 20, 40]
COMBOS = [f"{i:03b}" for i in range(8)]
STEP = "1e-17"
HALF = "5e-18"
NP_ENERGY = "13.5"
NP_DELAY = "12.25"  # input change at 6.125 T: >=0.1 T away from every clock-zone switching edge


def vec(design, code):
    m = DESIGNS[design][1]
    s, d1, d0 = code
    return f"{m['S']}={s},{m['D1']}={d1},{m['D0']}={d0}"


# single-input transitions used for the delay check: (name, before, after)
TRANSITIONS = [
    ("D0", "000", "001"),  # S=0: data input D0 0->1, output 0->1
    ("D1", "100", "110"),  # S=1: data input D1 0->1, output 0->1
    ("S", "001", "101"),   # D0=1, D1=0: select 0->1, output 1->0
]


def energy_jobs():
    out = []
    # longest jobs first for load balancing
    for T in sorted(PERIODS, reverse=True):
        for d in ("USE", "SEN"):
            for c in COMBOS:
                out.append(f"E_{d}_T{T}_{c} {DESIGNS[d][0]} {T} {NP_ENERGY} {STEP} {vec(d, c)}")
    return out


def half_jobs():
    return [f"H_{d}_T10_{c} {DESIGNS[d][0]} 10 {NP_ENERGY} {HALF} {vec(d, c)}" for d in ("USE", "SEN") for c in ("000", "111")]


def delay_jobs(step):
    out = []
    for d in ("USE", "SEN"):
        for T in PERIODS:
            for name, a, b in TRANSITIONS:
                out.append(f"D2_{d}_T{T}_{name}_{step} {DESIGNS[d][0]} {T} {NP_DELAY} {step} {vec(d, a)} {vec(d, b)}")
    return out


if __name__ == "__main__":
    kind = sys.argv[1]
    lines = {"energy": energy_jobs, "half": half_jobs,
             "delay": lambda: delay_jobs("1e-16"), "delay17": lambda: [j for j in delay_jobs("1e-17") if "_T10_" in j]}[kind]()
    print("\n".join(lines))
