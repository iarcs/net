include(FetchContent)

set(BUILD_SHARED_LIBS OFF)
set(SIMDJSON_ALLOW_DOWNLOADS OFF)
set(SIMDJSON_COMPETITION OFF)
set(SIMDJSON_GOOGLE_BENCHMARKS OFF)
set(SIMDJSON_CXXOPTS OFF)
set(SIMDJSON_ENABLE_DOM_CHECKPERF OFF)
set(SIMDJSON_ENABLE_FUZZING OFF)
set(SIMDJSON_DEVELOPMENT_CHECKS OFF)
set(SIMDJSON_DEVELOPER_MODE OFF)
FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG  tags/v3.1.2
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(simdjson)

get_target_property(simdjson_cflags simdjson COMPILE_OPTIONS)
list(REMOVE_ITEM simdjson_cflags -Wall -Wextra -Werror)
set_target_properties(simdjson PROPERTIES COMPILE_OPTIONS "${simdjson_cflags}")
set_target_properties(parse_nonumberparsing PROPERTIES COMPILE_OPTIONS "${simdjson_cflags}")
