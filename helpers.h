
#ifndef _CCLI_HELPERS_H_
#define _CCLI_HELPERS_H_

/*
	A bunch of helper functions
*/

#include "structs.h"

// TODO declare/implement as needed

// transform uppercase
char *h_upper(char*);

// get "char *", "int", "int" from CPT_*
char *h_ctype(char);
// nil value for ctype
char *h_ctype_nil(char);

struct ct_option *h_iter_options(struct ct_base *cmd_or_opt);
struct ct_option *h_iter_options_short(struct ct_base *cmd_or_opt);
struct ct_option *h_iter_options_env(struct ct_base *cmd_or_opt);
struct ct_cmd *h_iter_parents(struct ct_cmd *cmd);
//struct ct_cmd *h_iter_subcmds(struct ct_cmd *cmd);


#endif
