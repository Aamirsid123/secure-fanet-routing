set(target_prefix scratch_)

find_package(OpenSSL REQUIRED)

function(create_scratch source_files)
  list(LENGTH source_files number_sources)
  if(number_sources EQUAL 0)
    return()
  endif()

  # Find the file containing main()
  set(scratch_src)
  foreach(source_file ${source_files})
    file(READ ${source_file} source_file_contents)
    string(REGEX MATCHALL "main[(| (]" main_position "${source_file_contents}")
    if(CMAKE_MATCH_0)
      list(APPEND scratch_src ${source_file})
    endif()
  endforeach()

  list(LENGTH scratch_src scratch_src_len)

  if(${scratch_src_len} EQUAL 0)
    message(FATAL_ERROR "No main() function found in scratch source files: ${source_files}")
  elseif(${scratch_src_len} GREATER 1)
    message(FATAL_ERROR "Multiple main() functions found: ${scratch_src}")
  endif()

  # Get directory and file names
  get_filename_component(scratch_dirname ${scratch_src} DIRECTORY)
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" scratch_dirname "${scratch_dirname}")
  string(REPLACE "/" "_" scratch_dirname "${scratch_dirname}")

  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14.0")
    get_filename_component(scratch_name ${scratch_src} NAME_WLE)
  else()
    get_filename_component(scratch_name ${scratch_src} NAME)
    string(FIND "${scratch_name}" "." ext_position REVERSE)
    if(${ext_position} EQUAL -1)
      message(FATAL_ERROR "No extension found in source: ${scratch_src}")
    else()
      string(SUBSTRING "${scratch_name}" 0 ${ext_position} scratch_name)
    endif()
  endif()

  # Set executable target name
  set(target_prefix scratch_)
  if(scratch_dirname)
    set(target_prefix scratch${scratch_dirname}_)
  endif()

  # Get relative output path
  get_filename_component(scratch_src ${scratch_src} ABSOLUTE)
  get_filename_component(scratch_absolute_directory ${scratch_src} DIRECTORY)
  string(REPLACE "${PROJECT_SOURCE_DIR}" "${CMAKE_OUTPUT_DIRECTORY}" scratch_directory ${scratch_absolute_directory})

  # Build executable
  build_exec(
    EXECNAME ${scratch_name}
    EXECNAME_PREFIX ${target_prefix}
    SOURCE_FILES "${source_files}"
    LIBRARIES_TO_LINK "${ns3-libs}" "${ns3-contrib-libs}" OpenSSL::SSL OpenSSL::Crypto
    EXECUTABLE_DIRECTORY_PATH ${scratch_directory}/
  )
endfunction()

# Build scratch examples
file(GLOB single_source_file_scratches CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/[^.]*.cc)
foreach(scratch_src ${single_source_file_scratches})
  create_scratch(${scratch_src})
endforeach()

# Build scratch subdirectories
file(GLOB scratch_subdirectories CONFIGURE_DEPENDS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(entry ${scratch_subdirectories})
  if(IS_DIRECTORY ${entry})
    if(EXISTS ${entry}/CMakeLists.txt)
      add_subdirectory(${entry})
    else()
      file(GLOB scratch_sources CONFIGURE_DEPENDS ${entry}/[^.]*.cc)
      create_scratch("${scratch_sources}")
    endif()
  endif()
endforeach()
