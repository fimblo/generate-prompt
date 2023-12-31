#!/usr/bin/env bats  # -*- mode: shell-script -*-
bats_require_minimum_version 1.5.0

# To run a test manually:
# cd path/to/project/root
# bats test/test.bats


helper__set_git_config() {
  # Set repo-local git config
  git config user.email "my@test.com"
  git config user.name "Test Person"
  git config pull.rebase true
}

helper__new_repo() {
  git init --initial-branch=main
  helper__set_git_config
}

helper__new_repo_and_add_file() {
  file_to_commit="$1"
  content_to_commit="$2"

  helper__new_repo
  echo "$content_to_commit" > $file_to_commit
  git add $file_to_commit
}

helper__new_repo_and_commit() {
  file_to_commit="$1"
  content_to_commit="$2"

  helper__new_repo_and_add_file "$file_to_commit" "$content_to_commit"
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
  unset GP_CONFLICT
  unset GP_NO_DATA
  unset GP_RESET

  # prompt patterns
  unset GP_DEFAULT_PROMPT
  export GP_GIT_PROMPT='REPO:\pR:LOCALBRANCH:\pL:WD:\pC:'

  # styles
  unset GP_WD_STYLE
  unset GP_CONFLICT_STYLE
  unset GP_REBASE_STYLE
  unset GP_A_DIVERGENCE_STYLE
  unset GP_B_DIVERGENCE_STYLE
  unset GP_AB_DIVERGENCE_STYLE


  # colour codes used by all tests
  UP_TO_DATE="\[\033[0;32m\]"
  MODIFIED="\[\033[0;33m\]"
  CONFLICT="\[\033[0;31m\]"
  NO_DATA="\[\033[0;37m\]"
  RESET="\[\033[0m\]"

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
  helper__new_repo

  # when we run the prompt
  run -${EXIT_NO_LOCAL_REF} $GENERATE_PROMPT

  # then it should behave as in a normal non-git repo
  [ "$output" =  "\\W $ " ]
}


# --------------------------------------------------
@test "adding file to git repo doesn't alter prompt" {
  # given we create a repo, add a file but don't commit
  helper__new_repo_and_add_file "newfile" "some text"

  
  # when we run the prompt
  run -${EXIT_NO_LOCAL_REF} $GENERATE_PROMPT

  # then we should get the default non-git prompt
  [ "$output" =  "\\W $ " ]
}


# --------------------------------------------------
@test "committing in empty git repo updates prompt" {
  # given we create a repo and commit a file
  helper__new_repo_and_commit "newfile" "some text"

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
  helper__new_repo_and_commit "newfile" "some text"
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
  helper__new_repo_and_commit "newfile" "some text"

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
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo
  helper__set_git_config


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
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo
  helper__set_git_config


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


# --------------------------------------------------
@test "prompt is updated if local is behind upstream" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it to anotherLocation/myRepo
  mkdir anotherLocation
  cd anotherLocation
  git clone ../myRepo
  cd myRepo
  helper__set_git_config
  cd ../..

  # given we commit a change in the first repo 
  cd myRepo
  echo "new text" > newfile
  git add newfile
  git commit -m 'update the file with "new text"'
  cd -

  # given we git fetch in anotherLocation/myRepo
  cd anotherLocation/myRepo
  git fetch

  # when we run the prompt
  export GP_GIT_PROMPT="REPO:\\pR:BEHIND:\\pb:"
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT


  # then we should get a git prompt where
  # - the repo field should be MODIFIED and
  # - behind is 1
  
  repo=myRepo
  l_branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)

  expected_prompt="REPO:${MODIFIED}${repo}${RESET}:BEHIND:1:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "prompt is updated if local is ahead of upstream" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it to anotherLocation/myRepo
  mkdir anotherLocation
  cd anotherLocation
  git clone ../myRepo
  cd myRepo
  helper__set_git_config
  cd ../..

  # given we commit a change in anotherLocation/myRepo
  cd anotherLocation/myRepo
  echo "new text" > newfile
  git add newfile
  git commit -m 'update the file with "new text"'


  # when we run the prompt
  export GP_GIT_PROMPT="REPO:\\pR:AHEAD:\\pa:"
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT


  # then we should get a git prompt where
  # - the repo field should be MODIFIED and
  # - ahead is 1

  repo=myRepo
  l_branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)

  expected_prompt="REPO:${MODIFIED}${repo}${RESET}:AHEAD:1:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "prompt is updated when local is both ahead and behind upstream" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it to anotherLocation/myRepo
  mkdir anotherLocation
  cd anotherLocation
  git clone ../myRepo
  cd myRepo
  helper__set_git_config
  cd ../..

  # given we commit a change in the first repo 
  cd myRepo
  echo "new text" > newfile
  git add newfile
  git commit -m 'update the file with "new text"'
  cd -

  # given we commit another change in anotherLocation/myRepo
  cd anotherLocation/myRepo
  echo "new text2" > otherfile
  git add otherfile
  git commit -m 'update a file with "new text2"'
  cd -

  # given we git fetch in anotherLocation/myRepo
  cd anotherLocation/myRepo
  git fetch

  # when we run the prompt
  export GP_GIT_PROMPT="REPO:\\pR:AHEAD_AND_BEHIND:\\pd:"
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT


  # then we should get a git prompt where
  # - the repo field should be MODIFIED and
  # - behind is 1
  
  repo=myRepo
  l_branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)

  expected_prompt="REPO:${MODIFIED}${repo}${RESET}:AHEAD_AND_BEHIND:(1,-1):"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "a conflict should update the prompt" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it to a new location
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo
  helper__set_git_config
  cd ../..

  # given we commit a change in the first repo 
  cd myRepo
  echo "new text" > newfile
  git add newfile
  git commit -m 'update the file with "new text"'
  cd -

  # given we commit a change to the same file in the second repo
  cd tmp/myRepo
  echo "different message" > newfile
  git add newfile
  git commit -m 'update the file with "different message"'

  # given we pull changes from upstream
  git pull || true


  # when we run the prompt
  export GP_GIT_PROMPT="REPO:\\pR:CONFLICT:\\pK:"
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT


  # then we should get a git prompt
  # where the repo field should be uncoloured and \pK should contain
  # "(conflict: 1)"
  repo=myRepo
  l_branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)

  expected_prompt="REPO:${CONFLICT}${repo}${RESET}:CONFLICT:${CONFLICT}(conflict: 1)${RESET}:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "an add after conflict should update the prompt" {
  # given we have a git repo
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  cd -

  # given we clone it to a new location
  mkdir tmp
  cd tmp
  git clone ../myRepo
  cd myRepo
  helper__set_git_config
  cd ../..

  # given we commit a change in the first repo 
  cd myRepo
  echo "new text" > newfile
  git add newfile
  git commit -m 'update the file with "new text"'
  cd -

  # given we commit a change to the same file in the second repo
  cd tmp/myRepo
  echo "different message" > newfile
  git add newfile
  git commit -m 'update the file with "different message"'

  # given we pull changes from upstream
  git pull || true

  # given we fix the file and add it
  echo "different message" > newfile
  git add newfile

  # when we run the prompt
  export GP_GIT_PROMPT="REPO:\\pR:CONFLICT:\\pK:REBASE:\\pi"
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT


  # then we should get a git prompt
  # where the repo field should be uncoloured and \pK should expand to
  # the empty string.
  repo=myRepo
  l_branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)

  expected_prompt="REPO:${NO_DATA}${repo}${RESET}:CONFLICT::REBASE:(interactive rebase)"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "wd style:basename shows only directory name" {
  # set things up
  helper__new_repo_and_commit "newfile" "some text"
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


# --------------------------------------------------
@test "interactive rebase updates prompt" {
  # given we have a git repo which we have some commits on
  mkdir myRepo
  cd myRepo
  helper__new_repo_and_commit "newfile" "some text"
  echo "hello" > newfile
  git commit -a -m '2nd commit'
  echo "bye" > newfile
  git commit -a -m '3rd commit'
  cd -

  # given we enter an interactive rebase session
  cd myRepo
  cat<<-EOF>edit.sh
	#!/bin/bash
	sed -i'bak' 's/^pick/edit/' \$1
	EOF
  chmod 755 edit.sh
  cat edit.sh >&2
  GIT_SEQUENCE_EDITOR=./edit.sh git rebase -i HEAD^1
  
  # then
  # - the repo should go into NO_DATA state
  # - \\pi

  # when we run the prompt
  export GP_GIT_PROMPT="REPO:\\pR:REBASE:\\pi:"
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT


  # then we should get a git prompt
  # where the repo field should be uncoloured and \pK should expand to
  # the empty string.
  repo=myRepo
  l_branch=$(cat .git/HEAD | tr '/' ' ' | cut -d\   -f 4)

  expected_prompt="REPO:${NO_DATA}${repo}${RESET}:REBASE:(interactive rebase):"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" =  "$evaluated_prompt" ]
}

# --------------------------------------------------
@test "prompt symbol is $ if user is not root" {
  # given we have a git repo
  helper__new_repo_and_commit "newfile" "some text"

  # when we run the prompt as a non-root user
  export GP_GIT_PROMPT="REPO:\\pP:"
  export USER="nonroot"
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT

  # then the prompt symbol should be $
  expected_prompt="REPO:${UP_TO_DATE}\$${RESET}:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" = "$evaluated_prompt" ]
}

# --------------------------------------------------
@test "prompt symbol is # if user is root" {
  # given we have a git repo
  helper__new_repo_and_commit "newfile" "some text"

  # when we run the prompt as a root user
  export GP_GIT_PROMPT="REPO:\\pP:"
  export USER="root"
  run -${EXIT_GIT_PROMPT} $GENERATE_PROMPT

  # then the prompt symbol should be $
  expected_prompt="REPO:${UP_TO_DATE}#${RESET}:"
  echo -e "Expected: $expected_prompt" >&2
  echo -e "Output:   $output" >&2

  evaluated_prompt=$(echo -e $expected_prompt)
  [ "$output" = "$evaluated_prompt" ]
}


# --------------------------------------------------
@test "wd style: cwd inside of \$HOME" {
  # will write later
  skip
}


# --------------------------------------------------
@test "wd style: cwd outside of \$HOME" {
  # will write later
  skip
}


# --------------------------------------------------
@test "wd style: gitrelpath_exclusive" {
  # will write later
  skip
}


# --------------------------------------------------
@test "wd style: gitrelpath_inclusive" {
  # will write later
  skip
}

# --------------------------------------------------
@test "usage: -h prints and exits 0" {
  # will write later
  skip
}

# --------------------------------------------------
@test "usage: -H prints and exits 0" {
  # will write later
  skip
}

# --------------------------------------------------
@test "usage: non-supported flag prints and exits 1" {
  # will write later
  skip
}
