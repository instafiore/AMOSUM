"""A model as printed (mirrors ClingoAssignment of fiore/cpp/fiore/model/assigment.h; pure Python): its shown atoms
(the hidden ones left out), its number, the result and the time of its solve call, the printed lines. Also the
interruption of a run (mirrors signalHandler of amosum/amoclingo/cpp/main.cpp): the best answer set known is printed."""
import os
import re
import signal
import sys
import time
from typing import Callable, List, Optional

from fiore.clingo.utility_clingo import ClingoResult
from fiore.utils.constants import ERROR_CODE

_start = time.time()


def wall_time() -> float:
    return time.time() - _start


class ClingoAssignment:
    _printed = 0            # answer sets printed so far (AnswersetsPool::getNextIdAnswersetPrinted)
    best_known: Optional["ClingoAssignment"] = None     # the last one printed (AnswersetsPool::getBestKnownAnswerSet)

    def __init__(self, shown: List[str], solve_time: float = 0.0, avoid_to_show: Optional[re.Pattern] = None):
        self.shown = [a for a in shown if avoid_to_show is None or not avoid_to_show.fullmatch(a)]
        self.solve_time = solve_time
        self.number: Optional[int] = None
        self.result = ClingoResult.satisfiable()

    def output_string(self) -> str:
        return "{ " + ", ".join(f"'{a}'" for a in self.shown) + "}"

    def to_output_string(self) -> str:
        if self.number is None:
            ClingoAssignment._printed += 1
            self.number = ClingoAssignment._printed
        prefix = "Optimum " if self.result.optimum else ("Unkown " if self.result.unknown_optimum else "")
        time_text = f"Time {self.solve_time:g} " if self.solve_time != 0 else ""
        return f"{time_text}{prefix}AnswerSet {self.number}° : {self.output_string()}"

    def print_assignment(self, prefix: str = "") -> None:
        print(f"{prefix}Wall Time: {wall_time():f}s {self.to_output_string()}", flush=True)
        ClingoAssignment.best_known = self


def print_final(best: Optional[ClingoAssignment], result: ClingoResult) -> None:
    """the last line of a run: the best answer set with the final result, or UNSATISFIABLE / UNKNOWN"""
    if result.is_sat() and best is not None:
        best.result = result
        print(best.to_output_string(), flush=True)
    else:
        print("UNSATISFIABLE" if result.is_unsat() else "UNKNOWN", flush=True)


# ---------------------------------------------------------------------------------------------------------------------
# interruption (SIGINT, SIGTERM): as amoclingo-core

_cleanups: List[Callable[[], None]] = []


def on_interrupt(signum, frame) -> None:
    """the best answer set known, not proven optimal (exit code 29), or "No solution found" (exit code 1, as
    amoclingo-core); then the cleanups and the exit. It does not raise: a signal often arrives inside a callback of
    clingo (the propagator), where an exception would become a clingo error."""
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    code = ERROR_CODE
    try:
        best = ClingoAssignment.best_known
        if best is not None:
            best.result = ClingoResult.satisfiable().set_unknown_optimum()
            best.print_assignment()
            code = best.result.exit_code()
        else:
            print(f"Wall Time: {wall_time():f}s No solution found", flush=True)
    except Exception:
        pass                # e.g. the signal arrived while stdout was being written
    for cleanup in _cleanups:
        try:
            cleanup()
        except Exception:
            pass
    try:
        sys.stdout.flush()
    except Exception:
        pass
    os._exit(code)          # no Python cleanup: clingo may be in the middle of a solve call


def handle_interrupts(*cleanups: Callable[[], None]) -> None:
    """SIGINT and SIGTERM end the run with on_interrupt; the cleanups run before the exit"""
    _cleanups.extend(cleanups)
    signal.signal(signal.SIGINT, on_interrupt)
    signal.signal(signal.SIGTERM, on_interrupt)


def add_interrupt_cleanup(cleanup: Callable[[], None]) -> None:
    _cleanups.append(cleanup)


def remove_interrupt_cleanup(cleanup: Callable[[], None]) -> None:
    if cleanup in _cleanups:
        _cleanups.remove(cleanup)
