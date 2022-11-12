
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "structs.h"


#include "helpers.h"

char *h_upper(char *in)
{
	int len = strlen(in);
	char *out = (char*)malloc(len+1);
	if(!out) {
		perror("failed to allocate memory");
		exit(1);
	}
	for(int i=0;i<len;i++) {
		out[i] = in[i];
		if(islower(out[i])) out[i] -= 0x20;
	}
	out[len] = 0;
	return out;
}

char *h_ctype(char in)
{
	switch(in){
	case CPT_STR:	return "char *";
	case CPT_INT:	return "int";
	case CPT_BOOL:	return "int";
	}
	return NULL;
}

char *h_ctype_nil(char in)
{
	switch(in){
	case CPT_STR:	return "NULL";
	case CPT_INT:	return "-1";
	case CPT_BOOL:	return "0";
	}
	return "0";
}

struct ct_option *h_iter_options(struct ct_base *cmd_or_opt)
{
	if(!cmd_or_opt)
		return NULL;
	if(cmd_or_opt->type == STYPE_CMD) {
		return ((struct ct_cmd*)cmd_or_opt)->options;
	}
	// else
	return ((struct ct_option*)cmd_or_opt)->next;
}

struct ct_option *h_iter_options_short(struct ct_base *cmd_or_opt)
{
	struct ct_option *n = h_iter_options(cmd_or_opt);
	while(n) {
		if(n->short_form)
			return n;
		// else
		n = h_iter_options(n);
	}
	return NULL;
}

struct ct_option *h_iter_options_env(struct ct_base *cmd_or_opt)
{
	struct ct_option *n = h_iter_options(cmd_or_opt);
	while(n) {
		if(n->env_form) return n;
		n = h_iter_options(n);
	}
}

struct ct_cmd *h_iter_parents(struct ct_cmd *cmd)
{
	return cmd->parent;
}

struct ct_cmd *h_iter_subcmds(struct ct_cmd *cmd)
{
	// tricky, is it
	return NULL;
}
