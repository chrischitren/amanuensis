gcc -Wpedantic -Wall -std=c89 -c ../monkdown/parse.c ../monkdown/write.c ../monkdown/nodes.c
gcc -Wpedantic -Wall -std=c89 -c ../src/pages.c ../src/fnodes.c
gcc -Wpedantic -Wall -std=c89 parse.o write.o pages.o fnodes.o nodes.o -o run
find . -name "*.o" -delete
