#cmakedefine SDL_VENDOR_INFO "@SDL_VENDOR_INFO@"

#ifdef SDL_VENDOR_INFO
#define SDL_REVISION "@SDL_REVISION@ (" SDL_VENDOR_INFO ")"
#else
#define SDL_REVISION "@SDL_REVISION@"
#endif
