#ifndef TUNER_MDUTILS
#define TUNER_MDUTILS

#include <llvm/IR/Metadata.h>
#include <llvm/IR/Constants.h>
#include <llvm/ADT/SetVector.h>

#include <cassert>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// NOTE
// these utils are included in both the clang pass plugin, and
// the runtime library, so we wrap this in an anonymous namespace so
// the definitions can be included wherever needed.
// this may cause "unused function" warnings, so we turn those off for these.

namespace {

  using namespace llvm;

  // make sure to keep this in sync with LoopKnob.h
  char const* TAG = "atJit.loop";

  Metadata* mkMDInt(IntegerType* Ty, uint64_t Val, bool isSigned = false) {
    auto ConstInt = ConstantInt::get(Ty, Val, isSigned);
    return ValueAsMetadata::get(ConstInt);
  }

  MDNode* createLoopName(LLVMContext& Context, unsigned &LoopIDs) {
    // build a custom knob tag for the loop
    MDString *Tag = MDString::get(Context, TAG);
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
      if (ValConst && Tag->getString() == TAG) {
        return ValConst->getZExtValue();
      }
    }
    report_fatal_error("not all loops have an atJit tag for tuning");
  }

  inline bool matchesLoopOption(Metadata *MD, StringRef &Key) {
      MDNode *MDN = dyn_cast<MDNode>(MD);
      if (!MDN || MDN->getNumOperands() < 1)
        return false;

      MDString *EntryKey = dyn_cast<MDString>(MDN->getOperand(0).get());

      if (!EntryKey || EntryKey->getString() != Key)
        return false;

      return true;
  }

  // a functional-style insertion with replacement that preserves all
  // non-matching operands of the MDNode, and returns a valid LoopMD.
  // This function can also be used to delete an entry of the given key if nullptr is provided as the Val.
  //
  // example:
  //
  //  BEFORE:
  //
  // !x = {!x, ... !1, ...}
  // !1 = {"loop.vectorize.enable", i32 1}
  //
  //  AFTER THE FOLLOWING ACTION
  //
  // updateLMD(!x, "loop.vectorize.enable", i32 0)
  //
  // !y = {!y, ... !1, ...}
  // !1 = {"loop.vectorize.enable", i32 0}
  //
  // The node !y will be returned.
  //
  MDNode* updateLMD(MDNode *LoopMD, StringRef Key, Metadata* Val, bool DeleteOnly = false) {
    SmallSetVector<Metadata *, 4> MDs(LoopMD->op_begin(), LoopMD->op_end());
    LLVMContext &Cxt = LoopMD->getContext();

    // if the loop-control option already exists, remove it.
    MDs.remove_if([&](Metadata *MD) {
      return matchesLoopOption(MD, Key);
    });

    if (!DeleteOnly) {
      // add the new option.
      MDString* KeyMD = MDString::get(Cxt, Key);

      if (Val)
        MDs.insert(MDNode::get(Cxt, {KeyMD, Val}));
      else
        MDs.insert(MDNode::get(Cxt, {KeyMD}));
    } else {
      // deletion is done by key only, we do not try to match values too.
      assert(!Val && "did not expect SOME value during deletion!");
    }

    // create the new MDNode
    MDNode *NewLoopMD = MDNode::get(Cxt, MDs.getArrayRef());

    // since this is Loop metadata, we need to recreate the self-loop.
    NewLoopMD->replaceOperandWith(0, NewLoopMD);

    return NewLoopMD;
  }

} // end namespace

#pragma GCC diagnostic pop

#endif // TUNER_MDUTILS
