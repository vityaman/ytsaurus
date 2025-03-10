#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

//===-- llvm/CodeGen/PseudoSourceValueManager.h -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the PseudoSourceValueManager class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_PSEUDOSOURCEVALUEMANAGER_H
#define LLVM_CODEGEN_PSEUDOSOURCEVALUEMANAGER_H

#include "llvm/ADT/StringMap.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/IR/ValueMap.h"
#include <map>

namespace llvm {

class GlobalValue;
class TargetMachine;

/// Manages creation of pseudo source values.
class PseudoSourceValueManager {
  const TargetMachine &TM;
  const PseudoSourceValue StackPSV, GOTPSV, JumpTablePSV, ConstantPoolPSV;
  std::map<int, std::unique_ptr<FixedStackPseudoSourceValue>> FSValues;
  StringMap<std::unique_ptr<const ExternalSymbolPseudoSourceValue>>
      ExternalCallEntries;
  ValueMap<const GlobalValue *,
           std::unique_ptr<const GlobalValuePseudoSourceValue>>
      GlobalCallEntries;

public:
  PseudoSourceValueManager(const TargetMachine &TM);

  /// Return a pseudo source value referencing the area below the stack frame of
  /// a function, e.g., the argument space.
  const PseudoSourceValue *getStack();

  /// Return a pseudo source value referencing the global offset table
  /// (or something the like).
  const PseudoSourceValue *getGOT();

  /// Return a pseudo source value referencing the constant pool. Since constant
  /// pools are constant, this doesn't need to identify a specific constant
  /// pool entry.
  const PseudoSourceValue *getConstantPool();

  /// Return a pseudo source value referencing a jump table. Since jump tables
  /// are constant, this doesn't need to identify a specific jump table.
  const PseudoSourceValue *getJumpTable();

  /// Return a pseudo source value referencing a fixed stack frame entry,
  /// e.g., a spill slot.
  const PseudoSourceValue *getFixedStack(int FI);

  const PseudoSourceValue *getGlobalValueCallEntry(const GlobalValue *GV);

  const PseudoSourceValue *getExternalSymbolCallEntry(const char *ES);
};

} // end namespace llvm

#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
