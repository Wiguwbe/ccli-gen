
import gen

# base (foo|bar)

base_cmd = gen.Cmd(
    "base",
    "handle_base",
    [
        gen.Opt("v","verbose", None, gen.CType.BOOL, "verbosity level"),
        gen.Opt("H","host", 'APP_HOST', gen.CType.STRING, "app host")
    ]
    # fill subcommands later
)

base_foo_cmd = gen.Cmd(
    "foo",
    "handle_base_foo",
    [
        gen.Opt("n", "name", None, gen.CType.STRING, "the name of the thing")
    ],
    parent=base_cmd
)

base_bar_cmd = gen.Cmd(
    "bar",
    "handle_base_bar",
    [
        gen.Opt("i", "id", None, gen.CType.INT, "the id of the thing")
    ],
    parent=base_cmd
)

base_cmd.sub_cmds.extend([base_foo_cmd, base_bar_cmd])

#c,h = gen.generate([base_cmd, base_foo_cmd, base_bar_cmd])
c,h = gen.generate(base_cmd, "generated")

with open("generated.h", "w") as fd: fd.write(h)
with open("generated.c", "w") as fd: fd.write(c)
