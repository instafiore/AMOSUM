import os
import re
import signal
import subprocess
import tempfile
from typing import List

from amosum import settings
from amosum.rewrite import rewrite_without_amosum as rewriteWithoutAmosum


class RunnerClingoCpp:
    '''
    Rewrites the ASP inputs with the amosum parser and runs amoclingo-core on them with the given options:
    amoclingo-core's output goes straight to stdout/stderr and run() returns its exit code
    '''

    SOLVER_EXE = "amoclingo-core"

    def __init__(self, inputs: List[str], args: List[str]) -> None:
        self.inputs = inputs        # ASP files, rewritten before the run
        self.args = args            # amoclingo-core's options, passed as they are

    def run(self) -> int:
        exe = f"{settings.PROPAGATOR_DIR_LOCATION_CLINGO_C_BIN}/{RunnerClingoCpp.SOLVER_EXE}"

        with tempfile.TemporaryDirectory(prefix="amoclingo_") as tmp:
            rewritten = [rewriteWithoutAmosum(i, tmp) for i in self.inputs]
            # the options before the files: amoclingo-core's parser takes everything after the first file as a file
            command = [exe, *self.args, *rewritten]
            proc = subprocess.Popen(command)

            # SIGINT/SIGTERM are forwarded: amoclingo-core prints its best answer set and exits
            def forward(signum, frame):
                proc.send_signal(signum)
            signal.signal(signal.SIGINT, forward)
            signal.signal(signal.SIGTERM, forward)

            return proc.wait()      # the temporary directory and the rewritten files are removed here