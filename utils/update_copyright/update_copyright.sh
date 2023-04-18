#! /bin/bash

#[[
# @file update_copyright.sh
# @brief The script to update the copyright of the files.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2023-04-17
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

if test ! -d include -o ! -d src ; then
  echo "*** This script must be run from the top-level directory of SubrosaDG."
  exit 1
fi

YEAR=$(date +%Y)

FILES="
  CMakeLists.txt
  $(find -L ./include | grep -E '\.(h|hpp)$')
  $(find -L ./src | grep -E '\.(c|cpp)$')
  $(find -L ./examples | grep -E '\.(c|cpp|cfg)$')
  $(find -L ./tests | grep -E '\.(c|cpp)$')
  $(find -L ./cmake | grep -E '\.(cmake|in)$')
  $(find -L ./docs | grep -E '\.(tex)$')
  $(find -L ./utils | grep -E '\.(sh)$')
"
FILES=$(echo "$FILES" | xargs realpath | sort -u)

for FILE in $FILES ; do
  sed -i "s/\(.*Copyright (c) [0-9]\{4\} - \)$((YEAR-1))/\1$YEAR/g" "$FILE"
done
