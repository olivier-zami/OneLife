########################################################################################################################
#compilation flags
CFL_STRICT = -Wall
CFL_SDL = -L/usr/X11R6/lib -lGL  -lGLU -lX11 -lSDL
CFL_THREAD = -lpthread

########################################################################################################################
DIR_PROJECT=../..

#subdirectories
DIR_APP=${DIR_PROJECT}
DIR_BUILD=${DIR_PROJECT}/build
DIR_BIN=${DIR_PROJECT}/bin
DIR_SRC=${DIR_PROJECT}/src
DIR_TMP=${DIR_PROJECT}/var
DIR_DATA=${DIR_PROJECT}/data
DIR_TEST=${DIR_PROJECT}/test

DIR_SRC_LIB=$(DIR_SRC)/third_party
DIR_IMGUI=${DIR_SRC}/third_party/imgui

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

UNAME_S := $(shell uname -s)

CXXFLAGS = -I$(DIR_IMGUI) -I$(DIR_IMGUI)/backends -I${DIR_PROJECT}
CXXFLAGS += -g -Wall -Wformat

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL -ldl `sdl2-config --libs`

	CXXFLAGS += `sdl2-config --cflags`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
	LIBS += -L/usr/local/lib -L/opt/local/lib

	CXXFLAGS += `sdl2-config --cflags`
	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`

	CXXFLAGS += `pkg-config --cflags sdl2`
	CFLAGS = $(CXXFLAGS)
endif

########################################################################################################################
PREFIX_RULE_BUILD=build
PREFIX_RULE_BUILD_LIB=build/lib

REG_DIR_SRC=$(subst /,\/,$(DIR_SRC))
REG_DIR_SRC_LIB=$(subst /,\/,$(DIR_SRC_LIB))
REG_PREFIX_RULE_BUILD=$(subst /,\/,$(PREFIX_RULE_BUILD))
REG_PREFIX_RULE_BUILD_LIB=$(subst /,\/,$(PREFIX_RULE_BUILD_LIB))

########################################################################################################################
CMD_RENAME_DIRECTORY = sed ' \
s/^$(REG_DIR_SRC_LIB)/$(REG_PREFIX_RULE_BUILD_LIB)/; \
s/^$(REG_DIR_SRC)/$(REG_PREFIX_RULE_BUILD)/; \
'

########################################################################################################################
APP_NAME=mapManager

BUILD_RULE_INIT=
SOURCE_FILE=${DIR_SRC}/mapManager/main.cpp
SOURCE_FILE+=${DIR_SRC}/mapManager/Application.cpp
SOURCE_FILE+=${DIR_SRC}/mapManager/component/window/MapWindow.cpp
SOURCE_FILE+=${DIR_SRC}/mapManager/component/window/WorldMap.cpp
SOURCE_FILE+=$(DIR_IMGUI)/imgui.cpp $(DIR_IMGUI)/imgui_demo.cpp $(DIR_IMGUI)/imgui_draw.cpp $(DIR_IMGUI)/imgui_tables.cpp $(DIR_IMGUI)/imgui_widgets.cpp
SOURCE_FILE+=$(DIR_IMGUI)/backends/imgui_impl_sdl.cpp $(DIR_IMGUI)/backends/imgui_impl_sdlrenderer.cpp
SOURCE_FILE+=$(DIR_IMGUI)/backends/imgui_impl_opengl3.cpp

########################################################################################################################

RULE_BUILD_APP=$(APP_NAME)

BUILD_INIT_RULES=
$(foreach srcFile,$(SOURCE_FILE),$(eval BUILD_INIT_RULES:=$(BUILD_INIT_RULES) $(shell echo $(srcFile) | $(CMD_RENAME_DIRECTORY) )))

FILES_OBJ=

########################################################################################################################
all: READ_SOURCE $(SOURCE_FILE) $(RULE_BUILD_APP)
	@echo Build complete for ...

READ_SOURCE: $(BUILD_INIT_RULES)

$(RULE_BUILD_APP): $(RULES_OBJ)
	$(CXX) -o $(DIR_BIN)/$@ $(FILES_OBJ) $(CXXFLAGS) $(LIBS)

$(PREFIX_RULE_BUILD)/%.cpp:
	$(eval TMP_FILE_SRC=$(subst build, ../../src,$(subst build/lib,build/third_party,$@)))
	$(eval TMP_DIR_BIN=$(DIR_PROJECT)/$(dir $(subst build,bin/objects,$@)))
	@if [ ! -d $(TMP_DIR_BIN) ]; then mkdir -p $(TMP_DIR_BIN); fi
	$(eval TMP_FILE_OBJ=$(TMP_DIR_BIN)$(basename $(notdir $@)).o)
	$(CXX) $(CXXFLAGS) -c -o $(TMP_FILE_OBJ) $(TMP_FILE_SRC)
	$(eval FILES_OBJ+=$(TMP_FILE_OBJ))

${DIR_SRC}/%.cpp:
	$(info ----- build -c -o $@)
	#$(CXX) $(CXXFLAGS) -c -o $@ $<
