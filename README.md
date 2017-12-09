# rundispoff

This is a simple program for executing a background task while the X display
is powered off (in DPMS mode).  When the display goes to sleep, it will start
the command provided.  When the display wakes up, SIGINT is sent to the child
process.  If the child process does not exit after 10 seconds, SIGKILL is sent
to the child.

This may be useful for running GPU-intensive tasks that I want to run while
the machine is idle, but should exit while there's something worth looking at
on the display that the GPU task may make sluggish.  The subprocess must be
okay with being killed, or should clean up any state it needs to write to disk
and exit when it receives the SIGINT.

Usage:

    $ rundispoff <command> [any arguments to pass to the command]


