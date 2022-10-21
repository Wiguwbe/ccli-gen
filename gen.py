
import pathlib as pl

from dataclasses import dataclass
from enum import Enum

import jinja2 as j2

class CType(Enum):
    STRING = 0
    INT = 1
    BOOL = 2

ctype_conversion = [
    # STRING
    "char *",
    # INT
    "int",
    # BOOL, save as int
    "int",
]

ctype_nil = [
    # STRING
    'NULL',
    # INT
    '-1',
    # BOOL
    '0',
]

class Opt:

    short: str
    long: str
    env: str
    ctype: CType
    desc: str
    param_name: str

    def __init__(self, short:str, long:str, env:str, ctype:CType, desc:str, param_name: str=None):
        self.short = short
        self.long = long
        self.env = env
        self.ctype = ctype
        self.desc = desc
        self.param_name = param_name

# just to make it easy to identify
class Handler:
    name: str
    desc: str

    def __init__(self, name:str, desc: str):
        self.name = name
        self.desc = desc

class Cmd:

    name: str
    #desc: str
    callback: str
    options: list[Opt]
    sub_cmds: list[object]
    parent: object = None
    description: str

    def __init__(self, name:str, callback:str = None, options:list[Opt]=None, sub_cmds:list[object]=None, parent:object=None, description:str=None):
        self.name = name.replace('-', '_')
        self.callback = callback
        self.options = options if options is not None else []
        self.sub_cmds = sub_cmds if sub_cmds is not None else []
        self.parent = parent
        self.description = description

    @property
    def short_options(self):
        return [
            opt
            for opt in self.options
            if opt.short is not None
        ]

    @property
    def long_options(self):
        return [
            opt
            for opt in self.options
            if opt.long is not None
        ]

    @property
    def env_options(self):
        return [
            opt
            for opt in self.options
            if opt.env is not None
        ]

    def get_fullname(self):
        base = ''
        if self.parent is not None:
            base = self.parent.get_fullname() + '_'
        return base + self.name

    def get_parent_list(self, reversed=False) -> list:
        if self.parent is None:
            return []
        # else
        pl = self.parent.get_parent_list()
        if reversed:
            return [self.parent] + pl
        # else
        return pl + [self.parent]

    @staticmethod
    def create(name, params: list):
        c = Cmd(name)
        for item in params:
            if type(item) == Cmd:
                item.parent = c
                c.sub_cmds.append(item)
            elif type(item) == Opt:
                c.options.append(item)
            elif type(item) == Handler:
                c.callback = item.name
                c.description = item.desc
        return c

# TODO parse file

# template.render(cmds:list[Cmd], ctype=CType)

def _flatten_commands(cmd:Cmd) -> list[Cmd]:
    cmds = [cmd]
    p = 0
    c = 1
    while p != c:
        for i in range(p,c):
            cmds.extend(cmds[i].sub_cmds)
        p = c
        c = len(cmds)
    return cmds


def generate(cmd:Cmd, basename:str) -> tuple[str,str]:
    template_dir = pl.Path(__file__).parent.resolve() / 'templates'
    env = j2.Environment(
        loader=j2.FileSystemLoader(template_dir),
        trim_blocks=True, lstrip_blocks=True
    )

    c_template = env.get_template("parser-c.j2")
    h_template = env.get_template("parser-h.j2")

    # flatten list of cmds
    cmds = _flatten_commands(cmd)


    c_file = c_template.render(
        cmds=cmds,
        ctype=CType,
        ctype_conversion=ctype_conversion,
        ctype_nil=ctype_nil,
        basename=basename
    )
    h_file = h_template.render(
        cmds=cmds,
        ctype=CType,
        ctype_conversion=ctype_conversion,
        ctype_nil=ctype_nil,
        basename=basename
    )

    return c_file, h_file
