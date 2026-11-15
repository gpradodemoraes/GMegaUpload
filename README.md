# esqueleto

## GIT HOOKS! ##

rodar o comando abaixo e se certificar de que clang-format está no seu PATH

git config core.hooksPath .githooks


## SUBMODULOS ##

git submodule init

git submodule update

git submodule update --init --recursive .\third_party\mega-sdk\
git submodule update --init --recursive ./third_party/mega-sdk

## INSTALAR NO LINUX ##
para gerar a build em Linux e instalar:
cmake --preset RLS_Linux
cmake --build out/build/RLS_Linux/
sudo cmake --install out/build/RLS_Linux --prefix /usr/local


## PARA CONSEGUIR GERAR O compile_commands.json PARA USO NO VSCODE ##

Somente o Ninja consegue gerar o compile_commands.json

Então precisa iniciar o cmd com esse comando para ele usar o v142

cmd /k '"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -vcvars_ver=14.29'

Além disso, precisa fazer essa modificação dentro o MEGA SDK:

diff --git a/cmake/modules/sdklib_variables.cmake b/cmake/modules/sdklib_variables.cmake
index d3e7d3548..5fc27c075 100644
--- a/cmake/modules/sdklib_variables.cmake
+++ b/cmake/modules/sdklib_variables.cmake
@@ -8,7 +8,9 @@ if (NOT WIN32)
     set(USE_CPPTHREAD 0)
 else()
     set(USE_CPPTHREAD 1)
-    set(CMAKE_GENERATOR_TOOLSET "v142")
+       if(CMAKE_GENERATOR MATCHES "Visual Studio")
+               set(CMAKE_GENERATOR_TOOLSET "v142")
+       endif()
 endif()

 include(detect_host_architecture)


