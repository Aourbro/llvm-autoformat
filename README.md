# llvm-autoformat

# env
os: Ubuntu 18.04  
llvm version: 15.0.0

# usage
0. llvm should be installed and configured properly before running this  
1. MyHello folder should be put in "/llvm-project/llvm/lib/Transforms/", it contains a pass and a cmakelist  
2. add a single line "add_subdirectory(MyHello)" in file "/llvm-project/llvm/lib/Transforms/CMakeLists.txt"  
3. open a terminal at "/llvm-project/build/" and make, there should be a new file "LLVMMyHello.so" at "/llvm-project/build/lib/"  
4. use "opt -load \<somewhere\>/llvm-project/build/lib/LLVMMyHello.so -myhello \<original llvm bitcode\> \> \<output bitcode\> -enable-new-pm=0 -af-buffer=\<buf arg id\> -af-len=\<buf len arg id\> -af-parser=\<func name\> \[-af-struct=\<struct name\>\ -af-struct-off=\<offset in struct\>]" to run the pass  
5. use "clang \<output bitcode\> -o \<output exe file\>" to finish the compilation  
  
# update logs, 20220828  
add 2 args when running pass, -af-struct & -af-struct-off  
  this is because some server may pack their buffer, take "tiny" as an example  
add file tiny.c & autoformat.cpp  
  tiny.c is the server, the pass run correctly on this tiny server  
  autoformat.cpp has been changed, after running the pass, when the server received a message, the server will open a file called "af_log.txt" and print the logs into it  
