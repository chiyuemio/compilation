# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/llvm_10/homework2/homework2

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/llvm_10/homework2/homework2/build

# Include any dependencies generated for this target.
include CMakeFiles/llvmassignment.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/llvmassignment.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/llvmassignment.dir/flags.make

CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.o: CMakeFiles/llvmassignment.dir/flags.make
CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.o: ../LLVMAssignment.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/llvm_10/homework2/homework2/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.o -c /home/llvm_10/homework2/homework2/LLVMAssignment.cpp

CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/llvm_10/homework2/homework2/LLVMAssignment.cpp > CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.i

CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/llvm_10/homework2/homework2/LLVMAssignment.cpp -o CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.s

# Object files for target llvmassignment
llvmassignment_OBJECTS = \
"CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.o"

# External object files for target llvmassignment
llvmassignment_EXTERNAL_OBJECTS =

llvmassignment: CMakeFiles/llvmassignment.dir/LLVMAssignment.cpp.o
llvmassignment: CMakeFiles/llvmassignment.dir/build.make
llvmassignment: /usr/local/llvm10ra/lib/libLLVMCore.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMIRReader.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMPasses.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMCodeGen.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMTarget.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMipo.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMIRReader.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMAsmParser.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMInstrumentation.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMScalarOpts.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMAggressiveInstCombine.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMInstCombine.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMVectorize.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMBitWriter.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMLinker.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMTransformUtils.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMAnalysis.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMObject.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMBitReader.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMMCParser.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMMC.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMDebugInfoCodeView.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMDebugInfoMSF.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMTextAPI.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMProfileData.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMCore.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMBinaryFormat.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMRemarks.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMBitstreamReader.a
llvmassignment: /usr/local/llvm10ra/lib/libLLVMSupport.a
llvmassignment: /usr/lib/x86_64-linux-gnu/libz3.so
llvmassignment: /usr/local/llvm10ra/lib/libLLVMDemangle.a
llvmassignment: CMakeFiles/llvmassignment.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/llvm_10/homework2/homework2/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable llvmassignment"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/llvmassignment.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/llvmassignment.dir/build: llvmassignment

.PHONY : CMakeFiles/llvmassignment.dir/build

CMakeFiles/llvmassignment.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/llvmassignment.dir/cmake_clean.cmake
.PHONY : CMakeFiles/llvmassignment.dir/clean

CMakeFiles/llvmassignment.dir/depend:
	cd /home/llvm_10/homework2/homework2/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/llvm_10/homework2/homework2 /home/llvm_10/homework2/homework2 /home/llvm_10/homework2/homework2/build /home/llvm_10/homework2/homework2/build /home/llvm_10/homework2/homework2/build/CMakeFiles/llvmassignment.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/llvmassignment.dir/depend
