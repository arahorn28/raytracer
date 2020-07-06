TARGET = raytracer
LIBS = -pthread -llua
CXX = g++
CFLAGS = -std=c++17 -O3

OBJDIR = obj
SRCROOT = src
SRCDIRS = $(SRCROOT) $(SRCROOT)/thirdparty
LUA = $(SRCROOT)/thirdparty/lua
INCLUDE = $(foreach d, $(SRCDIRS), -I$d)

SOURCES = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))
OBJECTS = $(foreach file, $(SOURCES), $(OBJDIR)/$(basename $(notdir $(file))).o)
HEADERS = $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.h))
HEADERS += $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.hpp))

vpath %.cpp $(SRCDIRS)

.PHONY: default all clean

default: $(TARGET)
all: default


$(OBJECTS): $(OBJDIR)/%.o: %.cpp $(HEADERS) | obj
	$(CXX) $(CFLAGS) -c $< -I$(LUA)/src $(INCLUDE) -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	cd $(LUA) && $(MAKE) posix
	$(CXX) $(OBJECTS) -L$(LUA)/src $(LIBS) -o $@

obj:
	mkdir -p $@

clean:
	-rm -f $(OBJDIR)/*.o
	-rm -f $(TARGET)
	cd $(LUA) && $(MAKE) clean