# Bugs found so far
# Variable name initialisation in the graph for both mutexes and shared variables does NOT work if the variable isnt global, but is passed as an argument to the 
# thread function called using pthreads_create. Will have to investigate how common this is to see if this is an issue
# If shared var is passed in this way, the graph will not even create read/write nodes for reads/writes (but does create lock/unlock)
# nodes just without a var name.

lockset = {}

variable_locksets = {x: {all_locks}} 

def visit(v, lockset):
    if v == lock(x):
        lockset.add(x)
    elif v == unlock(x):
        lockset.remove(x)
    elif v == read(x):
        if v.state == Exclusive and v.owning_thread != tid:
            v.lockset = v.lockset.intersect(lockset)
            if not v.lockset:
                reportDataRace()
    elif v == write(x):
        v.state = Exclusive
        v.owner = tid
        v.lockset = v.lockset.intersect(lockset)
        if not v.lockset:
            reportDataRace()
    elif v == call(f):
        visit(f, lockset)
    visit(v.next, lockset)



