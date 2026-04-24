# --- toolchain ---

CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17 -MMD -MP
LDFLAGS  :=

# --- verbosity ---
# V=0 (default): silent build with informative 1-line progress.
# V=1: full command echo — every flag visible, for build debugging.

V ?= 0
ifeq ($(V),0)
  Q := @
else
  Q :=
endif

# --- paths ---

SRC_DIR  := src
INC_DIR  := inc
INCLUDES := -I $(INC_DIR)	# resolve unqualified include paths relative to inc/

# --- variant names and objdirs ---

NAME     := webserv
DBG_NAME := webserv_debug
LKS_NAME := webserv_leaks

OBJ_DIR     := obj
DBG_OBJ_DIR := obj_debug
LKS_OBJ_DIR := obj_leaks

# --- src collection ---
# Explicit list (no wildcards): keep this in sync when adding/removing .cpp files.

SRC_FILES := \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/base/Fd.cpp \
	$(SRC_DIR)/base/format.cpp \
	$(SRC_DIR)/base/logging.cpp \
	$(SRC_DIR)/base/utils.cpp \
	$(SRC_DIR)/config/Config/Config.cpp \
	$(SRC_DIR)/config/ConfigFrontend/ConfigFrontend.cpp \
	$(SRC_DIR)/core/Server/Server.cpp \
	$(SRC_DIR)/core/signal.cpp \
	$(SRC_DIR)/http/HttpMethods/HttpMethods_cgi.cpp \
	$(SRC_DIR)/http/HttpMethods/HttpMethods_delete.cpp \
	$(SRC_DIR)/http/HttpMethods/HttpMethods_get.cpp \
	$(SRC_DIR)/http/HttpMethods/HttpMethods_handle_request.cpp \
	$(SRC_DIR)/http/HttpMethods/HttpMethods_post.cpp \
	$(SRC_DIR)/http/HttpRequest/HttpRequest.cpp \
	$(SRC_DIR)/http/HttpRequestFrontend/HttpRequestFrontend.cpp \
	$(SRC_DIR)/http/HttpRequestFrontend/HttpRequestFrontend_1_buffer.cpp \
	$(SRC_DIR)/http/HttpRequestFrontend/HttpRequestFrontend_2_request_line.cpp \
	$(SRC_DIR)/http/HttpRequestFrontend/HttpRequestFrontend_3_headers.cpp \
	$(SRC_DIR)/http/HttpRequestFrontend/HttpRequestFrontend_4_body.cpp \
	$(SRC_DIR)/http/HttpResponseBuilder/HttpResponseBuilder.cpp \
	$(SRC_DIR)/http/HttpStatus/HttpStatus.cpp \
	$(SRC_DIR)/net/Connection/Connection.cpp \
	$(SRC_DIR)/net/EPollHandler/EPollHandler.cpp \
	$(SRC_DIR)/net/Listener/Listener.cpp \
	$(SRC_DIR)/WebServ/WebServ.cpp \
	$(SRC_DIR)/WebServ/WebServ_display.cpp \
	$(SRC_DIR)/WebServ/WebServ_init.cpp \
	$(SRC_DIR)/WebServ/WebServ_load_config.cpp \
	$(SRC_DIR)/WebServ/WebServ_quit.cpp \
	$(SRC_DIR)/WebServ/WebServ_run.cpp

# --- precondition guard ---
# fail at parse time if no sources found — prevents a silent
# hollow-binary build from an empty or mislocated src tree.

ifeq ($(SRC_FILES),)
  $(error no source files found under $(SRC_DIR)/)
endif

# --- derived object and dependency lists ---
# := ensures immediate expansion after all inputs are defined.
# each variant has its own object list; DEP_FILES spans all 3.

REL_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
DBG_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(DBG_OBJ_DIR)/%.o,$(SRC_FILES))
LKS_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(LKS_OBJ_DIR)/%.o,$(SRC_FILES))
DEP_FILES := $(REL_OBJS:.o=.d) $(DBG_OBJS:.o=.d) $(LKS_OBJS:.o=.d)

# --- phony declarations ---

# variant-agnostic operations
.PHONY: clean fclean

# 3 variants: release, debug, leak (rows)
# per-variant (col):
#               build     clean      rebuild    run
.PHONY:         all                  re         run
.PHONY:         debug   debugclean   debugre    debugrun
.PHONY:         leaks   leaksclean   leaksre    leaksrun
.PHONY:         test test-integration \
			test-external \
			test-tester \
			test-tester-resources \
			test-tester-stress \
			test-cgi-tester-env \
			test-get \
			test-post \
			test-delete \
			test-cgi-keep-alive \
			test-virtual-hosting \
			test-invalid-http \
			test-slowloris \
			test-leaks test-leaks-integration \
			test-leaks-get \
			test-leaks-post \
			test-leaks-delete \
			test-leaks-cgi-keep-alive \
			test-leaks-virtual-hosting \
			test-leaks-invalid-http \
			test-leaks-slowloris \
			siege-test siege-baseline siege-medium siege-heavy siege-stress \
			siege-static siege-cgi siege-diagnose

# --- python test discovery ---
# `make test` should run all *normal* Python integration tests.
# Exclude scripts that are driven by the external tester/stress harness.

PY_INTEGRATION_TESTS := $(sort $(filter-out \
	test/1_integration/test_tester_stress.py,\
	$(wildcard test/1_integration/test_*.py)))

# --- siege defaults ---

SIEGE_BIN          ?= siege
SIEGE_URL_FILE     ?= ./test/1_integration/siege/urls.txt
SIEGE_STATIC_URL_FILE ?= ./test/1_integration/siege/urls_static.txt
SIEGE_CGI_URL_FILE ?= ./test/1_integration/siege/urls_cgi.txt
SIEGE_RESULTS_DIR  ?= ./test/1_integration/siege/results
SIEGE_SERVER_CONFIG ?= ./config/valid/full.conf
SIEGE_RC_FILE      ?= $(HOME)/.siege/siege.conf

SIEGE_CONCURRENCY  ?= 50
SIEGE_DURATION     ?= 2M
SIEGE_DELAY        ?= 2
SIEGE_TIMEOUT      ?= 5
SIEGE_TIMEOUT_STATIC ?= 3
SIEGE_TIMEOUT_CGI  ?= 10
SIEGE_DIAG_DURATION ?= 30S

# --- external tester binaries defaults ---

TESTER_BIN      ?= ./tester
CGI_TESTER_BIN  ?= ./cgi_tester
TESTER_URL      ?= http://127.0.0.1:8080
TESTER_SERVER_CONFIG ?= ./config/valid/tester.conf
TESTER_RESOURCE_SAMPLE_INTERVAL ?= 1
TESTER_STRESS_WORKERS ?= 20
TESTER_STRESS_ITERATIONS ?= 5
TESTER_STRESS_BODY_SIZE ?= 100000000
TESTER_STRESS_PATH ?= /directory/youpi.bla

# --- variant configuration ---
# target-specific variables propagate to the entire subgraph
# rooted at each binary target. EXTRA_CFLAGS is referenced in
# COMPILE_OBJ, which expands during phase 2 when these values
# are active. EXTRA_LDFLAGS is composed into each link rule.

$(NAME):     EXTRA_CFLAGS  := -O3
$(NAME):     EXTRA_LDFLAGS :=

$(DBG_NAME): EXTRA_CFLAGS  := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
$(DBG_NAME): EXTRA_LDFLAGS :=

$(LKS_NAME): EXTRA_CFLAGS  := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
$(LKS_NAME): EXTRA_LDFLAGS :=

# --- 42-required targets ---

all: $(NAME)
# 1st target in file = default goal, so `make`≡ `make all`

clean:
	@echo "  RM   $(OBJ_DIR) $(DBG_OBJ_DIR) $(LKS_OBJ_DIR)"
	@$(RM) -r $(OBJ_DIR) $(DBG_OBJ_DIR) $(LKS_OBJ_DIR)

fclean: clean
	@echo "  RM   $(NAME) $(DBG_NAME) $(LKS_NAME)"
	@$(RM) -f $(NAME) $(DBG_NAME) $(LKS_NAME)

re: fclean all

# --- link rules ---
# LDFLAGS and EXTRA_LDFLAGS carry the link-phase flags.
# $^ expands to the full obj list for this variant.
# echo line always visible; full command gated by Q.

$(NAME): $(REL_OBJS)
	@echo "  LD   $@"
	$(Q)$(CXX) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

$(DBG_NAME): $(DBG_OBJS)
	@echo "  LD   $@"
	$(Q)$(CXX) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

$(LKS_NAME): $(LKS_OBJS)
	@echo "  LD   $@"
	$(Q)$(CXX) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

# --- compilation rule body ---
# define/endef stores text; expansion deferred to phase 2.
# EXTRA_CFLAGS expands with the target-specific value active.
# mkdir: always @, pure infrastructure.
# echo: always visible — the readable progress signal in V=0.
# compiler invocation: gated by Q; fully visible in V=1.

define COMPILE_OBJ
@mkdir -p $(@D)
@echo "  CXX  $<"
$(Q)$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(INCLUDES) -c $< -o $@
endef

# --- pattern rules ---
# 3 heads required: Make's pattern matching is syntactic;
# the directory prefix cannot be a variable. shared body
# via COMPILE_OBJ; EXTRA_CFLAGS provides variant-specific
# flags at expansion time.

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

$(DBG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

$(LKS_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

# --- dependency inclusion ---
# placed after all pattern rules. .d files contain explicit rules;
# explicit rules take precedence over pattern rules for the same target.
# the dash suppresses errors on the 1st build when no .d files exist yet.

-include $(DEP_FILES)

# --- variant targets ---

debug: $(DBG_NAME)
leaks: $(LKS_NAME)

# --- variant maintenance ---
# variant-specific re targets for iterating on a single variant
# without paying the cost of cleaning all 3.

debugclean:
	@echo "  RM   $(DBG_OBJ_DIR) $(DBG_NAME)"
	@$(RM) -r $(DBG_OBJ_DIR)
	@$(RM) -f $(DBG_NAME)

debugre: debugclean debug

leaksclean:
	@echo "  RM   $(LKS_OBJ_DIR) $(LKS_NAME)"
	@$(RM) -r $(LKS_OBJ_DIR)
	@$(RM) -f $(LKS_NAME)

leaksre: leaksclean leaks

# --- run targets ---

run: $(NAME)
	./$(NAME)

debugrun: $(DBG_NAME)
	./$(DBG_NAME)

leaksrun: $(LKS_NAME)
	@valgrind --leak-check=full --track-fds=yes --show-leak-kinds=all \
	  --error-exitcode=1 ./$(LKS_NAME)

# --- integration tests ---

test: test-python

test-python: $(NAME)
	@set -eu; \
	for t in $(PY_INTEGRATION_TESTS); do \
		python3 "$$t"; \
		echo; \
	done

test-integration: $(NAME) \
	test-get \
	test-post \
	test-delete \
	test-cgi-keep-alive \
	test-virtual-hosting \
	test-invalid-http \
	test-slowloris

test-get: $(NAME)
	@python3 test/1_integration/test_get.py

test-post: $(NAME)
	@python3 test/1_integration/test_post.py

test-delete: $(NAME)
	@python3 test/1_integration/test_delete.py

test-cgi-keep-alive: $(NAME)
	@python3 test/1_integration/test_cgi_keep_alive.py

test-virtual-hosting: $(NAME)
	@python3 test/1_integration/test_virtual_hosting.py

test-invalid-http: $(NAME)
	@python3 test/1_integration/test_invalid_http.py

test-slowloris: $(NAME)
	@python3 test/1_integration/test_slowloris.py

# --- external tester binaries ---

test-external: test-tester test-cgi-tester-env

test-tester: $(NAME)
	@set -eu; \
	if [ ! -f "$(TESTER_BIN)" ]; then \
		echo "Error: tester binary not found at $(TESTER_BIN)"; \
		exit 1; \
	fi; \
	chmod +x "$(TESTER_BIN)"; \
	echo "Starting webserv with $(TESTER_SERVER_CONFIG)"; \
	./$(NAME) "$(TESTER_SERVER_CONFIG)" >/tmp/webserv-tester.log 2>&1 & \
	ws_pid=$$!; \
	trap 'kill -TERM $$ws_pid 2>/dev/null || true; wait $$ws_pid 2>/dev/null || true' EXIT INT TERM; \
	sleep 1; \
	echo "Running $(TESTER_BIN) $(TESTER_URL)"; \
	"$(TESTER_BIN)" "$(TESTER_URL)"

test-tester-resources: $(NAME)
	@set -eu; \
	if [ ! -f "$(TESTER_BIN)" ]; then \
		echo "Error: tester binary not found at $(TESTER_BIN)"; \
		exit 1; \
	fi; \
	chmod +x "$(TESTER_BIN)"; \
	echo "Starting webserv with $(TESTER_SERVER_CONFIG)"; \
	./$(NAME) "$(TESTER_SERVER_CONFIG)" >/tmp/webserv-tester.log 2>&1 & \
	ws_pid=$$!; \
	trap 'kill -TERM $$ws_pid 2>/dev/null || true; wait $$ws_pid 2>/dev/null || true' EXIT INT TERM; \
	sleep 1; \
	( \
		while kill -0 $$ws_pid 2>/dev/null; do \
			rss=$$(ps -o rss= -p $$ws_pid 2>/dev/null | tr -d ' '); \
			fd_count=$$(ls /proc/$$ws_pid/fd 2>/dev/null | wc -l | tr -d ' '); \
			cgi_count=$$(pgrep -af cgi_tester 2>/dev/null | wc -l | tr -d ' '); \
			printf '[resource] rss_kb=%s open_fds=%s cgi_procs=%s\n' "$$rss" "$$fd_count" "$$cgi_count"; \
			sleep $(TESTER_RESOURCE_SAMPLE_INTERVAL); \
		done \
	) & \
	monitor_pid=$$!; \
	trap 'kill -TERM $$monitor_pid 2>/dev/null || true; kill -TERM $$ws_pid 2>/dev/null || true; wait $$monitor_pid 2>/dev/null || true; wait $$ws_pid 2>/dev/null || true' EXIT INT TERM; \
	echo "Running $(TESTER_BIN) $(TESTER_URL)"; \
	"$(TESTER_BIN)" "$(TESTER_URL)"; \
	kill -TERM $$monitor_pid 2>/dev/null || true; \
	wait $$monitor_pid 2>/dev/null || true

test-tester-stress: $(NAME)
	@set -eu; \
	if [ ! -f "test/1_integration/test_tester_stress.py" ]; then \
		echo "Error: stress script not found at test/1_integration/test_tester_stress.py"; \
		exit 1; \
	fi; \
	echo "Starting webserv with $(TESTER_SERVER_CONFIG)"; \
	./$(NAME) "$(TESTER_SERVER_CONFIG)" >/tmp/webserv-tester-stress.log 2>&1 & \
	ws_pid=$$!; \
	trap 'kill -TERM $$ws_pid 2>/dev/null || true; wait $$ws_pid 2>/dev/null || true' EXIT INT TERM; \
	sleep 1; \
	( \
		while kill -0 $$ws_pid 2>/dev/null; do \
			rss=$$(ps -o rss= -p $$ws_pid 2>/dev/null | tr -d ' '); \
			fd_count=$$(ls /proc/$$ws_pid/fd 2>/dev/null | wc -l | tr -d ' '); \
			cgi_count=$$(pgrep -af cgi_tester 2>/dev/null | wc -l | tr -d ' '); \
			printf '[resource] rss_kb=%s open_fds=%s cgi_procs=%s\n' "$$rss" "$$fd_count" "$$cgi_count"; \
			sleep $(TESTER_RESOURCE_SAMPLE_INTERVAL); \
		done \
	) & \
	monitor_pid=$$!; \
	trap 'kill -TERM $$monitor_pid 2>/dev/null || true; kill -TERM $$ws_pid 2>/dev/null || true; wait $$monitor_pid 2>/dev/null || true; wait $$ws_pid 2>/dev/null || true' EXIT INT TERM; \
	python3 test/1_integration/test_tester_stress.py; \
	kill -TERM $$monitor_pid 2>/dev/null || true; \
	wait $$monitor_pid 2>/dev/null || true

test-cgi-tester-env:
	@set -eu; \
	if [ ! -f "$(CGI_TESTER_BIN)" ]; then \
		echo "Error: CGI tester binary not found at $(CGI_TESTER_BIN)"; \
		exit 1; \
	fi; \
	chmod +x "$(CGI_TESTER_BIN)"; \
	echo "Running $(CGI_TESTER_BIN) with CGI environment"; \
	REQUEST_METHOD=GET \
	GATEWAY_INTERFACE=CGI/1.1 \
	SERVER_PROTOCOL=HTTP/1.1 \
	SCRIPT_NAME=/cgi_tester \
	PATH_INFO=/cgi_tester \
	QUERY_STRING= \
	SERVER_NAME=localhost \
	SERVER_PORT=8080 \
	REMOTE_ADDR=127.0.0.1 \
	"$(CGI_TESTER_BIN)"

# --- integration tests with leak detection ---

test-leaks: test-leaks-integration

test-leaks-integration: $(LKS_NAME) \
	test-leaks-get \
	test-leaks-post \
	test-leaks-delete \
	test-leaks-cgi-keep-alive \
	test-leaks-virtual-hosting \
	test-leaks-invalid-http

test-leaks-get: $(LKS_NAME)
	@WEBSERV_BINARY=$(PWD)/$(LKS_NAME) WEBSERV_VALGRIND=1 python3 test/1_integration/test_get.py

test-leaks-post: $(LKS_NAME)
	@WEBSERV_BINARY=$(PWD)/$(LKS_NAME) WEBSERV_VALGRIND=1 python3 test/1_integration/test_post.py

test-leaks-delete: $(LKS_NAME)
	@WEBSERV_BINARY=$(PWD)/$(LKS_NAME) WEBSERV_VALGRIND=1 python3 test/1_integration/test_delete.py

test-leaks-cgi-keep-alive: $(LKS_NAME)
	@WEBSERV_BINARY=$(PWD)/$(LKS_NAME) WEBSERV_VALGRIND=1 python3 test/1_integration/test_cgi_keep_alive.py

test-leaks-virtual-hosting: $(LKS_NAME)
	@WEBSERV_BINARY=$(PWD)/$(LKS_NAME) WEBSERV_VALGRIND=1 python3 test/1_integration/test_virtual_hosting.py

test-leaks-invalid-http: $(LKS_NAME)
	@WEBSERV_BINARY=$(PWD)/$(LKS_NAME) WEBSERV_VALGRIND=1 python3 test/1_integration/test_invalid_http.py

test-leaks-slowloris: $(LKS_NAME)
	@WEBSERV_BINARY=$(PWD)/$(LKS_NAME) WEBSERV_VALGRIND=1 python3 test/1_integration/test_slowloris.py

# --- siege load tests ---

define RUN_SIEGE_TEST
	@set -eu; \
	if ! command -v $(SIEGE_BIN) >/dev/null 2>&1; then \
		echo "Error: '$(SIEGE_BIN)' is not installed. Install it (e.g. sudo apt install siege) and retry."; \
		exit 1; \
	fi; \
	if [ ! -s "$(SIEGE_URL_FILE)" ]; then \
		echo "Error: URL file '$(SIEGE_URL_FILE)' is missing or empty."; \
		exit 1; \
	fi; \
	mkdir -p "$(SIEGE_RESULTS_DIR)"; \
	ts=$$(date +%Y%m%d-%H%M%S); \
	result_file="$(SIEGE_RESULTS_DIR)/siege-$1-$$ts.log"; \
	tmp_rc="/tmp/siege-$1-$$ts.conf"; \
	rc_src="$(SIEGE_RC_FILE)"; \
	if [ ! -f "$$rc_src" ]; then \
		$(SIEGE_BIN) -C >/dev/null 2>&1 || true; \
	fi; \
	if [ ! -f "$$rc_src" ]; then \
		echo "Error: siege rc file not found at $$rc_src"; \
		exit 1; \
	fi; \
	cp "$$rc_src" "$$tmp_rc"; \
	echo "timeout = $5" >> "$$tmp_rc"; \
	echo "Running siege $1 test..."; \
	echo "Starting webserv with $(SIEGE_SERVER_CONFIG)"; \
	./$(NAME) "$(SIEGE_SERVER_CONFIG)" >/tmp/webserv-siege-$$ts.log 2>&1 & \
	ws_pid=$$!; \
	trap 'kill -TERM $$ws_pid 2>/dev/null || true; wait $$ws_pid 2>/dev/null || true; rm -f "$$tmp_rc"' EXIT INT TERM; \
	sleep 1; \
	if [ "$2" -gt 255 ] 2>/dev/null; then \
		echo "Warning: requested concurrency=$2, but siege may cap users at 255 (check ~/.siegerc limit)."; \
	fi; \
	echo "siege -c $2 -d $3 -t $4 -f $(SIEGE_URL_FILE) (timeout=$5s)"; \
	$(SIEGE_BIN) -R "$$tmp_rc" -c "$2" -d "$3" -t "$4" -f "$(SIEGE_URL_FILE)" | tee "$$result_file"; \
	fails=$$(grep -Eo '"failed_transactions":[[:space:]]*[0-9]+' "$$result_file" | awk -F: '{print $$2}' | tr -d ' ' | tail -n1 || true); \
	if [ -n "$$fails" ] && [ "$$fails" -gt 0 ]; then \
		echo "Issue detected: failed_transactions=$$fails (see $$result_file)"; \
		exit 1; \
	fi; \
	longest=$$(grep -Eo '"longest_transaction":[[:space:]]*[0-9.]+' "$$result_file" | awk -F: '{print $$2}' | tr -d ' ' | tail -n1 || true); \
	if [ -n "$$longest" ]; then \
		echo "Longest transaction: $$longest s"; \
	fi; \
	echo "Siege results saved to $$result_file";
endef

siege-test: $(NAME)
	$(call RUN_SIEGE_TEST,default,$(SIEGE_CONCURRENCY),$(SIEGE_DELAY),$(SIEGE_DURATION),$(SIEGE_TIMEOUT))

siege-baseline: $(NAME)
	$(call RUN_SIEGE_TEST,baseline,10,$(SIEGE_DELAY),1M,$(SIEGE_TIMEOUT))

siege-medium: $(NAME)
	$(call RUN_SIEGE_TEST,medium,50,$(SIEGE_DELAY),2M,$(SIEGE_TIMEOUT))

siege-heavy: $(NAME)
	$(call RUN_SIEGE_TEST,heavy,100,0,2M,$(SIEGE_TIMEOUT))

siege-stress: $(NAME)
	$(call RUN_SIEGE_TEST,stress,200,0,2M,$(SIEGE_TIMEOUT))

siege-static: $(NAME)
	@$(MAKE) --no-print-directory siege-test SIEGE_URL_FILE=$(SIEGE_STATIC_URL_FILE) SIEGE_CONCURRENCY=200 SIEGE_DELAY=0 SIEGE_DURATION=2M SIEGE_TIMEOUT=$(SIEGE_TIMEOUT_STATIC)

siege-cgi: $(NAME)
	@$(MAKE) --no-print-directory siege-test SIEGE_URL_FILE=$(SIEGE_CGI_URL_FILE) SIEGE_CONCURRENCY=120 SIEGE_DELAY=0 SIEGE_DURATION=2M SIEGE_TIMEOUT=$(SIEGE_TIMEOUT_CGI)

siege-diagnose: $(NAME)
	@echo "Running siege diagnose suite (static then cgi)..."
	@$(MAKE) --no-print-directory siege-test SIEGE_URL_FILE=$(SIEGE_STATIC_URL_FILE) SIEGE_CONCURRENCY=200 SIEGE_DELAY=0 SIEGE_DURATION=$(SIEGE_DIAG_DURATION) SIEGE_TIMEOUT=$(SIEGE_TIMEOUT_STATIC)
	@$(MAKE) --no-print-directory siege-test SIEGE_URL_FILE=$(SIEGE_CGI_URL_FILE) SIEGE_CONCURRENCY=120 SIEGE_DELAY=0 SIEGE_DURATION=$(SIEGE_DIAG_DURATION) SIEGE_TIMEOUT=$(SIEGE_TIMEOUT_CGI)
