# C CLI parser generator

A command line arguments parser generator, with a focus on subcommands. Inspired by python's CLI.

The generated code will expose an entrypoint function `parse`, which takes the `argc` and `argv`, parses the arguments and calls the handler function. All unused arguments will be passed as extra `argc` and `argv`.

Currently, the generated code only calls the target command, doesn't call handlers as they are found: on the example below, if it finds that the (sub)command is the `foo` one, it will only call `handle_base_foo`.

There is nothing fancy here (exclusivity, defaults, validation, required, etc...), the handlers should (and can) handle it themselves.

The `ccli-gen` tool takes a file (e.g. `params`) and generates `.c` and `.h` files, named after the input file (e.g. `params.c` and `params.h`).

### Usage

`ccli-gen <params-file>`

### Example of input file

```
# a comment

# this specifies the base command name
base {
    # this specifies the C function that handles it
    # along with a description of the command
    @handle_base the base command does x and y

    # options, short and envvar are optional
    # specifying a placeholder for the variable is also optional
    # this also needs the type (str, int, bool)
    # along with a description
    -H, --host=HOST, $APP_HOST, str: this is the description of this option
    --debug, bool: shows verbose/debug output

    # a subcommand
    foo {
        @handle_base_foo the foo subcommand does that

        -n, --name=NAME, str: some name needed for some reason
    }

    # another subcommand
    bar {
        # description is optional
        @handle_base_bar
        -i, --id=ID, int: some id needed
    }
}
```

### Example of output header file

```
#ifndef _GENERATED_H_
#define _GENERATED_H_

int parse(int argc, char**argv);

int handle_base(
    char * host,
    int verbose,
    int argc_left,
    char** argv_len
);
int handle_base_foo(
    char * name,
    char * host,
    int verbose,
    int argc_left,
    char** argv_len
);
int handle_base_bar(
    int id,
    char * host,
    int verbose,
    int argc_left,
    char** argv_len
);

#endif
```

The handlers will be passed their declared options, along with the parents' (all of them) in order:
- my options;
- parent options;
- grand-parent options;
- ...
- argc_left, argv_left

### Example of C file that uses the generated code:

_See `test.sh` for a more updated example_

```
#include <stdio.h>
#include "generated.h"

static void print_args(int argc, char**argv)
{
    for(int i=0;i<argc;i++) {
        printf(", %s", argv[i]);
    }
    fflush(stdout);
}

int handle_base(char* host, int verbose, int argc, char**argv)
{
    printf("handle_base(%d, %s, %d", verbose, host, argc);
    fflush(stdout);
    print_args(argc, argv);
    printf(")\n");

    return 0;
}

int handle_base_foo(char *name, char *host, int verbose, int argc, char**argv)
{
    printf("handle_base_foo(%s, %d, %s, %d", name, verbose, host, argc);
    print_args(argc, argv);
    printf(")\n");

    return 0;
}

int handle_base_bar(int id, char *host, int verbose, int argc, char**argv)
{
    printf("handle_base_bar(%d, %d, %s, %d", id, verbose, host, argc);
    print_args(argc, argv);
    printf(")\n");
    return 0;
}

int main(int argc, char**argv)
{
    return parse(argc, argv);
}
```

### Features

- Sub commands;
- Order of options doesn't matter: `base --name=dred foo` is the same as `base foo --name=dred`. _If two options have the same name, the order would indeed matter_;
- Env vars: if an option that provides a envvar name isn't provided, the parser will try to get from env;
- No runtime, code is generated.

### SIMDITF

_something I may do in the future_

- Implement `--help`/`-h` (is it useful here?);
- ~~Fix~~ Remove the special features;
- Generate main function.

### ~~Bugs~~ Special features

- If an argument (not an option) appears before a subcommand name, it will still be passed to the subcommand's handler:
`./base a-argument foo --name=dred` is the same as `./base foo a-argument`.
This happens because, if an option (`... --opt value ...`) is from a child, the parser doesn't know if the `--opt` is boolean or not
(if it consumes the `value`).
This clearly opens a possibility for bugs :)
