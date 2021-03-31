# graphics-opengles2

A collection of OpenGL ES2 projects written in C

System Setup
- Developed on Ubuntu 18.04
- Requires libglfw3-dev and libgles2-mesa-dev (sudo apt-get install libglfw3-dev libgles2-mesa-dev)
- Build command is located at the top of the file

<a href="url"><img src="https://user-images.githubusercontent.com/81455676/112710788-11d1c500-8e9a-11eb-8da4-dc66f4137177.png" align="right" height="384" width="512" ></a>
SpaceScene.c
- Simulates orbiting planets, each with their own planetary roataion
- Texture-based rings, two layer color + mask
- Camera pan, rotation and zoom
- Planet textures from: http://planetpixelemporium.com/

<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>

<a href="url"><img src="https://user-images.githubusercontent.com/81455676/112711267-76425380-8e9d-11eb-8739-689a802de7aa.png" align="right" height="384" width="512" ></a>
ObjModelViewer.c
- Wavefront .obj files (https://en.wikipedia.org/wiki/Wavefront_.obj_file)
- Currenlty supports parsing of vertices, multiple textures and faces, (v/vt/f)
- Loading of material file, texture filenames
- 3D scanned Obj models from: https://www.artec3d.com/3d-models/obj 
- Camera pan, rotation and zoom
