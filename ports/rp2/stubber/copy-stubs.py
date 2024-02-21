# pylint: disable=missing-module-docstring,missing-function-docstring,missing-class-docstring

import os
import shutil
import sys

def read_modules_txt():
    modules = []
    started = False
    for line in open("modules.txt", encoding="utf-8"):
        if started:
            if "Plus any modules" in line:
                break
            modules.extend(line.split())

        if "// Paste output below" in line:
            started = True

    modules.sort()
    return modules

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
    print(f"Copy {stub_path}")
    shutil.copy(stub_path, dest_path)
    return True

def copy_stubs(modules, dest_dir):
    print(f"=== Copy stubs to {dest_dir}")
    missing = []
    pyi_dirs = [
        "stubs/micropython-preview-docstubs",
        "stubs/micropython-preview-frozen/rp2/GENERIC"
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
    print(f"=== Create empty py files in  {dest_dir}")
    shutil.rmtree(dest_dir, ignore_errors=True)
    os.makedirs(dest_dir, exist_ok=True)

    for mod in modules:
        create_file(mod, dest_dir, ".py")

def create_manual_stubs(missing, dest_dir):
    print(f"=== Create manual stubs for missing files in  {dest_dir}")
    # do not rmtree, do not overwrite existing files
    os.makedirs(dest_dir, exist_ok=True)

    for mod in missing:
        mod_path = os.path.join(dest_dir, mod + ".pyi")
        if os.path.exists(mod_path):
            print(f"Already exists {mod_path}")
        else:
            print(f"Created emtpy stub {mod_path}")
            create_file(mod, dest_dir, ".pyi", "TODO: edit manual stub")

def detect_duplicate_manual_stubs(modules, missing, dest_dir):
    for root, dirs, files in os.walk(dest_dir):
        for file in files:
            if file.endswith(".pyi"):
                mod = file[:-4]
                if (mod in modules) and (not mod in missing):
                    print(f"WARNING: Duplicate stub for {mod} in {root}")

def main():
    try:
        jpo_path = os.environ["JPO_PATH"]
    except KeyError:
        print("ERROR: Environment variable JPO_PATH not set")
        exit(-1)

    print("JPO_PATH:", jpo_path)

    modules = read_modules_txt()
    # for mod in modules:
    #     print(mod)

    dir_rp2 = os.path.join(jpo_path, "resources/py_stubs/auto")
    dir_manual = os.path.join(jpo_path, "resources/py_stubs/manual")
    dir_pylint = os.path.join(jpo_path, "resources/py_stubs/pylint")

    create_empty_py_files(modules, dir_pylint)
    missing = copy_stubs(modules, dir_rp2)
    create_manual_stubs(missing, dir_manual)
    detect_duplicate_manual_stubs(modules, missing, dir_manual)

if __name__ == "__main__":
    main()
