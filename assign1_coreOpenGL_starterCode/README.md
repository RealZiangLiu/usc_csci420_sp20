# CSCI 420 Assignment 1
* Author: Ziang Liu
* Email: ziangliu@usc.edu

## Toggle
* Translate: hold "t" + mouse.
* Rotate: mouse.
* Scale: hold "shift" + mouse.

## Screenshots
* Saved under "screenshots" in base directory.
* Press "s" to start recording, and press again to stop.
* Taking screenshot every 4 frames to extend time.

## Modes
* Mode 1: Point.
* Mode 2: Wireframe.
* Mode 3: Solid.
* Mode 4: Smooth.
* Mode 5: Solid with wirefr ame overlay.
* Mode 6: Interesting binary mapping.
* Color Mode: Use RGB format input automatically displays in color mode.
* Color Mapping Mode: Pass three parameters to command line to activate color mapping mode. Usage: ./hw1 PATH_TO_HEIGHT_IMAGE PATH_TO_COLOR_IMAGE
Note that the dimensions must match.
* Some RGB images for testing are in the img folder under "hw1-starterCode".

## Extra Credits
* Used Element Arrays and glDrawElements to speed up (works smoothly with the largest file provided).
* Rendered wireframe overlay on top of triangles using glPolygonOffset.
* Mode 6. Not complicated at all but still interesting.
* Using GL_TRIANGLE_STRIP
* Support RGB format
* Implemented color mapping: extract color from one image and map to the other
