{#
#include <stdlib.h>
#include "structs.h"
#include "helpers.h"
#}
{$
	char *basename;
	struct cmd_list *cmds;
$}
{{
	char*bname_upper = h_upper($basename);
	struct ct_cmd *cmd, *pcmd = NULL;
	struct ct_option *opt = NULL;
}}

#ifndef _{>s bname_upper>}_H_
#define _{>s bname_upper>}_H_

{* entrypoint *}
int parse(int argc, char**argv);

{* handlers *}
{% for struct cmd_list *cmd_i = $cmds; cmd_i; cmd_i=cmd_i->next %}
{{ cmd = cmd_i->item; }}
int {>s cmd_i->item->handler->name >}(
	{* iterate options *}
	{% while (opt=h_iter_options(opt?opt:cmd)) %}
	{>s h_ctype(opt->param_type) >} {>s opt->long_form >},
	{%end%}
	{* iterate parents self-root (bottom-top) *}
	{% while (pcmd=h_iter_parents(pcmd?pcmd:cmd)) %}
	{% while (opt=h_iter_options(opt?opt:pcmd)) %}
	{>s h_ctype(opt->param_type)>} {>s opt->long_form >},
	{% end %}
	{% end %}
	int argc_left,
	char** argv_left
);
{% endfor %}

#endif
{{
	free(bname_upper);
}}
