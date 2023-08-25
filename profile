# -*- mode: shell-script -*-
#
# usage:
#  $ source profile

prompt_cmd() {
  # This assumes that the binary generate-prompt has been copied into PATH
  PS1="$(generate-prompt)"
}
PROMPT_COMMAND=prompt_cmd


# This prompt is the one used by generate-prompt when standing outside
# of a git repository. It's passed as-is as stdout.
# Note: all terminal escape codes, if used, are preserved.
unset GP_DEFAULT_PROMPT
export GP_DEFAULT_PROMPT="\[\033[01;32m\]\u@\h\[\033[00m\] \[\033[01;34m\]\W\[\033[00m\] $ "


# This prompt is used when standing inside of a git repo. After the
# special control sequences are replaced by generate-prompt, the
# resulting string is sent to stdout.
#
# Special control sequences:
#   \pR    replaced with the git repo name, coloured with state
#   \pB    replaced with the local branch name, coloured with state
#   \pC    replaced with the current working directory(cwd), coloured with state
#
# The colour switches between green and yellow, or GP_UP_TO_DATE and
# GP_MODIFIED (if these are set in the environment).
#
# 
# repo name:    green   there local and remote branches are the same
#               yellow  there is a difference.
# branch name:  green   there is nothing to commit
#               yellow  something was added, deleted, renamed, etc
# cwd:          green   no tracked files have been modified
#               yellow  some tracked file(s) have been modified
#
# Note: all terminal escape codes, if used, are preserved.
unset GP_GIT_PROMPT
export     GP_GIT_PROMPT="[\pR/\pB/\pC]\n$ ";


unset GP_UP_TO_DATE
unset GP_MODIFIED
# The state colours mentioned above can be overridden using these
# environment variables.
# export     GP_UP_TO_DATE="\033[0;32m"
# export       GP_MODIFIED="\033[0;33m"
