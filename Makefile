
main:

build-debug:
	msbuild myfilterplugin\win\MyFilter.vcxproj /p:Configuration=Debug /p:Platform=x64 /m

build-release:
	msbuild myfilterplugin\win\MyFilter.vcxproj /p:Configuration=Release /p:Platform=x64 /m

clean:
	if exist myfilterplugin\build rmdir /s /q myfilterplugin\build

install: # need admin privileges
	copy "myfilterplugin\build\Release\Antialias.8bf" "C:\Program Files\Adobe\Adobe Photoshop 2026\Plug-ins\"

install2:
	copy "myfilterplugin\win\build\build\Release\Antialias.8bf" "C:\Program Files\Adobe\Adobe Photoshop 2026\Plug-ins\"