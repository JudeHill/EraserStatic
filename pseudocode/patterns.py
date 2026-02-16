from preamble import *
def write_then_read():
    # many threads
    lock(l)
    write(shared_var)
    unlock(l)

    wait(b) 
    # Can now read without a lock
    read(shared_var)

def switch_locks():
    lock(l1)
    write(shared_var)
    unlock(l1)

    wait(b)

    lock(l2)
    write(shared_var)
    unlock(l2)


    





    