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
include CMakeFiles/rip.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/rip.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/rip.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/rip.dir/flags.make

CMakeFiles/rip.dir/ripd.c.o: CMakeFiles/rip.dir/flags.make
CMakeFiles/rip.dir/ripd.c.o: /home/out/Documents/ripv2/src/ripd.c
CMakeFiles/rip.dir/ripd.c.o: CMakeFiles/rip.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/out/Documents/ripv2/src/build_debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/rip.dir/ripd.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/rip.dir/ripd.c.o -MF CMakeFiles/rip.dir/ripd.c.o.d -o CMakeFiles/rip.dir/ripd.c.o -c /home/out/Documents/ripv2/src/ripd.c

CMakeFiles/rip.dir/ripd.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/rip.dir/ripd.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/out/Documents/ripv2/src/ripd.c > CMakeFiles/rip.dir/ripd.c.i

CMakeFiles/rip.dir/ripd.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/rip.dir/ripd.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/out/Documents/ripv2/src/ripd.c -o CMakeFiles/rip.dir/ripd.c.s

# Object files for target rip
rip_OBJECTS = \
"CMakeFiles/rip.dir/ripd.c.o"

# External object files for target rip
rip_EXTERNAL_OBJECTS =

rip: CMakeFiles/rip.dir/ripd.c.o
rip: CMakeFiles/rip.dir/build.make
rip: librip/liblibrip.a
rip: utils/libutils.a
rip: /usr/lib/x86_64-linux-gnu/librt.a
rip: CMakeFiles/rip.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/out/Documents/ripv2/src/build_debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable rip"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/rip.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/rip.dir/build: rip
.PHONY : CMakeFiles/rip.dir/build

CMakeFiles/rip.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/rip.dir/cmake_clean.cmake
.PHONY : CMakeFiles/rip.dir/clean

CMakeFiles/rip.dir/depend:
	cd /home/out/Documents/ripv2/src/build_debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/out/Documents/ripv2/src /home/out/Documents/ripv2/src /home/out/Documents/ripv2/src/build_debug /home/out/Documents/ripv2/src/build_debug /home/out/Documents/ripv2/src/build_debug/CMakeFiles/rip.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/rip.dir/depend
