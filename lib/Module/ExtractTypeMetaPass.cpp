#include "Passes.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Endian.h"

#include <cstring>
#include <iostream>
#include <sstream>

using namespace llvm;

char klee::ExtractTypeMetaCheck::ID = 0;
const char* klee::ExtractTypeMetaCheck::KLEE_META_FILENAME = "type-meta.kmeta";

klee::ExtractTypeMetaCheck::ExtractTypeMetaCheck(InterpreterHandler *ih) : llvm::ModulePass(ID),
																		   ih(ih), isEndiannessWritten(false) {
    metaOutFile = ih->openOutputFile(KLEE_META_FILENAME);
}

klee::ExtractTypeMetaCheck::~ExtractTypeMetaCheck() {
	delete metaOutFile;
}

void klee::ExtractTypeMetaCheck::writeLine(std::string line) {
	if (metaOutFile) {
		metaOutFile->write(line.c_str(), line.length());
		metaOutFile->write("\n", 1);
	}
}

std::string klee::ExtractTypeMetaCheck::getTypeMetaData(Type *t, const llvm::DataLayout &dl) {
	std::stringstream ss;
	//TODO should somehow find out if integer is signed or unsigned,
	//however llvm does not store this information
	if (t->isIntegerTy()) {
		IntegerType *it = dyn_cast<IntegerType>(t);
		if (it->getBitWidth() == 8) {
			ss << "char:" << it->getBitWidth() / 8;
		} else {
			ss << "int:" << it->getBitWidth() / 8;
		}
	} else if (t->isArrayTy()) {
		ArrayType *at = dyn_cast<ArrayType>(t);
		ss << "array:" << at->getNumElements() << ":" << getTypeMetaData(at->getElementType(), dl);
	} else if (t->isStructTy()) {
		StructType *st = dyn_cast<StructType>(t);
		const StructLayout *sl = dl.getStructLayout(st);
		ss << "struct:" << (st->hasName() ? st->getName().str() : "unknown") << ":";
		size_t i = 0;
		//TODO currently recursively traverses the whole struct, maybe should limit the depth?
		for (llvm::StructType::element_iterator seb = st->element_begin(), see = st->element_end(); seb != see; ++seb) {
			Type* currentType = *seb;
			uint64_t offset = sl->getElementOffset(i);
			if (seb+1 < see) {
				ss << getTypeMetaData(currentType, dl) << ":" << offset << ">>>";
			} else {
				ss << getTypeMetaData(currentType, dl) << ":" << offset << "###";
			}
			++i;
		}
	} else {
		ss << "unhandled";
	}
	return ss.str();
}

std::string klee::ExtractTypeMetaCheck::getSymbolicName(llvm::CallInst *ci) {
	StringRef name = "";
	if (ci) {
		ConstantExpr *ce = dyn_cast<ConstantExpr>(ci->getArgOperand(2));
		if (ce) {
			GlobalVariable *gv = dyn_cast<GlobalVariable>(ce->getOperand(0));
			if (gv) {
			  	ConstantDataArray *cda = dyn_cast<ConstantDataArray>(gv->getInitializer());
			   	if (cda) {
					name = cda->getAsString();
			   	}
			}
		} // TODO could not handle the case when the third parameter of klee_make_symbolic
		  // is a variable
	}
	return name.str();
}

Type* klee::ExtractTypeMetaCheck::getSymbolicType(llvm::CallInst *ci) {
	if (ci) {
		Value* v = ci->getArgOperand(0)->stripPointerCasts();
		PointerType *pt = dyn_cast<PointerType>(v->getType());
		if (pt) {
			return pt->getElementType();
		}
	}
	return nullptr;
}

bool klee::ExtractTypeMetaCheck::runOnModule(llvm::Module &M) {
	bool changed = false;
	if (!metaOutFile) {
		return changed;
	}
	DataLayout dl = DataLayout(M.getDataLayout());
	if (!isEndiannessWritten) {
#if LLVM_VERSION_CODE < LLVM_VERSION(3, 5)
		writeLine(M.getEndianness() == llvm::Module::Endianness::LittleEndian ? "little" : "big");
#else
		writeLine(dl.isLittleEndian() ? "little" : "big");
#endif
		isEndiannessWritten = true;
	}
	for (Module::iterator mi = M.begin(), me = M.end(); mi != me; ++mi) {
		for (Function::iterator bi = mi->begin(), be = mi->end(); bi != be; ++bi) {
			for (BasicBlock::iterator ii = bi->begin(), ie = bi->end(); ii != ie; ++ii) {
				if (strcmp(ii->getOpcodeName(), "call") == 0) {
					CallInst *ci = cast<CallInst>(ii);
					if (ci) {
						Function *f = ci->getCalledFunction();
						if (f) {
							StringRef name = ci->getCalledFunction()->getName();
							if (name.equals("klee_make_symbolic")) {
								writeLine(getSymbolicName(ci));
								writeLine(getTypeMetaData(getSymbolicType(ci), dl));
							}
						}
					}
				}
			}
		}
	}
	return changed;
}