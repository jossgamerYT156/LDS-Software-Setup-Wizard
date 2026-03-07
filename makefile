COMPILER=g++
INCLUDES=-ISRC/
PKG_CONFIG=`pkg-config gtkmm-3.0 --cflags --libs`
SOURCES=SRC/main.cpp
LIBS=$(PKG_CONFIG)
FLAGS=-fpermissive
STD=-std=c++17
OUT=setup

.PHONY: $(OUT) clean init run

$(OUT): $(SOURCES)
	@clear
	$(COMPILER) $(SOURCES) -o $(OUT) $(INCLUDES) $(LIBS) $(FLAGS) $(STD)

all: $(OUT)
	@echo "Targets will be built automatically."

clean:
	rm $(OUT)

init:
	@echo "The following Command was formatted for Debian/Linux - GNU, if you are in another distribution, it is recommended to use your distribution's package manager to install the required dependencies."
	@echo "ATTENTION: If you are new to Building From Source, the following command will install required packages using Debian's Package Manager, if you do not know what this means, it is recommended to investigate what this does before installing."
	bash SRC/init-deps.sh

run: $(OUT)
	@echo "This is a test run of the setup, please ensure everything looks okay for your program's setup."
	@exec ./setup &