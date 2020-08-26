#!/bin/bash

make BOARD=artemis_thing_plus clean > /dev/null
make BOARD=artemis_thing_plus V=1 2>&1 | grep "undefined\ reference" | tee unref.lst > /dev/null
cat unref.lst | awk '{split($0,ary,"`"); gsub("\047", "",ary[2]); print ary[2]}' | sort | uniq -c | tee unref_todo.txt
