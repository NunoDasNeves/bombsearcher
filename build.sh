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

LINKER_DEBUG_FLAGS="-pg"
DEBUG_FLAGS="-DDEBUG -DTEST -g -pg"

OTHER_FLAGS="-DPLATFORM_GL_MAJOR_VERSION=3 -DPLATFORM_GL_MINOR_VERSION=3"
COMPILER_FLAGS="-c -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -fPIC"

LINKER_FLAGS="$(sdl2-config --libs --cflags) -ldl"

VERSION=1.0

BUILD_GLAD=0
BUILD_IMGUI=0
BUILD_GAME=0
NO_LINK=0
DEBUG=0

usage () {
    echo "USAGE: $0 [--debug] [--nolink] [--all] [--glad] [--imgui] [--game] [OUT_DIR]"
    echo "Builds (--all) by default, otherwise compile and link any of glad, imgui, game."
    echo "Produces built game files and tarball in OUT_DIR (\"\$PWD/out\" by default)."
    echo "Specify --nolink to disable the above - only compile .o files"
    echo "Specify --debug to build with debugger info, gprof info, and #defines DEBUG and TEST"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --all)
            BUILD_GLAD=1
            BUILD_IMGUI=1
            BUILD_GAME=1
            shift
            ;;
        --debug)
            DEBUG=1
            shift
            ;;
        --nolink)
            NO_LINK=1
            shift
            ;;
        --glad)
            BUILD_GLAD=1
            shift
            ;;
        --imgui)
            BUILD_IMGUI=1
            shift
            ;;
        --game)
            BUILD_GAME=1
            shift
            ;;
        -*|--*)
            echo "Unknown option \"$1\""
            usage
            exit 1
            ;;
        *)
            if [ -n "$OUT_DIR" ]; then
                echo "Too many positional args; OUT_DIR already specified: \"$OUT_DIR\""
                usage
                exit 1
            fi
            OUT_DIR="$1"
            shift
            ;;
    esac
done

if [ -z "$OUT_DIR" ];
then
    OUT_DIR="$PWD/out"
fi

# if none specified, build all
if [ "$BUILD_GLAD" -eq "0" ] && [ "$BUILD_IMGUI" -eq "0" ] && [ "$BUILD_GAME" -eq "0" ];
then
    BUILD_GLAD=1
    BUILD_IMGUI=1
    BUILD_GAME=1
fi

if [ "$DEBUG" -eq "1" ]; then
    COMPILER_FLAGS="$COMPILER_FLAGS $DEBUG_FLAGS"
    LINKER_FLAGS="$LINKER_FLAGS $LINKER_DEBUG_FLAGS"
fi

mkdir -p "${BUILD_DIR}"
pushd "${BUILD_DIR}" > /dev/null

if [ "$BUILD_GLAD" -eq "1" ]
then
    echo "compiling glad"
    time g++ ${GLAD_DIR}/*.c ${COMPILER_FLAGS} "-I${GLAD_DIR}/include" "${OTHER_FLAGS}" || exit 1
fi

if [ "$BUILD_IMGUI" -eq "1" ]
then
    echo "compiling imgui"
    time g++ \
        ${IMGUI_DIR}/*.cpp \
        ${IMGUI_DIR}/backends/imgui_impl_sdl.cpp \
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp \
        ${COMPILER_FLAGS} \
        "-I${IMGUI_DIR}" \
        "-I${IMGUI_DIR}/backends" \
        "-I${SDL_INCLUDE_DIR}" || exit 1
fi

if [ "$BUILD_GAME" -eq "1" ]
then
    echo "compiling game .c files:"
    echo "${GAME_C_SRCS}"
    time gcc ${GAME_C_SRCS} ${COMPILER_FLAGS} ${GAME_INCLUDE_DIRS} ${OTHER_FLAGS} || exit 1
    echo "compiling game .cpp files:"
    echo "${GAME_CPP_SRCS}"
    time g++ ${GAME_CPP_SRCS} ${COMPILER_FLAGS} ${GAME_INCLUDE_DIRS} ${OTHER_FLAGS} || exit 1
fi

if [ "$NO_LINK" -eq "0" ];
then
    echo "linking .o files:"
    echo *.o
    time g++ *.o ${LINKER_FLAGS} -o ${EXECUTABLE_NAME} || exit 1
    echo "done"

    popd > /dev/null
    NAME_VERSION=${EXECUTABLE_NAME}-v${VERSION}
    install -d "${OUT_DIR}"
    rm -f "${OUT_DIR}/${NAME_VERSION}.tar.gz"
    install -d "${OUT_DIR}/shaders"
    install -d "${OUT_DIR}/assets"
    install -C "${BUILD_DIR}/${EXECUTABLE_NAME}" "${OUT_DIR}"
    install -C "${SDL_LIB_DIR}/${SDL_LIB}.so" "${OUT_DIR}"
    install -C shaders/* "${OUT_DIR}/shaders"
    install -C assets/* "${OUT_DIR}/assets"
    # produce tar
    pushd "${OUT_DIR}" > /dev/null
    set -x
    # add a directory at the root
    tar cvz --transform="s#^#${NAME_VERSION}/#" -f "${NAME_VERSION}.tar.gz" *
    popd > /dev/null
fi
