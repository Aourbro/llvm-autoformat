add_llvm_library( LLVMMyHello MODULE
  MyHello.cpp

  PLUGIN_TOOL
  opt
  )
