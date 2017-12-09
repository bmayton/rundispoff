/*
rundispoff - tool for running a background command while the display is off
Copyright (c) 2017 Brian Mayton <bmayton@bdm.cc>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/dpms.h>

Display *dpy;
pid_t child;
char **args;
bool awake;

bool start_subprocess() {
    if(child != 0) {
        fprintf(stderr, "rundispoff: tried to start child when already running");
        return false;
    }

    child = vfork();
    if(child == 0) {
        if(execvp(args[0], args) != 0) {
            perror("rundispoff: failed to execute command");
            exit(1);
        }
    } else if(child == -1) {
        perror("rundispoff: vfork failed:");
        child = 0;
        return false;
    }

    fprintf(stderr, "rundispoff: child process %d started\n", child);
    return true;
}

bool stop_subprocess() {
    int status;
    if(child > 0) {
        fprintf(stderr, "rundispoff: sending SIGINT to child\n");
        int ret = kill(child, SIGINT);
        if(ret) {
            perror("rundispoff: failed to send SIGINT to child");
            return false;
        }
        usleep(10000);
        for(int i=10; i>0; i--) {
            int status;
            pid_t pidret = waitpid(child, &status, WNOHANG);
            if(pidret == child) {
                fprintf(stderr, "rundispoff: child %d exited with status %d\n",
                    child, WEXITSTATUS(status));
                child = 0;
                return true;
            } else if(pidret == -1) {
                perror("rundispoff: failed to wait for child process");
            }
            fprintf(stderr, "rundispoff: waiting for child to exit\n");
            sleep(1);
        }

        fprintf(stderr, "rundispoff: sending SIGKILL to child\n");
        ret = kill(child, SIGKILL);
        pid_t pidret = waitpid(child, &status, 0);
        if(pidret != child) {
            perror("rundispoff: failed to kill child");
            return false;
        }
        fprintf(stderr, "rundispoff: child %d killed\n", 
            child);
        child = 0;
        return true;
    } else {
        fprintf(stderr, "rundispoff: stop child called, but not running\n");
    }
    return true;
}

void sigint_handler(int signum) {
    fprintf(stderr, "rundispoff: caught SIGINT, exiting\n");
    if(child > 0) {
        stop_subprocess();
    }
    exit(0);
}

int main(int argc, char **argv) {
    child = 0;
    awake = true;

    int backoff = 10;
    int n = 0;

    if(argc < 2) {
        fprintf(stderr, "usage: %s <command> [arguments]\n",
            argv[0]);
        exit(2);
    }

    args = &argv[1];

    dpy = XOpenDisplay(NULL);
    if(!dpy) {
        fprintf(stderr, "rundispoff: failed to open display.\n");
        exit(1);
    }

    signal(SIGINT, sigint_handler);


    int status;
    while(true) {
        if(child > 0) {
            pid_t pidret = waitpid(child, &status, WNOHANG);
            if(pidret == child) {
                fprintf(stderr, "rundispoff: child exited with status %d\n",
                    WEXITSTATUS(status));
                child = 0;
            }
        }
        
        CARD16 power_level;
        BOOL dpms_state;
        DPMSInfo(dpy, &power_level, &dpms_state);

        if(power_level == DPMSModeOn) {
            awake = true;
            backoff = 10;
            n = 0;
            if(child > 0) {
                fprintf(stderr, "rundispoff: display woke up, stopping child\n");
                stop_subprocess();
            }
        } else {
            if(child == 0) {
                if(awake) {
                    fprintf(stderr, "rundispoff: display is asleep, starting child\n");
                    start_subprocess();
                    awake = false;
                } else if(++n >= backoff) {
                    n = 0;
                    backoff *= 2;
                    if(backoff > 300) backoff = 300;
                    fprintf(stderr, "rundispoff: restarting process\n");
                    start_subprocess();
                }
            }
        }

        sleep(1);
    }

    return 0;
}
