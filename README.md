# llvm-autoformat

# env
os: Ubuntu 18.04  
llvm version: 15.0.0

# usage
0. llvm should be installed and configured properly before running this  
1. MyHello folder should be put in "/llvm-project/llvm/lib/Transforms/", it contains a pass and a cmakelist  
2. add a single line "add_subdirectory(MyHello)" in file "/llvm-project/llvm/lib/Transforms/CMakeLists.txt"  
3. open a terminal at "/llvm-project/build/" and make, there should be a new file "LLVMMyHello.so" at "/llvm-project/build/lib/"  
4. use "opt -load \<somewhere\>/llvm-project/build/lib/LLVMMyHello.so -myhello \<original llvm bitcode\> \> \<output bitcode\> -enable-new-pm=0 \[-af-buffer=\<buf arg id\> -af-len=\<buf len arg id\>\] -af-parser=\<func name\> \[-af-struct=\<struct name\>\ -af-struct-off=\<offset in struct\>]" to run the pass  
5. use "clang \<output bitcode\> -o \<output exe file\>" to finish the compilation  
6. after running the pass and start the server and received a package, and there should be a file called "af_log.txt", then, compile and run the "./ifentifier af_log.txt", it will dump the field tree on the terminal  
  
# update logs, 20221008  
extended the pass, the header and the identifier for binary protocols  
added function "autoformat_memcmp", "autoformat_memchr" to deal with "memcmp", "memchr" and "strchr"  
-they were found in babeld  
  
# update logs, 20220918  
finished algorithm 2(parallel field identifier)  
-there is a problem while handling the situation that the parallel fields are not neighbors  
  
# update logs, 20220916  
the field tree could be refined properly  
-tokenization and deletion of redundant nodes  
  
# update logs, 20220910  
implemented the algorithm 1 mentioned in the paper, it could generate the field tree with a single log file  
-there is no order between the siblings in the tree, but it doesn't matter and could be modified later  
-the pre-process program will be integrated into this identifier later  
-by the way, I prefer c++
 
# update logs, 20220908  
update the method to deal with "strcmp", "strcasecmp", "strstr", "index", "strcpy", "strcat"
add function "autoformat_strcmp", "autoformat_strcpy"
-logically, the algorithm could now recognize and separate the request line of http protocol, e.g., packet(request_line(method(chars), filename(chars), CHARS), request_header0(chars), request_header1(chars), ...), notice that CHARS is http version but "tiny" server never use it, so there won't be a node for it
  
# update logs, 20220904  
update the method to deal with "memcpy"  
-seperate "autoformat_add_base" function into two("autoformat_add_base" & "autoformat_memcpy"), making the logic of "autoformat.cpp" more clearly  
add function "autoformat_sscanf"  
-now the pass will insert a function call before "sscanf" invocation since this function will also "copy" some chars to another location  
logically, the algorithm mentioned in the paper(not implemented yet) could now recognize the request line of http protocol(generate this node on the field tree) aiming at "tiny" server  
  
# update logs, 20220901  
fixed the bug mentioned before, now it could handle "memcpy" properly  
  
# update logs, 20220831
NOTICE: it's still buggy, there are much more redundant lines in the log file  
-this is because some parser function does not have a buffer as an argument, e.g. in "tiny" server, "doit" is the main parser function, it only has one single argument which is indeed a file pointer, and inside this function, there is a struct used to store the message called rio  
-and I'm also trying to handle memcpy properly, e.g. if the program memcpy a src which is one of the buffer base to a dest, then we should add the dest address to the base set  
-there are other functions I need to handle like "sscanf"
  
# update logs, 20220828  
add 2 args when running pass, -af-struct & -af-struct-off  
-this is because some server may pack their buffer, take "tiny" as an example  
add file tiny.c & autoformat.cpp  
-tiny.c is the server, the pass run correctly on this tiny server  
-autoformat.cpp has been changed, after running the pass, when the server received a message, the server will open a file called "af_log.txt" and print the logs into it(this file should be removed before running the pass for the next turn)  
