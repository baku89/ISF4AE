# glslCanvas4AE

![](./Assets/screenshot.gif)

After Effects plug-in to run codes written at [The Book of Shaders Editor](http://editor.thebookofshaders.com/).

**NOTE: This codes is working in progress and has many problems yet. I'm focusing on building the plug-in only for latest AE on macOS at first, and then prepare for other environment. It would be awesome if you contribute this project to support Windows!**

## How to Build

1. Download [After Effects Plug-in SDK 2022 Mac - Oct 2021](https://adobe.io/after-effects/).

2. Put the SDK folder at the same directory as cloned folder of this repository and rename it to "SDK" like below:
    ```
    (parent folder)
        ├ glslCanvas4AE
        └ SDK
    ```

Move `(parent folder)/glslCanvas4AE/Mac/build/GLSLCanvas.plugin` to `/Applications/Adobe After Effects 2022/Plug-ins/GLSLCanvas.plugin`. 

## License

This plugin is published under a MIT License so far. See the included [LICENSE file](./LICENSE).

## Achknowledgements

 - [mizt](https://github.com/mizt): He gave me some detailed advice for coding AE plug-in.
 - [cryo9](https://github.com/cryo9): His working-in-progress [AE-GLSL](https://github.com/cryo9/AE-GLSL) hinted me for this plug-in.
 - [Patricio Gonzalez Vivo](https://github.com/patriciogonzalezvivo): One of my reason for being hooked on shaders is his stunning project [The Book of Shaders](http://thebookofshaders.com). 
