# Compiler and flags
CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17
NAME := webserv

# Debug flags
DEBUG_FLAGS := -DDEBUG=1 -g
DEBUG_NAME  := webserv_debug

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
DBG_OBJ_DIR := obj_debug

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

# Includes
INCLUDES := -I$(INC_DIR)

# =====================
# Normal build
# =====================

all: $(NAME)

$(NAME): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# =====================
# Debug build
# =====================

debug: $(DEBUG_NAME)

$(DEBUG_NAME): $(DBG_OBJ_FILES)
	$(CXX) $(DBG_OBJ_FILES) -o $@

$(DBG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(INCLUDES) -c $< -o $@

# =====================
# Cleaning
# =====================

clean:
	$(RM) -r $(OBJ_DIR) $(DBG_OBJ_DIR)

fclean: clean
	$(RM) -f $(NAME) $(DEBUG_NAME)

re: fclean all

# =====================
# Debug-only helpers
# =====================

debugclean:
	$(RM) -r $(DBG_OBJ_DIR)
	$(RM) -f $(DEBUG_NAME)

debugre: debugclean debug

# =====================
# Run helpers
# =====================

run: $(NAME)
	./$(NAME)

debugrun: $(DEBUG_NAME)
	./$(DEBUG_NAME)

.PHONY: all debug clean fclean re debugclean debugre run debugrun
