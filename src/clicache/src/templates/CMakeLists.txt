cmake_minimum_required(VERSION 3.4)
project(nativeclient.clicache.templates)

# Set the .NET Target Framework (Note: This should match the build for gfclicache.)
set (DOTNET_TARGET_FRAMEWORK_VERSION "v4.5.2")


foreach(var CMAKE_CURRENT_SOURCE_DIR CMAKE_SOURCE_DIR CMAKE_BINARY_DIR CMAKE_CURRENT_BINARY_DIR)
  file(TO_NATIVE_PATH ${${var}} ${var}_NATIVE)
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Templates.csproj.in ${CMAKE_BINARY_DIR}/clicache/templates/Templates.csproj)  

include_external_msproject(
      Templates   ${CMAKE_BINARY_DIR}/clicache/templates/Templates.csproj
      TYPE FAE04EC0-301F-11D3-BF4B-00C04F79EFBC)
