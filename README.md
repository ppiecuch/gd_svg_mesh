Vector Graphics for Godot.

* This is an experiment
* It's written in C++ 11, so it's editor-only
* All vector graphics can be converted into `MeshInstance2D`s
* I have very little time to work on this

Live Demo:

https://www.youtube.com/watch?v=EsTSf5dytbs&feature=youtu.be

# Install and build

* Check out this repo as `git clone --recurse-submodules https://github.com/poke1024/godot_vector_graphics`
* Rename the whole repo folder to `vector_graphics` and move it into your Godot `/modules` folder (i.e. as `/modules/vector_graphics`).
* Build godot using `scons platform=your_platform svg=patch`.
* The `svg=patch` part in the `scons` build instructs the `vector_graphics` module to apply a patch, so that you can drag and drop SVGs as vector graphics into Godot (see the SVG Import Patch section below to see what exactly is patched).

# Basic usage in Godot

Drag and drop an SVG into the 2d canvas.

Or: add a new VGPath node in your scene (under a Node2D).

In the inspector, set its Renderer to a new VGAdaptiveRenderer. Now, in
the toolbar at the top, click on the circle. Drag and drop on the canvas
to draw a vector circle.

Select a VGPath and double click onto it (while having the arrow tool
selected) to see and edit control points and curves.

You can add new control points by clicking on a curve.

Select a control point and hit the delete key to remove it.

Rendering quality can be changed by editing the VGAdaptiveRenderer's
quality setting (lower number means less triangles). You can do this
interactively in the Godot editor.

# Reference

 * check https://github.com/castle-games/castle-client/blob/master/mobile/android/love/src/jni/ghost-extensions/tove2d for some up-to-date source code config
 * https://github.com/godot-extended-libraries/godot_vector_graphics.git for updats and extensions (Lottie importer)
