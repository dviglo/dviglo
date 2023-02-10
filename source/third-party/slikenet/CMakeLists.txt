#
# This file was taken from RakNet 4.082.
# Please see licenses/RakNet license.txt for the underlying license and related copyright.
#
#
#
# Modified work: Copyright (c) 2016, SLikeSoft UG (haftungsbeschränkt)
#
# This source code was modified by SLikeSoft. Modifications are licensed under the MIT-style
# license found in the license.txt file in the root directory of this source tree.
#

# Лицензия на изменения:
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT


# Название таргета
set(target_name slikenet)

add_definitions (-DRAKNET_ENABLE_STATIC -D_RAKNET_SUPPORT_NatTypeDetectionServer=0 -D_RAKNET_SUPPORT_UDPForwarder=0 -D_RAKNET_SUPPORT_TwoWayAuthentication=0)
add_definitions (-D_RAKNET_SUPPORT_CloudClient=0 -D_RAKNET_SUPPORT_CloudServer=0 -D_RAKNET_SUPPORT_ConnectionGraph2=0 -D_RAKNET_SUPPORT_NatPunchthroughServer=0)
add_definitions (-D_RAKNET_SUPPORT_RelayPlugin=0 -D_RAKNET_SUPPORT_LibVoice=0 -D_RAKNET_SUPPORT_DynDNS=0 -D_RAKNET_SUPPORT_HTTPConnection2=0 -D_RAKNET_SUPPORT_HTTPConnection=0)
add_definitions (-D_RAKNET_SUPPORT_EmailSender=0 -D_RAKNET_SUPPORT_UDPProxyClient=0 -D_RAKNET_SUPPORT_UDPProxyCoordinator=0 -D_RAKNET_SUPPORT_TeamManager=0 -D_RAKNET_SUPPORT_TeamBalancer=0)
add_definitions (-D_RAKNET_SUPPORT_NatTypeDetectionClient=0 -D_RAKNET_SUPPORT_FullyConnectedMesh2=0 -D_RAKNET_SUPPORT_ConnectionGraph2=0 -D_RAKNET_SUPPORT_FullyConnectedMesh=0 -D_RAKNET_SUPPORT_TelnetTransport=0)
add_definitions (-D_RAKNET_SUPPORT_PacketLogger=0 -D_RAKNET_SUPPORT_DirectoryDeltaTransfer=0 -D_RAKNET_SUPPORT_FileOperations=0 -D_RAKNET_SUPPORT_UDPProxyServer=0)
add_definitions (-D_RAKNET_SUPPORT_ConsoleServer=0 -D_RAKNET_SUPPORT_RakNetTransport=0 -D_RAKNET_SUPPORT_TCPInterface=0 -D_RAKNET_SUPPORT_LogCommandParser=0 -D_RAKNET_SUPPORT_RakNetCommandParser=0)
add_definitions (-D_RAKNET_SUPPORT_Rackspace=0 -D_RAKNET_SUPPORT_PacketizedTCP=0 -D_RAKNET_SUPPORT_RPC4Plugin=0 -D_RAKNET_SUPPORT_Router2=0 -D_RAKNET_SUPPORT_ReplicaManager3=0)
add_definitions (-D_RAKNET_SUPPORT_ReadyEvent=0 -D_RAKNET_SUPPORT_MessageFilter=0 -D_RAKNET_SUPPORT_FileListTransfer=0)

# Check specific function availability which may be missing from different MinGW versions
check_cxx_source_compiles ("#include <sys/types.h>\n#include <sys/socket.h>\n#include <netinet/in.h>\n#include <arpa/inet.h>\nint main() {\n    struct sockaddr_in sa;\n    char str[INET_ADDRSTRLEN];\n    inet_pton(AF_INET, \"192.0.2.33\", &(sa.sin_addr));\n    inet_ntop(AF_INET, &(sa.sin_addr), str, INET_ADDRSTRLEN);\n}" INET_FUNCTIONS_EXISTS_1)

set (ORIG_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ws2_32)
check_cxx_source_compiles ("#include <winsock2.h>\n#include <ws2tcpip.h>\nint main() {\n    struct sockaddr_in sa;\n    char str[INET_ADDRSTRLEN];\n    inet_pton(AF_INET, \"192.0.2.33\", &(sa.sin_addr));\n    inet_ntop(AF_INET, &(sa.sin_addr), str, INET_ADDRSTRLEN);\n}" INET_FUNCTIONS_EXISTS_2)
set (CMAKE_REQUIRED_LIBRARIES ${ORIG_CMAKE_REQUIRED_LIBRARIES})

if (NOT INET_FUNCTIONS_EXISTS_1 AND NOT INET_FUNCTIONS_EXISTS_2 AND NOT HAVE_INET_NTOP)
    add_definitions(-DINET_FUNCTIONS_MISSING=1)
endif()

# Check specific function availability which may be missing from different MinGW versions
check_cxx_source_compiles ("#include <stdio.h>\nint main() { char s[80];\nint x = 1;\nsprintf_s(s, 80, \"Test:%d\", x); }" SPRINTFS_FUNCTION_EXISTS)

if (NOT SPRINTFS_FUNCTION_EXISTS)
    add_definitions(-DSPRINTFS_FUNCTION_MISSING=1)
endif()

# Check specific function availability which may be missing from different MinGW versions
check_cxx_source_compiles ("#include <stdio.h>\nint main() {\nchar s[80];\nint x = 1;\nva_list args = {};\nvsnprintf_s(s, 80, \"Test:%d\", args);}" VSNPRINTFS_FUNCTION_EXISTS)

if (NOT VSNPRINTFS_FUNCTION_EXISTS)
    add_definitions(-DVSNPRINTFS_FUNCTION_MISSING=1)
endif()

if (URHO3D_LIB_TYPE STREQUAL SHARED)
    add_definitions(-D_RAKNET_DLL=1)
endif ()

# Создаём список файлов
file(GLOB_RECURSE source_files Source/*.cpp Source/*.h)

# Создаём статическую библиотеку
add_library(${target_name} STATIC ${source_files})

# Делаем заголовочные файлы доступными библиотеке и таргетам, которые используют текущую библиотеку
target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

if (MINGW)
    # Требуется для компиляции движка в MinGW
    target_link_libraries(${target_name} PRIVATE iphlpapi)
endif ()

if (MINGW AND NOT URHO3D_LIB_TYPE STREQUAL SHARED)
    # Требуется для компиляции статической версии движка в MinGW
    target_link_libraries(${target_name} PRIVATE ws2_32)
endif ()

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source FILES ${source_files})
