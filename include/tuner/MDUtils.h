#ifndef TUNER_MDUTILS
#define TUNER_MDUTILS

#include <llvm/IR/Metadata.h>
#include <llvm/IR/Constants.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// NOTE
// these utils are included in both the clang pass plugin, and
// the runtime library, so we wrap this in an anonymous namespace so
// the definitions can be included wherever needed.
// this may cause "unused function" warnings, so we turn those off for these.

namespace {

  using namespace llvm;

  namespace loop_name {
     char const* TAG = "atJit.loop";
  }

  MDNode* createLoopName(LLVMContext& Context, unsigned &LoopIDs) {
    // build a custom knob tag for the loop
    MDString *Tag = MDString::get(Context, loop_name::TAG);
    auto Val = ConstantInt::get(IntegerType::get(Context, 32),
                                LoopIDs++, /*Signed=*/false);

    MDNode *KnobTag = MDNode::get(Context, {Tag, ValueAsMetadata::get(Val)});
    return KnobTag;
  }

  // parse the LoopMD, looking for the tag added by createLoopName
  unsigned getLoopName(MDNode* LoopMD) {
    for (const MDOperand& Op : LoopMD->operands()) {
      MDNode *Entry = dyn_cast<MDNode>(Op.get());
      if (!Entry || Entry->getNumOperands() != 2)
        continue;

      MDString *Tag = dyn_cast<MDString>(Entry->getOperand(0).get());
      ValueAsMetadata *Val = dyn_cast<ValueAsMetadata>(Entry->getOperand(1).get());

      if (!Tag || !Val)
        continue;

      ConstantInt *ValConst = dyn_cast_or_null<ConstantInt>(Val->getValue());
      if (ValConst && Tag->getString() == loop_name::TAG) {
        return ValConst->getZExtValue();
      }
    }
    report_fatal_error("not all loops have an atJit tag for tuning");
  }

} // end namespace

#pragma GCC diagnostic pop

#endif // TUNER_MDUTILS
