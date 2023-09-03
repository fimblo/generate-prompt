# -*- mode: shell-script -*-
#
# For details on all the environment variables supported, see
# README.org
#
# usage:
#  $ source profile
#


prompt_cmd() {
  # This assumes that the binary generate-prompt has been copied into PATH
  PS1="$(generate-prompt)"
}
PROMPT_COMMAND=prompt_cmd


# This prompt is the one used by generate-prompt when standing outside
# of a git repository.
unset GP_DEFAULT_PROMPT
export GP_DEFAULT_PROMPT="\[\033[01;32m\]\u@\h\[\033[00m\] \[\033[01;34m\]\W\[\033[00m\] $ "


# This prompt is used when standing inside of a git repo. 
unset GP_GIT_PROMPT
export GP_GIT_PROMPT="[\pR/\pL/\pC]\n$ ";


unset GP_UP_TO_DATE
unset GP_MODIFIED
unset GP_NO_DATA
unset GP_RESET
# The state colours mentioned above can be overridden using these
# environment variables.
# export     GP_UP_TO_DATE="\033[0;32m"
# export       GP_MODIFIED="\033[0;33m"
# export     GP_NO_DATA="\033[0;37m"
# export     GP_RESET="\033[0m"
