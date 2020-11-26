#!/usr/bin/env python3

import argparse
import subprocess
import sys
from multiprocessing.pool import ThreadPool
from pathlib import Path

_SCRIPT_DIR = Path(__file__).resolve().parent
_SRC_DIR = _SCRIPT_DIR.parent / "src"

_EXCLUDE_DIRS = ["third_party"]


def collect_sources(file_ext):
    # Find all files in the src folder.
    files = []
    files.extend(_SRC_DIR.glob(f"*{file_ext}"))

    # Find all files in non-excluded sub-folders.
    dirs = _SRC_DIR.glob("*")
    dirs = [d for d in dirs if d.is_dir() and d.name not in _EXCLUDE_DIRS]
    for dir in dirs:
        files.extend(dir.glob(f"**/*{file_ext}"))

    return files


def clang_tidy(src, build_path, fix):
    file_name = src.relative_to(_SRC_DIR)
    print(f"clang-tidy: {file_name}")
    args = ["clang-tidy", "-p", build_path]
    if fix:
        args.append("--fix")
    args.append(src)
    proc = subprocess.run(
        args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8"
    )
    return proc.stdout


def run_clang_tidy(build_path, fix):
    sources = collect_sources(".cpp")

    # Run jobs in a thread pool.
    pool = ThreadPool()
    results = []
    for src in sources:
        results.append(
            pool.apply_async(
                clang_tidy,
                [
                    src,
                    build_path,
                    fix,
                ],
            )
        )

    # Wait for all workers to finish.
    pool.close()
    pool.join()

    return [r.get() for r in results if r.get()]


def run_linters(build_path, fix):
    errors = run_clang_tidy(build_path, fix)

    all_ok = True
    if errors:
        all_ok = False
        print(f"\n***ERROR: Some checks failed 😣")
        for err in errors:
            print(err)
    else:
        print("\nAll checks passed! 👍")

    return all_ok


def main():
    parser = argparse.ArgumentParser(description="Run linter checks.")
    parser.add_argument("-p", "--build-path", required=True, help="build path")
    parser.add_argument("--fix", action="store_true", help="try to fix problems")
    args = parser.parse_args()

    if not run_linters(args.build_path, args.fix):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
