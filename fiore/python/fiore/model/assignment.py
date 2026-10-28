"""A model as printed (mirrors ClingoAssignment of fiore/cpp/fiore/model/assigment.h; pure Python): its shown atoms
(the hidden ones left out), its number, the result and the time of its solve call, the printed lines."""
import re
import time
from typing import List, Optional

from fiore.clingo.utility_clingo import ClingoResult

_start = time.time()


def wall_time() -> float:
    return time.time() - _start


class ClingoAssignment:
    _printed = 0            # answer sets printed so far (AnswersetsPool::getNextIdAnswersetPrinted)

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


def print_final(best: Optional[ClingoAssignment], result: ClingoResult) -> None:
    """the last line of a run: the best answer set with the final result, or UNSATISFIABLE / UNKNOWN"""
    if result.is_sat() and best is not None:
        best.result = result
        print(best.to_output_string(), flush=True)
    else:
        print("UNSATISFIABLE" if result.is_unsat() else "UNKNOWN", flush=True)
