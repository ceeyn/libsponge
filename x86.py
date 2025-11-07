#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Minimal x86(ish) assembly runner for teaching/demo purposes.

Supported:
- Labels: `label:`
- Instructions (AT&T-like):
  - mov src, %reg        ; src: $imm | %reg
  - add src, %reg        ; also: sub, inc, dec
  - cmp src, %reg        ; sets ZF from (%reg - src)
  - jmp label            ; je/jne label use ZF
  - loop label           ; decrements %cx and jump if %cx != 0
  - nop

CLI:
- -p PROGRAM.s           ; program path
- -t NUM_THREADS         ; only 1 is supported
- -i INTERVAL            ; interrupt every N instructions (default 100)
- -R REG [REG ...]       ; registers to report on interrupt (e.g., dx)
- -c                     ; check mode: print only values per interrupt

The goal is to be good enough for a simple `loop.s` exercise that
traces %dx every N instructions. This is NOT a faithful CPU emulator.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from typing import Dict, List, Tuple, Optional


# ----------------------------- Utilities -----------------------------------


def parse_int(text: str) -> int:
    text = text.strip()
    base = 10
    if text.lower().startswith("0x"):
        base = 16
    return int(text, base)


def normalize_reg(reg: str) -> str:
    assert reg.startswith("%"), f"expected register like %dx, got: {reg}"
    name = reg[1:].lower()
    # Map 32-bit to 16-bit names when convenient (edx->dx)
    if name.startswith("e") and len(name) >= 3 and name[1:] in {"ax", "bx", "cx", "dx", "si", "di", "bp", "sp"}:
        name = name[1:]
    return name


def value_of(operand: str, regs: Dict[str, int]) -> int:
    operand = operand.strip()
    if operand.startswith("$"):
        return parse_int(operand[1:])
    if operand.startswith("%"):
        return regs.get(normalize_reg(operand), 0)
    # bare number
    return parse_int(operand)


# ---------------------------- Program Model ---------------------------------


@dataclass
class Instruction:
    op: str
    args: Tuple[str, ...]


@dataclass
class Program:
    instructions: List[Instruction]
    labels: Dict[str, int]

    @staticmethod
    def parse(text: str) -> "Program":
        instructions: List[Instruction] = []
        labels: Dict[str, int] = {}

        # Remove comments starting with ';' or '#' (common in examples)
        comment_re = re.compile(r"[;#].*$")

        for raw_line in text.splitlines():
            line = comment_re.sub("", raw_line).strip()
            if not line:
                continue

            # label
            if line.endswith(":"):
                label = line[:-1].strip()
                if not label:
                    raise ValueError(f"empty label in line: {raw_line}")
                labels[label] = len(instructions)
                continue

            # instruction
            parts = [p.strip() for p in line.split(None, 1)]
            op = parts[0].lower()
            args: Tuple[str, ...] = tuple()
            if len(parts) == 2:
                args_str = parts[1]
                args = tuple([a.strip() for a in args_str.split(",") if a.strip()])
            instructions.append(Instruction(op, args))

        return Program(instructions=instructions, labels=labels)


# ----------------------------- CPU Runner -----------------------------------


class SimpleCPU:
    def __init__(self) -> None:
        # Use 16-bit GPRs by name. Values are Python ints.
        self.regs: Dict[str, int] = {
            "ax": 0,
            "bx": 0,
            "cx": 0,
            "dx": 0,
            "si": 0,
            "di": 0,
            "bp": 0,
            "sp": 0,
        }
        self.ip: int = 0
        self.zf: int = 0  # zero flag

    # --- helpers ---
    def set_reg(self, reg: str, value: int) -> None:
        name = normalize_reg(reg)
        self.regs[name] = value & 0xFFFF  # 16-bit wrap to be predictable

    def get_reg(self, reg: str) -> int:
        return self.regs.get(normalize_reg(reg), 0)

    # --- execution ---
    def step(self, prog: Program) -> None:
        if not (0 <= self.ip < len(prog.instructions)):
            raise StopIteration

        ins = prog.instructions[self.ip]
        op = ins.op
        args = ins.args

        # Default: advance to next instruction
        next_ip = self.ip + 1

        def arith(op_name: str, src: str, dst_reg: str) -> None:
            src_val = value_of(src, self.regs)
            dst_val = self.get_reg(dst_reg)
            if op_name == "add":
                res = dst_val + src_val
            elif op_name == "sub":
                res = dst_val - src_val
            else:
                raise ValueError(f"unsupported arith op: {op_name}")
            self.set_reg(dst_reg, res)

        try:
            if op in ("mov", "movw", "movl"):
                assert len(args) == 2, f"{op} needs two operands"
                src, dst = args
                self.set_reg(dst, value_of(src, self.regs))
            elif op in ("add", "sub"):
                assert len(args) == 2
                arith(op, args[0], args[1])
            elif op == "inc":
                assert len(args) == 1
                self.set_reg(args[0], self.get_reg(args[0]) + 1)
            elif op == "dec":
                assert len(args) == 1
                self.set_reg(args[0], self.get_reg(args[0]) - 1)
            elif op == "cmp":
                assert len(args) == 2
                src, dst = args
                # AT&T style: cmp src, dst  => dst - src
                self.zf = int((self.get_reg(dst) - value_of(src, self.regs)) == 0)
            elif op == "jmp":
                assert len(args) == 1
                lbl = args[0]
                if lbl not in prog.labels:
                    raise KeyError(f"unknown label: {lbl}")
                next_ip = prog.labels[lbl]
            elif op in ("je", "jz"):
                assert len(args) == 1
                if self.zf == 1:
                    lbl = args[0]
                    if lbl not in prog.labels:
                        raise KeyError(f"unknown label: {lbl}")
                    next_ip = prog.labels[lbl]
            elif op in ("jne", "jnz"):
                assert len(args) == 1
                if self.zf == 0:
                    lbl = args[0]
                    if lbl not in prog.labels:
                        raise KeyError(f"unknown label: {lbl}")
                    next_ip = prog.labels[lbl]
            elif op == "loop":
                assert len(args) == 1
                new_cx = (self.regs.get("cx", 0) - 1) & 0xFFFF
                self.regs["cx"] = new_cx
                if new_cx != 0:
                    lbl = args[0]
                    if lbl not in prog.labels:
                        raise KeyError(f"unknown label: {lbl}")
                    next_ip = prog.labels[lbl]
            elif op == "nop":
                pass
            else:
                raise NotImplementedError(f"unsupported instruction: {op} {args}")
        finally:
            self.ip = next_ip


# ------------------------------- Runner -------------------------------------


def run_program(
    program_text: str,
    interrupt_interval: int,
    trace_regs: List[str],
    check_mode: bool,
) -> None:
    prog = Program.parse(program_text)
    cpu = SimpleCPU()

    trace_regs = [r.lower().lstrip("%") for r in trace_regs]

    steps = 0
    max_steps = 10_000_000  # safety cap

    def emit_interrupt() -> None:
        if check_mode:
            values = [str(cpu.regs.get(r, 0)) for r in trace_regs]
            print(" ".join(values))
        else:
            reg_dump = ", ".join(f"%{r}={cpu.regs.get(r, 0)}" for r in trace_regs)
            print(f"[instr={steps}] {reg_dump}")

    try:
        while True:
            cpu.step(prog)
            steps += 1
            if interrupt_interval > 0 and steps % interrupt_interval == 0:
                emit_interrupt()
            if steps >= max_steps:
                print("[warn] step limit reached; stopping.")
                break
    except StopIteration:
        pass

    # Final interrupt on program end if no exact multiple reached
    if interrupt_interval == 0:
        # If interval is 0 we never emit; still show final state
        emit_interrupt()


def main(argv: Optional[List[str]] = None) -> int:
    p = argparse.ArgumentParser(description="Very small x86-like runner for simple exercises")
    p.add_argument("-p", dest="program", required=True, help="path to assembly program (.s)")
    p.add_argument("-t", dest="threads", type=int, default=1, help="number of threads (only 1 supported)")
    p.add_argument("-i", dest="interval", type=int, default=100, help="interrupt interval in instructions")
    p.add_argument("-R", dest="regs", nargs="+", default=["dx"], help="registers to trace on interrupt")
    p.add_argument("-c", dest="check", action="store_true", help="check mode: print values only")

    args = p.parse_args(argv)

    if args.threads != 1:
        print("error: this minimal runner only supports -t 1 (single thread)", file=sys.stderr)
        return 2

    try:
        with open(args.program, "r", encoding="utf-8") as f:
            program_text = f.read()
    except FileNotFoundError:
        print(f"error: program not found: {args.program}", file=sys.stderr)
        return 2

    run_program(program_text, interrupt_interval=args.interval, trace_regs=args.regs, check_mode=args.check)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())



