# Include before project()/find_package(), including in parent CMake projects,
# so imported dependencies use their release libraries for this configuration.
set(CMAKE_MAP_IMPORTED_CONFIG_RELEASEWITHTRACY Release CACHE STRING
    "Imported dependency configuration for ReleaseWithTracy")

get_property(_moonlight_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_moonlight_multi_config)
  if(NOT DEFINED CMAKE_CONFIGURATION_TYPES)
    if(DEFINED ENV{CMAKE_CONFIGURATION_TYPES})
      set(CMAKE_CONFIGURATION_TYPES "$ENV{CMAKE_CONFIGURATION_TYPES}")
    else()
      set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)
    endif()
  endif()
  list(APPEND CMAKE_CONFIGURATION_TYPES ReleaseWithTracy)
  list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
      "Available build configurations" FORCE)
endif()
unset(_moonlight_multi_config)

# Call after project() has initialized the compiler's platform-specific flags.
# RelWithDebInfo supplies release optimization, NDEBUG, and debugger symbols.
function(moonlight_initialize_tracy_flags)
  foreach(_language C CXX)
    if(DEFINED CMAKE_${_language}_FLAGS_RELWITHDEBINFO AND
        NOT CMAKE_${_language}_FLAGS_RELEASEWITHTRACY)
      set(CMAKE_${_language}_FLAGS_RELEASEWITHTRACY
          "${CMAKE_${_language}_FLAGS_RELWITHDEBINFO}" CACHE STRING
          "${_language} flags for ReleaseWithTracy" FORCE)
      mark_as_advanced(CMAKE_${_language}_FLAGS_RELEASEWITHTRACY)
    endif()
  endforeach()
  foreach(_kind EXE SHARED MODULE STATIC)
    if(DEFINED CMAKE_${_kind}_LINKER_FLAGS_RELWITHDEBINFO AND
        NOT CMAKE_${_kind}_LINKER_FLAGS_RELEASEWITHTRACY)
      set(CMAKE_${_kind}_LINKER_FLAGS_RELEASEWITHTRACY
          "${CMAKE_${_kind}_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING
          "${_kind} linker flags for ReleaseWithTracy" FORCE)
      mark_as_advanced(CMAKE_${_kind}_LINKER_FLAGS_RELEASEWITHTRACY)
    endif()
  endforeach()
endfunction()
