#include "llvm/Pass.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

static cl::opt<int> BufferArg(
        "af-buffer-arg",
        cl::desc("allows we set break point at some place"),
        cl::init(-1));

static cl::opt<int> LenArg(
        "af-len",
        cl::desc("allows we set break point at some place"),
        cl::init(-1));

static cl::opt<int> StructOffset(
        "af-struct-off",
        cl::desc("allows we set break point at some place"),
        cl::init(-1));

static cl::opt<std::string> BufferName(
        "af-buffer-name",
        cl::desc("allows we set break point at some place"),
        cl::init(""));

static cl::opt<std::string> Struct(
        "af-struct",
        cl::desc("allows we set break point at some place"),
        cl::init(""));

static cl::opt<std::string> ParsingFunc(
        "af-parser",
        cl::desc("allows we set break point at some place"),
        cl::init(""));

namespace {
struct MyHello : public ModulePass {
  static char ID;
  MyHello() : ModulePass(ID) {}

  bool stop = false;

  bool runOnModule(Module &M) override {

    //Type *int32Type = Type::getInt32Ty(M.getContext());
    Type *int64Type = Type::getInt64Ty(M.getContext());
    Type *voidType = Type::getVoidTy(M.getContext());
    Type *ptr8Type = Type::getInt8PtrTy(M.getContext());

    const DataLayout *DL = &M.getDataLayout();

    FunctionType *BeginningFuncTy = FunctionType::get(voidType, {}, false);
    FunctionType *AddBaseFuncTy = FunctionType::get(voidType, {ptr8Type, ptr8Type, int64Type}, false);
    FunctionType *RemoveBaseFuncTy = FunctionType::get(voidType, {ptr8Type}, false);
    FunctionType *EndFuncTy = FunctionType::get(voidType, {}, false);
    FunctionType *LoggingFuncTy = FunctionType::get(voidType, {ptr8Type, int64Type}, false);
    FunctionType *PushingFuncTy = FunctionType::get(voidType, {int64Type}, false);
    FunctionType *PoppingFuncTy = FunctionType::get(voidType, {}, false);

    /// these functions should be implemented as a third-party lib and linked to the instrumented code
    /// @{
    /// static void *buf_base = 0;
    /// static unsigned buf_len = 0;
    /// static bool start_logging = 0;
    /// void autoformat_beginning(void* base, uint64_t buf_len) {
    ///     buf_base = base; buf_len = len;
    ///     start_logging = 1;
    /// }
    static const char *BeginningFunction = "autoformat_beginning";
    static Function *BeginningFunc = (Function*)M.getOrInsertFunction(BeginningFunction, BeginningFuncTy).getCallee();

    /// void autoformat_add_base() {
    ///     buf_base = 0; buf_len = 0;
    ///     start_logging = 0;
    /// }
    static const char *AddBaseFunction = "autoformat_add_base";
    static Function *AddBaseFunc = (Function*)M.getOrInsertFunction(AddBaseFunction, AddBaseFuncTy).getCallee();

    /// void autoformat_end() {
    ///     buf_base = 0; buf_len = 0;
    ///     start_logging = 0;
    /// }
    static const char *RemoveBaseFunction = "autoformat_remove_base";
    static Function *RemoveBaseFunc = (Function*)M.getOrInsertFunction(RemoveBaseFunction, RemoveBaseFuncTy).getCallee();

    /// void autoformat_end() {
    ///     buf_base = 0; buf_len = 0;
    ///     start_logging = 0;
    /// }
    static const char *EndFunction = "autoformat_end";
    static Function *EndFunc = (Function*)M.getOrInsertFunction(EndFunction, EndFuncTy).getCallee();

    /// void autoformat_logging(void* ptr, uint64_t nbytes) {
    ///    for (unsigned x = 0; x < nbytes; ++x)
    ///     if (start_logging && ptr + x - buf_base >= 0 && ptr + x - buf_base < buf_len) {
    ///          // logging the offset and the global call stack
    ///      }
    /// }
    static const char *LoggingFunction = "autoformat_logging";
    static Function *LoggingFunc = (Function*)M.getOrInsertFunction(LoggingFunction, LoggingFuncTy).getCallee();

    /// uint64_t *call_stack = 0;
    /// uint64_t call_stack_len = 0;
    /// uint64_t call_stack_ptr = 0;
    /// void autoformat_pushing(uint64_t func_id) {
    ///    if (!start_logging) return;
    ///    if (call_stack == 0) {
    ///         call_stack = malloc(1000); call_stack_len = 1000;
    ///     } else if (call_stack_ptr >= call_stack_len) {
    ///         call_stack = realloc(call_stack_len + 500); call_stack_len += 500;
    ///     }
    ///     call_stack[call_stack_ptr++] = func_id;
    /// }
    static const char *PushingFunction = "autoformat_pushing";
    static Function *PushingFunc = (Function*)M.getOrInsertFunction(PushingFunction, PushingFuncTy).getCallee();

    /// void autoformat_popping() {
    ///      if (!start_logging) return;
    ///      assert(call_stack_ptr > 0);
    ///      call_stack_ptr--;
    ///      if (call_stack_ptr == 0) start_logging = 0; // this means we exit the entry func
    /// }
    static const char *PoppingFunction = "autoformat_popping";
    static Function *PoppingFunc = (Function*)M.getOrInsertFunction(PoppingFunction, PoppingFuncTy).getCallee();
    /// @}

    for(Module::iterator I = M.begin(); I != M.end(); I++){
      Function &F = *I;

      if(F.isDeclaration()) continue;

      errs() << "MyHello: instrumenting " << F.getName() << "\n";

      for(Function::iterator I = F.begin(); I != F.end(); I++){
        BasicBlock &BB = *I;
        for(BasicBlock::iterator I = BB.begin(); I != BB.end(); I++){
          Instruction &Inst = *I;
          IRBuilder<> builder(&Inst);
          std::vector<Value*> MCpydests;

          Inst.dump();

          // call inst, push & pop & log
          CallInst *callInst = dyn_cast<CallInst>(&Inst);
          if(callInst){
            builder.SetInsertPoint(callInst);
            builder.CreateCall(PushingFuncTy, PushingFunc, {ConstantInt::getSigned(int64Type, (int64_t) callInst)});
            if(callInst->getCalledFunction() && (
               callInst->getCalledFunction()->getIntrinsicID() == Intrinsic::memcpy_element_unordered_atomic
            || callInst->getCalledFunction()->getIntrinsicID() == Intrinsic::memcpy
            )){
              Value *dest = callInst->getArgOperand(0);
              dest = builder.CreateBitOrPointerCast(dest, ptr8Type, "castto8");
              Value *src = callInst->getArgOperand(1);
              src = builder.CreateBitOrPointerCast(src, ptr8Type, "castto8");
              Value *size = callInst->getArgOperand(2);
              assert(size->getType()->isIntegerTy());
              if(!size->getType()->isIntegerTy(64)) size = builder.CreateZExtOrBitCast(size, int64Type, "castto64");
              builder.CreateCall(LoggingFuncTy, LoggingFunc, {src, size});
              builder.CreateCall(AddBaseFuncTy, AddBaseFunc, {dest, src, size});
              MCpydests.push_back(dest);
            }
            builder.SetInsertPoint(callInst->getNextNode());
            builder.CreateCall(PoppingFuncTy, PoppingFunc, {});
            I++;
          }

          // load inst, log
          LoadInst *loadInst = dyn_cast<LoadInst>(&Inst);
          if(loadInst && loadInst->getType()->isIntegerTy()){
            builder.SetInsertPoint(loadInst);
            Value *ptr = builder.CreateBitOrPointerCast(loadInst->getPointerOperand(), ptr8Type, "castto8");
            builder.CreateCall(LoggingFuncTy, LoggingFunc, {ptr, ConstantInt::getSigned(int64Type, DL->getTypeStoreSize(loadInst->getType()))});
          }

          // return isnt, free some space
          ReturnInst *returnInst = dyn_cast<ReturnInst>(&Inst);
          if(returnInst){
            while(!MCpydests.empty()){
              Value *dest = MCpydests.back();
              MCpydests.pop_back();
              builder.CreateCall(RemoveBaseFuncTy, RemoveBaseFunc, {dest});
            }
          }
        }
      }

      if(F.getName().equals(ParsingFunc.getValue())){
        // insert start function
        int BufArgId = BufferArg.getValue();
        int LenArgId = LenArg.getValue();
        int StructOff = StructOffset.getValue();
        BasicBlock &main_begin_bb = *F.begin();
        Instruction &main_begin_inst = *main_begin_bb.begin();
        IRBuilder<> builder(&main_begin_inst);
        builder.SetInsertPoint(&main_begin_inst);
        builder.CreateCall(BeginningFuncTy, BeginningFunc, {});

        if(BufArgId >= 0){
          assert(LenArgId >= 0);
          Value *BufferPtr;
          if(StructOff == -1)
            BufferPtr = builder.CreateBitOrPointerCast(F.getArg(BufArgId), ptr8Type, "castto8");
          else{
            assert(!Struct.getValue().empty());
            BufferPtr = builder.CreateStructGEP(StructType::getTypeByName(M.getContext(), "struct." + Struct.getValue()), F.getArg(BufArgId), StructOff, "gep");
          }
          Value *BufferLen = builder.CreateZExtOrBitCast(F.getArg(LenArgId), int64Type, "castto64");
          builder.CreateCall(AddBaseFuncTy, AddBaseFunc, {BufferPtr, BufferLen});
        }

        for(Function::iterator I = F.begin(); I != F.end(); I++){
          BasicBlock &BB = *I;
          for(BasicBlock::iterator I = BB.begin(); I != BB.end(); I++){
            Instruction &Inst = *I;

            Inst.dump();

            AllocaInst *allocaInst = dyn_cast<AllocaInst>(&Inst);
            if(allocaInst && BufferArg == -1){
              builder.SetInsertPoint(allocaInst->getNextNode());
              if(allocaInst->getName().equals(BufferName)){
                Value *BufferLen = ConstantInt::get(int64Type, dyn_cast<ArrayType>(allocaInst->getAllocatedType())->getNumElements());
                Value *BufferPtr = builder.CreateBitOrPointerCast(allocaInst, ptr8Type, "castto8");
                builder.CreateCall(AddBaseFuncTy, AddBaseFunc, {BufferPtr, BufferPtr, BufferLen});
              }

              if(allocaInst->getAllocatedType()->isStructTy() && allocaInst->getAllocatedType()->getStructName().equals("struct." + Struct.getValue())){
                assert(LenArgId > 0);
                Value *BufferLen = ConstantInt::get(int64Type, LenArgId);
                Value *BufferPtr = builder.CreateStructGEP(StructType::getTypeByName(M.getContext(), "struct." + Struct.getValue()), allocaInst, StructOff, "gep");
                builder.CreateCall(AddBaseFuncTy, AddBaseFunc, {BufferPtr, BufferPtr, BufferLen});
              }
            }

            ReturnInst *returnInst = dyn_cast<ReturnInst>(&Inst);
            if(returnInst){
              builder.SetInsertPoint(returnInst);
              builder.CreateCall(EndFuncTy, EndFunc, {});
            }
          }
        }
      }
    }

    return false;
  }
}; // end of struct Hello
}  // end of anonymous namespace

char MyHello::ID = 0;
static RegisterPass<MyHello> X("myhello", "My Hello World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

static RegisterStandardPasses Y(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM) { PM.add(new MyHello()); });
