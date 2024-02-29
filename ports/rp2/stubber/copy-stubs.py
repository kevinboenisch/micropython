# pylint: disable=missing-module-docstring,missing-function-docstring,missing-class-docstring

import os
import shutil
#import sys

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

S_AUTOGEN_PREFIX  = "# AUTO-GENERATED "
S_AUTOGEN_STUBBER = S_AUTOGEN_PREFIX + "by micropython-stubber"
S_AUTOGEN_EMPTY   = S_AUTOGEN_PREFIX + "as empty"
S_SECOND_LINE     = "# If you manually edit this file, remove the comment above."

def is_autogen(stub_path):
    if stub_path.endswith(".pyi"):
        with open(stub_path, "r", encoding="utf-8") as f:
            content = f.readline()
        return content.startswith(S_AUTOGEN_PREFIX)
    return False

def delete_auto_generated_stubs(dest_dir, skipped):
    print(f"=== Delete auto-generated stubs in {dest_dir}")
    deleted = []
    for root, _, files in os.walk(dest_dir):
        for file in files:
            stub_path = os.path.join(root, file)
            if is_autogen(stub_path):
                os.remove(stub_path)
                deleted.append(stub_path)
                #print(f"Deleted {stub_path}")
            else:
                print(f"Skipped {stub_path}")
                skipped.append(stub_path)
    print(f"Deleted {len(deleted)} files")
    return deleted

def prepend(file_path, text):
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(text)
        f.write(content)

def copy_stub(mod, dest_dir, pyi_dirs, missing, existing):
    stub_path = find_stub(mod, pyi_dirs)
    if stub_path is None:
        print(f"WARNING: No stub for {mod}")
        missing.append(mod)
        return False

    create_subdir(mod, dest_dir)
    dest_path = os.path.join(dest_dir, mod + ".pyi")
    if os.path.exists(dest_path):
        print(f"Already exists {dest_path}")
        existing.append(mod)
        return False

    print(f"Copy {stub_path}")
    shutil.copy(stub_path, dest_path)
    prepend(dest_path, S_AUTOGEN_STUBBER + "\n" + S_SECOND_LINE + "\n")
    return True

def copy_stubber_stubs(modules, dest_dir, missing):
    print(f"=== Copy stubber generated stubs to {dest_dir}")
    # do not overwrite existing files
    pyi_dirs = [
        "stubs/micropython-preview-docstubs",
        "stubs/micropython-preview-frozen/rp2/GENERIC"
    ]
    for dd in pyi_dirs:
        if not os.path.exists(dd):
            raise FileNotFoundError(f"Directory {dd} not found")

    os.makedirs(dest_dir, exist_ok=True)

    copied = []
    for mod in modules:
        if copy_stub(mod, dest_dir, pyi_dirs, missing, []):
            copied.append(mod)

    return copied

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

def create_empty_stubs(missing, dest_dir):
    print(f"=== Create empty stubs for missing files in {dest_dir}")
    # do not overwrite existing files
    os.makedirs(dest_dir, exist_ok=True)

    created = []
    for mod in missing:
        mod_path = os.path.join(dest_dir, mod + ".pyi")
        if os.path.exists(mod_path):
            print(f"Already exists {mod_path}")
        else:
            print(f"Created emtpy stub {mod_path}")
            create_file(mod, dest_dir, ".pyi",
                S_AUTOGEN_EMPTY + "\n" + S_SECOND_LINE + "\n")
            created.append(mod)
    return created

def replace(file_path, old_text, new_text):
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()
    new_content = content.replace(old_text, new_text)
    if new_content != content:
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(new_content)
        return True
    return False

def replace_in_stubs(stub_dirs, cur_text, new_text):
    print(f"=== Replace {cur_text} with {new_text} in stubs")
    file_count = 0
    for sdir in stub_dirs:
        for root, _, files in os.walk(sdir):
            for file in files:
                if file.endswith(".pyi"):
                    stub_path = os.path.join(root, file)
                    if replace(stub_path, cur_text, new_text):
                        file_count += 1
            # stop after first level, do not go into subdirs
            break
    print(f"Replaced in {file_count} files")

def print_imports(modules):
    # print imports to copy/paste for testing
    for mod in modules:
        print("import " + mod.replace("/", "."))
    exit(0)

def save_info_txt(dest_dir, manual_files, stubber_mods, empty_mods):
    file_path = os.path.join(dest_dir, "___info.txt")
    with open(file_path, "w", encoding="utf-8") as f:        
        f.write("Manual files:\n")
        for file in manual_files:
            f.write(file + "\n")

        f.write("\nEmpty stubs:\n")
        for mod in empty_mods:
            f.write(mod + "\n")
        
        f.write("\nStubber generated:\n")
        for mod in stubber_mods:
            f.write(mod + "\n")

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

    # TODO: change to stdlib?
    dir_pylance = os.path.join(jpo_path, "resources/py_stubs/pylance")
    dir_pylint = os.path.join(jpo_path, "resources/py_stubs/pylint")

    create_empty_py_files(modules, dir_pylint)

    manual_files = []
    delete_auto_generated_stubs(dir_pylance, manual_files)
    missing_mods = []
    stubber_mods = copy_stubber_stubs(modules, dir_pylance, missing_mods)
    empty_mods = create_empty_stubs(missing_mods, dir_pylance)

    mpy_url = 'https://docs.micropython.org/en/v1.20.0'
    mpy_url_preview = 'https://docs.micropython.org/en/preview'
    replace_in_stubs([dir_pylance], mpy_url_preview, mpy_url)

    save_info_txt(dir_pylance, manual_files, stubber_mods, empty_mods)
    #detect_duplicates([dir_pylance])

if __name__ == "__main__":
    main()
