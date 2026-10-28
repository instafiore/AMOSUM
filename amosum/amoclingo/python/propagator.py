"""The clingo propagator of one amosum aggregate (port of AmoclingoPropagator): the engine of amosum.core.engine, one
per solver thread, behind fiore's EnginePropagator."""
import clingo

from fiore.program import Program
from fiore.propagator import TOP, EnginePropagator

from amosum.core.constraint import AmosumConstraint
from amosum.core.engine import FALSE, TRUE, AmosumEngine, Options, ProgramGroupData, build_solving_groups


class AmosumPropagator(EnginePropagator):

    def __init__(self, constraint: AmosumConstraint, program: Program, options: Options):
        super().__init__()
        self.constraint, self.program, self.options = constraint, program, options

    def build(self, init: clingo.PropagateInit) -> None:
        def solver_literal(plit):
            if plit is None:
                return FALSE                    # the atom is not in the ground program: never true
            s = init.solver_literal(abs(plit))
            s = s if plit > 0 else -s
            return TRUE if s == TOP else (FALSE if s == -TOP else s)

        def program_literal(member):
            literal = self.program.literal(member.name)
            if literal is None:
                return None
            return literal.id if member.positive else -literal.id

        groups = [ProgramGroupData(g.id, [(program_literal(m), m.weight) for m in g.members], g.aux)
                  for g in self.constraint.groups]
        shared = build_solving_groups(groups, solver_literal)
        bound = lambda: self.constraint.bound
        self.engines = [AmosumEngine(shared, self.constraint.ge, self.constraint.eo, self.options, bound)
                        for _ in range(init.number_of_threads)]
        for l in shared.watched:
            init.add_watch(l)
        init.add_watch(TOP)
