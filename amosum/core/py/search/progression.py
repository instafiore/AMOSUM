"""--progression: how far the bound moves at step n = 0, 1, 2, ... (mirrors core/cpp/search/progression.cpp).

An expression in n with integers, n, + - * ^ and parentheses: 1 (linear, the default), 2, 2^n, n^2, 3*n+1, ...
A value below 1 counts as 1, values are capped at 2^62.

    expr  := term (('+'|'-') term)*        term  := unary ('*' unary)*
    unary := '-' unary | power             power := atom ('^' unary)?      (right associative)
    atom  := number | 'n' | '(' expr ')'
"""

CAP = 1 << 62


def _clamp(v: int) -> int:
    return CAP if v > CAP else (-CAP if v < -CAP else v)


def _power(base: int, exponent: int) -> int:
    if exponent < 0:
        return 0
    result = 1
    for _ in range(exponent):
        result *= base
        if result > CAP or result < -CAP:
            return _clamp(result)
    return result


class _Parser:
    def __init__(self, text: str):
        self.text, self.pos = text, 0

    def fail(self, what: str):
        raise ValueError(f"--progression {self.text}: {what} at position {self.pos + 1}")

    def skip(self):
        while self.pos < len(self.text) and self.text[self.pos].isspace():
            self.pos += 1

    def accept(self, c: str) -> bool:
        self.skip()
        if self.pos < len(self.text) and self.text[self.pos] == c:
            self.pos += 1
            return True
        return False

    # a node: ('0', value) | ('n',) | ('u', child) | (op, left, right)
    def expr(self):
        node = self.term()
        while True:
            if self.accept("+"): node = ("+", node, self.term())
            elif self.accept("-"): node = ("-", node, self.term())
            else: return node

    def term(self):
        node = self.unary()
        while self.accept("*"):
            node = ("*", node, self.unary())
        return node

    def unary(self):
        if self.accept("-"):
            return ("u", self.unary())
        base = self.atom()
        if self.accept("^"):
            return ("^", base, self.unary())
        return base

    def atom(self):
        self.skip()
        if self.pos >= len(self.text):
            self.fail("unexpected end")
        if self.accept("("):
            node = self.expr()
            if not self.accept(")"):
                self.fail("')' expected")
            return node
        if self.accept("n"):
            return ("n",)
        if not self.text[self.pos].isdigit():
            self.fail(f"unexpected '{self.text[self.pos]}'")
        value = 0
        while self.pos < len(self.text) and self.text[self.pos].isdigit():
            value = min(value * 10 + int(self.text[self.pos]), CAP)
            self.pos += 1
        return ("0", value)

    def parse(self):
        node = self.expr()
        self.skip()
        if self.pos != len(self.text):
            self.fail(f"unexpected '{self.text[self.pos]}'")
        return node


def _evaluate(node, n: int) -> int:
    op = node[0]
    if op == "0": return node[1]
    if op == "n": return n
    if op == "u": return -_evaluate(node[1], n)
    a, b = _evaluate(node[1], n), _evaluate(node[2], n)
    if op == "+": return _clamp(a + b)
    if op == "-": return _clamp(a - b)
    if op == "*": return _clamp(a * b)
    return _power(a, b)


def _uses_step(node) -> bool:
    return node[0] == "n" or any(isinstance(c, tuple) and _uses_step(c) for c in node[1:])


class Progression:
    def __init__(self, expression: str):
        self.expression = expression
        self.root = _Parser(expression).parse()           # ValueError on a malformed expression
        # 1 at every step: the bound of sat-unsat never goes back, so its clauses can stay learnt
        self.linear = not _uses_step(self.root) and self.at(0) == 1

    def at(self, n: int) -> int:
        return max(1, _evaluate(self.root, min(n, CAP)))

    def is_linear(self) -> bool:
        return self.linear
