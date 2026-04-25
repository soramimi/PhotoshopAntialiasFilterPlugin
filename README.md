# MyFilter

This project is a sample for building custom Adobe Photoshop filter plug-ins.

MyFilter is a Windows Adobe Photoshop filter plug-in built with the Adobe Photoshop SDK.

## What It Does

- Inverts the selected pixels of the input image.
- For Grayscale images, it inverts the grayscale value.
- For RGB images, it inverts the RGB color channels.
- For layers with transparency, it preserves alpha values and only inverts color channels.
- Selection masking is handled by Photoshop's filter pipeline.

## Current Scope

The plug-in currently supports:

- RGB images
- Grayscale images
- Grayscale images with alpha
- RGB images with alpha
- Windows x64 build target

The plug-in does not currently target other Photoshop image modes.

## Project Layout

- [myfilterplugin/common/MyFilter.cpp](myfilterplugin/common/MyFilter.cpp): Photoshop filter implementation.
- [myfilterplugin/common/PiPLs.json](myfilterplugin/common/PiPLs.json): plug-in registration metadata.
- [myfilterplugin/win/MyFilter.vcxproj](myfilterplugin/win/MyFilter.vcxproj): Visual Studio/MSBuild project for the `.8bf` plug-in.
- [Makefile](Makefile): convenience build commands.

## Build Requirements

- Windows
- Visual Studio 2022 with MSVC build tools
- Adobe Photoshop SDK unpacked in this workspace under `pluginsdk`

This workspace already includes [vcvars.bat](vcvars.bat) to initialize the MSVC command-line environment.

## Expected Directory Layout

The build assumes that this repository is laid out like this:

```text
.
├── Makefile
├── README.md
├── vcvars.bat
├── myfilterplugin/
│   ├── common/
│   └── win/
└── pluginsdk/
    ├── photoshopapi/
    │   ├── photoshop/
    │   └── pica_sp/
    └── samplecode/
        └── common/
            └── includes/
```

Important points:

- `myfilterplugin/win/MyFilter.vcxproj` uses relative include paths.
- The Adobe Photoshop SDK must be available directly under [pluginsdk](pluginsdk).
- The project expects Photoshop SDK headers in:
	- [pluginsdk/photoshopapi/photoshop](pluginsdk/photoshopapi/photoshop)
	- [pluginsdk/photoshopapi/pica_sp](pluginsdk/photoshopapi/pica_sp)
- The project also uses shared SDK sample headers from:
	- [pluginsdk/samplecode/common/includes](pluginsdk/samplecode/common/includes)

If the Photoshop SDK is unpacked into a different folder name or depth, the include paths in [myfilterplugin/win/MyFilter.vcxproj](myfilterplugin/win/MyFilter.vcxproj) must be updated.

## Build Commands

From the workspace root, initialize the MSVC environment first:

```bat
call vcvars.bat
```

Then use one of the following commands.

Build Debug:

```bat
msbuild myfilterplugin\win\MyFilter.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
```

Build Release:

```bat
msbuild myfilterplugin\win\MyFilter.vcxproj /p:Configuration=Release /p:Platform=x64 /m
```

Or use the Makefile targets:

```bat
nmake build-debug
```

```bat
nmake build-release
```

```bat
nmake clean
```

There is also an `install` target in [Makefile](Makefile), but it writes to `C:\Program Files\...` and requires administrator privileges.

## Build Output

The generated Photoshop plug-in is:

- [myfilterplugin/build/Debug/MyFilter.8bf](myfilterplugin/build/Debug/MyFilter.8bf)

## Photoshop Usage

1. Copy `MyFilter.8bf` into a Photoshop plug-ins folder.
2. Start or restart Photoshop.
3. Open an RGB or Grayscale image.
4. Create a selection.
5. Run the filter from the `MyFilter` category.

Expected behavior:

- RGB document: the selected color values are inverted.
- Grayscale document: the selected grayscale values are inverted.
- Alpha channels are preserved.
- Selection boundaries are applied by Photoshop, not by custom mask blending inside the plug-in.

## Validation Performed

- The Photoshop plug-in project in [myfilterplugin/win/MyFilter.vcxproj](myfilterplugin/win/MyFilter.vcxproj) was built successfully with MSBuild.
- The `.8bf` output was generated without build warnings in the final validation pass.

## Notes

- The plug-in is implemented as a minimal filter with no custom UI.
- The current sample demonstrates grayscale, RGB, and alpha-aware packed/interleaved output handling for Photoshop filters.
- Unsupported image modes return a Photoshop filter mode error instead of attempting partial behavior.