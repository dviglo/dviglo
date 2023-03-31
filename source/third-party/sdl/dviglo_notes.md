Должен присутствовать файл VERSION.txt, иначе после каждого коммита CMake будет дёргать git,
генерировать новый файл SDL_revision.h, компилировать SDL, линковать SDL к движку, линковать движок к каждому примеру

```
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt")
  # If VERSION exists, it contains the SDL version
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" SDL_REVISION_CENTER)
  string(STRIP "${SDL_REVISION_CENTER}" SDL_REVISION_CENTER)
else()
  # If VERSION does not exist, use git to calculate a version
  git_describe(SDL_REVISION_CENTER)
  if(NOT SDL_REVISION_CENTER)
    set(SDL_REVISION_CENTER "${SDL3_VERSION}-no-vcs")
  endif()
endif()
```
