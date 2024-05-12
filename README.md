# An Example Polling State Machine in C++

A polling state machine is called periodically to check inputs or data structures
and changes states when conditions or events are detected, as opposed to normal state
machines where events are generally modeled as data and delivered to to machine from
outside it.

Polling state machines are common in bare metal MCU platforms, or in places where
a periodic call is preferred over a normal, reactive state machine. This can result
in shorter, clearer, more imperative code.

See polling_state_machine.h for more details.
