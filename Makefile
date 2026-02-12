# Compiler and flags
CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17
NAME := webserv

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj

# Recursive wildcard function
rwildcard = $(foreach d,$(wildcard $1*), \
              $(call rwildcard,$d/,$2) \
              $(filter $(subst *,%,$2),$d))

# Automatically collect files (recursive)
SRC_FILES := $(call rwildcard,$(SRC_DIR)/,*.cpp)
H_FILES   := $(call rwildcard,$(INC_DIR)/,*.hpp) \
             $(call rwildcard,$(INC_DIR)/,*.h)

# Object files (mirror src/ structure inside obj/)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Includes
INCLUDES := -I$(INC_DIR)

# Default target
all: $(NAME)

# Link executable
$(NAME): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -o $@

# Compile rule (creates subdirectories as needed)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean object files
clean:
	$(RM) -r $(OBJ_DIR)

# Clean all
fclean: clean
	$(RM) -f $(NAME)

# Rebuild
re: fclean all

.PHONY: all clean fclean re
