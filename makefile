OUT=ccli-gen

CTPL=ctemplate

OBJS=ccli.o helpers.o tpl-param.o tpl-param-data.o tpl-parser_c.o tpl-parser_h.o tpl-parser_c-data.o tpl-parser_h-data.o

$(OUT): $(OBJS)
	$(CC) -o $@ $^ -ggdb

ccli.o: ccli.c tpl-parser_c.c tpl-parser_h.c
	$(CC) -c $< -ggdb

helpers.o: helpers.c
	$(CC) -c $< -ggdb

tpl-param.o: tpl-param.c
	$(CC) -c $< -ggdb

tpl-parser_c.o: tpl-parser_c.c tpl-param.c
	$(CC) -c $< -ggdb

tpl-parser_h.o: tpl-parser_h.c
	$(CC) -c $< -ggdb

# tpl-%.c: %.tpl
# 	$(CTPL) $<
tpl-param.c: param.tpl
	$(CTPL) $<

tpl-parser_c.c: parser-c.tpl
	$(CTPL) $<

tpl-parser_h.c: parser-h.tpl
	$(CTPL) $<

test: ccli-test
	sh test.sh

ccli-test: test.o params.o
	$(CC) -o $@ $^

test.o: test.c params.h
	$(CC) -c $<

params.o: params.c params.h
	$(CC) -c $<

params.c: ccli-gen params
	./ccli-gen params

clean:
	rm tpl-*
	rm *.o
