# Your Project
# Copyright (C) 2026 Jan Aleksander Górski

# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would
#    be appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and must not
#    be misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source
#    distribution.
#!/usr/bin/env python3
import json
import subprocess
import os
import sys


def main():
    build_dir = "build"
    db_path = os.path.join(build_dir, "compile_commands.json")

    print(f">>> Cleaning GCC module flags from {db_path}...")
    with open(db_path, "r", encoding="utf-8") as f:
        compile_commands = json.load(f)

    for entry in compile_commands:
        if "command" in entry:
            parts = entry["command"].split()
            cleaned_parts = [
                p
                for p in parts
                if not p.startswith("-fdeps-format=")
                and not p.startswith("-fmodule-mapper=")
                and p != "-fmodules-ts"
            ]
            entry["command"] = " ".join(cleaned_parts)

        elif "arguments" in entry:
            cleaned_args = [
                arg
                for arg in entry["arguments"]
                if not arg.startswith("-fdeps-format=")
                and not arg.startswith("-fmodule-mapper=")
                and arg != "-fmodules-ts"
            ]
            entry["arguments"] = cleaned_args

    with open(db_path, "w", encoding="utf-8") as f:
        json.dump(compile_commands, f, indent=2)

    print(">>> Running Clang-Tidy analysis...")
    cwd = os.getcwd()

    tidy_command = [
        sys.executable,
        "scripts/run_clang-tidy.py",
        "-quiet",
        "-p",
        build_dir,
        f"-header-filter={cwd}/include/.*",
        "src/MPVGL/.*",
    ]

    result = subprocess.run(tidy_command)
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
