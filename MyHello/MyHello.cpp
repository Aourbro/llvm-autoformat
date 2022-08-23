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

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

namespace {
struct MyHello : public ModulePass {
  static char ID;
  MyHello() : ModulePass(ID) {}

  bool stop = false;

  //TODO:
  //  set these two var to args
  std::string FUNCNAME = "prt";
  std::string VARNAME = "buf.addr";

  bool runOnModule(Module &M) override {

    // some types that we need
    Type *intType = Type::getInt32Ty(M.getContext());
    Type *ptr64Type = Type::getInt64PtrTy(M.getContext());
    Type *arrType = ArrayType::get(ptr64Type, 1024);
    std::vector<Type *> printfArgsType({Type::getInt8PtrTy(M.getContext())});
    FunctionType *openType = FunctionType::get(ptr64Type, printfArgsType, true);
    FunctionType *printfType = FunctionType::get(intType, printfArgsType, true);
    FunctionCallee fopenFunc = M.getOrInsertFunction("fopen", openType);
    FunctionCallee fprintfFunc = M.getOrInsertFunction("fprintf", printfType);

    // file pointer of log.txt
    Constant *fp = M.getOrInsertGlobal("__no_repeat_fp__", ptr64Type);
    GlobalVariable *gv_fp = M.getNamedGlobal("__no_repeat_fp__");
    gv_fp->setDSOLocal(true);
    gv_fp->setInitializer(Constant::getNullValue(ptr64Type));
    gv_fp->setAlignment(MaybeAlign(8));

    // offset in the message
    Constant *offset = M.getOrInsertGlobal("__no_repeat_offset__", intType);
    GlobalVariable *gv_of = M.getNamedGlobal("__no_repeat_offset__");
    gv_of->setDSOLocal(true);
    gv_of->setInitializer(ConstantInt::get(intType, 0));
    gv_of->setAlignment(MaybeAlign(4));

    // shadow call stack
    Constant *call_stack = M.getOrInsertGlobal("__no_repeat_stack__", arrType);
    GlobalVariable *gv_cs = M.getNamedGlobal("__no_repeat_stack__");
    gv_cs->setDSOLocal(true);
    gv_cs->setInitializer(ConstantAggregateZero::get(arrType));
    gv_cs->setAlignment(MaybeAlign(16));

    // stack pointer of the call stack
    Constant *stack_pointer = M.getOrInsertGlobal("__no_repeat_pointer__", intType);
    GlobalVariable *gv_sp = M.getNamedGlobal("__no_repeat_pointer__");
    gv_sp->setDSOLocal(true);
    gv_sp->setInitializer(ConstantInt::get(intType, 0));
    gv_sp->setAlignment(MaybeAlign(4));

    for(Module::iterator I = M.begin(); I != M.end(); I++){
      Function &F = *I;

      // create and open log.txt
      if(F.getName().equals("main")){
        BasicBlock &main_begin_bb = *F.begin();
        Instruction &main_begin_inst = *main_begin_bb.begin();
        IRBuilder<> builder(&main_begin_inst);
        builder.SetInsertPoint(&main_begin_inst);
        Value *fopen_params[] = {
          builder.CreateGlobalStringPtr("log.txt", "fopenfile"),
          builder.CreateGlobalStringPtr("w+", "fopenmode")
        };
        CallInst *fopenfd = builder.CreateCall(fopenFunc, fopen_params, "fopenlog");
        builder.CreateStore(fopenfd, fp);
      }

      // push current function in stack
      if(!F.getBasicBlockList().empty()){
        BasicBlock &main_begin_bb = *F.begin();
        Instruction &main_begin_inst = *main_begin_bb.begin();
        IRBuilder<> builder(&main_begin_inst);
        builder.SetInsertPoint(&main_begin_inst);
        Value *load = builder.CreateLoad(intType, stack_pointer, "load");
        Value *add = builder.CreateAdd(load, ConstantInt::get(intType, 1));
        Constant *funcname = builder.CreateGlobalStringPtr(F.getName(), "funcname");
        builder.CreateStore(funcname, builder.CreateGEP(arrType, call_stack, {ConstantInt::get(intType, 0), add}, "gep", true));
        builder.CreateStore(add, stack_pointer);
      }

      for(Function::iterator I = F.begin(); I != F.end(); I++){
        BasicBlock &BB = *I;
        for(BasicBlock::iterator I = BB.begin(); I != BB.end(); I++){
          Instruction &Inst = *I;
          IRBuilder<> builder(&Inst);
          builder.SetInsertPoint(&Inst);

          Inst.dump();

          // concat function name and var name as the mark's name, var name is also the inst name if this inst creates a new var
          std::string new_var = F.getName().str() + Inst.getNameOrAsOperand();

          // alloca inst, if target var alloced here, mark it (1 means marked, 0 unmarked)
          AllocaInst *allocaInst = dyn_cast<AllocaInst>(&Inst);
          if(allocaInst){
            M.getOrInsertGlobal(new_var, intType);
            GlobalVariable *gv = M.getNamedGlobal(new_var);
            gv->setDSOLocal(true);
            gv->setInitializer(ConstantInt::get(intType, 0));
            gv->setAlignment(MaybeAlign(4));
            if(F.getName().equals(FUNCNAME) && !allocaInst->getNameOrAsOperand().compare(VARNAME)){
              builder.CreateStore(ConstantInt::get(builder.getInt32Ty(), 1), M.getNamedGlobal(new_var));
            }
            else{
              builder.CreateStore(ConstantInt::get(builder.getInt32Ty(), 0), M.getNamedGlobal(new_var));
            }
          }

          // call inst, incomplete
          CallInst *callInst = dyn_cast<CallInst>(&Inst);
          if(callInst){
            M.getOrInsertGlobal(new_var, intType);
            GlobalVariable *gv = M.getNamedGlobal(new_var);
            gv->setDSOLocal(true);
            gv->setInitializer(ConstantInt::get(intType, 0));
            gv->setAlignment(MaybeAlign(4));
            //TODO:
            //  propagate the annotation when it returns
          }

          // load inst, set new var marked as the pointer operand
          LoadInst *loadInst = dyn_cast<LoadInst>(&Inst);
          if(loadInst
             && M.getNamedGlobal(F.getName().str() + loadInst->getPointerOperand()->getName().str())){
            M.getOrInsertGlobal(new_var, intType);
            GlobalVariable *gv = M.getNamedGlobal(new_var);
            gv->setDSOLocal(true);
            gv->setInitializer(ConstantInt::get(intType, 0));
            gv->setAlignment(MaybeAlign(4));
            LoadInst *load = builder.CreateLoad(intType, M.getNamedGlobal(F.getName().str() + loadInst->getPointerOperand()->getName().str()), "load");
            builder.CreateStore(load, M.getNamedGlobal(new_var));
          }

          //store inst, set pointer operand marked as value operand
          StoreInst *storeInst = dyn_cast<StoreInst>(&Inst);
          if(storeInst
             && M.getNamedGlobal(F.getName().str() + storeInst->getValueOperand()->getName().str())
             &&M.getNamedGlobal(F.getName().str() + storeInst->getPointerOperand()->getName().str())){
            LoadInst *load;
            load = builder.CreateLoad(intType, M.getNamedGlobal(F.getName().str() + storeInst->getValueOperand()->getName().str()), "load");
            builder.CreateStore(load, M.getNamedGlobal(F.getName().str() + storeInst->getPointerOperand()->getName().str()));
          }

          // binary inst, incomplete
          BinaryOperator *binInst = dyn_cast<BinaryOperator>(&Inst);
          if(binInst){
            //TODO:
            //  union the annotation
          }

          // unary inst, set new var marked as the operand 
          UnaryOperator *unaInst = dyn_cast<UnaryOperator>(&Inst);
          if(unaInst
             && M.getNamedGlobal(F.getName().str() + unaInst->getOperand(0)->getNameOrAsOperand())){
            M.getOrInsertGlobal(new_var, intType);
            GlobalVariable *gv = M.getNamedGlobal(new_var);
            gv->setDSOLocal(true);
            gv->setInitializer(ConstantInt::get(intType, 0));
            gv->setAlignment(MaybeAlign(4));
            LoadInst *load = builder.CreateLoad(intType, M.getNamedGlobal(F.getName().str() + unaInst->getOperand(0)->getNameOrAsOperand()), "load");
            builder.CreateStore(load, M.getNamedGlobal(new_var));
          }

          // gep inst
          // when the program is getting element ptr from the target var(buf), the second operand(index) of gep inst should be the offset in the message
          // besides, the index should always be non-negative, so:
          // multiply this index plus 1 and the mark of the first operand(array), then:
          // if the first operand is marked, the result will be a positive number, else it will be 0, then:
          // store the result sub 1 to the offset and print the log, finally:
          // there might be some logs in the file whose offset are -1, and we could simply remove them
          GetElementPtrInst *getInst = dyn_cast<GetElementPtrInst>(&Inst);
          if(getInst
             && M.getNamedGlobal(F.getName().str() + getInst->getOperand(0)->getNameOrAsOperand())){
            M.getOrInsertGlobal(new_var, intType);
            GlobalVariable *gv = M.getNamedGlobal(new_var);
            gv->setDSOLocal(true);
            gv->setInitializer(ConstantInt::get(intType, 0));
            gv->setAlignment(MaybeAlign(4));
            Value *load = builder.CreateLoad(intType, M.getNamedGlobal(F.getName().str() + getInst->getOperand(0)->getNameOrAsOperand()), "load");
            Value *trunc = builder.CreateTrunc(getInst->getOperand(1), intType);
            Value *add = builder.CreateAdd(trunc, ConstantInt::get(builder.getInt32Ty(), 1));
            Value *mul = builder.CreateMul(add, load, "mul");
            builder.CreateStore(mul, offset);
            LoadInst *load_fp = builder.CreateLoad(Type::getInt64PtrTy(F.getContext()), fp, "load_fp");
            Value *load_offset = builder.CreateLoad(Type::getInt32Ty(F.getContext()), offset, "load_offset");
            Value *load_sp = builder.CreateLoad(intType, stack_pointer, "load_sp");
            Value *sub = builder.CreateSub(load_offset, ConstantInt::get(builder.getInt32Ty(), 1));
            Value *fputs_params[] = {
              load_fp,
              builder.CreateGlobalStringPtr("%d\t%d\t%s\t%x\n", "accept_insert"),
              sub,
              ConstantInt::get(builder.getInt32Ty(), 1),
              builder.CreateLoad(ptr64Type, builder.CreateGEP(arrType, call_stack, {ConstantInt::get(intType, 0), load_sp}, "gep", true)),
              ConstantInt::get(builder.getInt32Ty(), 3),
            };
            builder.CreateCall(fprintfFunc, fputs_params, "get_call");
          }

          // return inst, pop the stack
          ReturnInst *returnInst = dyn_cast<ReturnInst>(&Inst);
          if(returnInst){
            Value *load = builder.CreateLoad(intType, stack_pointer, "load");
            Value *sub = builder.CreateSub(load, ConstantInt::get(intType, 1));
            builder.CreateStore(sub, stack_pointer);
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
