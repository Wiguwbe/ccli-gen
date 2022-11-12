{#
#include "structs.h"
#include "helpers.h"
#}
{$
	char *basename;
	struct cmd_list *cmds;
$}
{{
	struct ct_cmd *cmd, *pcmd = NULL, *scmd;
	struct ct_option *opt = NULL;
}}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "{>s $basename >}.h"

{* structs *}
{% for struct cmd_list *cmd_i = $cmds; cmd_i; cmd_i=cmd_i->next %}
{{ cmd = cmd_i->item; }}
struct {>s cmd->fullname>}_args {
	{% while (opt=h_iter_options(opt?opt:cmd)) %}
	{>s h_ctype(opt->param_type)>} {>s opt->long_form >};
	{% end %}
};
{% endfor %}

{* helper function *}
void _get_extra_args(int *argc, char**argv)
{
    int count = 0;
    int placer = 0;
    for(int runner = 0; runner < *argc; runner++) {
        if(argv[runner]) {
            argv[placer++] = argv[runner];
            count++;
        }
    }
    *argc = count;
}

{* function definitions *}
{% for struct cmd_list *cmd_i = $cmds; cmd_i; cmd_i = cmd_i->next %}
{{ cmd = cmd_i->item; }}
int _parse_{>s cmd->fullname >}(
	int argc,
	char **argv
	{% while (pcmd=h_iter_parents(pcmd?pcmd:cmd)) %}
	, struct {>s pcmd->fullname >}_args {>s pcmd->fullname >}_args
	{% end %}
);
{% endfor %}

{* function implementations *}
{% for struct cmd_list *cmd_i = $cmds; cmd_i; cmd_i = cmd_i->next %}
{{ cmd = cmd_i->item; }}
int _parse_{>s cmd->fullname>}(int argc, char**argv{%for pcmd=cmd->parent;pcmd;pcmd=pcmd->next%}, struct {>s pcmd->fullname>}_args {>s pcmd->fullname>}_args{%endfor%})
{
	{* our data, init to 0 *}
	struct {>s cmd->fullname >}_args {>s cmd->fullname>}_args = {
		{% while (opt=h_iter_options(opt?opt:cmd)) %}
		{>s h_ctype_nil(opt->param_type)>},
		{%end%}
	};
	int extra_args = 0;

	for(int i=0;i<argc;i++) {
		int used_extra = 0;
		char *ptr = argv[i];
		if(!ptr) {
			continue;
		}
		char *value;
		if(ptr[0] == '-') {
			if(ptr[1] == '-') {
				int opt_len = strlen(ptr+2);
				char *eq = strchr(ptr+2, '=');
				if(eq) {
					opt_len = eq-ptr-2;
					value = eq+1;
				} else {
					value = argv[i+1];
					used_extra = 1;
				}
				{* our options *}
				{% while (opt=h_iter_options(opt?opt:cmd)) %}
				if(!strncmp(ptr+2, "{>s opt->long_form>}", opt_len)) {
					goto _{>s opt->long_form>}_opt;
				}
				{% end %}
				{* parents' options *}
				{% while (pcmd=h_iter_parents(pcmd?pcmd:cmd)) %}
				{% while (opt=h_iter_options(opt?opt:pcmd)) %}
				if(!strncmp(ptr+2, "{>s opt->long_form>}", opt_len)) {
					goto _{>s pcmd->fullname>}_{>s opt->long_form>}_opt;
				}
				{%end%}
				{%end%}
			} else {
				if(strlen(ptr)>2) {
					value = ptr+2;
				} else {
					value = argv[i+1];
					used_extra =1;
				}
				switch(ptr[1]) {
					{% while (opt=h_iter_options_short(opt?opt:cmd)) %}
					case '{>c opt->short_form>}': goto _{>s opt->long_form>}_opt;
					{%end%}
					{% while (pcmd=h_iter_parents(pcmd?pcmd:cmd)) %}
					{% while (opt=h_iter_options_short(opt?opt:pcmd)) %}
					case '{>c opt->short_form>}': goto _{>s pcmd->fullname>}_{>s opt->long_form>}_opt;
					{% end %}
					{% end %}
				}
			}
		} else {
			{* arg/subcommand *}
			{* can't have sub-commands after args *}
			char *save = argv[i];
			argv[i] = NULL;
			{% for scmd=cmd->sub_cmds;scmd;scmd=scmd->next %}
			if(!strcmp(ptr, "{>s scmd->name>}")) {
				return _parse_{>s scmd->fullname>}(
					argc, argv
					{% while (pcmd = h_iter_parents(pcmd?pcmd:scmd)) %}
					, {>s pcmd->fullname >}_args
					{% end %}
				);
			}
			{% endfor %}
			{* not handled *}
			argv[i] = save;
		}

		{* option/arg not handled (yet) *}
		continue;

		{* handle our options *}
		{% while (opt=h_iter_options(opt?opt:cmd)) %}
	_{>s opt->long_form >}_opt:
		{% include "param" {.cmd = cmd, .opt = opt} %}
		goto _{>s cmd->fullname>}_continue;
		{%end%}
		{% while (pcmd=h_iter_parents(pcmd?pcmd:cmd)) %}
		{% while (opt=h_iter_options(opt?opt:pcmd)) %}
	_{>s pcmd->fullname >}_{>s opt->long_form >}_opt:
		{% include "param" {.cmd = pcmd, .opt = opt} %}
		goto _{>s cmd->fullname>}_continue;
		{%end%}
		{%end%}

	_{>s cmd->fullname>}_continue:
		argv[i] = NULL;
		if(used_extra) {
			argv[++i] = NULL;
		}
	}

	{* parse env *}
	{% while (opt=h_iter_options_env(opt?opt:cmd)) %}
	if({>s cmd->fullname>}_args.{>s opt->long_form>} == {>s h_ctype_nil(opt->param_type)>}) {
		char *value = getenv("{>s opt->env_form>}");
		if(value&&*value) {
			{% include "param" {.cmd = cmd, .opt = opt} %}
		}
	}
	{%end%}
	{* parents' *}
	{% while (pcmd=h_iter_parents(pcmd?pcmd:cmd)) %}
	{% while (opt=h_iter_options_env(opt?opt:cmd)) %}
	if({>s pcmd->fullname>}_args.{>s opt->long_form>} == {s h_ctype_nil(opt->param_type)>}) {
		char *value = getenv("{>s opt->env_form>}");
		if(value&&*value) {
			{% include "param" {.cmd=cmd, .opt=opt} %}
		}
	}
	{%end%}{%end%}

	_get_extra_args(&argc, argv);

	{* call handler *}
	return {>s cmd->handler->name >}(
		{* our options *}
		{% while (opt=h_iter_options(opt?opt:cmd)) %}
		{>s cmd->fullname>}_args.{>s opt->long_form>},
		{% end %}
		{* parents' *}
		{% while (pcmd=h_iter_parents(pcmd?pcmd:cmd)) %}
		{% while (opt=h_iter_options(opt?opt:pcmd)) %}
		{>s pcmd->fullname>}_args.{>s opt->long_form>},
		{%end%}{%end%}
		argc, argv
	);
}
{%endfor%}

{* entrypoint *}
int parse(int argc, char**argv)
{
	char *copy_argv[argc];
	for(int i=0;i<argc;i++) {
		copy_argv[i] = argv[i];
	}

	return _parse_{>s $cmds->item->fullname>}(argc-1, argv+1);
}
