project(${QFUSION_SERVER_NAME})

include_directories(${ZLIB_INCLUDE_DIR} ${CURL_INCLUDE_DIR})

file(GLOB SERVER_HEADERS
    "*.h"
	"../gameshared/q_*.h"
	"../gameshared/anorms.h"
	"../gameshared/config.h"	
	"../qcommon/*.h"
	"../angelwrap/qas_public.h"
	"../game/g_public.h"
	"../server/*.h"
)

file(GLOB SERVER_SOURCES
	"../qcommon/asyncstream.cpp"
	"../qcommon/autoupdate.cpp"
	"../qcommon/cm_*.cpp"
	"../qcommon/compression.cpp"
	"../qcommon/configstringstorage.cpp"
	"../qcommon/base64.cpp"
    "../qcommon/bsp.cpp"
    "../qcommon/patch.cpp"
    "../qcommon/common.cpp"
    "../qcommon/files.cpp"
	"../qcommon/glob.cpp"
	"../qcommon/half_float.cpp"
    "../qcommon/cmd.cpp"
    "../qcommon/mem.cpp"
    "../qcommon/net.cpp"
    "../qcommon/net_chan.cpp"
    "../qcommon/msg.cpp"
    "../qcommon/cvar.cpp"
    "../qcommon/dynvar.cpp"
    "../qcommon/library.cpp"
	"../qcommon/md5.cpp"
	"../qcommon/mmcommon.cpp"
	"../qcommon/mmlocalstorage.cpp"
	"../qcommon/mmquery.cpp"
	"../qcommon/mmrating.cpp"
	"../qcommon/mmreliablepipe.cpp"
    "../qcommon/maplist.cpp"
    "../qcommon/svnrev.cpp"
    "../qcommon/snap_demos.cpp"
    "../qcommon/snap_write.cpp"
	"../qcommon/snap_tables.cpp"
    "../qcommon/ascript.cpp"
    "../qcommon/anticheat.cpp"
    "../qcommon/wswcurl.cpp"
	"../qcommon/wswfs.cpp"
    "../qcommon/cjson.cpp"
    "../qcommon/threads.cpp"
    "../qcommon/steam.cpp"
	"../qcommon/q_trie.cpp"
    "*.cpp"
    "../gameshared/q_*.cpp"
	"../../third-party/sqlite-amalgamation/sqlite3.c"
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    file(GLOB SERVER_PLATFORM_SOURCES 
        "../win32/win_fs.cpp"
        "../win32/win_net.cpp"
        "../win32/win_sys.cpp"
        "../win32/win_console.cpp"
        "../win32/win_time.cpp"
        "../win32/win_lib.cpp"
        "../win32/win_threads.cpp"
        "../win32/conproc.cpp"
    )

    # Link Ole32.lib for CoCreateGUID()
    set(SERVER_PLATFORM_LIBRARIES "ws2_32.lib" "winmm.lib" "Ole32.lib")
    set(SERVER_BINARY_TYPE WIN32)
else()
    file(GLOB SERVER_PLATFORM_SOURCES 
        "../unix/unix_fs.cpp"
        "../unix/unix_net.cpp"
        "../unix/unix_sys.cpp"
        "../unix/unix_console.cpp"
        "../unix/unix_time.cpp"
        "../unix/unix_lib.cpp"
        "../unix/unix_threads.cpp"
    )

    # Link libuuid
    set(SERVER_PLATFORM_LIBRARIES "pthread" "dl" "m" "uuid")
    set(SERVER_BINARY_TYPE "")
endif()

if (MSVC)
	set_source_files_properties("../qcommon/cm_trace_sse42.cpp" PROPERTIES COMPILE_FLAGS "/arch:AVX")
	set_source_files_properties("../../third-party/sqlite-amalgamation/sqlite3.c" PROPERTIES COMPILE_FLAGS "/fp:precise")
else()
	set_source_files_properties("../qcommon/cm_trace_sse42.cpp" PROPERTIES COMPILE_FLAGS "-msse4.2")
	set_source_files_properties("../qcommon/cm_trace_avx.cpp" PROPERTIES COMPILE_FLAGS "-mavx")
	set_source_files_properties("../../third-party/sqlite-amalgamation/sqlite3.c" PROPERTIES COMPILE_FLAGS "-fno-fast-math")
endif()

add_executable(${QFUSION_SERVER_NAME} ${SERVER_BINARY_TYPE} ${SERVER_HEADERS} ${SERVER_SOURCES} ${SERVER_PLATFORM_SOURCES})
add_dependencies(${QFUSION_SERVER_NAME} game)
target_link_libraries(${QFUSION_SERVER_NAME} PRIVATE ${CURL_LIBRARY} ${ZLIB_LIBRARY} ${SERVER_PLATFORM_LIBRARIES})
qf_set_output_dir(${QFUSION_SERVER_NAME} "")

set_target_properties(${QFUSION_SERVER_NAME} PROPERTIES COMPILE_DEFINITIONS "DEDICATED_ONLY")