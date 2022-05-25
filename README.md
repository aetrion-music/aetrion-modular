# aetrion-modular
aetrion VCV Rack modules

# Chord Vault
![Image of Chord Vault module](../images/chordvault.png)

Chord Vault is a chord sequencer with a little twist.
Record polyphonic cv/gate pairs into up to 16 steps and choose from a one of 8 sequencing modes.
Its main purpose is to be an easy way for playing chords on a midi keyboard and "saving" the notes for playback.
But of course you can hook up anything that sends gates and cv.

## Quick Start
This quick example shows the **basic routing** for polyphonic note input from a midi keyboard. If you understand this basic concept you can of course work without midi input.
Note that the default polyphony is set to 5 (can be changed in right click menu).

[Download Quick Start](../examples/ChordVault_QuickStart.vcvs?raw=true)

1. Make sure the module is in RECORD State (**REC**, top button).
2. Play a chord. You'll hear it while keys are pressed. Notice how the module auto advances to the next step once you let go of the keys (step number increases by one).
3. After you played in a few chords, switch module to PLAY state (**PLAY**, top button).
4. Start the CLOCKED Module by pressing the run button
5. Your chords will play in order (first SEQ Mode). Use SEQ MODE button to play around with different modes

*Quick Note on setup for the MIDI > CV module: Use right click menu to set number of polyphony channels "going into" chord vault and also make sure to select polyphony mode option called "reset". This ensures a clean input for each step.*

## Panel

![Image of Chord Vault controls](../images/chordvault_controls.png)
![Image of Chord Vault module](./images/Chord%20Vault%20Manual%20Numbered%20Panel.png)

1. **REC / PLAY State:** Switches between both states, with REC being used to input notes and PLAY turning on the sequencer (if a clock signal is present)
	- REC Status behavior: as long as gate is high, all notes played are recorded into the current step (up to max. number of poly channels defined), the module auto advances to the next step when gate is low.
	- PLAY Status behavior: when CLOCK is unpatched or not running you can manually go through each step by turning the STEP knob
2. **STEP knob and display:** display shows currently selected or active step, knob automatically moves to further indicate the current step.
	- STEP knob behavior: manually select a step in REC status to record into it (or replace step content), manually select a step to audition steps (works in PLAY mode too when clock is stopped)
3. **STEP CV input:** Used exclusively with the corresponding SEQ Mode "CV" to change step number based on CV input (0-5V default, see SEQ mode list below)
4. **SEQ button and LEDs:** cycles through different SEQ modes (explained below), LED shows currently active mode
5. **LENGTH knob and display:** display shows currently selected length, knob manually selects a sequence length independent of how many steps have content recorded into them
6. **LENGTH CV input:** change sequence length via CV (0-5V default, with 0V being 1 step and 5 being 16 steps seq length)
7. **RESET Button and input:** Resets the sequencer to the first step (which may not be step 1 depending on SEQ mode), input uses trigger or gate (xV???)
8. **GATE input:** Polyphonic gate input, number of channels needs to match number of max. polyphony channels used in ChordVault or you'll not get all notes into the step as expected. if you have a mono gate, use a module like (BOGAUDIO MODULE) to "duplicate" the gate across the correct number of channels. Incoming gate length should be 1ms or more, but is not relevant to playback (see clock input)
9. **V/OCT input:** Polyphonic CV (1V/Oct) input, records incoming CV while gate input is high 
10. **CLOCK Input:** Advances the sequencer to the "next" step or retriggers gate for current step (depending on SEQ mode), clock is only active in PLAY status
11. **Gate Output:** Polyphonic Gate signal to attach to a voice or envelope generator. Gate length is dependent of clock pulse length (play around, gate is high as long as clock input is high)
12. **V/OCT Output:** - Polyphonic CV (1V/Oct) output with notes ordered from low to high. If you have a chord that doesn't use all poly channels, notes from the previous chords may be carrying over to help with longer env release times (not cutting notes off).

## SEQUENCE MODES
*Notice that step selection is **always dependent on the sequence length** set by the length knob (or length cv).*

### Default Modes (blue led)
1. **Forward ( > )** - Plays steps in regular order (low to high numbers).
2. **Backward ( < )** - Plays steps in reverse order (high to low numbers).
3. **Random ( RND )** - Randomized step selection (without step repeats).
4. **CV Control ( CV )** - Special mode that let's you select steps via CV. Needs a **CV input signal** patched to the jack and uses unipolar 0-5V range to select from steps *with respect to the seq length*. 
**CV Control clock behavior:** Sample & Hold, so you may retrigger a current step with an additional clock pulse, while step CV input is unchanged. Changes in step CV input are sent to output "together" with the clock. 
### Special Modes (pink led)
Press and hold the SEQ button for 1 second to access these modes. You can also access them using the right click menu which also shows their full names.

5. **Skip ( > )** - Plays steps in regular order but randomly skips a step. Chance to skip is controlled by STEP CV input. With a range of 0-10V for 0-100% chance. If no cable is patched to Step CV input, the module has a 20% chance to skip a step.
6. **PingPong ( < )** - Plays steps in regular order and then backwards in reverse order, without repeating the first or last step.
7. **Shuffle ( RND )** - Randomized step selection, but selects a new step (that hasn't been played) until every step has been selected once and then starts over.
8. **Glide ( CV )** - Same as CV Control, but V/Oct output sends out pitch changes immediately while gate is high, without waiting for the next clock.



## Right Click Menu Options & Advanced Features

**option** - description

### Advanced Features

**Step Knob CV start point offset** - description


### Bypass

When ChordVault is bypassed all outputs stay at 0V.

## Patch Examples

### EXAMPLE 1

![Image of Example 1](../images/example_1.png)

[Download Example 1](../examples/ChordVault_Example1.vcvs?raw=true)

In this example...

### EXAMPLE 2

![Image of Example 2](../images/example_2.png)

[Download Example 2](../examples/ChordVault_Example2.vcvs?raw=true)

In this example...


## License

The aetrion brand and logo are copyright (c) 2022 Mirko Melcher (m@aetrion-music.com), all rights reserved.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

All **graphics** in the `res` and `res_src` directory are copyright © 2022 Mirko Melcher (m@aetrion-music.com).
All **graphics** in the `res` and `res_src`, other than the aetrion logo, are licensed under [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).
