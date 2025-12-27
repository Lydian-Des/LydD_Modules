### ::Mom and Dad::



###### A Chaotic attractor similar to Lorenz, but expanded into the 4th dimension

###### With equations by mathematicians S. Dadras and H.R. Momeni

###### Altered further for thickness in the 4th dimension by Me, meaning it's more unstable

###### Meant for it's favorite, slow modulation, in Quad-Triplicate - Can really crawl

##### 

##### Outputs::



**X, Y, Z**, and the fabled **W** outputs, independently for Mom, Dad, and Spawn (didn't want 'Child')

 	- In each Block, the Dimensions are in order as Top Left, Top Right, Bottom Left, Bottom Right - (will add more labeling)

 	- The output may not, per say, go up when the line on the screen goes up. The viewport rotates on its own to give perspective of the whole shape that emerges





##### Inputs \& Parameters::



* **Reset** - Sets everyone back to initial conditions.

 	- Input resets if rises above 1v

* **Synchronize** - Draws Mom and Dad to the current location of Spawn

 	- Input triggers sync if rises above 1v (will put smoothing in eventually, snaps to for now).

* **Separation** - Spreads Mom and Dad apart from Spawn until chaos consumes them all. (the tiniest bit will result in massive changes over enough time)

 	- Works inversely on Mom and Dad, Input accepts +/-5v, and adds to the Knob position.

* **Mom's and Dad's Influence** (colored "M" and "D" with decal between) - Drags Spawn toward respective Parent. At full, Spawn sits right in between its Parents.

 	- Inputs accept +/-5v, take the absolute values, and add to the Knob positions.

* **Haste** - General speed of the system.

 	- is Technically Volt per Octave Tuned, but don't expect a voice out of it.

* **Phase** - skews the internal Timestep in the derivative function, similar to Separation but for time instead of space.

 	- Works inversely on Mom and Dad, Input accepts +/-5v, and adds to the Knob position.

* **Difference** - Alters Speed of Mom and Dad relative to Spawn.

 	- Works inversely on Mom and Dad, Input accepts +/-5v, and adds to the Knob position.

* **Force, Split, Dwell, Hold** - controls the 4 Constants that make up the Strange attractor (this can result in stagnation or runaways, safety guards are in place for voltage extremes)

 	- Inputs accept +/-5v, and add to the Knob position.

* **Scrawl** Button - when engaged(Red), changes the equation for the W coordinates, resulting in a 'scratchy' output from the W's
* **Axis** Switch - changes what Axes are viewed, Viewport does also constantly slowly rotate on its 3 visible axes

 	- Bottom position - normal X and Y with Z as depth, no 4D rotation

 	- Middle - Z horizontal and Y vertical, X as depth, with 4D Rotation of W

 	- Top - W horizontal, X vertical, Y as depth, with 4D Rotation of Z

