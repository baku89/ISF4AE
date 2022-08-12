# GLSLCanvas4AE

![](./README/screenshot.gif)

After Effects plug-in to run codes written in [The Book of Shaders Editor](http://editor.thebookofshaders.com/) format.

**NOTE: This code is working in progress and has many problems yet. I'm focusing on developing the plug-in only for the latest AE on macOS at first and then will prepare for other environments. It would be helpful if you contribute to this project for supporting Windows.**

## How to Build

1. Download [After Effects Plug-in SDK 2022 Mac - Oct 2021](https://adobe.io/after-effects/).

2. Clone this repo and put it into `(AESDK)/Examples/Effect/(repo)`.

3. The project has a dependency on GLFW and glm. Install them with `brew install glfw glm`.

4. Open the Xcode project and build it. The binary will automatically be copied under `/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/`.

## License

This plug-in has been published under an MIT License. See the included [LICENSE file](./LICENSE).

## Acknowledgments

- [mizt](https://github.com/mizt): He gave me lots of useful advices for handling AESDK.
- [cryo9](https://github.com/cryo9): His working-in-progress [AE-GLSL](https://github.com/cryo9/AE-GLSL) hinted me for the fundamental idea of this plug-in.
- [Patricio Gonzalez Vivo](https://github.com/patriciogonzalezvivo): One of my reason for being hooked on shaders is his stunning project [The Book of Shaders](http://thebookofshaders.com).
- [PixelsWorld](https://aescripts.com/pixelsworld/): Scripting environment that supports Lua, GLSL, and shadertoy.com on AE.
