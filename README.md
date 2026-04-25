# Antialias

[日本語](README_ja.md)

Antialias is a Windows Adobe Photoshop filter plug-in built with the Adobe Photoshop SDK.

## What It Does

- Applies an antialiasing filter to the selected pixels of the input image.
- Supports Grayscale and RGB images.
- Preserves alpha values.

## Current Scope

The plug-in currently supports:

- RGB images
- RGB images with alpha
- Grayscale images
- Grayscale images with alpha
- Windows x64 build target

The plug-in does not currently target other Photoshop image modes.
32-bit float pixel formats are not supported.

## Project Layout

- [myfilterplugin/common/MyFilter.cpp](myfilterplugin/common/MyFilter.cpp): Photoshop filter implementation.
- [myfilterplugin/common/PiPLs.json](myfilterplugin/common/PiPLs.json): plug-in registration metadata.
- [myfilterplugin/win/MyFilter.vcxproj](myfilterplugin/win/MyFilter.vcxproj): Visual Studio/MSBuild project for the `.8bf` plug-in.
- [Makefile](Makefile): convenience build commands.

## Build Requirements

- Windows
- Visual Studio 2026 with MSVC build tools
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

- [myfilterplugin/build/Debug/Antialias.8bf](myfilterplugin/build/Debug/Antialias.8bf)

## Photoshop Usage

1. Copy `Antialias.8bf` into a Photoshop plug-ins folder.
2. Start or restart Photoshop.
3. Open an RGB or Grayscale image.
4. Create a selection.
5. Run the filter from the `Antialias` category.

Expected behavior:

- RGB document: the selected color values are antialiased.
- Grayscale document: the selected grayscale values are antialiased.
- Alpha channels are preserved.

