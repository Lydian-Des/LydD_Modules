# ::Poppy Fields::

A Fractal Generator turned Sequencer/Stepped Modulator;
A technically infinite interactive topographical map of sequences.
---

##### 

##### Outputs::

* **X CV** - Horizontal location in Sequence, conformed to associated "Range" Switch
* **Y CV** - Vertical location(inverted visually a.k.a. Bottom of screen is Higher CV) conformed to associated "Range" Switch
* **X Shift CV** - one phase Shift Register of X CV with adjustable register step by "Shift" Knob
* **Y Shift CV** - one phase Shift Register of YX CV with adjustable register step by "Shift" Knob - tied thereby to X Shift by location
* **Trigger** outputs for X and Y - taken from associated clock inputs
* **Aux** -- this one has 4 Modes -
   	1) Sum - Simply sums together the X and Y outputs
   	2) Magnitude - Outputs the scalar magnitude (always Positive) of the current steps location
   	3) Centroid - Computes the average scalar value of the entire chosen sequence - this becomes smooth semi-random when modulating the fractal parameters
   	4) Beginning of Cycle - outputs short trigger at beginning of cycle

##### Inputs \& Parameters::

* **Clock** - Button advances Sequence by 1 in both directions
   	-Input Simply determines if input is over 1v, so can be driven by pretty much anything (try a VCO!) - Is the Master Clock
* **Yi-Clock** - Normalled to Clock, but allows the breaking apart of the X and Y axes - ('Yi' just tells you these are spooky complex numbers)
* **Reset** - Button sets step to the chosen start point
   	 -Input resets if rises over 1v
* **Reverse** - Button toggles reverse on and off
   	-Input toggles if rises over 1v
* **Mutagens**(Mut.) - Controls the Z variable in the fractal math, and conceptually mutates the soil to grow different and stranger Flowers
  -Stacked knobs, Outer controls X(horizontal) and Inner controls Yi(vertical)
   	-Inputs accept +/-5v and are added to Knob position - capped at edges of map
* **Soil** - Controls the C variable in the fractal math, and conceptually chooses which flower in the field you pluck by location
   	-Knobs in same arrangement as Mutagens
   	-Inputs accept +/-5v and are added to knob positions - capped at edges of current visible screen (dynamic Zoom and Move)
* **Petals** - controls the exponent in the fractal math, and conceptually presents the recursive nature of the whole thing by showing that the Field itself is a Flower
   	-Inputs accept +/-5v (takes the absolute value of the input plus a small offset) and are added to the Knob - you can go higher than the knob alone with this
* **Start** - chooses the starting point of the sequence, up to 64
   	-Input replaces Knob, and accepts +/-5v, removes negative voltages, and clamps out edges
* **Length** - chooses the length of the sequence, up to 64 (sequence wraps around to 0 at the end)
   	-Input replaces Knob, and accepts +/-5v, removes negative voltages, and clamps out edges
* **Smooth** - slews the X and Y outputs respectively (also affects sum and magnitude in Aux). Max Slew is based on clock speed (dynamic average)
   	-Inputs replace Knobs, and accept +/-5v, removes negative voltages, and clamps out edges
* **Move** - Does what it says, but to the whole screen instead of just the point like Soil
   	-Stacked Knobs in the same configuration
* **Zoom** - Zooms in on the center of the screen, up to 50x (will eventually have direct zoom rather than proportional, 50x will get a lot further then)
* **Fractal** Button- chooses from between 6 different fractals, all variations of the Mandelbrot, with slight tuning offsets
* **Julia** Button- enters Julia mode for whatever fractal has been chosen (either very pretty or mostly blank, they vary)
* **C-Move** Button(up arrow close to "Soil" knob) - takes whatever location has been chosen with the "Soil" Knobs and copies it to the Move knob such that that point is Centered (sets Soil knobs to 0 so sequence stays put)
* **Z-Move** Button(down arrow close to "Mutagens" knob) - takes Move knob location and maps it to the "Mutagens" knob (useful to find a Julia set of a particular location easily)
* **Aux type** Button - cycles through "Aux" types listed above
* **Mirror Y** Button- Flips the Y axis (difference is more pronounced in asymmetrical fractals)
* **Invert** Button- Flips X and Y outputs around 0 (You're playing that Bach upside Down!)
* **Shift** Offset Knob - Stepped, determines how many steps behind the current step the shift registers should pull from
* **Range Switch** X and Y - sets respective ranges for Outputs: 0v -> +2v, -2v -> +2v, -5V -> +5v
* **Secret Button** under the Move knob - chooses visual styles

 	- Blank (No Drawing) for less robust graphics cards

 	- MonoChrome, just black and silvery-white

 	- Flower, a sort of rainbow thing

 	- Buddhabrot, a point map of a set of the sequences that exist, in rainbow

