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
    def file_sha1(cls, filename):
        import hashlib
        sha1 = hashlib.sha1()
        with open(filename, "rb") as f:
            while True:
                data = f.read(4096)
                if not data:
                    break
                sha1.update(data)
        hash = sha1.digest()
        cls.print_mgmt_value(hash.hex())