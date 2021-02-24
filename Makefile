.PHONY: all clean


ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif


DEP_LIBS:=-ludev
DEP_LIBS+=-lusb-1.0
DEP_LIBS+=-lhidapi-hidraw
DEP_LIB_PATH:=
SHARED_LIB_NAME:=mcp2221_hidapi
STATIC_LIB_NAME:=mcp2221_hidapi_static

STATIC_LIB_FILE:=lib$(STATIC_LIB_NAME).a
SHARED_LIB_FILE:=lib$(SHARED_LIB_NAME).so

ifeq ($(detected_OS),Windows)
MACHINE:=windows
DEP_LIBS:=-lhidapi
DEP_LIB_PATH:=-Llibs/$(MACHINE)
SHARED_LIB_FILE:=$(SHARED_LIB_NAME).dll
else
MACHINE:=$(shell uname -m)
endif

all: test $(STATIC_LIB_FILE) $(SHARED_LIB_FILE)

test.o: test.c
	gcc -c -Wall -Wextra -Wstrict-prototypes -Wunused-result -O3 -std=c99 -fmessage-length=0 -fPIC -Ilibs $^ -o $@

reset.o: reset.c
	gcc -c -Wall -Wextra -Wstrict-prototypes -Wunused-result -O3 -std=c99 -fmessage-length=0 -fPIC -Ilibs $^ -o $@

mcp2221_hidapi.o: mcp2221_hidapi.c
	gcc -c -Wall -Wextra -Wstrict-prototypes -Wunused-result -O3 -std=c99 -fmessage-length=0 -fPIC -Ilibs $^ -o $@

$(SHARED_LIB_FILE): mcp2221_hidapi.o
	gcc -s -static-libgcc -static-libstdc++ -shared $^ -o $@ $(DEP_LIBS) $(DEP_LIB_PATH) -Wl,-rpath=libs
	@mkdir -p libs/$(MACHINE)
	@cp -fv $@ libs/$(MACHINE)

$(STATIC_LIB_FILE): mcp2221_hidapi.o
	ar rcu $@ $<
	@mkdir -p libs/$(MACHINE)
	@cp -fv $@ libs/$(MACHINE)



test: test.o mcp2221_hidapi.o
	gcc -o $@ $^ $(DEP_LIBS) $(DEP_LIB_PATH)
	@cp -v libs/windows/hidapi.dll .

reset: reset.o mcp2221_hidapi.o
	gcc -o $@ $^ $(DEP_LIBS) $(DEP_LIB_PATH)
	@cp -v libs/windows/hidapi.dll .

test_static: test.o $(STATIC_LIB_FILE)
	gcc -o $@ $< -l$(STATIC_LIB_NAME) -L. $(DEP_LIBS) $(DEP_LIB_PATH)
	@cp -v libs/windows/hidapi.dll .

test_shared: test.o $(SHARED_LIB_FILE)
	gcc -o $@ $< -l$(SHARED_LIB_NAME) -Wl,-rpath=libs -Llibs $(DEP_LIBS) $(DEP_LIB_PATH)
	@cp -v libs/windows/hidapi.dll .


clean:
	rm -rf *.o *.exe
	rm -rf test test_shared test_static $(STATIC_LIB_FILE) $(SHARED_LIB_FILE) hidapi.dll
