#!/bin/bash

# Function to format the time as 0:00.24
format_time() {
    echo "$1" | awk '{print int($1/60)":"sprintf("%02d", int($1%60))"."substr(sprintf("%.3f", $1*100), length(sprintf("%.3f", $1*100))-2, 2)}'
}

# Clean up old binaries
rm -f ./gfx ./gfx-debug

# Create build directory for object files if it doesn't exist
mkdir -p build/objects

# Compile all source files
echo "Starting compilation..."
start_time=$(date +%s.%N)
for file in src/*.cpp; do
    ccache g++ -std=c++20 -Wall -Wextra -g -c "$file" -o "build/objects/$(basename ${file%.cpp}.o)" \
    -Iinclude -I3rdParty/imgui -I3rdParty/stb -I3rdParty/meshoptimizer/src -I/usr/include/eigen3 -I/usr/include/vendor/assimp-install/include
    if [ $? -ne 0 ]; then
        echo "Compilation failed for $file"
        exit 1
    fi
done
end_time=$(date +%s.%N)
compile_time=$(echo "$end_time - $start_time" | bc)
formatted_compile_time=$(format_time $compile_time)
echo "Compile Time: $formatted_compile_time"

# Link object files to create the executable
echo "Starting linking..."
start_time=$(date +%s.%N)
ccache g++ -o gfx build/objects/*.o \
-L. -Llib -L3rdParty/imgui -L/usr/include/vendor/assimp-install/lib \
-L/usr/lib/x86_64-linux-gnu \
-limgui -lglfw -lGL -lGLU -ldl -lX11 -lpthread -lXrandr -lXi -lGLEW -lfmt -lfcl -lccd \
/usr/include/vendor/assimp-install/lib/libassimp.a -lz -lminizip -DGLEW_STATIC
if [ $? -ne 0 ]; then
    echo "Linking failed"
    exit 1
fi
end_time=$(date +%s.%N)
link_time=$(echo "$end_time - $start_time" | bc)
formatted_link_time=$(format_time $link_time)
echo "Link Time: $formatted_link_time"

# Run the executable
echo ""
echo "Running the executable..."
./gfx
if [ $? -ne 0 ]; then
    echo "Execution failed"
    exit 1
fi


