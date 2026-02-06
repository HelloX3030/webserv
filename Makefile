# Compiler and flags
CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17
NAME := webserv

# vpath for header files
vpath %.hpp include/

# vpath for source files
vpath %.cpp src/

# .h and .hpp files
H_FILES := webserv.hpp

# .cpp source files
SRC_FILES := main.cpp

# Object directory and object files
OBJ_DIR := obj
OBJ_FILES := $(addprefix $(OBJ_DIR)/, $(SRC_FILES:.cpp=.o))

# includes
INCLUDES := -I include

# Default target
all: $(NAME)

# Link the executable
$(NAME): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -o $(NAME)

# Compile object files
$(OBJ_DIR)/%.o: src/%.cpp $(H_FILES) | $(OBJ_DIR)
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
