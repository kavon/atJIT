#pragma once

#include <llvm/IR/Metadata.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/ADT/SetVector.h>

#include <cassert>
#include <list>

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
  char const* TAG = "llvm.loop.id";

  char const* TRANSFORM_ATTR = "looptransform";

  Metadata* mkMDInt(IntegerType* Ty, uint64_t Val, bool isSigned = false) {
    auto ConstInt = ConstantInt::get(Ty, Val, isSigned);
    return ValueAsMetadata::get(ConstInt);
  }

  // return val indicates whether the module was changed
  // NOTE the transform metadata structure should follow
  // the work in Kruse's pragma branches
  bool addLoopTransformGroup(Function* F, std::list<MDNode*> &newXForms) {
    if (newXForms.empty())
      return false;

    SmallVector<Metadata*, 8> AllTransforms;
    auto &Ctx = F->getContext();

    // collect the existing transforms, if any
    auto FuncMD = F->getMetadata(TRANSFORM_ATTR);
    if (FuncMD)
      for (auto &X : FuncMD->operands())
        AllTransforms.push_back(X.get());

    // add new transforms to the group
    for (auto X : newXForms)
      AllTransforms.push_back(X);

    auto AllTransformsMD = MDNode::get(Ctx, AllTransforms);
    F->setMetadata(TRANSFORM_ATTR, AllTransformsMD);
    return true;
  }

  MDNode* createTilingMD(LLVMContext& Cxt, const char* XFORM_NAME, std::vector<std::pair<unsigned, uint16_t>> Dims) {
    IntegerType* i64 = IntegerType::get(Cxt, 64);

    SmallVector<Metadata*, 8> Names;
    SmallVector<Metadata*, 8> Sizes;

    for (auto Dim : Dims) {
      Names.push_back(MDString::get(Cxt, std::to_string(Dim.first)));
      Sizes.push_back(mkMDInt(i64, Dim.second));
    }

    // build the arguments to the transform
    Metadata* NameList = MDNode::get(Cxt, Names);
    Metadata* SizeList = MDNode::get(Cxt, Sizes);

    // build the transform node itself
    Metadata* XFormName = MDString::get(Cxt, XFORM_NAME);
    MDNode* Transform = MDNode::get(Cxt, {XFormName, NameList, SizeList});

    return Transform;
  }


  MDNode* createLoopName(LLVMContext& Context, unsigned &LoopIDs) {
    // build a Polly-compatible ID for the loop
    MDString *Tag = MDString::get(Context, TAG);
    unsigned IntVal = LoopIDs++;
    MDString* Val = MDString::get(Context, std::to_string(IntVal));

    MDNode *KnobTag = MDNode::get(Context, {Tag, Val});
    return KnobTag;
  }

  // parse the LoopMD, looking for the tag added by createLoopName
  unsigned getLoopName(MDNode* LoopMD) {
    for (const MDOperand& Op : LoopMD->operands()) {
      MDNode *Entry = dyn_cast<MDNode>(Op.get());
      if (!Entry || Entry->getNumOperands() != 2)
        continue;

      MDString *Tag = dyn_cast<MDString>(Entry->getOperand(0).get());
      MDString *Val = dyn_cast<MDString>(Entry->getOperand(1).get());

      if (!Tag || !Val)
        continue;

      if (Tag->getString() != TAG)
        continue;

      llvm::StringRef Str = Val->getString();
      unsigned IntVal = ~0;
      Str.getAsInteger(10, IntVal);

      if (IntVal == ~0)
        report_fatal_error("bad loop ID metadata on our tag!");

      return IntVal;
    } // end loop

    report_fatal_error("not all loops have an ID tag for tuning");
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
