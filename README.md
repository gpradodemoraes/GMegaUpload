# esqueleto

## GIT HOOKS! ##

rodar o comando abaixo e se certificar de que clang-format está no seu PATH

git config core.hooksPath .githooks


## SUBMODULOS ##

git submodule init

git submodule update

## INSTALAR NO LINUX ##
para gerar a build em Linux e instalar:
cmake --preset RLS_Linux
cmake --build out/build/RLS_Linux/
sudo cmake --install out/build/RLS_Linux --prefix /usr/local
