#!/usr/bin/env bats  # -*- mode: shell-script -*-
bats_require_minimum_version 1.5.0

# To run a test manually:
# cd path/to/project/root
# bats test/test.bats

# Binary to test
GENERATE_PROMPT="$BATS_TEST_DIRNAME/../bin/generate-prompt"


# run before each test
setup () {
  RUN_TMPDIR=$( mktemp -d "$BATS_TEST_TMPDIR/tmp.XXXXXX" )
  cd $RUN_TMPDIR

  # colours used by generate-prompt
  UP_TO_DATE="\033[0;32m"
  MODIFIED="\033[0;33m"
  NO_DATA="\033[0;37m"
  RESET="\033[0m"

  # remove all relevant overrides
  unset GP_UP_TO_DATE
  unset GP_MODIFIED
  unset GP_NO_DATA
  unset GP_DEFAULT_PROMPT

  export GP_GIT_PROMPT='REPO:\pR:BRANCH:\pB:WD:\pC:'

}

# run after each test
teardown () {
 rm -rf $RUN_TMPDIR
}



# --------------------------------------------------
@test "that default prompt shows in a non-git dir " {
  run -0 $GENERATE_PROMPT
  [ "$output" =  "\\W $ " ]
}



# --------------------------------------------------
@test "that default prompt respects GP_DEFAULT_PROMPT" {
  export GP_DEFAULT_PROMPT="SOME STRING"
  
  run -0 $GENERATE_PROMPT
  [ "$output" = "SOME STRING" ]
}


# --------------------------------------------------
@test "that creating an empty git repo returns default prompt" {
  git init

  run -0 $GENERATE_PROMPT
  [ "$output" =  "\\W $ " ]
}


# --------------------------------------------------
@test "that adding one file to repo returns default prompt" {
  git init
  touch FOO
  git add FOO

  run -0 $GENERATE_PROMPT
  [ "$output" =  "\\W $ " ]
}


# --------------------------------------------------
@test "that committing a file to an empty repo shows R:no-up-ref, branch:up2date, wc:up2date" {
  git init          >&2
  touch FOO         >&2
  git add FOO       >&2
  git commit -m '.' >&2


  run -0 $GENERATE_PROMPT

  repo=$(basename $PWD)
  branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)
  wd=$(basename $PWD)

  expected_prompt="REPO:${NO_DATA}${repo}${RESET}:BRANCH:${UP_TO_DATE}${branch}${RESET}:WD:${UP_TO_DATE}${wd}${RESET}:"

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}

