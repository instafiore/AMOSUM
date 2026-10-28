"""Constants of amosum (mirrors core/cpp/settings.h; pure Python)."""
import re

TRUE_STR = "true"
FALSE_STR = "false"
LAZY_HYBRID = "hybrid"

# the atoms of the rewriting (the parser) and of the normalization (core/py/program.py)
PREDICATE_GROUP, PREDICATE_LB, PREDICATE_UB, PREDICATE_AUX = "__group__", "__lb__", "__ub__", "__aux__"
PREDICATE_ELEMENT, PREDICATE_TUPLE, PREDICATE_SAME = "__element__", "__tuple__", "__same__"
AMOMAXIMIZE = "amomaximize"

# the auxiliary atoms: true in the models, never printed (SETTINGS::auxRegex)
AUX_REGEX = re.compile(r"__(element|group|aux|lb|ub|tuple|same)__(\(.*)?")
