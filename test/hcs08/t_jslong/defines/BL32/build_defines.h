/**********************************************************************
*
*  This file contains #defines that change the compilation of the build
*  There should be a different build_defines.h for each build type
*  Cannot be located in a sub-folder in the source directory.
*
***********************************************************************/

#ifndef BUILD_DEFINES
 #define BUILD_DEFINES

 #define BL32
  #define APP_RESERVED_FLASH_START_ADDRESS 0x8000
  #define APP_PROGRAM_FLASH_START_ADDRESS  0x8400


#endif //BUILD_DEFINES
