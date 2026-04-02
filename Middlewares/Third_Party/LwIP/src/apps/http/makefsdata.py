#!/usr/bin/env python3
import os
from pathlib import Path

# This script creates our output file were we will store all the converted html data to C arrays.
# The data is saved into the file in HEX values but the data is written to correspond to the
# following structure. At the end of the script the arrays are pointed to the structures.
#
#  struct httpd_fsdata_file_noconst
#  {
#    struct httpd_fsdata_file *next;
#    unsigned char *name;
#    unsigned char *data;
#    int len;
#  };

OUTPUT_NAME = "fsdata_custom.c"
FS_DIR = Path("fs")
ITEMS_PER_LINE = 10


def should_skip(name: str) -> bool:
    if name.startswith("."):
        return True
    if "CVS" in name:
        return True
    if "~" in name:
        return True
    return False


def print_file_names(files):
    print(" ".join(files))


def collect_files(root: Path):
    files = []
    queue = []

    for entry in os.listdir(root):
        if should_skip(entry):
            continue
        queue.append(entry)

    print("\nopening root directory, found the following files and all subdirectories to process:\n  ")
    print_file_names(queue)

    while queue:
        rel = queue.pop(0)
        full = root / rel
        if full.is_dir() and not rel.startswith("."):
            print(f"\nProcessing directory {rel}")
            newfiles = []
            for entry in os.listdir(full):
                if should_skip(entry):
                    continue
                newfiles.append(entry)
            print(f"  Adding files: {' '.join(newfiles)}")
            for entry in newfiles:
                queue.append(f"{rel}/{entry}")
            continue
        files.append(rel)

    return files


def file_var_name(path: str) -> str:
    return path.replace("/", "_").replace(".", "_")


def file_flags(file_path: str) -> str:
    flags = "FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT"
    if file_path.endswith(".shtml"):
        flags += " | FS_FILE_FLAGS_SSI"
    return flags


def write_bytes(out, data: bytes, indent: str = "\t"):
    if not data:
        return
    for i, b in enumerate(data):
        if i % ITEMS_PER_LINE == 0:
            out.write(indent)
        out.write(f"0x{b:02X}")
        if i != len(data) - 1:
            out.write(", ")
        if (i + 1) % ITEMS_PER_LINE == 0:
            out.write("\n")
    if len(data) % ITEMS_PER_LINE != 0:
        out.write("\n")


def main():
    os.chdir(FS_DIR)
    files = collect_files(Path("."))

    print("\n\nStarting convert files to arrays process:\n")

    fvars = []
    pfiles = []

    with open(Path("..") / OUTPUT_NAME, "w", newline="\n") as out:
        for rel_path in files:
            full = Path(rel_path)
            if not full.is_file():
                continue

            print(f"  Adding file: {rel_path}")

            size = full.stat().st_size
            file_path = f"/{rel_path}"
            fvar = file_var_name(file_path)

            out.write(f"static const unsigned char data{fvar}[] = {{\n")
            out.write(f"\t/* File: {file_path} */\n\t")

            name_bytes = file_path.encode("utf-8")
            name_line = ", ".join(f"0x{b:02X}" for b in name_bytes)
            out.write(f"{name_line}, 0,\n")

            out.write("\t/* here starts the data */\n")

            with open(full, "rb") as inp:
                data = inp.read()

            write_bytes(out, data)

            out.write("};\n\n")

            print(f"  {rel_path}, Bytesread = {size}\n")
            fvars.append(fvar)
            pfiles.append(file_path)

        for i, fvar in enumerate(fvars):
            file_path = pfiles[i]
            prevfile = "NULL" if i == 0 else f"file{fvars[i - 1]}"
            name_len = len(file_path) + 1
            flags = file_flags(file_path)
            out.write(f"const struct fsdata_file file{fvar}[] = {{\n")
            out.write(f"    {{{prevfile}, data{fvar}, data{fvar} + {name_len},\n")
            out.write(f"     sizeof(data{fvar}) - {name_len}, {flags}}}}};\n\n")

        if fvars:
            out.write(f"#define FS_ROOT file{fvars[-1]}\n\n")
        else:
            out.write("#define FS_ROOT NULL\n\n")
        out.write(f"#define FS_NUMFILES {len(fvars)}\n")


if __name__ == "__main__":
    main()
