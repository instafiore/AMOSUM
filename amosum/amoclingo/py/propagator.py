"""The clingo propagator of one amosum aggregate (mirrors amoclingo/cpp/propagator.h and propagator.cpp): it maps the
program groups to solver literals (build_solving_groups), watches them and runs one AmosumPropagator (core) per solver
thread."""
import clingo

from fiore.model.program import Program
from fiore.model.propagator_pb_constraint import PropagatorPBConstraint
from fiore.utils.constants import TOP

from amosum.core.py.program import AmosumConstraint
from amosum.core.py.propagator import FALSE, TRUE, AmosumOptions, AmosumPropagator, build_solving_groups


class AmoclingoPropagator(PropagatorPBConstraint):

    def __init__(self, constraint: AmosumConstraint, program: Program, options: AmosumOptions):
        super().__init__("amosum " + constraint.id)
        self.constraint, self.program, self.options = constraint, program, options
        self.solving_groups = None

    def init_first_time(self, init: clingo.PropagateInit) -> None:
        def solver_literal(member):
            literal = self.program.literal(member.name)
            if literal is None:
                return FALSE                    # the atom is not in the ground program: never true
            s = init.solver_literal(literal.id)
            s = s if member.positive else -s
            return TRUE if s == TOP else (FALSE if s == -TOP else s)

        # 1-3. the groups over solver literals (core); 4. the watches, both polarities
        self.solving_groups = build_solving_groups(self.constraint, solver_literal)
        bound = lambda: self.constraint.bound
        self.implementations = [AmosumPropagator(self.solving_groups, self.constraint.ge, self.constraint.eo, self.options, bound)
                                for _ in range(init.number_of_threads)]
        for l in self.solving_groups.watched:
            init.add_watch(l)
        init.add_watch(TOP)
        # 5. the __group__ atoms are false: rules added by core/py/program.py (":- __group__(...)")
