basic doom-style 3D demo in pure software rendering in < 400 LOC.
sdl is only used to blit the finished frame to a window and handle input

heavily inspired by
[bisqwit's qbasic demo](https://youtu.be/HQYsFshbkYw?t=42s)

![](https://media.giphy.com/media/xUOwGeOoHG9u6DX6W4/giphy.gif)

# rationale
most 3D tutorials jump right into transformation matrices, OpenGL etc.
starting from how it was done in the very first 3D games is much simpler

# usage
```gcc -O3 wall.c -lSDL2 -lm -o wall && ./wall```

controls: WASD, mouse (horizontal only)

# license
this is free and unencumbered software released into the public domain.
refer to the attached UNLICENSE or http://unlicense.org/
