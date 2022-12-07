#/bin/bash
EXECUTABLE_NAME="BombSearcher"
BUILD_DIR="build"
GAME_DIR="${PWD}/game"
SDL_LIB_DIR="/usr/local/lib"
SDL_LIB="libSDL2"
SDL_INCLUDE_DIR="/usr/local/include/SDL2"
IMGUI_DIR="${PWD}/imgui"
STB_DIR="${PWD}/stb"
GLAD_DIR="${PWD}/glad"
GAME_C_SRCS=$(find "${GAME_DIR}" | grep -v "windows" | grep "\.c$")
GAME_CPP_SRCS=$(find "${GAME_DIR}" | grep -v "windows" | grep "\.cpp$")
GAME_INCLUDE_DIRS="-I${GAME_DIR}/include -I${GLAD_DIR}/include -I${IMGUI_DIR} -I${IMGUI_DIR}/backends -I${STB_DIR} -I${SDL_INCLUDE_DIR}"

OTHER_FLAGS="-DDEBUG -DTEST -DPLATFORM_GL_MAJOR_VERSION=3 -DPLATFORM_GL_MINOR_VERSION=3"
COMPILER_FLAGS="-c -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -fPIC -g -pg"

LINKER_FLAGS="-pg $(sdl2-config --libs --cflags) -ldl"

while [[ $# -gt 0 ]]; do
    case $1 in
        -l|--libs)
            BUILD_LIBS=1
            shift
            ;;
        -*|--*)
            echo "Unknown option $1"
            exit 1
            ;;
        *)
            echo "Unknown arg $1"
            exit 1
            ;;
    esac
done

mkdir -p "${BUILD_DIR}"
pushd "${BUILD_DIR}"

if [ -n "$BUILD_LIBS" ]
then
    echo "compiling glad"
    g++ ${GLAD_DIR}/*.c ${COMPILER_FLAGS} "-I${GLAD_DIR}/include" "${OTHER_FLAGS}" || exit 1

    echo "compiling imgui"
    g++ \
        ${IMGUI_DIR}/*.cpp \
        ${IMGUI_DIR}/backends/imgui_impl_sdl.cpp \
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp \
        ${COMPILER_FLAGS} \
        "-I${IMGUI_DIR}" \
        "-I${IMGUI_DIR}/backends" \
        "-I${SDL_INCLUDE_DIR}" || exit 1
fi

echo "compiling game .c files:"
echo "${GAME_C_SRCS}"
gcc ${GAME_C_SRCS} ${COMPILER_FLAGS} ${GAME_INCLUDE_DIRS} ${OTHER_FLAGS} || exit 1
echo "compiling game .cpp files:"
echo "${GAME_CPP_SRCS}"
g++ ${GAME_CPP_SRCS} ${COMPILER_FLAGS} ${GAME_INCLUDE_DIRS} ${OTHER_FLAGS} || exit 1

echo "linking .o files:"
echo *.o
g++ *.o ${LINKER_FLAGS} -o ${EXECUTABLE_NAME} || exit 1
echo "done"

popd
mv "${BUILD_DIR}/${EXECUTABLE_NAME}" .
cp "${SDL_LIB_DIR}/${SDL_LIB}.so" .
