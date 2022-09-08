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
    FunctionType *AddBaseFuncTy = FunctionType::get(voidType, {ptr8Type, int64Type, int64Type}, false);
    FunctionType *RemoveBaseFuncTy = FunctionType::get(voidType, {ptr8Type}, false);
    FunctionType *EndFuncTy = FunctionType::get(voidType, {}, false);
    FunctionType *LoggingFuncTy = FunctionType::get(voidType, {ptr8Type, int64Type}, false);
    FunctionType *PushingFuncTy = FunctionType::get(voidType, {int64Type}, false);
    FunctionType *PoppingFuncTy = FunctionType::get(voidType, {}, false);
    FunctionType *MemcpyFuncTy = FunctionType::get(voidType, {ptr8Type, ptr8Type, int64Type}, false);
    FunctionType *SscanfFuncTy = FunctionType::get(voidType, {ptr8Type, ptr8Type, }, true);
    FunctionType *StrcmpFuncTy = FunctionType::get(voidType, {ptr8Type, ptr8Type}, false);
    FunctionType *StrcpyFuncTy = FunctionType::get(voidType, {ptr8Type, ptr8Type}, false);

    static const char *BeginningFunction = "autoformat_beginning";
    static Function *BeginningFunc = (Function*)M.getOrInsertFunction(BeginningFunction, BeginningFuncTy).getCallee();

    static const char *AddBaseFunction = "autoformat_add_base";
    static Function *AddBaseFunc = (Function*)M.getOrInsertFunction(AddBaseFunction, AddBaseFuncTy).getCallee();

    static const char *RemoveBaseFunction = "autoformat_remove_base";
    static Function *RemoveBaseFunc = (Function*)M.getOrInsertFunction(RemoveBaseFunction, RemoveBaseFuncTy).getCallee();

    static const char *EndFunction = "autoformat_end";
    static Function *EndFunc = (Function*)M.getOrInsertFunction(EndFunction, EndFuncTy).getCallee();

    static const char *LoggingFunction = "autoformat_logging";
    static Function *LoggingFunc = (Function*)M.getOrInsertFunction(LoggingFunction, LoggingFuncTy).getCallee();

    static const char *PushingFunction = "autoformat_pushing";
    static Function *PushingFunc = (Function*)M.getOrInsertFunction(PushingFunction, PushingFuncTy).getCallee();

    static const char *PoppingFunction = "autoformat_popping";
    static Function *PoppingFunc = (Function*)M.getOrInsertFunction(PoppingFunction, PoppingFuncTy).getCallee();

    static const char *MemcpyFunction = "autoformat_memcpy";
    static Function *MemcpyFunc = (Function*)M.getOrInsertFunction(MemcpyFunction, MemcpyFuncTy).getCallee();

    static const char *SscanfFunction = "autoformat_sscanf";
    static Function *SscanfFunc = (Function*)M.getOrInsertFunction(SscanfFunction, SscanfFuncTy).getCallee();

    static const char *StrcmpFunction = "autoformat_strcmp";
    static Function *StrcmpFunc = (Function*)M.getOrInsertFunction(StrcmpFunction, StrcmpFuncTy).getCallee();

    static const char *StrcpyFunction = "autoformat_strcpy";
    static Function *StrcpyFunc = (Function*)M.getOrInsertFunction(StrcpyFunction, StrcpyFuncTy).getCallee();

    for(Module::iterator I = M.begin(); I != M.end(); I++){
      Function &F = *I;

      if(F.isDeclaration()) continue;

      errs() << "MyHello: instrumenting " << F.getName() << "\n";

      std::vector<Value*> MCpydests;
      std::vector<LoadInst*> loadInsts;

      for(Function::iterator I = F.begin(); I != F.end(); I++){
        BasicBlock &BB = *I;
        for(BasicBlock::iterator I = BB.begin(); I != BB.end(); I++){
          Instruction &Inst = *I;
          IRBuilder<> builder(&Inst);

          Inst.dump();

          // call inst, push & pop & log
          CallInst *callInst = dyn_cast<CallInst>(&Inst);
          if(callInst){
            builder.SetInsertPoint(callInst);
            builder.CreateCall(PushingFuncTy, PushingFunc, {ConstantInt::getSigned(int64Type, (int64_t) callInst)});

            // memcpy
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
              builder.CreateCall(MemcpyFuncTy, MemcpyFunc, {dest, src, size});
              MCpydests.push_back(dest);
            }

            // sscanf
            if(callInst->getCalledFunction() && callInst->getCalledFunction()->getName().equals("__isoc99_sscanf")){
              builder.SetInsertPoint(callInst->getNextNode());
              int arg_num = callInst->arg_size();
              std::vector<Value *> args;
              for(int i = 0; i < arg_num; ++i) args.push_back(callInst->getArgOperand(i));
              for(int i = 2; i < arg_num; ++i) MCpydests.push_back(callInst->getArgOperand(i));
              builder.CreateCall(SscanfFuncTy, SscanfFunc, args);
              I++;
            }

            // strcmp
            if(callInst->getCalledFunction() && (
               callInst->getCalledFunction()->getName().equals("strcmp")
            || callInst->getCalledFunction()->getName().equals("strcasecmp")
            || callInst->getCalledFunction()->getName().equals("strstr")
            || callInst->getCalledFunction()->getName().equals("index"))){
              Value *str1 = callInst->getOperand(0);
              Value *str2 = callInst->getOperand(1);
              str2 = builder.CreateBitOrPointerCast(str2, ptr8Type, "castto8");
              builder.CreateCall(StrcmpFuncTy, StrcmpFunc, {str1, str2});
            }

            // strcpy
            if(callInst->getCalledFunction() && callInst->getCalledFunction()->getName().equals("strcpy")){
              Value *dest = callInst->getOperand(0);
              Value *src = callInst->getOperand(1);
              builder.CreateCall(StrcpyFuncTy, StrcpyFunc, {dest, src});
            }

            // strcat
            if(callInst->getCalledFunction() && callInst->getCalledFunction()->getName().equals("strcat")){
              Value *src = callInst->getOperand(1);
              builder.CreateCall(StrcpyFuncTy, StrcpyFunc, {src, src});
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
            loadInsts.push_back(loadInst);
          }

          // store inst, similar to memcpy
          StoreInst *storeInst = dyn_cast<StoreInst>(&Inst);
          if(storeInst){
            for(uint i = 0; i < loadInsts.size(); ++i){
              if(storeInst->getValueOperand()->getType()->isIntegerTy() && !storeInst->getValueOperand()->getNameOrAsOperand().compare(loadInsts[i]->getNameOrAsOperand())){
                builder.SetInsertPoint(storeInst);
                Value *dest = storeInst->getPointerOperand();
                dest = builder.CreateBitOrPointerCast(dest, ptr8Type, "castto8");
                Value *src = builder.CreateBitOrPointerCast(loadInsts[i]->getPointerOperand(), ptr8Type, "castto8");
                builder.CreateCall(MemcpyFuncTy, MemcpyFunc, {dest, src, ConstantInt::get(int64Type, 1)});
                MCpydests.push_back(dest);
              }
            }
          }

          // return isnt, free some space
          ReturnInst *returnInst = dyn_cast<ReturnInst>(&Inst);
          if(returnInst){
            while(!MCpydests.empty()){
              Value *dest = MCpydests.back();
              dest = builder.CreateBitOrPointerCast(dest, ptr8Type, "castto8");
              MCpydests.pop_back();
              //builder.CreateCall(RemoveBaseFuncTy, RemoveBaseFunc, {dest});
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
          builder.CreateCall(AddBaseFuncTy, AddBaseFunc, {BufferPtr, ConstantInt::get(int64Type, 0), BufferLen});
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
                builder.CreateCall(AddBaseFuncTy, AddBaseFunc, {BufferPtr, ConstantInt::get(int64Type, 0), BufferLen});
              }

              if(allocaInst->getAllocatedType()->isStructTy() && allocaInst->getAllocatedType()->getStructName().equals("struct." + Struct.getValue())){
                assert(LenArgId > 0);
                Value *BufferLen = ConstantInt::get(int64Type, LenArgId);
                Value *BufferPtr = builder.CreateStructGEP(StructType::getTypeByName(M.getContext(), "struct." + Struct.getValue()), allocaInst, StructOff, "gep");
                builder.CreateCall(AddBaseFuncTy, AddBaseFunc, {BufferPtr, ConstantInt::get(int64Type, 0), BufferLen});
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

      MCpydests.clear();
      loadInsts.clear();
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
