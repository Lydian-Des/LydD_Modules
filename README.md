A collection of modules with a focus on the strange and mathematical,
somewhat to teach myself to Code.

Screen equipped Modules Can Currently Hog GPU drawing time a bit.



New Modules for Christmas!



**Concierge**: a master clock equipped for most any time signature you could desire,

 	including some that don't technically exist.

 	- accepts CV (0v = 120bpm) and External Clock(assumes quarter note)

 	  external clock takes running average of last 4 beats, so it takes a little bit to catch on

 	- outputs clocks from 16th notes to more than 16 measures, with lots in between

 	  as well as Phase outputs for the longer clocks



**Ledger**: Any concierge must have a ledger! - Quintuple normalled logic Blocks

 	- once both inputs for a block are connected, it becomes independent, and trickles down

 	- if the top input (of any but the first) is not connected it is normalled to NOT the top input above it

 	- if the bottom input (same) is not connected,

 	  each operation receives in its place the result of same operation from above(C AND D becomes C AND (A AND B))



**Dobbs**: Dual - Dual Envelope Generator, named after a Mountain I loved as a child

 	- Comes with Pretty Lights!!

 	- each Peak (ha) comes with cv over attack and decay as well as a fast mode that makes it real snappy

 	- each Peak comes with a twin Peak (hA) with its own attack and decay

 	- twin peaks (HA) have cv controllable start time offset relative to the main envelope

 	- each pair of Peaks has cv control over their shape(log - exponential) and an ASR Mode

 	- all four total envelopes also come with end of cycle triggers



**Onceler**: ever need something to just.. wait a while? well with the onceler you chop down Trees!

 	- feed it a gate(an Axe) and wait for it to cut the tree down.

 	- when its down the Tree will scream(give you gate) for as long as you like

 	- you can also Speak to the tree, and once you cut it down it will Talk back what you say

 	- start over anytime with Redon't. or Don't!

 	- comes with several modes for all your various waiting and screaming needs



**Shear**: A true 12 band IIR Comb Filter with insertable feedback loop, in stereo

 	- cutoff tuned 1v/oct

 	-its IIR so you can also give it a trigger and listen to it ping away

 	- emphasize even or odd harmonics, or skew the bands for more enharmonic percussive sounds

 	- shove a reverb in the feedback loop, that'll go fine trust me!

 	- better yet, put another Shear in there. chain 5!





New Modules For Summer!



Domain: a 4x4 Switching Unit with a Big'n in the middle that flips all of em

&#x09;- left side is 2 in -- 1 out,

&#x20;	- right side is 1 in -- 2 out,

&#x09;- right switch gate input is normalled to the left input next to it

&#x20;	- each left/right pair shares a mode switch between momentary(only high while gate is high) or latching

&#x09;- Big Boy Switch flips all switches from whatever they are, and has its own mode switch



Reflection: a Windowed Reflector, or generally a wavefolder of sorts

&#x09;- Width determines the voltage span of the windows

&#x09;- Height determines dc offset of windows

&#x20;	- input is reflected off of those windows, e.g. small window big voltage = much folding but low gain

&#x09;- approach to window boundaries can be curved in either direction

&#x09;- all in duplicate, with inverted outputs for fun(inversion happens before windowing)



Tonnet: Quad Quantizer with specials

&#x20;	- the layout is in a version of the Tonnetz format -- up/down is 5ths, straight over one is a half step

&#x09;- 8 banks to switch between that remember themselves

&#x20;	- 16 channel polyphonic input allows you to play the quantizer with your keys

&#x09;- "Page" mode allows for discrete note inputs, e.g. specific pitches that do not repeat across octaves,

&#x09;- also playable with the poly input for lovely jazz chords

&#x20;	- octaves get a bit confusing there with the layout



Seethe: multiband Saturator with curves and whatnot

&#x20;	- lowpass, bandpass, and highpass with their own ins and outs that sum to ALL

&#x09;- ALL input also feeds to band outputs for submixing

&#x09;- Drive past 12oclock introduces feedback in that band

&#x20;	- saturation curves go from full to half to inverted to semi-sinusoidal with Morph

&#x09;- band cutoffs are movable with Curve, which also slightly alters the saturation curve



Canyon: Stereo Delay stuffed with things

&#x09;- delay times from just about a Pitch to 8 seconds,

&#x09;- granular crossfading design, so no pitch shifting when modulating delay times

&#x20;	- dual Mono, Ping-Pong, and Cross-Feed modes(cross feeds both back into both at different proportions)

&#x20;	- Freezable and Reversible with looping length controlled by feedback knob -

&#x09;	 from a waveform size to roughly 2 seconds	

&#x09;- when frozen delay times scrub the buffer

&#x09;- also Pitchable, which actually sounds pretty clean when the buffer is frozen

&#x09;- its really 10 channels, with 5 for each side

&#x09;- Scatter slowly introduces each of those extra channels to the feedback loop

&#x09;	 (theyll keep playing out for a bit once you turn it down)

