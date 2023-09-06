#!/usr/bin/env bats  # -*- mode: shell-script -*-
bats_require_minimum_version 1.5.0

# To run a test manually:
# cd path/to/project/root
# bats test/test.bats


helper__init_repo() {
  git init
}

helper__add_a_file() {
  file_to_commit="$1"
  content_to_commit="$2"

  helper__init_repo
  echo "$content_to_commit" > $file_to_commit
  git add $file_to_commit
}

helper__commit_to_new_repo() {
  file_to_commit="$1"
  content_to_commit="$2"

  helper__add_a_file "$file_to_commit" "$content_to_commit"
  git commit -m 'Initial commit'
}



# Binary to test
GENERATE_PROMPT="$BATS_TEST_DIRNAME/../bin/generate-prompt"


# run before each test
setup () {
  RUN_TMPDIR=$( mktemp -d "$BATS_TEST_TMPDIR/tmp.XXXXXX" )
  cd $RUN_TMPDIR


  # Revert most environment variables to default state
  # pre- and postfix patterns
  unset GP_UP_TO_DATE
  unset GP_MODIFIED
  unset GP_NO_DATA
  unset GP_RESET

  # prompt patterns
  unset GP_DEFAULT_PROMPT
  export GP_GIT_PROMPT='REPO:\pR:LOCALBRANCH:\pL:WD:\pC:'

  # styles
  unset GP_WD_STYLE


  # Set global git user/email if it is not set
  if ! [ -e ~/.gitconfig ] ; then
    git config --global user.email "my@test.com"
    git config --global user.name "Test Person"
  fi


  # colour codes used by all tests
  UP_TO_DATE="\033[0;32m"
  MODIFIED="\033[0;33m"
  NO_DATA="\033[0;37m"
  RESET="\033[0m"

  # exit codes
  EXIT_GIT_PROMPT=0
  EXIT_DEFAULT_PROMPT=1
  EXIT_NO_LOCAL_REF=2

  EXIT_FAIL_GIT_STATUS=255
  EXIT_FAIL_REPO_OBJ=254

}

# run after each test
teardown () {
  rm -rf $RUN_TMPDIR
}



# --------------------------------------------------
@test "default prompt is displayed when not in a git repository" {
  # given we stand in a directory which isn't a git repo

  # when we run the prompt
  run -${EXIT_DEFAULT_PROMPT} $GENERATE_PROMPT

  # then we should get the default prompt
  [ "$output" =  "\\W $ " ]
}



# --------------------------------------------------
@test "override default prompt with GP_DEFAULT_PROMPT" {
  # given we set a envvar to override the default prompt
  export GP_DEFAULT_PROMPT="SOME STRING"

  # when we run the prompt
  run -${EXIT_DEFAULT_PROMPT} $GENERATE_PROMPT

  # then it should return what we set to be the prompt
  [ "$output" = "SOME STRING" ]
}


# --------------------------------------------------
@test "empty git repository shows default prompt" {
  # given we create a repo - and we do nothing more
  git init

  # when we run the prompt
  run -${EXIT_NO_LOCAL_REF} $GENERATE_PROMPT

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
  run -${EXIT_NO_LOCAL_REF} $GENERATE_PROMPT

  # then we should get the default non-git prompt
  [ "$output" =  "\\W $ " ]
}


# --------------------------------------------------
@test "committing in empty git repo updates prompt" {
  # given we create a repo and commit a file
  helper__commit_to_new_repo "newfile" "some text"

  # when we run the prompt
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT

  # then we should get a git prompt, where
  # - the repo name should be the git project name and upstream ref is
  #   undefined (NO_DATA)
  # - the branch name should be what is written in .git/HEAD, and
  #   status is up-to-date (all added changes are committed)
  # - the working directory should be the folder name and status is
  #   up-to-date (there are no tracked files which are modified.)
  #
  repo=$(basename $(git rev-parse --show-toplevel))
  l_branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)
  wd=$(basename $PWD)

  expected_prompt="REPO:${NO_DATA}${repo}${RESET}:LOCALBRANCH:${UP_TO_DATE}${l_branch}${RESET}:WD:${UP_TO_DATE}${wd}${RESET}:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "modifying tracked file updates prompt" {
  # given we modify a tracked file
  helper__commit_to_new_repo "newfile" "some text"
  echo > newfile

  # when we run the prompt
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT

  # then we should get a git prompt, where
  # - the repo name should be the git project name and upstream ref is
  #   undefined (NO_DATA)
  # - the branch name should be what is written in .git/HEAD, and
  #   status is up-to-date (all added changes are committed)
  # - the working directory should be the folder name and status is
  #   modified
  #
  repo=$(basename $(git rev-parse --show-toplevel))
  l_branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)
  wd=$(basename $PWD)

  expected_prompt="REPO:${NO_DATA}${repo}${RESET}:LOCALBRANCH:${UP_TO_DATE}${l_branch}${RESET}:WD:${MODIFIED}${wd}${RESET}:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "changing localbranch updates prompt" {
  # given we have a git repo
  helper__commit_to_new_repo "newfile" "some text"

  # given we change localbranch
  git checkout -b featureBranch

  # when we run the prompt
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT

  # then we should get a git prompt, where
  # - the localbranch name should be featureBranch

  repo=$(basename $(git rev-parse --show-toplevel))
  l_branch="featureBranch"
  wd=$(basename $PWD)

  expected_prompt="REPO:${NO_DATA}${repo}${RESET}:LOCALBRANCH:${UP_TO_DATE}${l_branch}${RESET}:WD:${UP_TO_DATE}${wd}${RESET}:"
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
  helper__commit_to_new_repo "newfile" "some text"
  cd -

  # given we clone it
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo


  # when we run the prompt
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT

  # then we should get a git prompt
  # where each of the three fields are up-to-date
  repo=$(basename $(git rev-parse --show-toplevel))
  l_branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)
  wd=$(basename $PWD)

  expected_prompt="REPO:${UP_TO_DATE}${repo}${RESET}:LOCALBRANCH:${UP_TO_DATE}${l_branch}${RESET}:WD:${UP_TO_DATE}${wd}${RESET}:"
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
  helper__commit_to_new_repo "newfile" "some text"
  cd -

  # given we clone it
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo


  # given we commit a change
  echo > newfile
  git add newfile
  git commit -m 'update the file'

  # when we run the prompt
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT

  # then we should get a git prompt
  # where the repo field should be set to MODIFIED
  repo=$(basename $(git rev-parse --show-toplevel))
  l_branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)
  wd=$(basename $PWD)

  expected_prompt="REPO:${MODIFIED}${repo}${RESET}:LOCALBRANCH:${UP_TO_DATE}${l_branch}${RESET}:WD:${UP_TO_DATE}${wd}${RESET}:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}

@test "a conflict should update the prompt" {
  skip
}
@test "local/upstream divergence (ahead) updates prompt" {
  skip
}
@test "local/upstream divergence (behind) updates prompt" {
  skip
}
@test "local/upstream divergence (both ahead and behind) updates prompt" {
  skip
}
# --------------------------------------------------
@test "wd style:basename shows only directory name" {
  # set things up
  helper__commit_to_new_repo "newfile" "some text"
  export GP_GIT_PROMPT="[\\pC]"

  # given set wd_style to show the basename of the cwd
  export GP_WD_STYLE="basename"


  # when we run the prompt
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT

  # then
  # - the working directory should be the folder name
  # - it should be coloured up-to-date

  wd=$(basename $PWD)

  expected_prompt="[${UP_TO_DATE}${wd}${RESET}]"
  echo -e "Expected: '$expected_prompt'" >&2
  echo -e "Output:   '$output'" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}
@test "wd style: cwd" {
  skip
}
@test "wd style: gitrelpath_exclusive" {
  skip
}
@test "wd style: gitrelpath_inclusive" {
  skip
}
