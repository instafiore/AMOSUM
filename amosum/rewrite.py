"""The amosum statements of ASP files rewritten into plain ASP by the parser, in temporary files (shared by the C++
wrapper, the Python version and amowasp)."""
import os
import re
import tempfile
from typing import List

from amosum.amosum_parser.__main__ import run as run_rewriter


def rewrite_without_amosum(path: str, directory: str) -> str:
    """the rewriting of path, in a temporary file inside directory"""
    name = re.sub(r"\.(asp|lp)$", "", os.path.basename(path))
    with tempfile.NamedTemporaryFile("w", dir=directory, prefix=f"{name}_without_amosum_", suffix=".asp", delete=False) as f:
        f.write(run_rewriter(input=path))
        return f.name


def rewrite_all(paths: List[str], directory: str) -> List[str]:
    return [rewrite_without_amosum(p, directory) for p in paths]


def shows_only_some_atoms(paths: List[str]) -> bool:
    """whether the program has #show statements (then only the shown atoms are printed and known to WASP)"""
    return any(re.search(r"#show\b", open(p).read()) for p in paths)
