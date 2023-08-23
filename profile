# -*- mode: shell-script -*-
#
# usage:
#  $ source profile

prompt_cmd() {
  # This assumes that the binary generate-prompt has been copied into PATH
  PS1="$(generate-prompt)"
}
PROMPT_COMMAND=prompt_cmd


# If you want to override the default prompt, which will be shown when
# outside of a git repo, uncomment this row. You can change this as
# you see fit, as if it was your normal PS1 variable.
#
# export GP_DEFAULT_PROMPT="\[\033[01;32m\]\u@\h\[\033[00m\] \[\033[01;34m\]\W\[\033[00m\] $ "


# If you want to override the way the prompt looks inside of a git
# repo, then you'll need to set all four of these environment
# variables.
#
# This first envvar needs to be set to anything other than the empty
# string. If it is unset, the remaining three envvars will not be
# used.
# export GP_USE_GIT_PROMPTS_FROM_ENV="true"
#
# These three can be modified to your heart's extent.
# export GP_UP_TO_DATE_PROMPT="UPTODATE: repo: repo_name, branch: branch_name $ ";
# export GP_MODIFIED_PROMPT="MODIFIED: repo: repo_name, branch: branch_name $ ";
# export GP_STAGED_PROMPT="STAGED: repo: repo_name, branch: branch_name $ ";



unset GP_DEFAULT_PROMPT
unset GP_UP_TO_DATE
unset GP_MODIFIED
unset GP_STAGED
unset GP_CWD


export     GP_GIT_PROMPT="[\pR][\pB][\pC]\n$ ";
export GP_DEFAULT_PROMPT="\[\033[01;32m\]\u@\h\[\033[00m\] \[\033[01;34m\]\W\[\033[00m\] $ "


# export     GP_UP_TO_DATE="\033[0;32m"
# export       GP_MODIFIED="\033[01;33m"
# export         GP_STAGED="\033[01;31m"
# export            GP_CWD="\033[1;34m"

