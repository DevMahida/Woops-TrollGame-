# "Whoops!" - Math Trap Level Designer Setup Guide

This guide is for the Unreal Engine level designer / integrator. In under **3 minutes**, you can drop the Math Trap room into your level.

---

## 1. Quick Rebuild
1. Pull the latest repository updates (`git pull`).
2. Right-click `TrollGame.uproject` $\rightarrow$ **Generate Visual Studio project files** (if needed).
3. Open the project in **Unreal Engine 5**. The engine will compile the new `TrollGame` module with the Math Trap classes.

---

## 2. Actors Included in this Trap

| C++ Class | Blueprint Recommendation | Purpose |
| :--- | :--- | :--- |
| `AMathTrapBuzzer` | `BP_MathTrapBuzzer` | Interactive buzzer numbered 1 to 10. Can be stepped on or pressed. |
| `AMathTrapExitDoor` | `BP_MathTrapExitDoor` | Locked exit door that smoothly swings open when the correct buzzer is pressed. |
| `AMathTrapManager` | `BP_MathTrapManager` | Central controller managing equations, screen messages, and troll progression. |

---

## 3. Step-by-Step Level Setup (3 Minutes)

### Step 1: Place the Exit Door
1. From the Content Drawer / Place Actors panel, drag `AMathTrapExitDoor` (or your Blueprint child `BP_MathTrapExitDoor`) into the doorway of the room.
2. In the **Details** panel:
   * **DoorFrameMesh**: Select your doorway frame mesh.
   * **DoorMesh**: Select your door mesh (pivot should be on the hinge side).
   * **OpenYawAngle**: Set swing angle (default is `-90.0`).
   * **UnlockSound**: (Optional) Assign an unlock or sliding door sound cue.

### Step 2: Place the 10 Buzzers
1. Drag `AMathTrapBuzzer` (or `BP_MathTrapBuzzer`) onto the floor.
2. Duplicate it 9 times (Alt + Drag) in a row or semi-circle.
3. Select each buzzer and set its **BuzzerNumber** in the Details panel:
   * Buzzer 1 $\rightarrow$ `BuzzerNumber = 1`
   * Buzzer 2 $\rightarrow$ `BuzzerNumber = 2`
   * ... up to `BuzzerNumber = 10`
4. *(Optional Visuals)* Assign a pedestal mesh to `BaseMesh` and a cylinder/box button mesh to `ButtonMesh`. The button automatically animates down when pressed!

### Step 3: Place the Room Screen & Start Point
* **Screen**: Place any wall panel or TV mesh actor and name its Actor Label **"Screen"** (or add the Actor Tag `Screen`). The manager will automatically spawn and align neon 3D text in front of it!
* **Start Point**: Place a TargetPoint or actor at the entrance of the room and label it **"Start"** (or add the Actor Tag `Start`).

### Step 4: Drop the Manager
1. Drag `AMathTrapManager` into the room.
2. **Auto-Discovery**: By default, the manager will automatically find all 10 buzzers, the exit door, the screen, and the start point!
3. *(Optional Details Settings)*:
   * **ConsequenceType**: Choose between `Teleport to Room Start`, `Launch Backwards`, or `Kill and Respawn Player`.
   * **Audio**: Assign `WrongBuzzerSound`, `PrankLaughSound` (airhorn or cartoon laugh), and `VictorySound`.
   * **EquationPool**: Pre-populated with 70 mathematically verified BODMAS, radical, and exponent equations covering integer answers 1 to 10.

---

## 4. How the Troll Progression Plays Out in Game

1. **Attempt 1 (Question with NO Hint)**:
   * Screen displays initial equation (e.g. `(8 / 2 * (1 + 3) - 14) = ?`) with NO hint.
   * Player presses any buzzer.
   * **Result**: *WRONG!* Consequence triggers, player teleported, numbers shuffle.

2. **Attempt 2 (Summation Hint + NEW Equation)**:
   * Screen displays a brand **NEW equation** along with the Summation troll hint:
     *"DID YOU REALLY THINK IT WAS THAT EASY? A true mathematician knows you must do the SUMMATION of the answer! (e.g., If answer is 2 then 2 + 2 = 4, press buzzer #4!)"*
   * Player presses a buzzer.
   * **Result**: *WRONG!* Consequence triggers, player teleported, numbers shuffle.

3. **Attempt 3 (Dev Oopsie Hint + NEW Equation)**:
   * Screen displays a brand **NEW equation** along with the Dev Oopsie troll hint:
     *"DEV OOPSIE! The developer forgot to say the entire hint: 'If answer is 2 then 2 + 2 = 4, then press the 4th buzzer from the labeled buzzer!' "*
   * Player presses a buzzer.
   * **Result**: *WRONG!* Consequence triggers, player teleported, numbers shuffle.

4. **Attempt 4 (The Roast & Dev Fake Highlight Prank)**:
   * Screen:
     *"DEV APOLOGY 😇: Okay look... the developer felt bad for you. I highlighted the correct buzzer with a bright glowing green light so you can just escape already. Go press the glowing green buzzer!"*
   * A fake buzzer in the room lights up with a **neon glowing green spotlight**!
   * Player presses the glowing green buzzer (or any other buzzer).
   * **Result**: *WRONG!* Highlight turns off, prank laugh sound plays!

5. **Attempt 5 (Fooled You & The Full Cipher Revealed)**:
   * Screen:
     *"FOOLED YOU! HAHAHA! 😂 There are no shortcuts in 'WHOOPS!' Those hints weren't fake—they are the EXACT RULES to escape! 1. Solve equation (A) 2. Summation (A + A = D) 3. Find buzzer labeled D, and press the Dth buzzer from it! SOLVE: (Equation) = ?"*

6. **Attempt 6+ (The Real Cipher Stage & Progressive Left Hint)**:
   * **First 1–4 Cipher Tries**: Standard wrong buzzer message prompting the player to apply the 3 cipher rules.
   * **After 4–5 Failed Tries on Cipher Stage**: The screen displays the progressive directional clue:
     *"STILL STUCK? 💡 Hint: Try counting the buzzers in the LEFT direction as well! Find the buzzer labeled with your summation result, and count that many steps to your LEFT!"*
   * **Solving Logic**:
     * Example: If equation answer $A = 7 \implies 7 + 7 = 14 \rightarrow D = 4$.
     * Find the pedestal displaying number `4`, count `4` buzzers to the LEFT (or right), and press that buzzer!
   * **Outcomes**:
     * **Correct Target Buzzer**: **VICTORY!** Victory sound plays, exit door unlocks and swings open!
     * **Wrong Buzzer**: Plays wrong buzzer sound, generates a **NEW equation**, reshuffles buzzer numbers, and teleports player back to checkpoint to try again.

