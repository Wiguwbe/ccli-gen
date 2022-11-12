#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "helpers.h"
// already included, but to be sure
#include "structs.h"

// the templates
#include "tpl-parser_c.h"
#include "tpl-parser_h.h"

/*
	Best memory management is no management,

	when program ends, the memory is freed (thumbs-up)
*/

//static char * _ignore(char *input, int(*what)(int));
static char * _whitespace(char *input);
static char * _comment(char *input);
static struct ct_cmd* parse_cmd(char *input, char**endptr, struct ct_cmd *parent);
static struct ct_option* parse_option(char *input, char **endptr);
static struct ct_handler* parse_handler(char *input, char **endptr);
static struct cmd_list * _normalize_list(struct cmd_list *, struct ct_cmd *);

int main(int argc, char **argv)
{
	if(argc<2) {
		/* TODO we may want some arguments */
		fprintf(stderr, "usage: %s <params-file>\n", *argv);
		return 1;
	}

	// top level command, we shall get from parser function
	struct ct_cmd *top_level;
	// we shall build later
	struct cmd_list *cmd_list;
	// we shall read the file into memory
	char *data;
	char basename[256];

	{ /* read file */
		FILE *input = fopen(argv[1], "r");
		if(!input) {
			perror("failed to open input file");
			return 1;
		}
		if(fseek(input, 0L, SEEK_END)) {
			perror("failed to seek file");
			return 1;
		}
		int data_len;
		if((data_len=ftell(input)) < 0) {
			perror("failed to ftell");
			return 1;
		}
		data = (char*)malloc(data_len+1);
		if(!data) {
			perror("failed to allocate data");
			return 1;
		}
		// read it
		if(fseek(input, 0L, SEEK_SET)) {
			perror("failed to rewind file");
			return 1;
		}
		if(fread(data, 1, data_len, input) != data_len) {
			fprintf(stderr, "failed to read data: %s\n", strerror(errno));
			return 1;
		}
		// we're just reading, if it fails, it fails
		fclose(input);
		// final null byte
		data[data_len] = 0;
	}

	// parse
	char *end;
	top_level = parse_cmd(data, &end, NULL);
	if(!top_level) {
		// some error occurred
		return 1;
	}

	// assert we've reached the end
	end = _whitespace(end);
	if(!end) {
		// some error
		return 1;
	}

	if(*end) {
		fprintf(stderr, "unexpected data at the end of file: '%s'\n", end);
		return 1;
	}

	// normalize cmd_list
	cmd_list = _normalize_list(NULL, top_level);
	// test it
	for(struct cmd_list *cmdi = cmd_list; cmdi; cmdi = cmdi->next) {
		fprintf(stderr, "%p %s\n", cmdi, cmdi->item->name);
	}

	{ /* gen files */
		int name_len = strlen(argv[1]);
		memcpy(basename, argv[1], name_len);
		// c file
		{
			basename[name_len] = '.';
			basename[name_len+1] = 'c';
			basename[name_len+2] = 0;
			FILE *c_file = fopen(basename, "w");
			if(!c_file) {
				perror("failed to open/create file");
				return 1;
			}
			struct parser_c_ctx ctx = {
				.basename = argv[1],
				.cmds = cmd_list
			};
			if(parser_c_gen(c_file, &ctx)) {
				return 1;
			}
			if(fclose(c_file)) {
				perror("failed to write/close file");
				return 1;
			}
		}
		// h file
		{
			basename[name_len] = '.';
			basename[name_len+1] = 'h';
			basename[name_len+2] = 0;
			FILE *h_file = fopen(basename, "w");
			if(!h_file) {
				perror("failed to open/create file");
				return 1;
			}
			struct parser_h_ctx ctx = {
				.basename = argv[1],
				.cmds = cmd_list
			};
			if(parser_h_gen(h_file, &ctx)) {
				return 1;
			}
			if(fclose(h_file)) {
				perror("failed to write/close file");
				return 1;
			}
		}
	}

	// all done
	// files are closed
	// memory shall be free'ed at exit

	return 0;
}

static struct cmd_list *
_normalize_list(struct cmd_list *parent, struct ct_cmd *cmd)
{
	struct cmd_list *self = (struct cmd_list*)malloc(sizeof(struct cmd_list));
	if(!self) {
		perror("failed to allocate memory");
		return NULL;
	}
	self->item = cmd;
	struct cmd_list *sublist = self;
	for(struct ct_cmd *scmd = cmd->sub_cmds; scmd; scmd = (struct ct_cmd*)scmd->next) {
		sublist = _normalize_list(sublist, scmd);
		if(!sublist) {
			return NULL;
		}
	}
	sublist->next = NULL;
	if(parent) {
		parent->next = self;
	}
	// else
	return self;
}

static char* _whitespace(char*input)
{
	while(1) {
		while(isspace(*input)) input++;
		if(*input == '#') {
			// no error check here
			input = _comment(input);
		} else {
			return input;
		}
	}
}

static char* _comment(char *input)
{
	while(*input != '\n') {
		if(!*input)
			return input;
		input++;
	}
	// place at next item
	return input+1;
}

static struct ct_cmd* parse_cmd(char *input, char **endptr, struct ct_cmd *parent)
{
	fprintf(stderr, "enter parse_cmd\n");
	struct ct_cmd *cmd = (struct ct_cmd*)malloc(sizeof(struct ct_cmd));
	if(!cmd) {
		perror("failed to alloc struct ct_cmd");
		return NULL;
	}
	memset(cmd, 0, sizeof(struct ct_cmd));
	cmd->type = STYPE_CMD;
	cmd->parent = parent;

	input = _whitespace(input);
	if(!*input) {
		fprintf(stderr, "unexpected EOF while searching for command name\n");
		return NULL;
	}
	{ /* get name */
		char *name_index = input;
		// find next space/brace
		while(*input) {
			if(isspace(*input) || *input == '{') {
				// got it
				break;
			}
			// else, part of name
			input++;
		}
		int name_len = input-name_index;
		cmd->name = (char*)malloc(name_len+1);
		if(!cmd->name) {
			perror("failed to allocate memory");
			return NULL;
		}
		memcpy(cmd->name, name_index, name_len);
		cmd->name[name_len] = 0;

		// and replace for cname
		cmd->cname = (char*)malloc(name_len+1);
		if(!cmd->cname) {
			perror("failed to allocate memory");
			return NULL;
		}
		memcpy(cmd->cname, name_index, name_len);
		cmd->cname[name_len] = 0;
		for(char *ptr = cmd->cname; *ptr; ptr++) {
			if(*ptr == '-') *ptr = '_';
		}

		// and fullname
		char *fullname = NULL;
		if(parent) {
			// add to it
			int pfn_len = strlen(parent->fullname);
			int mfn_len = pfn_len+1+strlen(cmd->cname);
			cmd->fullname = (char*)malloc(mfn_len+1);
			if(!cmd->fullname) {
				perror("failed to allocate memory");
				return NULL;
			}
			memcpy(cmd->fullname, parent->fullname, pfn_len);
			cmd->fullname[mfn_len] = 0;
			cmd->fullname[pfn_len] = '_';
			fullname = cmd->fullname + pfn_len+1;
		} else {
			int mfn_len = strlen(cmd->cname);
			cmd->fullname = (char*)malloc(mfn_len+1);
			if(!cmd->fullname) {
				perror("failed to allocate memory");
				return NULL;
			}
			cmd->fullname[mfn_len] = 0;
			fullname = cmd->fullname;
		}
		memcpy(fullname, cmd->cname, strlen(cmd->cname));
	}
	// find opening brace
	input = _whitespace(input);
	if(!*input) {
		fprintf(stderr, "unexpected EOF while searching for command arguments\n");
		return NULL;
	}
	if(*input != '{') {
		fprintf(stderr, "unexpected '%c' after command name (expected '{')\n", *input);
		return NULL;
	}
	input ++;

	// right, get internal stuff
	input = _whitespace(input);	// initial space
	while(*input != '}') {
		switch(*input) {
		case '-':
		case '$':
			// option
			{
				struct ct_option *opt = parse_option(input, &input);
				if(!opt) {
					return NULL;
				}
				if(cmd->options) {
					struct ct_option *ptr = cmd->options;
					while(ptr->next) ptr = (struct ct_option*)ptr->next;
					ptr->next = (struct ct_base*)opt;
				} else {
					cmd->options = opt;
				}
			}
			break;
		case '@':
			// handler
			{
				struct ct_handler *hlr = parse_handler(input, &input);
				if(!hlr) {
					return NULL;
				}
				if(cmd->handler) {
					fprintf(stderr, "multiple handlers for cmd '%s'\n", cmd->name);
					return NULL;
				}
				// else
				cmd->handler = hlr;
			}
			break;
		case 0:
			// unexpected EOF
			fprintf(stderr, "unexpected EOF while searching for command arguments\n");
			return NULL;
		default:
			// sub command
			{
				struct ct_cmd *sub = parse_cmd(input, &input, cmd);
				if(!sub) {
					return NULL;
				}
				if(cmd->sub_cmds) {
					struct ct_cmd *ptr = cmd->sub_cmds;
					while(ptr->next) ptr = (struct ct_cmd*)ptr->next;
					ptr->next = (struct ct_base*)sub;
				} else {
					cmd->sub_cmds = sub;
				}
			}
		}
		input = _whitespace(input);
	}

_end:
	if(endptr) {
		*endptr = input+1;
	}

	fprintf(stderr, "end parse_cmd\n");

	return cmd;
}

static struct ct_option *parse_option(char *input, char**endptr)
{
	fprintf(stderr, "enter parse_option\n");
	struct ct_option *opt = (struct ct_option*)malloc(sizeof(struct ct_option));
	if(!opt) {
		perror("failed to allocate struct ct_option");
		return NULL;
	}
	memset(opt, 0, sizeof(struct ct_option));
	opt->type = STYPE_OPT;

	while(*input != '\n' && *input != '}') {
		switch(*input) {
		case '-':
			// short/long form
			if(input[1] == '-') {
				fprintf(stderr, "got long form option\n");
				// long form
				char *beg = input+2;
				// look for space, comma or '='
				while(!isspace(*input) && *input!='=' && *input!=',' && *input != ':' && *input != '}') {
					if(!*input) {
						fprintf(stderr, "unexpected EOF while searching for option name\n");
						return NULL;
					}
					input++;
				}
				int name_len = input-beg;
				opt->long_form = (char*)malloc(name_len+1);
				if(!opt->long_form) {
					perror("failed to allocate memory");
					return NULL;
				}
				memcpy(opt->long_form, beg, name_len);
				opt->long_form[name_len] = 0;
				fprintf(stderr, "long form value is: '%s'\n", opt->long_form);
				if(*input == '=') {
					fprintf(stderr, "got a varname on long form\n");
					// var name
					beg = ++input;
					while(!isspace(*input) && *input!=',' && *input != ':' && *input != '}') {
						if(!*input) {
							fprintf(stderr, "unexpected EOF while parsing var name\n");
							return NULL;
						}
						input++;
					}
					name_len = input-beg;
					opt->param_name = (char*)malloc(name_len+1);
					if(!opt->param_name) {
						perror("failed to allocate memory");
						return NULL;
					}
					memcpy(opt->param_name, beg, name_len);
					opt->param_name[name_len] = 0;
					fprintf(stderr, "varname is: '%s'\n", opt->param_name);
				}
			} else {
				fprintf(stderr, "got a short form option\n");
				// a single char
				if(*(++input)) {
					opt->short_form = *(input++);
				} else {
					// its EOF
					fprintf(stderr, "unexpected EOF while parsing option\n");
					return NULL;
				}
				fprintf(stderr, "short form key is '%c'\n", opt->short_form);
			}
			break;
		case '$':
			// env form
			{
				fprintf(stderr, "got env form option\n");
				char *beg = ++input;
				while(!isspace(*input) && *input !=',' && *input != ':' && *input != '}') {
					if(!*input) {
						fprintf(stderr, "unexpected EOF while parsing envvar name\n");
						return NULL;
					}
					input++;
				}
				int name_len = input-beg;
				opt->env_form = (char*)malloc(name_len+1);
				if(!opt->env_form) {
					perror("failed to allocate memory");
					return NULL;
				}
				memcpy(opt->env_form, beg, name_len);
				opt->env_form[name_len] = 0;
				fprintf(stderr, "env form is: '%s'\n", opt->env_form);
			}
			break;
		case ':':
			// description
			{
				fprintf(stderr, "got option description\n");
				++input;
				// skip first whitespace
				while(isblank(*input)) input++;
				if(!*input) {
					fprintf(stderr, "unexpected EOF while parsing description\n");
					return NULL;
				}
				char *beg = input;
				// until '\n'
				while(*input != '\n' && *input != '}') {
					if(!*input) {
						fprintf(stderr, "unexpected EOF while parsing description\n");
						return NULL;
					}
					input++;
				}
				int desc_len = input-beg;
				opt->description = (char*)malloc(desc_len+1);
				if(!opt->description) {
					perror("failed to allocate memory\n");
					return NULL;
				}
				memcpy(opt->description, beg, desc_len);
				opt->description[desc_len] = 0;
				fprintf(stderr, "option description is '%s'\n", opt->description);
			}
			break;
		case 0:
			// EOF
			fprintf(stderr, "unexpected EOF while parsing option\n");
			return NULL;
		default:
			// param type
			{
				char *beg = input;
				while(!isspace(*input) && *input != ',' && *input != ':' && *input != '}') {
					if(!*input) {
						fprintf(stderr, "unexpected EOF while parsing type name\n");
						return NULL;
					}
					input++;
				}
				// TODO make param_type a "char *" to be a validator
				// instead of this:
				switch(*beg){
				case 's':
				case 'i':
				case 'b':
					// quick hack since CPT_* use the first item
					opt->param_type = *beg;
					break;
				default:
					*input=0;
					fprintf(stderr, "unknown param type '%s'\n", beg);
					return NULL;
				}
			}
		}
		// skip whitespace
		while(isblank(*input)) input++;
		if(!*input) {
			fprintf(stderr, "unexpected EOF between command options\n");
			return NULL;
		}
		// skip comma
		if(*input == ',') input++;
		// more whitespace
		while(isblank(*input)) input++;
		if(!*input) {
			fprintf(stderr, "unexpected EOF between command options\n");
			return NULL;
		}

		// ready for next take
	}

	// end of cli option, make sure we got all we need
	if(!opt->param_type) {
		// TODO default `str`?
		fprintf(stderr, "option doesn't have a type\n");
		return NULL;
	}
	if(!opt->long_form) {
		fprintf(stderr, "option's long form (--<something>) is required\n");
		return NULL;
	}

	if(endptr) {
		*endptr = input;
	}
	return opt;
}

static struct ct_handler *parse_handler(char *input, char**endptr)
{
	fprintf(stderr, "enter parse_handler\n");
	struct ct_handler *hdl = (struct ct_handler*)malloc(sizeof(struct ct_handler));
	if(!hdl) {
		perror("failed to allocate struct ct_handler\n");
		return NULL;
	}
	memset(hdl, 0, sizeof(struct ct_handler));
	hdl->type = STYPE_HDL;

	// input should be at '@'
	{ /* parse name */
		char *beg = ++input;
		while(!isspace(*input)) {
			if(!*input) {
				fprintf(stderr, "unexpected EOF while parsing handler name\n");
				return NULL;
			}
			input++;
		}
		int name_len = input-beg;
		hdl->name = (char*)malloc(name_len +1);
		if(!hdl->name) {
			perror("failed to allocate memory");
			return NULL;
		}
		memcpy(hdl->name, beg, name_len);
		hdl->name[name_len] = 0;
	}

	// description
	if(isblank(*input)) {
		// description it is
		char *beg = ++input;
		while(*input!='\n' && *input != '}') {
			if(!*input) {
				fprintf(stderr, "unexpected EOF while parsing command description\n");
				return NULL;
			}
			input++;
		}
		int desc_len = input-beg;
		hdl->desc = (char*)malloc(desc_len+1);
		if(!hdl->desc) {
			perror("failed to allocate memory");
			return NULL;
		}
		memcpy(hdl->desc, beg, desc_len);
		hdl->desc[desc_len] = 0;
	}

	// input should be at '\n' or '}'
	if(endptr) {
		*endptr = input;
	}

	return hdl;
}
