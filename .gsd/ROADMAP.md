# ROADMAP.md

> **Current Milestone**: Hallway Pusher Trap (`v1.1-PusherTrap`)
> **Goal**: Implement 0.5s delayed HallwayPush & HallwayPush2 movement trap when TrapFallFloorTrigger is activated.

## Must-Haves
- [x] Discover `"HallwayPush"` and `"HallwayPush2"` actors in level.
- [x] Implement 0.5s delay timer on `"TrapFallFloorTrigger"` activation.
- [x] Move `"HallwayPush"` and `"HallwayPush2"` towards each other in `Tick()`.
- [x] Drop trap floor and push player into void.
- [x] Reset pushers to original positions on checkpoint respawn.

## Phases

### Phase 1: Actor Discovery & State Setup
**Status**: ✅ Complete
**Objective**: Cache `"HallwayPush"` and `"HallwayPush2"` actor locations and setup delay timer variables.

### Phase 2: Delayed Movement & Floor Drop Implementation
**Status**: ✅ Complete
**Objective**: Implement 0.5s delay timer, smooth inter-pusher movement, and floor collision removal.

### Phase 3: Checkpoint Reset & Verification
**Status**: ✅ Complete
**Objective**: Reset pusher transforms on checkpoint respawn and verify build & execution.

