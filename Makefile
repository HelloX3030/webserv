# Compiler and flags
CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17
NAME := webserv

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj

# vpath for header and source files
vpath %.hpp $(INC_DIR)
vpath %.h   $(INC_DIR)
vpath %.cpp $(SRC_DIR)

# Automatically collect files
H_FILES   := $(wildcard $(INC_DIR)/*.hpp) $(wildcard $(INC_DIR)/*.h)
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)

# Object files
OBJ_FILES := $(addprefix $(OBJ_DIR)/, $(notdir $(SRC_FILES:.cpp=.o)))

# Includes
INCLUDES := -I $(INC_DIR)

# Default target
all: $(NAME)

# Link the executable
$(NAME): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -o $(NAME)

# Compile object files
$(OBJ_DIR)/%.o: %.cpp $(H_FILES) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $@

# Clean object files
clean:
	$(RM) -r $(OBJ_DIR)

# Clean all
fclean: clean
	$(RM) -f $(NAME)

# Rebuild
re: fclean all

.PHONY: all clean fclean re
