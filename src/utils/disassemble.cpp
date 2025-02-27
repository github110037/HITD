#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include <fmt/core.h>
#include <llvm/MC/MCContext.h>
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "disassemble.hpp"

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

using namespace llvm;
llvm::MCDisassembler *gDisassembler = nullptr;
llvm::MCSubtargetInfo *gSTI = nullptr;
llvm::MCInstPrinter *gIP = nullptr;
std::string s;
word_t valid_pc;
bool valid;

void llvm_disasm_init() {
  const char *triple = "mipsel-pc-linux-gnu";
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllDisassemblers();

  std::string errstr;
  std::string gTriple(triple);

  llvm::MCInstrInfo *gMII = nullptr;
  llvm::MCRegisterInfo *gMRI = nullptr;
  auto target = llvm::TargetRegistry::lookupTarget(gTriple, errstr);
  if (!target) {
    llvm::errs() << "Can't find target for " << gTriple << ": " << errstr
                 << "\n";
    assert(0);
  }

  MCTargetOptions MCOptions;
  gSTI = target->createMCSubtargetInfo(gTriple, "", "");
  std::string isa = target->getName();
  gMII = target->createMCInstrInfo();
  gMRI = target->createMCRegInfo(gTriple);
  auto AsmInfo = target->createMCAsmInfo(*gMRI, gTriple, MCOptions);
  auto llvmTripleTwine = Twine(triple);
  auto llvmtriple = llvm::Triple(llvmTripleTwine);
  auto Ctx = new llvm::MCContext(llvmtriple, AsmInfo, gMRI, nullptr);
  gDisassembler = target->createMCDisassembler(*gSTI, *Ctx);
  gIP = target->createMCInstPrinter(llvm::Triple(gTriple),
                                    AsmInfo->getAssemblerDialect(), *AsmInfo,
                                    *gMII, *gMRI);
  gIP->setPrintImmHex(true);
  gIP->setUseMarkup(true);
  gIP->setPrintBranchImmAsAddress(true);
}

static void disassemble(word_t inst_value, word_t pc) {
  MCInst inst;
  uint8_t *code = (uint8_t *)&inst_value;
  llvm::ArrayRef<uint8_t> arr(code, 4);
  uint64_t dummy_size = 0;
  gDisassembler->getInstruction(inst, dummy_size, arr, pc, llvm::nulls());
  s = fmt::format("{:02x} {:02x} {:02x} {:02x}", code[3], code[2], code[1],
                  code[0]);
  raw_string_ostream os(s);
  gIP->printInst(&inst, pc, "", *gSTI, os);
}

const std::string &llvm_disassemble(word_t pc, word_t inst) {
  if ((valid && pc == valid_pc)==false) {
    disassemble(inst, pc);
    valid = true;
    valid_pc = pc;
  }
  return s;
}
