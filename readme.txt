CPE 471 Final Project - Scaling Portals

Jared Speck (jspeck@calpoly.edu)

All source files are located in the src directory. There are a couple of options that can be changed if desired:

In main.cpp, at the top, there are #define's for MOVE_SPEED, NUM_OBJECTS, and PORTAL_SIZE. These can be modified to alter the camera movement speed, the number of objects to render, and the size of the portal respectively. Their limitations for acceptable values are listed above them.

In PortalBox.cpp, there is a #define SHOW_REVERSE. This can be changed from 0 to 1 to show the reverse of the built-in rendering optimization (hide close faces of portal box and render the normally occluded faces). This was built in to prove that the occluded faces are actually being culled, and their rendering and warp-detection are not being run.

Controls:
	SPACE - Capture mouse and keyboard input
	Mouse	- Look around
	WASD - Lateral Movement
	M - Change Texture for Scene

Portals teleport the camera, transform the scene, and preview the transforms before entry. They are also slightly colored to show which are paired, and each pair affects the same attribute in the transformation (A->B scales scene widths down, B->A scales scene widths up).