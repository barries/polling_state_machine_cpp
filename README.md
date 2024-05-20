# An Example Polling State Machine in C++

A polling state machine is called periodically to check inputs or data structures
and changes states when conditions or events are detected. Normal, conventional state
machines have events modeled as data and delivered to the machine from outside it.
You can think of this as "pull mode", where the polling state machine "pulls" data
from upstream sources (inputs, queues, data structures) whereas events are "pushed"
into a conventional state machine.

Polling state machines are common in bare metal MCU platforms, or in places where
a periodic call is preferred over a normal, reactive state machine. This can result
in shorter, clearer, more imperative code.

See polling_state_machine.h for more details.
