## Global configuration
COMPILER=g++
INCLUDES=-ISRC/
## Note, if you're on NixOS, you will have to change this manually, as of any attempt of automatizing this has resulted in a neverending headache.
# In Response To: Woynert@NixOS - "Dependency Hell"
PKG_CONFIG=`pkg-config gtkmm-3.0 --cflags --libs`
STD=-std=c++17
## This is the same everywhere else.
SOURCES=SRC/main.cpp
# we don't use the LIBS case anymore, we directly use PKG_CONFIG
#LIBS=$(PKG_CONFIG)

## Release specific flag and output, repetitive, but useful.
FLAGS_REL=-fpermissive
OUT=setup

## Debug Specific flags and output
FLAGS_DBG=-fpermissive -g -Wall -Wextra -Wpedantic -Werror
OUT_DBG=setup.dbg

.PHONY: $(OUT_DBG) $(OUT) release debug all clean init run

$(OUT_DBG): $(SOURCES)
	@clear
	$(COMPILER) $(SOURCES) -o $(OUT_DBG) $(INCLUDES) $(PKG_CONFIG) $(FLAGS) $(FLAGS_DBG) $(STD)

$(OUT): $(SOURCES)
	$(COMPILER) $(SOURCES) -o $(OUT) $(INCLUDES) $(PKG_CONFIG) $(FLAGS) $(STD)

all: $(OUT_DBG) $(OUT)
	@echo "Targets were be built automatically."

release: $(OUT)
	@echo "Target lacks Debug Symbols, you won't be able to fully debug this program after built task is completed."
	@echo "For the debug target, use 'make all' or 'make debug'"

debug: $(OUT_DBG)
	@echo "Target is a debugging test. do not use this on production."
clean:
	rm $(OUT)

init:
	@echo "The following Command was formatted for Debian/Linux - GNU, if you are in another distribution, it is recommended to use your distribution's package manager to install the required dependencies."
	@echo "ATTENTION: If you are new to Building From Source, the following command will install required packages using Debian's Package Manager, if you do not know what this means, it is recommended to investigate what this does before installing."
	bash SRC/init-deps.sh

run: $(OUT)
	@echo "This is a test run of the setup, please ensure everything looks okay for your program's setup."
	@exec ./setup &