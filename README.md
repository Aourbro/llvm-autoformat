# llvm-autoformat

# env
os: Ubuntu 18.04  
llvm version: 15.0.0

# usage
0. llvm should be installed and configured properly before running this  
1. MyHello folder should be put in "/llvm-project/llvm/lib/Transforms/", it contains a pass and a cmakelist  
2. add a single line "add_subdirectory(MyHello)" in file "/llvm-project/llvm/lib/Transforms/CMakeLists.txt"  
3. open a terminal at "/llvm-project/build/" and make, there should be a new file "LLVMMyHello.so" at "/llvm-project/build/lib/"  
4. use "opt -load \<somewhere\>/llvm-project/build/lib/LLVMMyHello.so -myhello \<original llvm bitcode\> \> \<output bitcode\> -enable-new-pm=0 -af-buffer=\<buf arg id\> -af-len=\<buf len arg id\> -af-parser=\<func name\>" to run the pass  
5. use "clang \<output bitcode\> -o \<output exe file\>" to finish the compilation  
  
# others
the file "test.c" is what I use to test the pass
