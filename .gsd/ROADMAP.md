# ROADMAP.md

> **Current Milestone**: Codebase Cleanup & Refactoring (`v1.0-Cleanup`)
> **Goal**: Clean up the codebase, remove unnecessary and redundant code blocks, and optimize trap detection.

## Must-Haves
- [ ] Audit and remove dead/redundant code blocks in `TrollGameCharacter.cpp` & `.h`.
- [ ] Refactor `ATrollPressurePlate` detection logic to eliminate duplicate fallback branches.
- [ ] Clean up debug HUD clutter while preserving trap functionality.
- [ ] Full build verification with 0 compilation errors.

## Phases

### Phase 1: Audit Codebase & Identify Redundancies
**Status**: ⬜ Not Started
**Objective**: Inspect `TrollGameCharacter` and `TrollPressurePlate` to catalog all redundant functions, dead variables, and duplicate logic paths.

### Phase 2: Refactor & Clean Codebase
**Status**: ⬜ Not Started
**Objective**: Remove identified redundant code blocks, simplify trap detection methods, and streamline component discovery.

### Phase 3: Build & Empirical Verification
**Status**: ⬜ Not Started
**Objective**: Run full Unreal build, verify zero errors, and empirically test character trap mechanics.
