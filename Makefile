CC := g++
CFLAGS := -g -Wall -std=c++11
LIBS := -lasound -lsndfile

ifdef DEBUG
  CFLAGS += -O0
else
  CFLAGS += -O3
endif

MAIN_SRCS = src/wavplayeralsa.cpp

HEADERS = $(wildcard src/*.h)

OBJECTS_SRC = $(wildcard src/*.cpp)
OBJECTS_SRC_FILTERED = $(filter-out $(MAIN_SRCS), $(OBJECTS_SRC))

OBJDIR := obj

OBJECTS   := $(addprefix $(OBJDIR)/,$(OBJECTS))
TARGETOBJ := $(OBJDIR)/src

TARGETDIR := bin
TRG_wavplayeralsa = $(TARGETDIR)/wavplayeralsa

.PHONY: default all clean
.PRECIOUS: $(OBJECTS)

.PHONY: wavplayeralsa
wavplayeralsa: $(TRG_wavplayeralsa)

default: all
all: wavplayeralsa

%.o: %.cc $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(TRG_wavplayeralsa): $(OBJECTS) $(TARGETOBJ)/wavplayeralsa.o
	@mkdir -p $(@D)
	$(CC) $(OBJECTS) $(TARGETOBJ)/wavplayeralsa.o $(LIBS) -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(TRG_wavplayeralsa)

