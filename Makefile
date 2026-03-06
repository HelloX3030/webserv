# ─── toolchain ────────────────────────────────────────────────

CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17 -MMD -MP
LDFLAGS  :=

# ─── paths ────────────────────────────────────────────────────

SRC_DIR  := src
INC_DIR  := include
INCLUDES := -I $(INC_DIR)

# ─── variant names and objdirs ────────────────────────────────

NAME     := webserv
DBG_NAME := webserv_debug
LKS_NAME := webserv_leaks

OBJ_DIR     := obj
DBG_OBJ_DIR := obj_debug
LKS_OBJ_DIR := obj_leaks

# ─── source collection ────────────────────────────────────────
# rwildcard: recursive wildcard traversal.
# used for SRC_FILES only — H_FILES is eliminated;
# header dependencies are derived per-TU by -MMD.

rwildcard = $(foreach d,$(wildcard $1*),\
              $(call rwildcard,$d/,$2)\
              $(filter $(subst *,%,$2),$d))

SRC_FILES := $(call rwildcard,$(SRC_DIR)/,*.cpp)

# ─── derived object and dependency lists ──────────────────────
# := ensures immediate expansion after all inputs are defined.
# each variant has its own object list; DEP_FILES spans all 3.

REL_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
DBG_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(DBG_OBJ_DIR)/%.o,$(SRC_FILES))
LKS_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(LKS_OBJ_DIR)/%.o,$(SRC_FILES))
DEP_FILES := $(REL_OBJS:.o=.d) $(DBG_OBJS:.o=.d) $(LKS_OBJS:.o=.d)

# ─── phony targets ────────────────────────────────────────────

.PHONY: all clean fclean re debug leaks \
        debugclean debugre leaksclean leaksre \
        run debugrun leaksrun

# ─── variant configuration ────────────────────────────────────
# target-specific variables propagate to the entire subgraph
# rooted at each binary target. EXTRA_CFLAGS is referenced in
# COMPILE_OBJ, which expands during phase 2 when these values
# are active. EXTRA_LDFLAGS is composed into each link rule.

$(NAME):     EXTRA_CFLAGS  :=
$(NAME):     EXTRA_LDFLAGS :=

$(DBG_NAME): EXTRA_CFLAGS  := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
$(DBG_NAME): EXTRA_LDFLAGS :=

$(LKS_NAME): EXTRA_CFLAGS  := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
$(LKS_NAME): EXTRA_LDFLAGS :=

# ─── 42-required targets ──────────────────────────────────────

all: $(NAME)

clean:
	$(RM) -r $(OBJ_DIR) $(DBG_OBJ_DIR) $(LKS_OBJ_DIR)

fclean: clean
	$(RM) -f $(NAME) $(DBG_NAME) $(LKS_NAME)

re: fclean all

# ─── link rules ───────────────────────────────────────────────
# CXXFLAGS is absent: it carries compilation-phase flags only.
# LDFLAGS and EXTRA_LDFLAGS carry the link-phase flags.
# $^ expands to the full object list for this variant.

$(NAME): $(REL_OBJS)
	$(CXX) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

$(DBG_NAME): $(DBG_OBJS)
	$(CXX) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

$(LKS_NAME): $(LKS_OBJS)
	$(CXX) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

# ─── compilation rule body ────────────────────────────────────
# define/endef stores text; expansion is deferred to phase 2.
# EXTRA_CFLAGS expands with the target-specific value active.
# @mkdir -p $(@D) creates the subdirectory under each objdir
# mirroring the source tree before the compiler writes to it.

define COMPILE_OBJ
@mkdir -p $(@D)
$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(INCLUDES) -c $< -o $@
endef

# ─── pattern rules ────────────────────────────────────────────
# 3 heads required: Make's pattern matching is syntactic;
# the directory prefix cannot be a variable. shared body
# via COMPILE_OBJ; EXTRA_CFLAGS provides the variant-specific
# flags at expansion time.

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

$(DBG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

$(LKS_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

# ─── dependency inclusion ─────────────────────────────────────
# placed after all pattern rules. .d files contain explicit
# rules; explicit rules take precedence over pattern rules for
# the same target. the dash suppresses errors on the first
# build when no .d files exist yet.

-include $(DEP_FILES)

# ─── variant targets ──────────────────────────────────────────

debug: $(DBG_NAME)
leaks: $(LKS_NAME)

# ─── variant maintenance ──────────────────────────────────────
# variant-specific re targets for iterating on a single variant
# without paying the cost of cleaning all 3.

debugclean:
	$(RM) -r $(DBG_OBJ_DIR)
	$(RM) -f $(DBG_NAME)

debugre: debugclean debug

leaksclean:
	$(RM) -r $(LKS_OBJ_DIR)
	$(RM) -f $(LKS_NAME)

leaksre: leaksclean leaks

# ─── run targets ──────────────────────────────────────────────

run: $(NAME)
	./$(NAME)

debugrun: $(DBG_NAME)
	./$(DBG_NAME)

leaksrun: $(LKS_NAME)
	valgrind --leak-check=full --track-fds=yes --show-leak-kinds=all \
	  --error-exitcode=1 ./$(LKS_NAME)