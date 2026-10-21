import os
import sys
from lark import Lark
from lark.exceptions import UnexpectedInput, VisitError
from amosum.amosum_parser.AmoSumTransformer import AmoSumTransformer # type: ignore
from amosum.amosum_parser.AmoSumGrammar import grammar as gr
from amosum.amosum_parser.utils import toString
from amosum.utility import read_file, ERROR_CODE


class AmosumParseError(Exception):
    '''The input is not a valid amosum program (syntax) or could not be rewritten (transformer).'''


def optimized_run():
    """
    Wrapper that re-runs the module with -O optimization flag.
    """
    os.execvp(sys.executable, [sys.executable, "-O", "-m", "amosum.amosum_parser", *sys.argv[1:]])

def run(input):
    if input is None or input == "": return ""
    input =  read_file(input) if os.path.isfile(input) else input

    amosum_transformer = AmoSumTransformer()
    # debug=False: with debug=True lark prints its state stack on stdout at every syntax error
    aspCoreParser = Lark(grammar=gr, start="s", parser='lalr', debug=False, transformer = amosum_transformer)
    try:
        amoSumParsed = aspCoreParser.parse(input)
    except UnexpectedInput as e:
        # syntax error: line, column and the text around it
        raise AmosumParseError(f"syntax error at line {e.line}, column {e.column}:\n{e.get_context(input)}") from e
    except VisitError as e:
        # error raised while rewriting a statement (lark wraps the transformer's exceptions)
        raise AmosumParseError(f"cannot rewrite a statement ({e.rule}): {e.orig_exc!r}") from e
    return toString(parsed=amoSumParsed)


def main(argv = None):
    argv = sys.argv[1:] if argv is None else argv
    files_contents = [read_file(f) for f in argv if os.path.isfile(f)]
    if not sys.stdin.isatty():
        files_contents.append(sys.stdin.read())
    input = "\n".join(files_contents)
    try:
        output = run(input=input)
    except AmosumParseError as e:
        # as fiore::setExitCode: the message on stderr, then exit with the error code
        print(f"amosumparser: {e}", file=sys.stderr)
        sys.exit(ERROR_CODE)
    print(output)

if __name__ == "__main__":
    main()