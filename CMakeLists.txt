cmake_minimum_required(VERSION 3.13)
project(ProtocoloCronosPP CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Núcleo portátil: regras, cenário, renderizador por software, síntese de áudio (sem API de sistema).
add_library(nucleo STATIC
  src/gfx.cpp src/audio.cpp src/calc.cpp src/dados.cpp src/mundo.cpp src/cinema.cpp src/jogo.cpp)
target_include_directories(nucleo PUBLIC src)
if(NOT MSVC)
  target_compile_options(nucleo PRIVATE -Wall -Wextra)
endif()

add_executable(CronosPP src/plataforma_sdl.cpp)
target_link_libraries(CronosPP nucleo)
set_target_properties(CronosPP PROPERTIES OUTPUT_NAME CronosPP)

if(EMSCRIPTEN)
  # Navegador (PC e celular): SDL2 vem do próprio Emscripten; as fontes vão empacotadas em /assets.
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  target_compile_options(nucleo PUBLIC -sUSE_SDL=2)
  target_compile_options(CronosPP PRIVATE -sUSE_SDL=2)
  target_link_options(CronosPP PRIVATE
    -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web
    --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
    --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html)
  set_target_properties(CronosPP PROPERTIES OUTPUT_NAME index)
else()
  find_package(SDL2 REQUIRED)
  target_link_libraries(CronosPP SDL2::SDL2)
  if(WIN32)
    set_target_properties(CronosPP PROPERTIES WIN32_EXECUTABLE ON)
  endif()
  # as fontes ficam em assets/ ao lado do executável
  add_custom_command(TARGET CronosPP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CronosPP>/assets)

  enable_testing()
  add_executable(teste tests/teste.cpp)
  target_link_libraries(teste nucleo)
  add_test(NAME teste COMMAND teste WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
