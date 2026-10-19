import os
import sys
from lark import Lark
from amosum.amosum_parser.AmoSumTransformer import AmoSumTransformer # type: ignore
from amosum.amosum_parser.AmoSumGrammar import grammar as gr
from amosum.amosum_parser.utils import toString
from amosum.utility import read_file



def optimized_run():
    """
    Wrapper that re-runs the module with -O optimization flag.
    """
    os.execvp(sys.executable, [sys.executable, "-O", "-m", "amosum.amosum_parser", *sys.argv[1:]])

def run(input):
    if input is None or input == "": return ""
    input =  read_file(input) if os.path.isfile(input) else input
  
    amosum_transformer = AmoSumTransformer()
    aspCoreParser = Lark(grammar=gr, start="s", parser='lalr',debug=True, transformer = amosum_transformer)
    amoSumParsed = aspCoreParser.parse(input)
    return toString(parsed=amoSumParsed)


def main(argv = None):
    argv = sys.argv[1:] if argv is None else argv
    files_contents = [read_file(f) for f in argv if os.path.isfile(f)]
    if not sys.stdin.isatty():
        files_contents.append(sys.stdin.read())
    input = "\n".join(files_contents)
    output = run(input=input)
    print(output)

if __name__ == "__main__":
    main()