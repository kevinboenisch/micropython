# pylint: disable=missing-module-docstring,missing-function-docstring,missing-class-docstring

import os
import shutil
import sys

def find_stub(mod, pyi_dirs):
    for dd in pyi_dirs:
        stub = os.path.join(dd, mod + ".pyi")
        if os.path.exists(stub):
            return stub
    return None

def copy_stub(mod, dest_dir, pyi_dirs):
    stub_path = find_stub(mod, pyi_dirs)
    if stub_path is None:
        print(f"WARNING: No stub for {mod}")
        return False

    create_subdir(mod, dest_dir)
    dest_path = os.path.join(dest_dir, mod + ".pyi")
    print(f"Copy from: {stub_path}")
    print(f"       to: {dest_path}")
    shutil.copy(stub_path, dest_path)
    return True

def copy_stubs(modules, dest_dir):
    print(f"=== Copy stubs to {dest_dir}")
    missing = []
    pyi_dirs = [
        "repos/micropython-stubs/stubs/micropython-v1_27_0_preview-docstubs",
        "repos/micropython-stubs/stubs/micropython-v1_27_0_preview-frozen/rp2/GENERIC"
    ]
    for dd in pyi_dirs:
        if not os.path.exists(dd):
            raise FileNotFoundError(f"Directory {dd} not found")

    shutil.rmtree(dest_dir, ignore_errors=True)
    os.makedirs(dest_dir, exist_ok=True)

    for mod in modules:
        ok = copy_stub(mod, dest_dir, pyi_dirs)
        if not ok:
            missing.append(mod)

    return missing

def create_subdir(mod, dest_dir):
    if "/" in mod:
        subdir = os.path.join(dest_dir, mod[:mod.index("/")])
        os.makedirs(subdir, exist_ok=True)

def create_file(mod, dest_dir, ext, text=""):
    create_subdir(mod, dest_dir)
    with open(os.path.join(dest_dir, mod + ext), "w", encoding="utf-8") as f:
        f.write(text)

def create_empty_py_files(modules, dest_dir):
    print(f"=== Create empty py files in {dest_dir}")
    shutil.rmtree(dest_dir, ignore_errors=True)
    os.makedirs(dest_dir, exist_ok=True)

    for mod in modules:
        create_file(mod, dest_dir, ".py")


def main():
    try:
        jpo_path = os.environ["JPO_PATH_DEV"]
    except KeyError:
        print("Environment variable JPO_PATH_DEV not set, trying JPO_PATH")
        try:        
            jpo_path = os.environ["JPO_PATH"]
        except KeyError:
            print("ERROR: Environment variable JPO_PATH not set")
            exit(-1)
    print("JPO_PATH:", jpo_path)

    modules = ["jpo"]

    dir_rp2 = os.path.join(jpo_path, "resources/py_stubs/auto")
    dir_pylint = os.path.join(jpo_path, "resources/py_stubs/pylint")

    create_empty_py_files(modules, dir_pylint)
    missing = copy_stubs(modules, dir_rp2)

    if len(missing) > 0:
        print(f"ERROR: Missing stubs: {missing}")


if __name__ == "__main__":
    main()
