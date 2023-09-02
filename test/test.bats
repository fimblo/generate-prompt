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
@test "default prompt is displayed when not in a git repository" {
  # given we stand in a directory which isn't a git repo

  # when we run the prompt
  run -0 $GENERATE_PROMPT

  # then we should get the default prompt
  [ "$output" =  "\\W $ " ]
}



# --------------------------------------------------
@test "override default prompt with GP_DEFAULT_PROMPT" {
  # given we set a envvar to override the default prompt
  export GP_DEFAULT_PROMPT="SOME STRING"
  
  # when we run the prompt
  run -0 $GENERATE_PROMPT

  # then it should return what we set to be the prompt
  [ "$output" = "SOME STRING" ]
}


# --------------------------------------------------
@test "empty git repository shows default prompt" {
  # given we create a repo - and we do nothing more
  git init

  # when we run the prompt
  run -0 $GENERATE_PROMPT

  # then it should behave as in a normal non-git repo
  [ "$output" =  "\\W $ " ]
}


# --------------------------------------------------
@test "adding file to git repo doesn't alter prompt" {
  # given we create a repo, add a file but don't commit
  git init
  touch FOO
  git add FOO

  # when we run the prompt
  run -0 $GENERATE_PROMPT

  # then we should get the default non-git prompt
  [ "$output" =  "\\W $ " ]
}


# --------------------------------------------------
@test "committing in empty git repo updates prompt" {
  # given we create a repo and commit a file
  git init          >&2
  touch FOO         >&2
  git add FOO       >&2
  git commit -m '.' >&2

  # when we run the prompt
  run -0 $GENERATE_PROMPT

  # then we should get a git prompt, where
  # - the repo name should be the git project name and upstream ref is
  #   undefined (NO_DATA)
  # - the branch name should be what is written in .git/HEAD, and
  #   status is up-to-date (all added changes are committed)
  # - the working directory should be the folder name and status is
  #   up-to-date (there are no tracked files which are modified.)
  # 
  repo=$(basename $(git rev-parse --show-toplevel))
  branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)
  wd=$(basename $PWD)

  expected_prompt="REPO:${NO_DATA}${repo}${RESET}:BRANCH:${UP_TO_DATE}${branch}${RESET}:WD:${UP_TO_DATE}${wd}${RESET}:"

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "modifying tracked file updates prompt" {
  # given we modify a tracked file
  git init
  touch FOO
  git add FOO
  git commit -m '.'
  echo > FOO

  # when we run the prompt
  run -0 $GENERATE_PROMPT

  # then we should get a git prompt, where
  # - the repo name should be the git project name and upstream ref is
  #   undefined (NO_DATA)
  # - the branch name should be what is written in .git/HEAD, and
  #   status is up-to-date (all added changes are committed)
  # - the working directory should be the folder name and status is
  #   modified
  # 
  repo=$(basename $(git rev-parse --show-toplevel))
  branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)
  wd=$(basename $PWD)

  expected_prompt="REPO:${NO_DATA}${repo}${RESET}:BRANCH:${UP_TO_DATE}${branch}${RESET}:WD:${MODIFIED}${wd}${RESET}:"

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "changing branch updates prompt" {
  # given we have a git repo
  git init
  touch FOO
  git add FOO
  git commit -m '.'

  # given we change branch
  git checkout -b featureBranch

  # when we run the prompt
  run -0 $GENERATE_PROMPT

  # then we should get a git prompt, where
  # - the branch name should be featureBranch

  repo=$(basename $(git rev-parse --show-toplevel))
  branch="featureBranch"
  wd=$(basename $PWD)

  expected_prompt="REPO:${NO_DATA}${repo}${RESET}:BRANCH:${UP_TO_DATE}${branch}${RESET}:WD:${UP_TO_DATE}${wd}${RESET}:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "cloning a repo and entering it updates prompt" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  git init
  touch FOO
  git add FOO
  git commit -m 'Initial commit'
  cd -

  # given we clone it
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo


  # when we run the prompt
  run -0 $GENERATE_PROMPT

  # then we should get a git prompt
  # where each of the three fields are up-to-date
  repo=$(basename $(git rev-parse --show-toplevel))
  branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)
  wd=$(basename $PWD)

  expected_prompt="REPO:${UP_TO_DATE}${repo}${RESET}:BRANCH:${UP_TO_DATE}${branch}${RESET}:WD:${UP_TO_DATE}${wd}${RESET}:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "committing a change in a cloned repo updates prompt" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  git init
  touch FOO
  git add FOO
  git commit -m 'Initial commit'
  cd -

  # given we clone it
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo


  # given we commit a change
  echo > FOO
  git add FOO
  git commit -m 'update the file'

  # when we run the prompt
  run -0 $GENERATE_PROMPT

  # then we should get a git prompt
  # where the repo field should be set to MODIFIED
  repo=$(basename $(git rev-parse --show-toplevel))
  branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)
  wd=$(basename $PWD)

  expected_prompt="REPO:${MODIFIED}${repo}${RESET}:BRANCH:${UP_TO_DATE}${branch}${RESET}:WD:${UP_TO_DATE}${wd}${RESET}:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}
