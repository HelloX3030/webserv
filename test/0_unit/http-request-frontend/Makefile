# test/0_unit/http-request-frontend/Makefile
#
# standalone unit test driver for HttpRequestFrontend.
# exercises advance() directly — no I/O, no Connection, no epoll.
#
# the test binary links only the TUs it exercises:
# HttpRequestFrontend (5 TUs) + HttpRequest (1 TU) + test TUs.
#
# C++20 for std::source_location in the test harness.
# the unit under test is C++17 — C++20 is a strict superset.

# --- paths ---

ROOT     := ../../..
INC_DIR  := $(ROOT)/inc
SRC_DIR  := $(ROOT)/src
INCLUDES := -I $(INC_DIR)

# --- toolchain ---

CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++20

# --- sources ---
# test driver + the TUs it exercises.
# HttpRequestFrontend: 5 TUs (constructor/advance/reset, buffer,
# request-line, headers, body).
# HttpRequest: 1 TU (keepAlive, contentLength).

FE_DIR := $(SRC_DIR)/http/HttpRequestFrontend

SRCS := main.cpp                                    \
        $(FE_DIR)/HttpRequestFrontend.cpp            \
        $(FE_DIR)/HttpRequestFrontend_1_buffer.cpp   \
        $(FE_DIR)/HttpRequestFrontend_2_request_line.cpp \
        $(FE_DIR)/HttpRequestFrontend_3_headers.cpp  \
        $(FE_DIR)/HttpRequestFrontend_4_body.cpp     \
        $(SRC_DIR)/http/HttpRequest/HttpRequest.cpp

# test TUs — added as written. each registers its tests at
# static initialisation; no edits to main.cpp needed.
SRCS += test_request_line.cpp	\
        test_headers.cpp

# --- variant binaries ---

NAME       := test_frontend
DBG_NAME   := test_frontend_debug
LEAKS_NAME := test_frontend_leaks

# --- variant flags ---

REL_FLAGS   :=
DBG_FLAGS   := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
LEAKS_FLAGS := -g -O0 -fno-omit-frame-pointer

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

run: $(NAME)
	./$(NAME)

debugrun: $(DBG_NAME)
	./$(DBG_NAME)

leaksrun: $(LEAKS_NAME)
	valgrind --leak-check=full --track-fds=yes --show-leak-kinds=all \
	  --error-exitcode=1 ./$(LEAKS_NAME)
