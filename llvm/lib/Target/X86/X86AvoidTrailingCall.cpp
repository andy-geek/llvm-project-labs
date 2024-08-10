#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

#define AVOIDCALL_DESC "X86 avoid trailing call pass"
#define AVOIDCALL_NAME "x86-avoid-trailing-call"

#define DEBUG_TYPE AVOIDCALL_NAME

using namespace llvm;

namespace {
class X86AvoidTrailingCallPass : public MachineFunctionPass {
public:
  X86AvoidTrailingCallPass() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  static char ID;

private:
  StringRef getPassName() const override { return AVOIDCALL_DESC; }
};
} // end anonymous namespace

char X86AvoidTrailingCallPass::ID = 0;

FunctionPass *llvm::createX86AvoidTrailingCallPass() {
  return new X86AvoidTrailingCallPass();
}

INITIALIZE_PASS(X86AvoidTrailingCallPass, AVOIDCALL_NAME, AVOIDCALL_DESC, false, false)

bool X86AvoidTrailingCallPass::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;
  const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();

  for (auto &MBB : MF) {
    for (auto I = MBB.begin(); I != MBB.end(); I++) {
      if (I->getOpcode() == X86::MULPDrr) {
        auto MulInstr = I;
        auto NextInstr = std::next(MulInstr);
        if (NextInstr->getOpcode() == X86::ADDPDrr) {
          if (MulInstr->getOperand(0).getReg() == NextInstr->getOperand(1).getReg()) {
            I--;
            MachineInstr &MI = *MulInstr;
            MachineInstrBuilder MIB = BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(X86::VFMADD213PDZ128r));

            MIB.addReg(NextInstr->getOperand(0).getReg(), RegState::Define);
            MIB.addReg(MulInstr->getOperand(1).getReg());
            MIB.addReg(MulInstr->getOperand(2).getReg());
            MIB.addReg(NextInstr->getOperand(2).getReg());

            MulInstr->eraseFromParent();
            NextInstr->eraseFromParent();

            Changed = true;
          }
        }
      }
    }
  }

  return Changed;
}