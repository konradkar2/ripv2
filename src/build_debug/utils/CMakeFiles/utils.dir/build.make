# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.26

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/out/.local/lib/python3.10/site-packages/cmake/data/bin/cmake

# The command to remove a file.
RM = /home/out/.local/lib/python3.10/site-packages/cmake/data/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/out/Documents/ripv2/src

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/out/Documents/ripv2/src/build_debug

# Include any dependencies generated for this target.
include utils/CMakeFiles/utils.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include utils/CMakeFiles/utils.dir/compiler_depend.make

# Include the progress variables for this target.
include utils/CMakeFiles/utils.dir/progress.make

# Include the compile flags for this target's objects.
include utils/CMakeFiles/utils.dir/flags.make

utils/CMakeFiles/utils.dir/vector.c.o: utils/CMakeFiles/utils.dir/flags.make
utils/CMakeFiles/utils.dir/vector.c.o: /home/out/Documents/ripv2/src/utils/vector.c
utils/CMakeFiles/utils.dir/vector.c.o: utils/CMakeFiles/utils.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/out/Documents/ripv2/src/build_debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object utils/CMakeFiles/utils.dir/vector.c.o"
	cd /home/out/Documents/ripv2/src/build_debug/utils && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT utils/CMakeFiles/utils.dir/vector.c.o -MF CMakeFiles/utils.dir/vector.c.o.d -o CMakeFiles/utils.dir/vector.c.o -c /home/out/Documents/ripv2/src/utils/vector.c

utils/CMakeFiles/utils.dir/vector.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/utils.dir/vector.c.i"
	cd /home/out/Documents/ripv2/src/build_debug/utils && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/out/Documents/ripv2/src/utils/vector.c > CMakeFiles/utils.dir/vector.c.i

utils/CMakeFiles/utils.dir/vector.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/utils.dir/vector.c.s"
	cd /home/out/Documents/ripv2/src/build_debug/utils && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/out/Documents/ripv2/src/utils/vector.c -o CMakeFiles/utils.dir/vector.c.s

utils/CMakeFiles/utils.dir/hashmap.c.o: utils/CMakeFiles/utils.dir/flags.make
utils/CMakeFiles/utils.dir/hashmap.c.o: /home/out/Documents/ripv2/src/utils/hashmap.c
utils/CMakeFiles/utils.dir/hashmap.c.o: utils/CMakeFiles/utils.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/out/Documents/ripv2/src/build_debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object utils/CMakeFiles/utils.dir/hashmap.c.o"
	cd /home/out/Documents/ripv2/src/build_debug/utils && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT utils/CMakeFiles/utils.dir/hashmap.c.o -MF CMakeFiles/utils.dir/hashmap.c.o.d -o CMakeFiles/utils.dir/hashmap.c.o -c /home/out/Documents/ripv2/src/utils/hashmap.c

utils/CMakeFiles/utils.dir/hashmap.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/utils.dir/hashmap.c.i"
	cd /home/out/Documents/ripv2/src/build_debug/utils && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/out/Documents/ripv2/src/utils/hashmap.c > CMakeFiles/utils.dir/hashmap.c.i

utils/CMakeFiles/utils.dir/hashmap.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/utils.dir/hashmap.c.s"
	cd /home/out/Documents/ripv2/src/build_debug/utils && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/out/Documents/ripv2/src/utils/hashmap.c -o CMakeFiles/utils.dir/hashmap.c.s

# Object files for target utils
utils_OBJECTS = \
"CMakeFiles/utils.dir/vector.c.o" \
"CMakeFiles/utils.dir/hashmap.c.o"

# External object files for target utils
utils_EXTERNAL_OBJECTS =

utils/libutils.a: utils/CMakeFiles/utils.dir/vector.c.o
utils/libutils.a: utils/CMakeFiles/utils.dir/hashmap.c.o
utils/libutils.a: utils/CMakeFiles/utils.dir/build.make
utils/libutils.a: utils/CMakeFiles/utils.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/out/Documents/ripv2/src/build_debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C static library libutils.a"
	cd /home/out/Documents/ripv2/src/build_debug/utils && $(CMAKE_COMMAND) -P CMakeFiles/utils.dir/cmake_clean_target.cmake
	cd /home/out/Documents/ripv2/src/build_debug/utils && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/utils.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
utils/CMakeFiles/utils.dir/build: utils/libutils.a
.PHONY : utils/CMakeFiles/utils.dir/build

utils/CMakeFiles/utils.dir/clean:
	cd /home/out/Documents/ripv2/src/build_debug/utils && $(CMAKE_COMMAND) -P CMakeFiles/utils.dir/cmake_clean.cmake
.PHONY : utils/CMakeFiles/utils.dir/clean

utils/CMakeFiles/utils.dir/depend:
	cd /home/out/Documents/ripv2/src/build_debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/out/Documents/ripv2/src /home/out/Documents/ripv2/src/utils /home/out/Documents/ripv2/src/build_debug /home/out/Documents/ripv2/src/build_debug/utils /home/out/Documents/ripv2/src/build_debug/utils/CMakeFiles/utils.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : utils/CMakeFiles/utils.dir/depend
