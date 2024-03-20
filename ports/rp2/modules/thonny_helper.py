# Helpers for device management
# Based on Thonny IDE's __thonny_helper code which downloads on each run.
# JPO customizes Micropython, so a frozen module is simpler and more efficient. 

# pylint: disable-all

class __thonny_helper:
    import builtins
    try:
        import uos as os
    except builtins.ImportError:
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
        
    @builtins.classmethod 
    def listdir(cls, x):
        if cls.builtins.hasattr(cls.os, "listdir"):
            return cls.os.listdir(x)
        else:
            return [rec[0] for rec in cls.os.ilistdir(x) if rec[0] not in ('.', '..')]

    # Provide unified interface with Unix variant, which has anemic uos
    @builtins.classmethod
    def getcwd(cls):
        return cls.os.getcwd()
    
    @builtins.classmethod
    def chdir(cls, x):
        return cls.os.chdir(x)
    
    @builtins.classmethod
    def rmdir(cls, x):
        return cls.os.rmdir(x)

    # JPO extensions
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

