#!/bin/sh
# post-commit v1.0
#file for save to git-commit
file="git_commit.h"
file_def=`echo $file|tr . _`
echo "#ifndef $file_def" > $file
echo "#define $file_def" >> $file
echo "static const char *git_commit_str = \""`git log --pretty=format:"(%ad %h)" --abbrev-commit --date=short -1`"""\";" >> $file

echo "#endif" >> $file

# out messages for all OK.
echo "--- Git commit is written to $file ---"
echo "--- !!! NOW PLEASE RECOMPILE YOUR PROJECT TO TAKE EFFECT !!! ---"