# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.19

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
CMAKE_COMMAND = /home/l1nkkk/opt/clion-2021.1.1/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /home/l1nkkk/opt/clion-2021.1.1/bin/cmake/linux/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/l1nkkk/project/mime/mysylar

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/l1nkkk/project/mime/mysylar/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/mysylar.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/mysylar.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/mysylar.dir/flags.make

CMakeFiles/mysylar.dir/main.cpp.o: CMakeFiles/mysylar.dir/flags.make
CMakeFiles/mysylar.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/l1nkkk/project/mime/mysylar/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/mysylar.dir/main.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/mysylar.dir/main.cpp.o -c /home/l1nkkk/project/mime/mysylar/main.cpp

CMakeFiles/mysylar.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/mysylar.dir/main.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/l1nkkk/project/mime/mysylar/main.cpp > CMakeFiles/mysylar.dir/main.cpp.i

CMakeFiles/mysylar.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/mysylar.dir/main.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/l1nkkk/project/mime/mysylar/main.cpp -o CMakeFiles/mysylar.dir/main.cpp.s

CMakeFiles/mysylar.dir/io/iomanager.cpp.o: CMakeFiles/mysylar.dir/flags.make
CMakeFiles/mysylar.dir/io/iomanager.cpp.o: ../io/iomanager.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/l1nkkk/project/mime/mysylar/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/mysylar.dir/io/iomanager.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/mysylar.dir/io/iomanager.cpp.o -c /home/l1nkkk/project/mime/mysylar/io/iomanager.cpp

CMakeFiles/mysylar.dir/io/iomanager.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/mysylar.dir/io/iomanager.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/l1nkkk/project/mime/mysylar/io/iomanager.cpp > CMakeFiles/mysylar.dir/io/iomanager.cpp.i

CMakeFiles/mysylar.dir/io/iomanager.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/mysylar.dir/io/iomanager.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/l1nkkk/project/mime/mysylar/io/iomanager.cpp -o CMakeFiles/mysylar.dir/io/iomanager.cpp.s

# Object files for target mysylar
mysylar_OBJECTS = \
"CMakeFiles/mysylar.dir/main.cpp.o" \
"CMakeFiles/mysylar.dir/io/iomanager.cpp.o"

# External object files for target mysylar
mysylar_EXTERNAL_OBJECTS =

mysylar: CMakeFiles/mysylar.dir/main.cpp.o
mysylar: CMakeFiles/mysylar.dir/io/iomanager.cpp.o
mysylar: CMakeFiles/mysylar.dir/build.make
mysylar: thread/libl1nkkkthread.a
mysylar: /usr/local/lib/libglog.so.0.5.0
mysylar: common/libl1nkkkcommon.a
mysylar: CMakeFiles/mysylar.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/l1nkkk/project/mime/mysylar/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable mysylar"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mysylar.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/mysylar.dir/build: mysylar

.PHONY : CMakeFiles/mysylar.dir/build

CMakeFiles/mysylar.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/mysylar.dir/cmake_clean.cmake
.PHONY : CMakeFiles/mysylar.dir/clean

CMakeFiles/mysylar.dir/depend:
	cd /home/l1nkkk/project/mime/mysylar/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/l1nkkk/project/mime/mysylar /home/l1nkkk/project/mime/mysylar /home/l1nkkk/project/mime/mysylar/cmake-build-debug /home/l1nkkk/project/mime/mysylar/cmake-build-debug /home/l1nkkk/project/mime/mysylar/cmake-build-debug/CMakeFiles/mysylar.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/mysylar.dir/depend

