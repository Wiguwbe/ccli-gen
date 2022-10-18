
import re
import gen as g
import time

_lrb = re.compile(r'[ \t\n]*\{[ \t\n]*')
_nls = re.compile(r'[ \t]*(\n[ \t]*)+')
_rc = re.compile(r'[ \t]*#.*')

_t_t_t = {
    'int': g.CType.INT,
    'str': g.CType.STRING,
    'bool': g.CType.BOOL
}

REC_SPLIT = '\x1e'

def parse_opt(text: str) -> tuple[g.Opt, str]:
    opt_text, text = text.split(REC_SPLIT, 1)
    data, desc = opt_text.split(':', 1)
    data_parts = data.split(',')
    short = None
    long = None
    env = None
    ctype = None
    name = None
    for part in data_parts:
        part = part.strip()
        if part.startswith('--'):
            try:
                long, name = part[2:].split('=', 1)
            except:
                long = part[2:]
        elif part.startswith('-'):
            assert len(part) == 2
            short = part[1]
        elif part.startswith('$'):
            env = part[1:]
        else:
            ctype = _t_t_t[part]
    return g.Opt(short, long, env, ctype, desc.strip(), name), text

def parse_handler(text: str) -> tuple[g.Handler, str]:
    ht, text = text.split(REC_SPLIT, 1)
    try:
        h,d = ht.split(' ', 1)
    except:
        h,d = ht, ''
    return g.Handler(h[1:], d.strip()), text

def parse_cmd(text: str) -> tuple[g.Cmd, str]:
    params: list = []
    # get name
    name, text = text.split('{', 1)
    while True:
        if text[0] == '}':
            # end of command
            text = text[2:]
            break
        elif text[0] == '@':
            h = parse_handler
        elif text[0] in ('-', '$'):
            h = parse_opt
        else:
            h = parse_cmd

        data, text = h(text)
        params.append(data)

    cmd = g.Cmd.create(name, params)
    return cmd, text


def parse_params(data: str):
    data = data.strip()

    # remove comments
    data = _rc.sub('', data)

    # remove new lines after blocks
    data = _lrb.sub('{', data)

    # and new lines are now semicolons
    data = _nls.sub('\x1e', data)

    #print(data.replace('\x1e', ';'))
    #sys.exit(1)

    # now split blocks
    cmd, text = parse_cmd(data)
    assert len(text) == 0
    return cmd

if __name__ == '__main__':
    import sys
    import tarfile
    import io

    # TODO a better CLI
    if len(sys.argv) not in (2,3) or 'help' in sys.argv[1] or '-h' in sys.argv[1:]:
        print('usage:', sys.argv[0], '<basename> [<rules-file>]', file=sys.stderr)
        sys.exit(1)

    if len(sys.argv) == 3:
        with open(sys.argv[2]) as fd:
            data = fd.read()
    else:
        data = sys.stdin.read()

    basename = sys.argv[1]

    cmd = parse_params(data)

    c_file, h_file = g.generate(cmd, basename)

    files = [
        (f'{basename}.c', c_file),
        (f'{basename}.h', h_file),
    ]
    # generate TAR archive into stdout
    with tarfile.open(mode='w|', fileobj=sys.stdout.buffer) as tf:
        for fname, data in files:
            ti = tarfile.TarInfo(fname)
            ti.size = len(data)
            ti.type = tarfile.REGTYPE
            ti.mtime = int(time.time())
            ti.type = tarfile.REGTYPE
            # why not
            ti.uname = 'ccli'
            ti.gname = 'ccli'
            # safe defaults i guess?
            ti.uid = 1000
            ti.gid = 1000
            ti.mode = 0o644
            tf.addfile(ti, io.BytesIO(data.encode()))
