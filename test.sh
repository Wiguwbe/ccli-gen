
set -ex

# run the test.py
./ccli-gen params

# got generated.c and generated.h

cat > test.c << EOF

#include <stdio.h>
#include "params.h"

static void print_args(int argc, char**argv)
{
    for(int i=0;i<argc;i++) {
        printf(", %s", argv[i]);
    }
    fflush(stdout);
}

int handle_base(char* host, int verbose, int argc, char**argv)
{
    printf("handle_base(%s, %d, %d", host, verbose, argc);
    fflush(stdout);
    print_args(argc, argv);
    printf(")\n");

    return 0;
}

int handle_base_foo(char *name, char *host, int verbose, int argc, char**argv)
{
    printf("handle_base_foo(%s, %s, %d, %d", name, host, verbose, argc);
    print_args(argc, argv);
    printf(")\n");

    return 0;
}

int handle_base_bar(int id, char *host, int verbose, int argc, char**argv)
{
    printf("handle_base_bar(%d, %s, %d, %d", id, host, verbose, argc);
    print_args(argc, argv);
    printf(")\n");
    return 0;
}

int main(int argc, char**argv)
{
    return parse(argc, argv);
}
EOF


gcc test.c params.c -o ccli-test -ggdb

# and the tests
function do_test() {
    local args=$1
    local expt=$2
    local op=${3:-=}

    local output=$(./ccli-test $args)
    test "$output" $op "$expt" || { echo "error here"; exit 1;}
}

# no input
do_test "" 'handle_base((null), 0, 0)'
# stuff on handle_base only
do_test "-v" 'handle_base((null), 1, 0)'
do_test "--host putas" 'handle_base(putas, 0, 0)'
do_test "--host=maluco" 'handle_base(maluco, 0, 0)'
do_test "-Hze" 'handle_base(ze, 0, 0)'
# envvars
APP_HOST=maluco do_test "" 'handle_base(maluco, 0, 0)'
# extra args on handle_base
do_test "-v -H putas extra twice" 'handle_base(putas, 1, 2, extra, twice)'

# stuff on handle_base_foo
do_test "foo -nNAME" 'handle_base_foo(NAME, (null), 0, 0)'
do_test "foo --host=123" 'handle_base_foo((null), 123, 0, 0)'
# out of order
do_test "--name macho foo -v" 'handle_base_foo(macho, (null), 1, 0)'
do_test "foo -v --name putas --host localhost extra" 'handle_base_foo(putas, localhost, 1, 1, extra)'

# stuff of handle_base_bar
do_test "-v bar" 'handle_base_bar(-1, (null), 1, 0)'
# out of order
do_test "-i 666 -v bar -Hdred" 'handle_base_bar(666, dred, 1, 0)'
# args out of order?
do_test "extra-arg bar another-arg -v" 'handle_base_bar(-1, (null), 1, 2, extra-arg, another-arg)'
