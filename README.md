# ProgrammablePulling
Programmable pulling experiments (based on OpenGL Insights "Programmable Vertex Pulling" article by Daniel Rakos)

# Sample image

![Sample image](http://i.imgur.com/MCfTAny.png)

# Explanation of parameters

## Flexibility

* Fixed-function: Passes per-vertex inputs using built-in vertex array object and vertex shader "in" variables
* Programmable: gl_VertexID comes from index buffer passed through VAO as usual. gl_VertexID is used to index the vertex data.
* Fully programmable: Non-indexed draw is used, and gl_VertexID is used to manually read the VertexID from the index buffer. Presumably circumvents [post-transform cache](https://www.khronos.org/opengl/wiki/Post_Transform_Cache).

## Layout

* AoS: Position data is stored as XYZ XYZ XYZ in memory (or sometimes XYZW for padding). VAO implementation passes a vec3 as usual. texture/image/SSBO implementations either load all four XYZW in one load (the W is padding), or they do three separate 1-float loads (to avoid needing padding.)
* SoA: Position data is stored as XXX YYY ZZZ in memory. VAO implementation passes 3 separate single float attributes. texture/image/SSBO implementations do three loads of a single float, one load from each buffer.

**Note**: Some people think I'm talking about PTN PTN PTN vs. PPP TTT NNN when I say AoS/SoA, but I'm actually talking about AoS/SoA at the next lower level, between the XYZ of individual vec3s.

For specifics, the shaders used in each configuration are independent, and in the shaders/ folder of the project, so they can be consulted to see the exact syntax used.

There is also a mode that measures PTN PTN PTN with VAOs (not programmable). In this benchmark, there are only positions and normals, as used in all tests. No texcoords.

# Installation

Check the "Releases" section of the GitHub repo if you want to just download the exe and run it. Requires Windows 10.

Alternatively, you can build it yourself from the Visual Studio solution (on Windows), or by running the build.sh script on Linux.
