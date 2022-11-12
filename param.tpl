{#
#include "structs.h"
#}
{$
	struct ct_cmd *cmd;
	struct ct_option *opt;
$}
{% if $opt->param_type == CPT_STR %}
{>s $cmd->fullname>}_args.{>s $opt->long_form>} = value;
{% elif $opt->param_type == CPT_INT %}
{
	char *endptr;
	{>s $cmd->fullname>}_args.{>s $opt->long_form>} = strtol(value, &endptr, 0);
	if(*endptr) {
		fprintf(stderr, "invalid integer for option --{>s $opt->long_form>}\n");
		return -1;
	}
}
{% else %}
{>s $cmd->fullname>}_args.{>s $opt->long_form>} = 1;
used_extra = 0;
{% end %}
