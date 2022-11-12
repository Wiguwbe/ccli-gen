
#ifndef _CCLI_STRUCTS_H_
#define _CCLI_STRUCTS_H_

/*
	Base structure
	linked-list
*/

#define CT_BASE struct ct_base *next; int type;

struct ct_base {
	/*
	struct ct_base *next;
	// below STYPE_*
	int type;
	*/
	CT_BASE
};

// command
#define STYPE_CMD 1
// handler
#define STYPE_HDL 2
// option
#define STYPE_OPT 2

struct ct_handler {
	CT_BASE
	char *name;
	char *desc;
};

// for `ct_option.param_type`
#define CPT_STR 's'
#define CPT_INT 'i'
#define CPT_BOOL 'b'

struct ct_option {
	CT_BASE
	char *long_form;
	char *env_form;
	char *description;
	char *param_name;
	char param_type;
	char short_form;
};

struct ct_cmd {
	CT_BASE
	// original name
	char *name;
	// C name
	char *cname;
	// fullname helper
	char *fullname;
	// handler function
	struct ct_handler *handler;
	// list of options
	struct ct_option *options;
	// list of sub-commands
	struct ct_cmd *sub_cmds;
	// a link to parent
	struct ct_cmd *parent;

	//char *description;
};

// list of commands (top level, used to iterate all commands)
struct cmd_list {
	struct cmd_list *next;
	struct ct_cmd *item;
};


#endif
