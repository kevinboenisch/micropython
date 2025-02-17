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
        "repos/micropython-stubs/stubs/micropython-preview-docstubs",
        "repos/micropython-stubs/stubs/micropython-preview-frozen/rp2/GENERIC"
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

    if "builtins" in missing:
        print("Special case: not creating builtins stub (it's in stdlib)")
        missing.remove("builtins")

    for mod in missing:
        mod_path = os.path.join(dest_dir, mod + ".pyi")
        if os.path.exists(mod_path):
            print(f"Already exists {mod_path}")
        else:
            print(f"Created emtpy stub {mod_path}")
            create_file(mod, dest_dir, ".pyi", "# TODO: edit manual stub")

def replace_in_stubs(cur_text, new_text, stub_dirs):
    print(f"=== Replace {cur_text} with {new_text} in stubs")
    file_count = 0
    for sdir in stub_dirs:
        for root, _, files in os.walk(sdir):
            for file in files:
                if file.endswith(".pyi"):
                    stub_path = os.path.join(root, file)
                    with open(stub_path, "r", encoding="utf-8") as f:
                        content = f.read()
                    new_content = content.replace(cur_text, new_text)
                    if new_content != content:
                        file_count += 1
                        with open(stub_path, "w", encoding="utf-8") as f:
                            f.write(new_content)
            # stop after first level, do not go into subdirs
            break
    print(f"Replaced in {file_count} files")


def detect_duplicates(stub_dirs):
    print("=== Detect duplicates in stub dirs")
    mod_files = {}
    dupes = []
    for sdir in stub_dirs:
        for root, dirs, files in os.walk(sdir):
            for file in files:
                if file.endswith(".pyi"):
                    mod = file[:-4]
                    if mod in mod_files:
                        print(f"DUPLICATE: {mod}.pyi in {root} and {mod_files[mod]}")
                        dupes += mod
                    else:
                        mod_files[mod] = root
            for dd in dirs:
                # check if the dir is a __init__ module
                init_path = os.path.join(root, dd, "__init__.pyi") 
                if os.path.exists(init_path):
                    #print(f"!! {init_path} exists")
                    mod = dd
                    if mod in mod_files:
                        print(f"DUPLICATE: {mod} dir in {root} and {mod_files[mod]}")
                        dupes += mod
                    else:
                        mod_files[mod] = root
            # stop after first level, do not go into subdirs
            break

    if len(dupes) == 0:
        print("No duplicates found")



def print_imports(modules):
    # print imports to copy/paste for testing
    for mod in modules:
        print("import " + mod.replace("/", "."))
    exit(0)

def main():
    try:
        jpo_path = os.environ["JPO_PATH"]
    except KeyError:
        print("ERROR: Environment variable JPO_PATH not set")
        exit(-1)

    print("JPO_PATH:", jpo_path)

    modules = read_modules_txt()
    # print_imports(modules)
    # exit(0)

    dir_rp2 = os.path.join(jpo_path, "resources/py_stubs/auto")
    dir_manual = os.path.join(jpo_path, "resources/py_stubs/manual")
    dir_stdlib = os.path.join(jpo_path, "resources/py_stubs/stdlib")
    dir_pylint = os.path.join(jpo_path, "resources/py_stubs/pylint")
    stub_dirs = [dir_rp2, dir_manual, dir_stdlib]

    create_empty_py_files(modules, dir_pylint)
    missing = copy_stubs(modules, dir_rp2)
    create_manual_stubs(missing, dir_manual)

    mpy_url = 'https://docs.micropython.org/en/v1.20.0'
    mpy_url_preview = 'https://docs.micropython.org/en/preview'
    replace_in_stubs(mpy_url_preview, mpy_url, stub_dirs)

    detect_duplicates(stub_dirs)

if __name__ == "__main__":
    main()
