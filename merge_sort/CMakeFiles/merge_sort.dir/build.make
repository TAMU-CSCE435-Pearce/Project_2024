# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.12

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /sw/eb/sw/CMake/3.12.1/bin/cmake

# The command to remove a file.
RM = /sw/eb/sw/CMake/3.12.1/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort

# Include any dependencies generated for this target.
include CMakeFiles/merge_sort.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/merge_sort.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/merge_sort.dir/flags.make

CMakeFiles/merge_sort.dir/merge_sort.cpp.o: CMakeFiles/merge_sort.dir/flags.make
CMakeFiles/merge_sort.dir/merge_sort.cpp.o: merge_sort.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/merge_sort.dir/merge_sort.cpp.o"
	/sw/eb/sw/GCCcore/8.3.0/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/merge_sort.dir/merge_sort.cpp.o -c /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort/merge_sort.cpp

CMakeFiles/merge_sort.dir/merge_sort.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/merge_sort.dir/merge_sort.cpp.i"
	/sw/eb/sw/GCCcore/8.3.0/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort/merge_sort.cpp > CMakeFiles/merge_sort.dir/merge_sort.cpp.i

CMakeFiles/merge_sort.dir/merge_sort.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/merge_sort.dir/merge_sort.cpp.s"
	/sw/eb/sw/GCCcore/8.3.0/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort/merge_sort.cpp -o CMakeFiles/merge_sort.dir/merge_sort.cpp.s

# Object files for target merge_sort
merge_sort_OBJECTS = \
"CMakeFiles/merge_sort.dir/merge_sort.cpp.o"

# External object files for target merge_sort
merge_sort_EXTERNAL_OBJECTS =

merge_sort: CMakeFiles/merge_sort.dir/merge_sort.cpp.o
merge_sort: CMakeFiles/merge_sort.dir/build.make
merge_sort: /scratch/group/csce435-f24/Caliper/caliper/lib64/libcaliper.so.2.11.0
merge_sort: /sw/eb/sw/impi/2019.9.304-iccifort-2020.4.304/intel64/lib/libmpicxx.so
merge_sort: /sw/eb/sw/impi/2019.9.304-iccifort-2020.4.304/intel64/lib/release/libmpi.so
merge_sort: /lib64/librt.so
merge_sort: /lib64/libpthread.so
merge_sort: /lib64/libdl.so
merge_sort: CMakeFiles/merge_sort.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable merge_sort"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/merge_sort.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/merge_sort.dir/build: merge_sort

.PHONY : CMakeFiles/merge_sort.dir/build

CMakeFiles/merge_sort.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/merge_sort.dir/cmake_clean.cmake
.PHONY : CMakeFiles/merge_sort.dir/clean

CMakeFiles/merge_sort.dir/depend:
	cd /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort /scratch/user/nikki_2021/ParrellComputing_Project_2024/Merge_Sort/CMakeFiles/merge_sort.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/merge_sort.dir/depend

