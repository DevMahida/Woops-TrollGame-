# SPEC.md — Project Specification

> **Status**: `FINALIZED`

## Vision
Implement the Hallway Pusher Trap mechanics triggered by `"TrapFallFloorTrigger"`. After a 0.5-second delay upon trigger overlap, `"HallwayPush"` and `"HallwayPush2"` move towards each other, pushing the player into the void as the trap floor opens.

## Goals
1. Detect `"TrapFallFloorTrigger"` overlap by player.
2. Introduce a precise **0.5-second delay** after trigger step before pusher movement and floor drop.
3. Move `"HallwayPush"` and `"HallwayPush2"` towards each other to squeeze the hallway and force player down.
4. Auto-reset pusher positions, delay timers, and floor collision when player respawns at a checkpoint.

## Non-Goals (Out of Scope)
- Modifying pressure plate shrink messages.
- Changing character movement velocity parameters outside trap scope.

## Users
Players progressing through TrollGame hallways.

## Constraints
- Must compile cleanly with 0 errors.
- 0.5-second delay must be exact.
- Resets seamlessly on checkpoint respawn or void kill.

## Success Criteria
- [ ] Stepping on `"TrapFallFloorTrigger"` starts 0.5s timer.
- [ ] After 0.5s delay, `"HallwayPush"` and `"HallwayPush2"` smoothly move towards each other.
- [ ] Floor drops / opens, sending player into void.
- [ ] Respawns at checkpoint reset pushers back to initial positions.
