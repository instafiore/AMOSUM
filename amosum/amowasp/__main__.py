import os
import sys

from lark import Lark
from amosum.utility import read_file
from amosum.amosum_parser.AmoSumTransformer import AmoSumTransformer
from amosum.amosum_parser.AmoSumGrammar import grammar as gr
from amosum.amosum_parser.utils import toString


def optimized_run():
    """
    Wrapper that re-runs the module with -O optimization flag.
    """
    os.execvp(sys.executable, [sys.executable, "-O", "-m", "amosum.amosum_parser", *sys.argv[1:]])

def run(input, path = True):
    if input is None or input == "": return ""
    input = read_file(input) if path else input
    amosum_transformer = AmoSumTransformer()
    aspCoreParser = Lark(grammar=gr, start="s", parser='lalr', debug=True, transformer = amosum_transformer)
    amoSumParsed = aspCoreParser.parse(input)
    return toString(parsed=amoSumParsed)


def main():
    input = sys.stdin.read()
    output = run(input=input, path=False)
    print(output)

if __name__ == "__main__":
    main()