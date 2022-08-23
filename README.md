# llvm-autoformat

# env
os: Ubuntu 18.04  
llvm version: 15.0.0

# usage
0. llvm should be installed and configured properly before running this  
1. MyHello folder should be put in "/llvm-project/llvm/lib/Transforms/", it contains a pass and a cmakelist  
2. add a single line "add_subdirectory(MyHello)" in file "/llvm-project/llvm/lib/Transforms/CMakeLists.txt"  
3. open a terminal at "/llvm-project/build/" and make, there should be a new file "LLVMMyHello.so" at "/llvm-project/build/lib/"  
4. use "opt -load <somewhere>/llvm-project/build/lib/LLVMMyHello.so -myhello <original llvm bitcode> > <output bitcode> -enable-new-pm=0" to run the pass  
5. use "clang <output bitcode> -o <output exe file>" to finish the compilation  
  
# problems (sorted by importance, perhaps)
1. function call stack is not printed entirely, only the top of the stack will appear in log file, how to create new basic blocks in IR so I can insert a loop and print them all? Use a single string to store the call stack is too complex  
2. only GetElementPtr instruction will the log be printed, sometimes GetElementPtr appears inside other instructions, why? And how could I deal with this? printing the log before every instruction is OK but, really?  
3. the way to update the offset is shown in the code, I wonder if it could handle most cases properly  
4. how to propagate the annotation when a called function returns?  
5. what is the "union" when we met an "add" in the paper? (in part 3.1, "e.g. for the add operation, the result is the union of the operands if they are both marked")
  
# others
the file "test.c" is what I use to test the pass
