include_guard(DIRECTORY)

# Native vertical canvas: pure model logic lives in a small static library so it
# can be unit tested without Qt or a running OBS instance.
add_library(obs-frontend-vertical-core STATIC)
add_library(OBS::frontend-vertical-core ALIAS obs-frontend-vertical-core)

target_sources(
  obs-frontend-vertical-core
  PRIVATE utility/vertical/VerticalModel.cpp
  PUBLIC utility/vertical/VerticalModel.hpp
)

target_include_directories(obs-frontend-vertical-core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(obs-frontend-vertical-core PUBLIC OBS::libobs)
set_target_properties(obs-frontend-vertical-core PROPERTIES FOLDER frontend POSITION_INDEPENDENT_CODE ON)

target_link_libraries(obs-studio PRIVATE OBS::frontend-vertical-core)
