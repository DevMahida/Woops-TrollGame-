# SPEC.md — Project Specification

> **Status**: `FINALIZED`

## Vision
Refactor and clean up the TrollGame C++ codebase to eliminate redundant, dead, or duplicate code blocks, clean up debug messages, and streamline trap detection logic while maintaining 100% feature functionality.

## Goals
1. Clean up unused and redundant code paths in `TrollGameCharacter.cpp` and `TrollPressurePlate.cpp`.
2. Consolidate pressure plate trap detection logic to avoid code duplication and ensure single-source-of-truth component discovery.
3. Clean up debug logging and ensure all build targets compile with zero warnings or errors.

## Non-Goals (Out of Scope)
- Deleting core game features (checkpoint system, math trap manager, character movement scaling).
- Modifying Unreal Engine asset references or breaking editor component bindings.

## Users
Players and developers working on TrollGame.

## Constraints
- Must compile cleanly with `Build.bat` (Unreal Editor Development Win64 target).
- Preserves all character trap mechanics, checkpoint resets, and room screen 3D text displays.

## Success Criteria
- [ ] No dead or redundant code blocks in `TrollGameCharacter.cpp` or `TrollPressurePlate.cpp`.
- [ ] All 3 pressure plate objects (`PressurePlate`, `SidePressurePlate`, `SidePressurePlate1`) work seamlessly with clean, readable code.
- [ ] Project builds cleanly via `Build.bat` with 0 errors.
