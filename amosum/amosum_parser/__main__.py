import os
import sys

dir = os.path.dirname(__file__)
path_dir = os.path.abspath(dir)

father_dir = os.path.join(dir, "../")
path_father_dir = os.path.abspath(father_dir)

sys.path.append(path_dir)
sys.path.append(path_father_dir)
# print(sys.path)
from utility import *
from lark import Lark
from amosum_parser.AmoSumTransformer import AmoSumTransformer # type: ignore
from amosum_parser.AmoSumGrammar import grammar as gr
from amosum_parser.utils import *

def run(input, path = True):
    if input is None or input == "":
        return ""

    if path:
        input = read_file(input)
  
    amosum_transformer = AmoSumTransformer()
    aspCoreParser = Lark(grammar=gr, start="s", parser='lalr',debug=True, transformer = amosum_transformer)
    amoSumParsed = aspCoreParser.parse(input)
    return toString(parsed=amoSumParsed)


def main(argv):
    input = sys.stdin.read()
    output = run(input=input, path=False)
    print(output)

if __name__ == "__main__":
    main(sys.argv)