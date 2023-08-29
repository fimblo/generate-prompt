1..5
ok 1 that default prompt shows in a non-git dir 
ok 2 that default prompt respects GP_DEFAULT_PROMPT
ok 3 that creating an empty git repo returns default prompt
ok 4 that adding one file to repo returns default prompt
not ok 5 that committing a file to repo returns git prompt
# (in test file test/test.bats, line 100)
#   `[ "$output" =  "$expected_prompt" ]' failed
# Initialized empty Git repository in /private/var/folders/q5/m1kh7qld3w1c7b74khhxstqh0000gn/T/bats-run-M9a11s/test/5/tmp.yRp2uJ/.git/
# [main (root-commit) d675f7a] .
#  1 file changed, 0 insertions(+), 0 deletions(-)
#  create mode 100644 FOO
# REPO:[0;37mtmp.yRp2uJ[0m:BRANCH:[0;32mmain[0m:WD:[0;32mtmp.yRp2uJ[0m:
#
# REPO:[0;37mtmp.yRp2uJ[0m:BRANCH:[0;32mmain[0m:WD:[0;32mtmp.yRp2uJ[0m:
#
