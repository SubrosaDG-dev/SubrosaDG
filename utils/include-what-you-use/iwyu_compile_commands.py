"""Prepare the CMake compilation database for IWYU's Clang driver."""

import json
from pathlib import Path
import shlex
import sys


compile_commands = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
for entry in compile_commands:
    # Keep SYCL enabled for host parsing and drop the icpx-only code generation flags.
    entry["arguments"] = [
        argument
        for argument in shlex.split(entry.pop("command"))
        if argument != "-ipo" and not argument.startswith("-fsycl-targets=")
    ]

Path(sys.argv[2]).write_text(json.dumps(compile_commands, indent=2) + "\n", encoding="utf-8")
