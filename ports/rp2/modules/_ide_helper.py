# Helpers for device management
# Based on Thonny IDE's __thonny_helper code which downloads on each run.
# JPO customizes Micropython, so a frozen module is simpler and more efficient. 

# pylint: disable-all

class __thonny_helper:
    import builtins
    import os
    import sys
    last_non_none_repl_value = None
    
    # for object inspector
    inspector_values = builtins.dict()
    @builtins.classmethod
    def print_repl_value(cls, obj):
        if obj is not None:
            cls.builtins.print('[object_link_for_thonny=%d]' % cls.builtins.id(obj), cls.builtins.repr(obj), '[/object_link_for_thonny]', sep='')
            cls.last_non_none_repl_value = obj
    
    @builtins.classmethod
    def print_mgmt_value(cls, obj):
        cls.builtins.print('<thonny>', cls.builtins.repr(obj), '</thonny>', sep='', end='')
    
    @builtins.classmethod
    def repr(cls, obj):
        try:
            s = cls.builtins.repr(obj)
            if cls.builtins.len(s) > 50:
                s = s[:50] + "..."
            return s
        except cls.builtins.Exception as e:
            return "<could not serialize: " + __thonny_helper.builtins.str(e) + ">"

    # JPO extensions        
    @builtins.classmethod 
    def listdir(cls, path = ".", include_hidden = False):
        result = {}
        try:
            names = cls.os.listdir(path)
        except cls.builtins.OSError as e:
            cls.print_mgmt_value(None) 
        else:
            for name in names:
                if not name.startswith(".") or include_hidden:
                    try:
                        if path[-1] != "/": 
                            path += "/"
                        result[name] = cls.os.stat(path + name)
                    except cls.builtins.OSError as e:
                        result[name] = cls.builtins.str(e)
            __thonny_helper.print_mgmt_value(result)

    @builtins.classmethod
    def _listall(cls, path, out_items):
        names = cls.os.listdir(path)
        for name in names:
            item_path = path + "/" + name
            out_items.append(item_path)
            if cls.is_dir(item_path):
                cls._listall(item_path, out_items)
    
    @builtins.classmethod
    def listall(cls, path):
        if path == "/": 
            path = ""

        out_items = []
        cls._listall(path, out_items)
        cls.print_mgmt_value(out_items)

    @builtins.classmethod
    def mkdirs(cls, path):
        parts = path.split("/")
        sofar = None
        for part in parts:
            if not sofar: 
                sofar = part
            else: 
                sofar = sofar + "/" + part
            try:
                cls.os.stat(sofar)
            except cls.builtins.OSError:
                cls.os.mkdir(sofar)

    @builtins.classmethod
    def _file_sha1(cls, filename):
        import hashlib
        sha1 = hashlib.sha1()        
        
        try:
            with open(filename, "rb") as f:
                while True:
                    data = f.read(4096)
                    if not data:
                        break
                    sha1.update(data)
        except OSError as e:
            return None
        
        hash = sha1.digest()
        return hash.hex()

    @builtins.classmethod
    def file_sha1s(cls, paths):
        hashes = []
        for path in paths:
            hh = cls._file_sha1(path)
            hashes.append(hh)
        cls.print_mgmt_value(hashes)

    @builtins.classmethod
    def file_sizes(cls, paths):
        sizes = []
        for path in paths:
            try:
                hh = cls.os.stat(path)[6]
                sizes.append(hh)
            except OSError as e:
                sizes.append(None)
                
        cls.print_mgmt_value(sizes)

    @builtins.classmethod
    def is_dir(cls, path):
        stat = cls.os.stat(path)
        return (stat[0] & 0o170000) == 0o040000


    @builtins.classmethod
    def deltree(cls, path, should_delete_fn = None):
        if path == "/":
            path = ""

        # print("deltree:", path)

        # check if it's a directory or file
        is_dir = False
        try:
            is_dir = cls.is_dir(path)
        except cls.builtins.OSError as e:
            # Path does not exist, return
            #print("Not exist:", path)
            raise e

        if is_dir:
            # remove directory contents
            names = cls.os.listdir(path)
            for name in names:
                #print("Consider path:", path, "name:", name)
                cls.deltree(path + "/" + name, should_delete_fn)            

        # remove either the file or the now-empty directory
        if path != "":
            if is_dir:
                try:
                    cls.os.remove(path)                
                except OSError as e:
                    pass
                    # print("Failed to remove:", path, e)
            else:
                if not should_delete_fn or should_delete_fn(path[1:]):
                    # print("--delete:", path)
                    cls.os.remove(path)
                else:
                    pass
                    # print("keep:", path)

