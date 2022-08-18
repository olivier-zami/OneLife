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
DIR_VAR=${DIR_PROJECT}/var
DIR_DATA=${DIR_PROJECT}/data
DIR_TEST=${DIR_PROJECT}/test

DIR_SRC_LIB=$(DIR_SRC)/third_party
DIR_IMGUI=${DIR_SRC}/third_party/imgui

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

UNAME_S := $(shell uname -s)

CXXFLAGS = -I$(DIR_IMGUI) -I$(DIR_IMGUI)/backends -I${DIR_PROJECT} -I$(DIR_SRC_LIB)
CXXFLAGS += -g -Wall -Wformat

CXXFLAGS+= -I$(DIR_SRC_LIB)/FortuneAlgo/FortuneAlgo/FortuneAlgo/Datastruct
CXXFLAGS+= -I$(DIR_SRC_LIB)/FortuneAlgo/FortuneAlgo/FortuneAlgo/Math
CXXFLAGS+= -I$(DIR_SRC_LIB)/FortuneAlgo/FortuneAlgo/FortuneAlgo/Types
CXXFLAGS+= -I$(DIR_SRC_LIB)/FortuneAlgo/FortuneAlgo/FortuneAlgo/Voronoi

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

COMPILE=$(CXX) $(CXXFLAGS) $(LIBS) -c

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

DEPENDENCY_FILES=
SOURCE_FILES=${DIR_SRC}/mapManager/main.cpp
SOURCE_FILES+=${DIR_SRC}/mapManager/Application.cpp
SOURCE_FILES+=${DIR_SRC}/mapManager/dummy.cpp
SOURCE_FILES+=${DIR_SRC}/mapManager/component/window/BiomeMatrix.cpp
SOURCE_FILES+=${DIR_SRC}/mapManager/component/window/MapWindow.cpp
SOURCE_FILES+=${DIR_SRC}/mapManager/component/window/Example.cpp
SOURCE_FILES+=${DIR_SRC}/mapManager/component/window/Preview.cpp
SOURCE_FILES+=${DIR_SRC}/mapManager/component/window/ToolBox.cpp
SOURCE_FILES+=${DIR_SRC}/mapManager/component/window/widget/MapBuildingScreen.cpp
SOURCE_FILES+=${DIR_SRC}/mapManager/component/window/widget/MapBuildingStreamingScreen.cpp
SOURCE_FILES+=${DIR_SRC}/gameSource/GridPos.cpp
#SOURCE_FILES+=${DIR_SRC}/gameSource/objectBank.cpp
#SOURCE_FILES+=${DIR_SRC}/gameSource/objectBank.cpp
SOURCE_FILES+=${DIR_SRC}/commonSource/Debug.cpp
SOURCE_FILES+=${DIR_SRC}/commonSource/fractalNoise.cpp
SOURCE_FILES+=${DIR_SRC}/commonSource/math/geometry.cpp
SOURCE_FILES+=${DIR_SRC}/commonSource/math/GridPos.cpp
SOURCE_FILES+=${DIR_SRC}/commonSource/math/misc.cpp
SOURCE_FILES+=${DIR_SRC}/server/component/cache/Biome.cpp
SOURCE_FILES+=${DIR_SRC}/server/component/cache/Blocking.cpp
SOURCE_FILES+=${DIR_SRC}/server/component/database/Biome.cpp
SOURCE_FILES+=${DIR_SRC}/server/component/database/LookTime.cpp
#SOURCE_FILES+=${DIR_SRC}/server/component/feature/apocalypse.cpp
SOURCE_FILES+=${DIR_SRC}/server/component/generator/Biome.cpp
SOURCE_FILES+=${DIR_SRC}/server/component/Log.cpp
SOURCE_FILES+=${DIR_SRC}/server/component/Map.cpp
#SOURCE_FILES+=${DIR_SRC}/server/component/Speech.cpp
SOURCE_FILES+=${DIR_SRC}/server/dataType/LiveObject.cpp
SOURCE_FILES+=${DIR_SRC}/server/dbCommon.cpp
SOURCE_FILES+=${DIR_SRC}/server/objectType/bank/Cache.cpp
SOURCE_FILES+=${DIR_SRC}/server/objectType/bank/LinearDB.cpp
SOURCE_FILES+=${DIR_SRC}/server/objectType/bank/linearDB/lineardb3.cpp
SOURCE_FILES+=${DIR_SRC}/server/spiral.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/game/doublePair.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/io/file/linux/PathLinux.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/io/file/unix/DirectoryUnix.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/util/crc32.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/util/stringUtils.cpp
#SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/util/log/AppLog.cpp
#SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/util/SettingsManager.cpp
#SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/util/log/Log.cpp
#SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/util/log/PrintLog.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/system/unix/TimeUnix.cpp
#SOURCE_FILES+=${DIR_SRC}/third_party/minorGems/crypto/hashes/sha1.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/Debug.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/debug/Console.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/extension/Sdl2.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/extension/sdl2/handler/TextureHandler.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/diagram/voronoi/FortuneAlgorithm.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/Debug.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/Datastruct/DCEL.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/objectType/BeachLine.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/objectType/EventInquirer.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/objectType/EventQueue.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/objectType/inquirer/EdgeDebug.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/objectType/Node.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/objectType/NodeInquirer.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/Types/Point2D.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/procedure/math/point2D_F32.cpp
SOURCE_FILES+=${DIR_SRC}/third_party/openLife/src/system/Console.cpp
SOURCE_FILES+=$(DIR_IMGUI)/imgui.cpp $(DIR_IMGUI)/imgui_demo.cpp $(DIR_IMGUI)/imgui_draw.cpp $(DIR_IMGUI)/imgui_tables.cpp $(DIR_IMGUI)/imgui_widgets.cpp
SOURCE_FILES+=$(DIR_IMGUI)/backends/imgui_impl_sdl.cpp $(DIR_IMGUI)/backends/imgui_impl_sdlrenderer.cpp
SOURCE_FILES+=$(DIR_IMGUI)/backends/imgui_impl_opengl3.cpp

########################################################################################################################
#
#
#

#Make rule name list
ALL_BUILD_RULES=$(subst ../../src,RULE_BUILD_DEP,$(SOURCE_FILES:.cpp=.build))
ALL_OBJECT_RULES=$(subst ../../src,RULE_BUILD_OBJ,${SOURCE_FILES:.cpp=.o})

#Required file list
ALL_OBJECT_FILES=

#
#RULE_BUILD_APP=$(APP_NAME)
BUILD_INIT_RULES=
$(foreach srcFile,$(SOURCE_FILES),$(eval BUILD_INIT_RULES:=$(BUILD_INIT_RULES) $(shell echo $(srcFile) | $(CMD_RENAME_DIRECTORY) )))

########################################################################################################################
#SHOW_RULES:
#	$(info -----)
#	$(info $(ALL_OBJECT_RULES))

########################################################################################################################


MAIN_DEP_FILE=$(DIR_VAR)/build/mapManager.dep

########################################################################################################################
#
# SET MAIN COMMAND
#
ifeq (,$(wildcard $(MAIN_DEP_FILE)))
$(shell if [ ! -d $(dir $(MAIN_DEP_FILE)) ]; then mkdir -p $(dir $(MAIN_DEP_FILE)); fi)
$(file >$(MAIN_DEP_FILE))
REQUIRED_COMMANDS=DEPENDENCY_FILES
else
REQUIRED_COMMANDS=APPLICATION
endif

########################################################################################################################
include $(MAIN_DEP_FILE)
#include $(DEPENDENCY_FILES)

########################################################################################################################
#
# MAIN COMMAND
#

MAIN: $(REQUIRED_COMMANDS)

########################################################################################################################
#
# SUB COMMANDS
#

APPLICATION: $(ALL_OBJECT_RULES)
	$(info )$(info ----- BUILD APPLICATION)
	$(CXX) -o $(DIR_BIN)/$(APP_NAME) $(ALL_OBJECT_FILES) $(CXXFLAGS) $(LIBS)

DEPENDENCY_FILES: $(ALL_BUILD_RULES)
	$(info )$(info ----- CREATE DEPENDENCY FILES)
	$(file >$(MAIN_DEP_FILE),DEPENDENCY_FILES=$(DEPENDENCY_FILES))
	$(info $(DEPENDENCY_FILES))
	$(MAKE)

clean-hard:
	$(info clean all : no checking)
	rm -rf $(DIR_VAR)/build $(DIR_BIN)/objects $(DIR_BIN)/$(APP_NAME)

########################################################################################################################
#
#
#
#MEMO:
#%	=> wildcard
#$@ => rule name
#$< => subRule or filename
#$* => % value
#

#generic rule : build object cc -o file.o file.c file.h file1.h file2.h ...
RULE_BUILD_OBJ/%.o:
	$(info )$(info ---- BUILD OBJECT : call rule $@)
	$(eval TMP_OBJ_FILE=$(subst RULE_BUILD_OBJ,$(DIR_BIN)/objects,$(subst third_party,lib,$@)))
	$(eval TMP_SRC_FILE=$(subst RULE_BUILD_OBJ,$(DIR_SRC),$(subst .o,.cpp,$@)))
	@if [ ! -d $(dir $(TMP_OBJ_FILE)) ]; then mkdir -p $(dir $(TMP_OBJ_FILE)); fi
	$(COMPILE) -o $(TMP_OBJ_FILE) $(TMP_SRC_FILE)
	$(eval ALL_OBJECT_FILES+=$(TMP_OBJ_FILE))

#generic rule: make $(ALL_BUILD_RULE)
RULE_BUILD_DEP/%.build:
	$(info )$(info ----- BUILD ENVIRONMENT : call rule $(DIR_SRC)/%.build)
	$(eval TMP_SRC_FILE=$(subst RULE_BUILD_DEP,../../src,$(subst .build,.cpp,$@)))
	$(eval TMP_DEP_FILE=$(subst RULE_BUILD_DEP,../../var/build,$(subst .build,.dep,$@)))
	$(eval TMP_RULE_BUILD_OBJ=$(subst RULE_BUILD_DEP,RULE_BUILD_OBJ,$(subst .build,.o,$@)))
	@if [ ! -d $(dir $(TMP_DEP_FILE)) ]; then mkdir -p $(dir $(TMP_DEP_FILE)); fi
	$(COMPILE) -c -MM -MT $(TMP_RULE_BUILD_OBJ) $(TMP_SRC_FILE) > $(TMP_DEP_FILE)
	@echo "" >> $(TMP_DEP_FILE)
	$(COMPILE) -c -MM -MT $(TMP_DEP_FILE) $(TMP_SRC_FILE) >> $(TMP_DEP_FILE)
	$(eval DEPENDENCY_FILES+=$(TMP_DEP_FILE))

#%.${DEP_EXT}: %.cpp
#${COMPILE} -MM -MT $*.o $< > $@
#@echo "" >> $@
#${COMPILE} -MM -MT $*.${DEP_EXT} $< >> $@

#$(RULE_BUILD_APP): $(RULES_OBJ)
#	$(CXX) -o $(DIR_BIN)/$@ $(ALL_OBJECT_FILES) $(CXXFLAGS) $(LIBS)