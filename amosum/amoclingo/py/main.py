"""amoclingo --lang py (mirrors amoclingo/cpp/main.cpp): the options that change the search (the others are for
amoclingo-core's log and stats), the Runner, the exit code."""
import argparse

from fiore.model.assignment import handle_interrupts

from amosum.amoclingo.py.runner import Runner


def main(known: argparse.Namespace) -> int:
    # SIGINT/SIGTERM: the best answer set known, then exit (as amoclingo-core's signalHandler)
    handle_interrupts()
    print("amoclingo version 3.0 (python)", flush=True)
    result = Runner().run(known.inputs, reason=known.reason or "nomin", lazy=known.lazy or "false",
                          strategy=known.opt_strategy or "sat-unsat", progression=known.progression or "1")
    return result.exit_code()
