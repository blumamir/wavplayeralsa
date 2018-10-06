CC := g++
CFLAGS := -g -Wall -std=c++11
LIBS := -lasound -lsndfile -pthread -lprotobuf -lzmq

ifdef DEBUG
  CFLAGS += -O0
else
  CFLAGS += -O3
endif

MAIN_SRCS = $(wildcard src/*.cpp)

HEADERS = $(wildcard src/*.h)
HEADERS := $(wildcard src/generated/*.h)

OBJDIR := obj

OBJECTS = wavplayeralsa.o single_file_player.o position_reporter.o position_report.pb.o player_command.pb.o
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

$(OBJDIR)/%.o: src/generated/%.cc $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: src/%.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(TRG_wavplayeralsa): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(OBJECTS) $(LIBS) -o $@

clean:
	rm -rf $(OBJDIR)
	rm -rf $(TRG_wavplayeralsa)

