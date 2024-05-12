# Configure make + bash to be faster, more powerful, and more reliable.

# Disable built-in implicit rules for speed and the clarity of explicit rules
MAKEFLAGS += --no-builtin-rules
SUFFIXES :=
.SUFFIXES:

.DELETE_ON_ERROR: # Delete targets that have changed if a recipe fails

.ONESHELL:        # Launch one shell per recipe for speed and multiline shell contructs

SHELL=bash

.SHELLFLAGS := $(.SHELLFLAGS) -o errexit -o errtrace -o nounset -o pipefail # nounset: unset variables cause errors; errexit: exit on first errored command; errtrace: ERR trap inherited by functions; pipefail: exit on non-last cmd in a pipeline fails

# Variables to work around make variable definition parsing issues
EMPTY_STRING  :=#
SPACE :=$(EMPTY_STRING) # A single space

VERBOSE :=
ifneq ($(VERBOSE),)
  .SHELLFLAGS := -x $(.SHELLFLAGS) # -x: trace shell cmds
endif

QUIET := @
TRACE :=
ifeq ($(TRACE),disable)
  override TRACE =
else ifeq ($(TRACE),enable)
  override TRACE = $(QUIET)echo $(subst .tmp,,$(@F))  # Recursive variable so $(@F) happens when recipe runs
endif

ifneq ($(QUIET),)
  # Suppress recipe printing, which include the "rm ..." command output from .INTERMEDATE file cleanup
  MAKEFLAGS += --silent --warn-undefined-variables
endif

