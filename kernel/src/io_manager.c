#include <io_manager.h>
void io_block(){
    // Check interfaces_list and retrieve socket related to that interface name
    // non-available -> Move to exit
    // open a detached_thread:
        // send to io
        // move PCB to blocked
        // busy -> send and they will wait on i/o
        // wait for response. pcb_updated?
    // move from blocked -> ready with prev_state
    

}
