## Controller Housing

### Ordering information

The housing is assembled from two separate pieces that are produced by 3D printing, painted black, and then screwed together. Files for 3D printing are located in `design/housing`. 

* The `feather-nephel-housing.stl` file prints an enclosed box for the printed circuit board with a square hole for the photodetector. 

* The `250-ml-band.stl` file prints a band that wraps around a 250 ml bottle, with holes for the photodetector and the LED. The `500-ml-band.stl` file prints a band that fits a 500 ml bottle instead.

These files can be printed in PLA plastic on widely available 3D printers (we use an Ultimaker 2+) or fabricated by 3D printing services.

### Equipment

* Small phillips screwdriver

### Consumables
* Krylon Ultra-flat Black 1602 spray paint

  This ultra-flat black spray-paint is highly absorbing in the near-infrared wavelength used to measure turbidity. Many other visually black materials reflect near-IR light.

* M2 x 8mm pan head phillips screw, 316 stainless: McMaster-Carr 90116A015

* M2 hex nut, 316 stainless: McMaster-Carr 94150A305	

  Assembling the housing and mounting the PCB requires eight screws and nuts.

### Assembly

The controller housing is printed as two separate components, a "band" that wraps around the growth chamber and a "box" that houses the detector circuit board. 

1. Remove any support material from the printed parts.

1. Spray paint the band and the box flat black, paying particular attention to the holes for the LED and the photodiode. Dry overnight.

1. Mount the PCB in its housing

   1. Insert the detector circuit board into the box, with the photodiode on the bottom passing through the square opening on the box and into the round hole in the band.

   1. Align the mounting holes on the detector circuit board with the mounting holes on the box.

   1. Insert M2 x 8mm screws through at least two mounting holes on the circuit board, passing through the box to protrude on the outside.

      * Some mounting holes are hard to access -- it isn't necessary to use all four screws.

   1. Thread M2 hex nuts onto the screws outside the box and tighten.

1. Attach the band to the PCB housing

   1. Fit the band against the box. 

      * The square protrusion from the band should align with the square hole on the box. 

      * The two flat arms extending from the band with screw holes should align with screw holes on the box.
   
   1. Insert M2 x 8mm screws through the holes in the band into the box.

   1. Thread M2 hex nuts onto the screws and tighten.
   
1. Mount the LED in the band

   1. ZZZ