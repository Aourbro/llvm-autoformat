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
