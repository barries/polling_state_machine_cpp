include make_config.mak

################################################################################
# Top level targets

.PHONY:        \
  all          \
  bin          \
  clean        \

all: bin

clean:
	rm -rf stoplights

bin: stoplights

stoplights: stoplights.cpp polling_state_machine.h X_macro_helpers.h
	$(CXX) -std=gnu++2b -Wall -Wpedantic -Werror "$<" -o "$@"

################################################################################
