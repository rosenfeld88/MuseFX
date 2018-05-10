# MuseFX
MuseFX is a system featuring hand-gesture controlled audio effects.

The file MuseFX_main.c contains all program functionality. Program functionality described below:
The current MuseFX prototype features a suite of four digital effects: distortion, phase shifter (phaser), flanger, and echo. MuseFX utilizes video input for capturing hand gestures and subsequently controlling the program. The program first calibrates the fist and five finger (open hand) gestures to increase robustness to varying hand sizes. The program uses the convex hull of the hand to recognize hand gestures. The system  also uses the convex hull centroid as a cursor on the effect-specific screens. After calibration, the program flows as follows:

1. System must first be turned on with the five finger gesture (indicator in top-left corner of screen turns green when the system is on). This screen is known as the effect-selection screen.

2. Effects can be selected with the following gestures: 2 - distortion, 3 - phaser, 4 - flanger, 5 - echo. Once on an effect-specific screen the corresponding effect's label is highlighted in blue (D - distortion, P - phaser, F - flanger, E - echo).

3. Effect parameters are modified by holding the 2 gesture and moving the convex hull centroid to the appropriate fader level.

4. The 3 and 4 gestures enable and disable effects, respectively (must be on the effect-specific page).

5. The 5 gesture can be used to return to the effect selection screen from effect-specific pages.

6. The fist can be used to turn the system off at any point in the program.

7. Enabled effects have corresponding labels highlighted in green. 

Gesture recognition is achieved with the following algorithms: skin detection, edge detection, Graham's Scan, and convex hull analysis.

MuseFX is designed for use with the TI DM6437 DSP unit.
