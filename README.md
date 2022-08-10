# glslCanvas4AE

![](./Assets/screenshot.gif)

After Effects plug-in to run codes written at [The Book of Shaders Editor](http://editor.thebookofshaders.com/).

**NOTE: This code is working in progress and has many problems yet. I'm focusing on building the plug-in only for the latest AE on macOS at first and then will prepare for other environments. It would be awesome if you contribute to this project to support Windows!**

## How to Build

1. Download [After Effects Plug-in SDK 2022 Mac - Oct 2021](https://adobe.io/after-effects/).

2. Put the SDK folder in the same directory as the cloned folder of this repository and rename it to “SDK” like below:
    ```
    (parent folder)
        ├ glslCanvas4AE
        └ SDK
    ```

Move `(parent folder)/glslCanvas4AE/Mac/build/GLSLCanvas.plugin` to `/Applications/Adobe After Effects 2022/Plug-ins/GLSLCanvas.plugin`. 

## License

This plug-in has been published under an MIT License so far. See the included [LICENSE file](./LICENSE).

## Acknowledgments

 - [mizt](https://github.com/mizt): He gave me some detailed advice for handling AESDK.
 - [cryo9](https://github.com/cryo9): His working-in-progress [AE-GLSL](https://github.com/cryo9/AE-GLSL) hinted me for the fundamental idea of this plug-in.
 - [Patricio Gonzalez Vivo](https://github.com/patriciogonzalezvivo): One of my reason for being hooked on shaders is his stunning project [The Book of Shaders](http://thebookofshaders.com). 
 - [PixelsWorld](https://aescripts.com/pixelsworld/): Scripting environment that supports Lua, GLSL, and shadertoy.com on AE.
