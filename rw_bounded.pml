byte ncrit;             // nr of procs in critical section

bool turn;
bool completed_turn;
int waiting_readers[2];

bool writer_spinlock;

active [3] proctype writer()
{
    bool observed_turn;

again:
    atomic {
        if
        :: (writer_spinlock == 0) -> { writer_spinlock = 1 }
        :: else -> { goto again }
        fi
    }

    atomic {
        turn = !turn;
        observed_turn = turn;
    }
    
    (waiting_readers[!observed_turn] == 0);

    ncrit++;
    assert(ncrit == 1);
    ncrit--;

    completed_turn = observed_turn;
    writer_spinlock = 0;

    goto again;
}

active [4] proctype reader()
{
    bool observed_turn;

again:
    waiting_readers[0]++;
    waiting_readers[1]++;
    observed_turn = turn;
    waiting_readers[!observed_turn]--;

    (completed_turn == observed_turn);

    assert(ncrit != 1);

    waiting_readers[observed_turn]--;

    goto again
}
