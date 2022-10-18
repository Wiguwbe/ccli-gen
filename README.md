# C CLI parser generator

A command line arguments parser generator, with a focus on subcommands. Inspired by python's CLI.

The generated code will expose an entrypoint function `parse`, which takes the `argc` and `argv`, parses the arguments and calls the handler function. All unused arguments will be passed as extra `argc` and `argv`.

Currently, the generated code only calls the target command, doesn't call handlers as they are found: on the example below, if it finds that the (sub)command is the `foo` one, it will only call `handle_base_foo`.

There are nothing fancy here (exclusivity, defautls, required, etc...), the handlers should (and can) handle it themselves.

The entrypoint (`parser.py` or docker image) prints a `tar` file to `STDOUT`, with the 2 generated files (source and header). This is mostly to be able to run through docker, which makes it easier to package the jinja files with, saving me such trouble and saving _you_ the trouble of mounting volumes and changing permissions :) .

### Usage

Through docker:

`docker run ccli:latest <basename> <[params-file]>`

or directly:

`python3 parser.py <basename> <[params-file]>`

The `basename` specifies the name of the output files (`<basename>.c` and `<basename>.h`);

The `params-file` is optional, it should be the params file. If not provided, will read from `STDIN` (don't we love pipes?).

### Example of input file

```
# a comment

# this specifies the base command name
base {
    # this specifies the C function which handles it
    # along with a description
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
    int verbose,
    char * host,
    int argc_left,
    char** argv_len
);
int handle_base_foo(
    char * name,
    int verbose,
    char * host,
    int argc_left,
    char** argv_len
);
int handle_base_bar(
    int id,
    int verbose,
    char * host,
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

int handle_base(int verbose, char* host, int argc, char**argv)
{
    printf("handle_base(%d, %s, %d", verbose, host, argc);
    fflush(stdout);
    print_args(argc, argv);
    printf(")\n");

    return 0;
}

int handle_base_foo(char *name, int verbose, char *host, int argc, char**argv)
{
    printf("handle_base_foo(%s, %d, %s, %d", name, verbose, host, argc);
    print_args(argc, argv);
    printf(")\n");

    return 0;
}

int handle_base_bar(int id, int verbose, char *host, int argc, char**argv)
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
- Env vars;
- No runtime, code is generated.

### SIMDITF

_something I may do in the future_

- Implement `--help`/`-h` (is it useful here?);
- ~~Fix~~ Remove the special features.

### ~~Bugs~~ Special features

- If an argument (not an option) appears before a subcommand name, it will still be passed to the subcommand's handler:
`./base a-argument foo --name=dred` is the same as `./base foo a-argument`.
This clearly opens a possibility for bugs :)
