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
   * **EquationPool**: Pre-populated with rich BODMAS and radical/exponent equations. You can add your own custom equations anytime!

---

## 4. How the Troll Progression Plays Out in Game

1. **Attempt 1 (Zero Clues)**:
   * Screen displays the complex equation (e.g. `(8 ÷ 2 × (1 + 3) - 14) = ?`).
   * Player calculates answer ($2$) and presses Buzzer #2.
   * **Result**: *WRONG!* Buzzer buzzer sounds, player gets punished.

2. **Attempt 2 (Summation Hint)**:
   * Screen: *"DID YOU REALLY THINK IT WAS THAT EASY? A true mathematician knows you must do the SUMMATION of the answer! (2 + 2 = 4). Go press buzzer #4!"*
   * Player presses Buzzer #4.
   * **Result**: *WRONG!* Consequence triggers.

3. **Attempt 3 (Dev Oopsie)**:
   * Screen: *"DEV OOPSIE! The developer forgot to write the entire hint! Here is the missing part: 'If the answer is 4, press the 4th buzzer from the 4th labeled buzzer!' (Hurry, press #8!)"*
   * Player presses Buzzer #8.
   * **Result**: *WRONG!* Consequence triggers.

4. **Attempt 4 (The Roast)**:
   * Screen: *"ARE YOU SERIOUS?! Even a calculator from 1980 has better logic than you! You're just blindly following random hints on a screen! Stop guessing and use your brain!"*
   * Player presses any buzzer.
   * **Result**: *WRONG!* Consequence triggers.

5. **Attempt 5 (Fake Developer Help)**:
   * Screen: *"DEV APOLOGY: The developer felt bad for you. I highlighted the correct buzzer with a bright glowing green light so you can just escape already. Go press the glowing buzzer!"*
   * A fake buzzer starts glowing brightly in neon green.
   * Player presses the glowing buzzer.
   * **Result**: **PRANK!** Plays cartoon laughter / airhorn sound!

6. **Attempt 6 (The "Fooled You!" & True Solution)**:
   * Screen: *"FOOLED YOU! HAHAHA! There are no shortcuts in 'WHOOPS!' Now stop looking for cheats and DO MATH PROPERLY! Solve the equation and press the TRUE answer buzzer!"*
   * Player now presses the actual true answer buzzer (#2).
   * **Result**: **VICTORY!** The exit door unlocks and swings open!
