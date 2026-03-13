# original/v1 from Lukas

# Compiler and flags
CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17
NAME := webserv

# Debug flags
DEBUG_FLAGS := -DDEBUG=1
DEBUG_NAME  := webserv_debug

# Leak-check build
LEAK_FLAGS := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
LEAK_NAME  := webserv_leaks

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
DBG_OBJ_DIR := obj_debug
LEAK_OBJ_DIR := obj_leaks

# Recursive wildcard function
rwildcard = $(foreach d,$(wildcard $1*), \
              $(call rwildcard,$d/,$2) \
              $(filter $(subst *,%,$2),$d))

# Automatically collect files (recursive)
SRC_FILES := $(call rwildcard,$(SRC_DIR)/,*.cpp)
H_FILES   := $(call rwildcard,$(INC_DIR)/,*.hpp) \
             $(call rwildcard,$(INC_DIR)/,*.h)

# Object files
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
DBG_OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(DBG_OBJ_DIR)/%.o,$(SRC_FILES))
LEAK_OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(LEAK_OBJ_DIR)/%.o,$(SRC_FILES))

# Includes
INCLUDES := -I$(INC_DIR)

# =====================
# Normal build
# =====================

.PHONY: all
all: $(NAME)

$(NAME): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# =====================
# Debug build
# =====================

.PHONY: debug
debug: $(DEBUG_NAME)

$(DEBUG_NAME): $(DBG_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(DBG_OBJ_FILES) -o $@

$(DBG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(INCLUDES) -c $< -o $@

# =====================
# Leak-check build
# =====================

.PHONY: leaks
leaks: $(LEAK_NAME)

$(LEAK_NAME): $(LEAK_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(LEAK_FLAGS) $(LEAK_OBJ_FILES) -o $@

$(LEAK_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LEAK_FLAGS) $(INCLUDES) -c $< -o $@

# =====================
# Cleaning
# =====================

.PHONY: clean
clean:
	$(RM) -r $(OBJ_DIR) $(DBG_OBJ_DIR) $(LEAK_OBJ_DIR)

.PHONY: fclean
fclean: clean
	$(RM) -f $(NAME) $(DEBUG_NAME) $(LEAK_NAME)

.PHONY: re
re: fclean all

# =====================
# Debug helpers
# =====================

.PHONY: debugclean
debugclean:
	$(RM) -r $(DBG_OBJ_DIR)
	$(RM) -f $(DEBUG_NAME)

.PHONY: debugre
debugre: debugclean debug

# =====================
# Leaks helpers
# =====================

.PHONY: leaksclean
leaksclean:
	$(RM) -r $(LEAK_OBJ_DIR)
	$(RM) -f $(LEAK_NAME)

.PHONY: leaksre
leaksre: leaksclean leaks

# =====================
# Run helpers
# =====================

.PHONY: run
run: $(NAME)
	./$(NAME)

.PHONY: debugrun
debugrun: $(DEBUG_NAME)
	./$(DEBUG_NAME)

.PHONY: leaksrun
leaksrun: $(LEAK_NAME)
	valgrind --leak-check=full --track-fds=yes --show-leak-kinds=all ./$(LEAK_NAME)
