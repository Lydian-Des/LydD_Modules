### ::Simone::



###### A Chaotic attractor by Professor Simone Conradi

###### Altered slightly to achieve movement in time, and turned into a Quad Voice VCO/LFO.

###### And what Pretty Pictures it makes.



##### Outputs::



**X** and **Y** (numbered) - 2D goodness for each of the four voices

**Outputs** (Mix) - pre-mixed output for each axis



##### Inputs \& Parameters::



* **Alpha** - a constant controlling the chaos - shifts the chaos left and right

 	- Inputs accept +/-5v, and add to the Knob.

* **Beta** - a constant controlling the chaos - shifts the chaos up and down

 	- Inputs accept +/-5v, and add to the Knob.

* **Wave** - my personal addition to the equations, functionally like the exponent in a fractal, and 'Squishes' the chaos.

 	- Input accepts +/-5v, and Knob becomes Attenuverter for input.

* **Gain** - determines the radius(amplitude) of the fundamental Circle driving the system.

 	- Input has associated attenuverter

* **Speed\[s]** - It's the Pitch.

 	- Big Knob is global offset, three small knobs control associated offsets up to 1 octave

&nbsp;	- Inputs ordered 1 - 4 from left to right (right above their outputs)

&nbsp;	- each Input is normalled to the one left of it (Yay cascading normals!).

* **Position\[s]** - chooses starting location for the associated Voice

&nbsp;	- due to mAtH, Farther away has to move around the circle 'faster' so adds harmonics.

 	- Inputs are cascaded, so knobs become attenuverters when any input above them is connected. Accepts +/-5v.

* **Pull\[s]** - Chooses the Iterate number being tracked(math mumbo jumbo)

&nbsp;	- essentially pulls it further into the Flow Field laid out by the chaos.

 	- Inputs are cascaded, accept +/-5v, take absolute value, and add to their respective Knob position.

 	- \*\*if anyone's actually reading this far, let me know if a button to turn cascading on and off would be useful.

* **FM** - simple, direct FM

&nbsp;	- Input accepts +/- 5v, and Knob becomes attenuverter.

* **Drive** - Fundamental Drive from VCV VCF - can go upside down!

 	- Input accepts +/- 5v, and Knob becomes attenuverter.

* **Speed** Button- three ranges, from Torpid to Waves to Voice

&nbsp;	- Torpid being quite slow

&nbsp;	- Waves being quick but not quite audible frequencies(mostly) 

&nbsp;	- Voice, which does That good chord organ thing.



