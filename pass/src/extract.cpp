#include "function_names.hpp"

#include "llvm/InitializePasses.h"
#include "llvm/Support/raw_ostream.h"

using namespace pass;
using namespace llvm;

namespace pass {

char ExtractFunctionToIR::ID = 0;

ExtractFunctionToIR::ExtractFunctionToIR() : llvm::ModulePass(ID) {
}

ExtractFunctionToIR::~ExtractFunctionToIR() {
}

bool ExtractFunctionToIR::doInitialization(llvm::Module &) {
	return false;
}

bool ExtractFunctionToIR::doFinalization(llvm::Module &) {
	return false;
}

void ExtractFunctionToIR::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
}

bool ExtractFunctionToIR::runOnModule(llvm::Module &M) {

    errs() << "on module!\n";
    return false;
}	

const char* command_line = "extract2ir";
const char* description = "Pass to extract functions to ir.";
bool is_analysis = false; 
bool is_cfg_only = false;

static RegisterPass<ExtractFunctionToIR> X(command_line, description, is_cfg_only, is_analysis);
}
