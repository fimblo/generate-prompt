# -*- mode: shell-script -*-
#
# usage:
#  $ source $git_profile

prompt_cmd() {
  # This assumes that the binary git-prompt has been copied into PATH
  PS1="$(git-prompt)"
}
PROMPT_COMMAND=prompt_cmd

export DEFAULT_PROMPT="\[\033[01;32m\]\u@\h\[\033[00m\] \[\033[01;34m\]\W\[\033[00m\] $ "
