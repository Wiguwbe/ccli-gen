
set -ex

# run the test.py
python3 test.py

# got generated.c and generated.h

cat > main.c << EOF

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
EOF


gcc main.c generated.c

# and the tests
function do_test() {
    local args=$1
    local expt=$2

    local output=$(./a.out $args)
    test "$output" = "$expt" || { echo "error here"; exit 1;}
}

# no input
do_test "" 'handle_base(0, (null), 0)'
# stuff on handle_base only
do_test "-v" 'handle_base(1, (null), 0)'
do_test "--host putas" 'handle_base(0, putas, 0)'
do_test "--host=maluco" 'handle_base(0, maluco, 0)'
do_test "-Hze" 'handle_base(0, ze, 0)'
# envvars
APP_HOST=maluco do_test "" 'handle_base(0, maluco, 0)'
# extra args on handle_base
do_test "-v -H putas extra twice" 'handle_base(1, putas, 2, extra, twice)'

# stuff on handle_base_foo
do_test "foo -nNAME" 'handle_base_foo(NAME, 0, (null), 0)'
do_test "foo --host=123" 'handle_base_foo((null), 0, 123, 0)'
# out of order
do_test "--name macho foo -v" 'handle_base_foo(macho, 1, (null), 0)'
do_test "foo -v --name putas --host localhost extra" 'handle_base_foo(putas, 1, localhost, 1, extra)'

# stuff of handle_base_bar
do_test "-v bar" 'handle_base_bar(-1, 1, (null), 0)'
# out of order
do_test "-i 666 -v bar -Hdred" 'handle_base_bar(666, 1, dred, 0)'
# args out of order?
do_test "extra-arg bar another-arg -v" 'handle_base_bar(-1, 1, (null), 2, extra-arg, another-arg)'
