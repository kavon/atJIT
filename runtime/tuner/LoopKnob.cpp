
#include <tuner/LoopKnob.h>
#include <tuner/MDUtils.h>

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/IR/LegacyPassManager.h>

#include <unordered_map>
#include <iostream>

namespace tuner {

  namespace {

    using namespace llvm;

    // {!"option.name"}, aka, no value
    MDNode* setPresenceOption(MDNode *LMD, std::optional<bool> const& Option, StringRef OptName) {
      if (Option) {
        if (Option.value()) // add the option
          return updateLMD(LMD, OptName, nullptr);
        else // delete any such options, if present
          return updateLMD(LMD, OptName, nullptr, true);
      }

      return LMD;
    }

#define SET_INT_OPTION(IntKind, FieldName, StringName) \
if (LS.FieldName) \
  LMD = updateLMD(LMD, loop_md::StringName, \
                        mkMDInt(IntKind, LS.FieldName.value()));



    MDNode* addToLoopMD(MDNode *LMD, LoopSetting const& LS) {
      LLVMContext &C = LMD->getContext();
      IntegerType* i32 = IntegerType::get(C, 32);
      IntegerType* i1 = IntegerType::get(C, 32);

      LMD = setPresenceOption(LMD, LS.UnrollDisable, loop_md::UNROLL_DISABLE);
      LMD = setPresenceOption(LMD, LS.UnrollFull, loop_md::UNROLL_FULL);

      LMD = setPresenceOption(LMD, LS.LICMVerDisable, loop_md::LICM_VER_DISABLE);

      SET_INT_OPTION(i32, UnrollCount, UNROLL_COUNT)
      SET_INT_OPTION(i32, VectorizeWidth, VECTORIZE_WIDTH)
      SET_INT_OPTION(i32, InterleaveCount, INTERLEAVE_COUNT)

      SET_INT_OPTION(i1, Distribute, DISTRIBUTE)

      return LMD;
    }

    class LoopKnobAdjustor : public LoopPass {
    private:
      // the group of loops we're modifying
      LoopKnob *MainLK;
      std::unordered_map<unsigned, LoopKnob*> SubLoops;

      void identifyLoops(LoopKnob *LK) {
        // we intentially skip adding the given node.
        // its parent should add it.
        for (auto Sub : *LK) {
          SubLoops[Sub->getLoopName()] = Sub;
          identifyLoops(Sub);
        }
      }

      bool addRegularMD(Loop *Loop, MDNode* LoopMD, LoopKnob *LK) {
        MDNode *NewLoopMD = addToLoopMD(LoopMD, LK->getVal());

        if (NewLoopMD == LoopMD) // then nothing changed.
          return false;

        Loop->setLoopID(NewLoopMD);
        return true;
      }

      // This simple loop nesting analysis builds n-dimensional loop
      // sectioning metadata, where n >= 2.
      //
      // n = 1 would be strip-mining, but that is useless to do for
      // two reasons:
      //    1. LLVM already does partial unrolling & vectorization that involves
      //       strip-mining.
      //    2. We're not following up with any other transforms.
      void analyzeSections(LLVMContext& Cxt, std::list<MDNode*>& XForms, LoopKnob* Root) {

        // Build up a maximal n-dimensional loop sectioning group
        // by walking down the spine of the tree until we reach
        // either a fork or a leaf.
        std::vector<std::pair<unsigned, uint16_t>> Dims;
        LoopKnob *Cur = Root;
        auto SectionSz = Cur->getVal().Section;

        while (SectionSz) {
          // consume the current sz
          Dims.push_back({Cur->getLoopName(), SectionSz.value()});

          // look for a nested sectioning sz
          if (Cur->children().size() != 1) {
            // stop here, hit a fork or leaf
            SectionSz = std::nullopt;
          } else {
            // process the single child
            Cur = Cur->children()[0];
            SectionSz = Cur->getVal().Section;
          }
        } // end loop

        if (Dims.size() >= 2) {
          // create the 2+ dimensional sectioning group
          XForms.push_back(createTilingMD(Cxt, loop_md::SECTION, Dims));
        }

        // recursively process the remaining children.
        for (auto Child : *Cur)
          analyzeSections(Cxt, XForms, Child);

      } // end func


    public:
      static char ID;

      // required to have a default ctor
      LoopKnobAdjustor() : LoopPass(ID), MainLK(nullptr) {}

      LoopKnobAdjustor(LoopKnob *LKnob)
        : LoopPass(ID), MainLK(LKnob) {
          identifyLoops(MainLK);
        };

      bool runOnLoop(Loop *Loop, LPPassManager &LPM) override {
        MDNode* LoopMD = Loop->getLoopID();
        if (!LoopMD) {
          // LoopNamer should have been run when embedding the bitcode.
          report_fatal_error("encountered a loop without llvm.loop metadata!");
          return false;
        }

        unsigned CurrentLName = getLoopName(LoopMD);

        // is it a subloop?
        auto result = SubLoops.find(CurrentLName);
        if (result != SubLoops.end()) {
          // then just add regular metadata to this subloop
          return addRegularMD(Loop, LoopMD, result->second);
        }

        // if it's also not the main loop, this loop is not part of our group.
        if (CurrentLName != MainLK->getLoopName())
          return false;

        // first, add regular metadata to the main loop
        bool regularChanged = addRegularMD(Loop, LoopMD, MainLK);

        // then, add n-dimensional sectioning metadata to all loops in this group.
        Function* F = Loop->getHeader()->getParent();
        std::list<MDNode*> Transforms;
        LLVMContext& Cxt = F->getContext();

        analyzeSections(Cxt, Transforms, MainLK);

        bool xformsChanged = addLoopTransformGroup(F, Transforms);

        return regularChanged || xformsChanged;
      }

    }; // end class

    char LoopKnobAdjustor::ID = 0;
    static RegisterPass<LoopKnobAdjustor> Register("loop-knob-adjustor",
      "Adjust the metadata of a loop and its sub-loops",
                            false /* only looks at CFG*/,
                            false /* analysis pass */);

  } // end anonymous namespace

void LoopKnob::apply (llvm::Module &M) {
  // We only run the adjustor pass on top-level loops in the module,
  // since it will apply the setting to all nested loops too.
  if (loopDepth() != 1)
    return;

  // initialize dependencies
  initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());

  legacy::PassManager Passes;
  Passes.add(new LoopKnobAdjustor(this));
  Passes.run(M);
}

} // end namespace tuner


#define PRINT_OPTION(X, Y) \
  if ((X)) { \
    if (pastFirst) JSON::comma(o); \
    pastFirst |= true; \
    JSON::output(o, tuner::loop_md::Y, (X).value(), false); }


// this file seemed fitting to define this function. can't put in header.
std::ostream& operator<<(std::ostream &o, tuner::LoopSetting &LS) {
  JSON::beginObject(o);

  bool pastFirst = false;

  PRINT_OPTION(LS.UnrollDisable, UNROLL_DISABLE)
  PRINT_OPTION(LS.UnrollFull, UNROLL_FULL)
  PRINT_OPTION(LS.UnrollCount, UNROLL_COUNT)

  PRINT_OPTION(LS.VectorizeWidth, VECTORIZE_WIDTH)

  PRINT_OPTION(LS.LICMVerDisable, LICM_VER_DISABLE)

  PRINT_OPTION(LS.InterleaveCount, INTERLEAVE_COUNT)

  PRINT_OPTION(LS.Distribute, DISTRIBUTE)

  PRINT_OPTION(LS.Section, SECTION)

  JSON::endObject(o);
  return o;
}

bool operator==(const tuner::LoopSetting& A, const tuner::LoopSetting& B) {
  return
    (A.UnrollDisable == B.UnrollDisable) &&
    (A.UnrollFull == B.UnrollFull) &&
    (A.UnrollCount == B.UnrollCount) &&

    (A.VectorizeWidth == B.VectorizeWidth) &&

    (A.LICMVerDisable == B.LICMVerDisable) &&

    (A.InterleaveCount == B.InterleaveCount) &&

    (A.Distribute == B.Distribute) &&

    (A.Section == B.Section)

    ;
}

bool operator!=(const tuner::LoopSetting& A, const tuner::LoopSetting& B) {
  return !(A == B);
}
