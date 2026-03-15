# test/config-frontend/Makefile
#
# standalone test driver for ConfigFrontend.
# 3 variants: release, debug, leaks (valgrind).
# config file path passed at invocation: make run CFG=<path>

# --- paths ---

ROOT     := ../..
INC_DIR  := $(ROOT)/include
SRC_DIR  := $(ROOT)/src
INCLUDES := -I $(INC_DIR)

# --- toolchain ---

CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17

# --- sources ---
# test driver + the 2 TUs it exercises.

SRCS := test_config.cpp \
        $(SRC_DIR)/classes/ConfigFrontend/ConfigFrontend.cpp \
        $(SRC_DIR)/classes/Config.cpp

# --- variant binaries ---

NAME       := test_config
DBG_NAME   := test_config_debug
LEAKS_NAME := test_config_leaks

# --- variant flags ---

REL_FLAGS   :=
DBG_FLAGS   := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
LEAKS_FLAGS := -g -O0 -fno-omit-frame-pointer

# --- config file ---
# override at invocation: make run CFG=path/to/config.conf
# default exercises the baseline.

CFG ?= $(ROOT)/config/valid/default.conf

# --- phony declarations ---

.PHONY: all clean re
.PHONY: debug debugclean debugre
.PHONY: leaks leaksclean leaksre
.PHONY: run debugrun leaksrun

# --- build targets ---

all: $(NAME)

$(NAME): $(SRCS)
	$(CXX) $(CXXFLAGS) $(REL_FLAGS) $(INCLUDES) $^ -o $@

debug: $(DBG_NAME)

$(DBG_NAME): $(SRCS)
	$(CXX) $(CXXFLAGS) $(DBG_FLAGS) $(INCLUDES) $^ -o $@

leaks: $(LEAKS_NAME)

$(LEAKS_NAME): $(SRCS)
	$(CXX) $(CXXFLAGS) $(LEAKS_FLAGS) $(INCLUDES) $^ -o $@

# --- clean targets ---

clean:
	$(RM) $(NAME)

debugclean:
	$(RM) $(DBG_NAME)

leaksclean:
	$(RM) $(LEAKS_NAME)

re: clean all

debugre: debugclean debug

leaksre: leaksclean leaks

# --- run targets ---
# CFG variable specifies config file. examples:
#   make run
#   make run CFG=../../config/valid/cgi.conf
#   make leaksrun CFG=../../config/valid/multi-server.conf

run: $(NAME)
	./$(NAME) $(CFG)

debugrun: $(DBG_NAME)
	./$(DBG_NAME) $(CFG)

leaksrun: $(LEAKS_NAME)
	valgrind --leak-check=full --track-fds=yes --show-leak-kinds=all \
	  --error-exitcode=1 ./$(LEAKS_NAME) $(CFG)
