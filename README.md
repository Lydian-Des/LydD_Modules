A collection of modules with a focus on the strange and mathematical,
somewhat to teach myself to Code.

Currently Hogs GPU drawing time a bit.

Module Descriptions and Parameters:

::Poppy Fields::

A Fractal Generator turned Sequencer/Stepped Modulator;
A technically infinite interactive topographical map of sequences.
 
Outputs::

- X and Y CV respectively, with associated Range switch from 0 / +2, -2 / +2, and -5 / +5.

- Trigger(gate) outputs for X and Y  taken from associated clock inputs.

- Aux Output -- this one has 4 Modes -
 	1) Sum - Simply sums together the X and Y outputs.
 	2) Magnitude - Outputs the scalar magnitude (always Positive) of the current steps location.
 	3) Centroid - Computes the average scalar value of the entire chosen sequence.
 	4) Beginning of Cycle - does that. (very short trigger right now).
  
Inputs & Paramters::

- Clock - Button advances Sequence by 1.
 	-Input Simply determines if input is over 1v, so can be driven by pretty much anything. Is Master Clock.
  
- Yi-Clock - Normalled to Clock, but allows the breaking apart of the X and Y axes.  ('Yi' just tells you these are spooky     complex numbers).

- Reset - Button sets step to the chosen start point,
 	 -If Input goes over 1v, resets to sequence start, and holds it there until input drops below 1v.
   
- Reverse - Button toggles reverse on and off ( might fix input in a similar way ).
 	-Input Reverses sequence as long as input is over 1v.
  
- Mutagens(Mut.) - Controls the Z variable in the fractal math, and conceptually mutates the soil to grow different and         stranger Flowers.
  -Stacked knobs, Outer controls X(horizontal) and Inner controls Yi(vertical).
 	-nputs accept =/-5v and are added to Knob position.

- Soil - Controls the C variable in the fractal math, and conceptually chooses which flower in the field you pluck by          location.
 	-Knobs in same arrangement as Mutagens.
 	-Inputs accept +/-5v and are added to knob positions.

- Petals - controls the exponent in the fractal math, and conceptually presents the recursive nature of the whole thing by     showing that the Field itself is a Flower.
 	-Inputs accept +/-5v (takes the absolute value of the input) and are added to the Knob.

- Start - chooses the starting point of the sequence, up to 128.
 	-Input replaces Knob, and accepts +/-5v but simply cuts out negative voltages.

- Length - chooses the length of the sequence, up to 128 (sequence wraps around to 0 at the end).
 	-Input replaces Knob, and accepts +/-5v but simply cuts out negative voltages.

- Smooth - slews the X and Y outputs respectively (also affects sum and magnitude in Aux). Max Slew is based on clock speed.
 	-Inputs replace Knobs, and accept +/-5v but simply cut out negative voltages.

- Move - Does what it says, but to the whole screen instead of just the point like Soil.
 	-Stacked Knobs in the same configuration.

- Zoom - Zooms in on the center of the screen, up to 50x.
- secret button under the Move knob - chooses visual styles, including Buddhabrot, which is a literal show of all(most of)     the sequences the exist.

- Fractal - chooses from between 6 different fractals, all variations of the Mandelbrot (one day they may all have their      own tuning attributes or something to make them more unique).

- Julia - enters Julia mode for whatever fractal has been chosen (very pretty).

- (up arrow close to Soil knob) - takes whatever location is chosen and copies it to the Move knob such that that point is     Centered (sets Soil knobs to 0 so sequence stays put).

- (down arrow close to Z knob) - takes Move knob location and maps it to the Z knob (useful to find a Julia set of a           particular location easily).

- Aux type Button - cycles through Aux types listed above.

- Mirror Y - Flips the Y axis (difference is more pronounced in asymmetrical fractals, but needs some fixing anyway).

- Invert - Flips X and Y outputs around 0 (You're playing that Bach upside Down!).

::Mom and Dad::

A Chaotic attractor similar to Lorenz, but expanded into the 4th dimension with additional Scrolls by mathematicians S Dadras and HR Momeni, and altered further for thickness in the 4th dimension by Me. Meant for it's favorite, slow modulation, in Triplicate. Can really crawl.

Outputs::

X, Y, Z, and the fabled W outputs independently for Mom, Dad, and Spawn (didn't want 'Child').
 - In each Block, the Dimensions are in order as Top Left, Top Right, Bottom Left, Bottom Right. (will add more labeling)

Inputs & Parameters::

- Reset - Sets everyone back to initial conditions.
 	- If Input goes over 1v it holds everyone at ~0v until input drops below 1v.

- Synchronize - This one Works Right! (will implement this for other button simulating inputs).
 	- if Input goes from < 1v to > 1v, Draws Mom and Dad to Spawn, then leaves them alone until the next time the voltage     rises over 1v. (will put smoothing in eventually, snaps to for now).

- Separate - Spreads Mom and Dad apart from Spawn until chaos consumes them all.
 	- Works inversely on Mom and Dad, Input accepts +/-5v, and adds to the Knob position.

- Mom's and Dad's Influence(currently unlabeled, below Haste, each on the appropriate side) - Drags Spawn toward respective     Parent. At full, Spawn sits right in between its Parents.
 	- Inputs accept +/-5v, take the absolute values, and add to the Knob positions.

- Haste - General speed of the system.
 	- is Technically Volt per Octave Tuned, but don't expect a voice out of it.

- Phase - Needs a better name, but skews the internal dt in the derivative function, similar to separation but for time         instead of space.
 	- Works inversely on Mom and Dad, Input accepts +/-5v, and adds to the Knob position.

- Difference - Alters Speed of Mom and Dad.
 	- Works inversely on Mom and Dad, Input accepts +/-5v, and adds to the Knob position.

- Force, Split, Dwell, Hold - controls the 4 Constants that make up the Strange attractor (this can result in stagnation or     runaways, safety guards are in place for voltage extremes). These may also get different names.
 	- Inputs accept +/-5v, and add to the Knob position.

::Simone::
A Chaotic attractor by Professor Simone Conradi, altered slightly to achieve movement in time, and turned into a Quad Voice Oscillator/LFO.
And what Pretty Pictures it makes.
No theme for this yet, so naming is straightforward.

Outputs::

X and Y outputs for each of Four voices, each pair arranged vertically, ordered 1 - 4 from left to right.
Mix outputs, X's on the left, Y's on the right.

Inputs & Parameters::

- A and B - the constants controlling the chaos
 	- Inputs accept +/-5v, and add to the Knob.

- Wave - my personal addition to the equations, functionally like the exponent in a fractal, and 'Squishes' the chaos.
 	- Input accepts +/-5v, and Knob becomes Attenuverter for input.

- T rad - determines the radius(amplitude) of the fundamental Circle driving the system.
 	- Input accepts +/-5v, takes the absolute value, and Knob both adds to and attenuates the result.

- Speed(s) - It's the Pitch.
 	- Big Knob is global offset, Inputs ordered 1 - 4 from left to right (right above their outputs), and each Input is         normalled to the one above it (Yay cascading normals!).

- Position(s) - ordered 1 - 4 from top to bottom, chooses starting location for the associated Voice. due to the                mathematics, Farther away has to move around the circle 'faster' so adds harmonics.
 	- Inputs are cascaded, and add to their respective Knob value. Accepts +/-5v.

- Intensity(ies) - Chooses the Iterate number being tracked, essentially pulling it further into the Flow Field laid out by     the chaos.
 	- Inputs accept +/-5v, take absolute value, and add to their respective Knob position (will add cascading).
 	- if anyone's actually reading this far, let me know if a button to turn cascading on and off would be useful.

- FM(currently unlabeled, topmost knob on the right) - simple, direct FM
 	- Input accepts +/- 5v, and Knob becomes attenuverter.

- PM(also unlabeled, below FM, wow you get both) - simple PM on the fundamental Circle.
 	- Input accepts +/- 5v, and Knob becomes attenuverter.

- Range Mode(unlabeled button in the top right) - three ranges, from Torpid to Waves to Voice, Torpid being quite slow,       Waves being quick but not quite audible frequencies, and of course Voice doing That.

::"Torus"?::

Personally created toroidal (and otherwise) equations producing a somewhat strange 2-OP FM Voice with an Additive feel. Also DISTORTION.
This one's UI is still truly in early stages, no labels or nothing.

Outputs::

Center Bottom, only things with any color near them.
X on the left, Y on the right, Z below. A Torus is 3D after all.

Inputs & Parameters::

- Big Pitch - controls pitch of outer winding. Input top of left-hand triangle of jacks.

- Little Pitch - controls pitch of inner winding. Input Bottom-left of left-hand triangle.

- FM -  also attenuverts input. Input Bottom-left of left-hand triangle.

- Drive - its distortion, like the drive in the Fundamentals VCF.

- Radial Difference - controls difference between inner and outer radii.

- Big Windings -weighted average of number of times around outer circle for one rotation of base sin.

- Little Windings -weighted average of number of times around inner circle   for one rotation of base sin.

- Equation switch - chooses equations, joyously named 'Electron', 'Folding', and 'Toroid'.

- LFO 1 -turns Big Pitch (outer circle) into LFO.

- LFO 2 - does the same for Little Pitch (inner circle).




