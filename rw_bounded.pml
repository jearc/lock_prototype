// if the assertion does not hold, then you have existence of reader parallelism
#define CHECK_READER_PARALLELISM 1

byte ncrit;             // nr of writer procs in critical section
int reader_sum;

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
    
#if CHECK_READER_PARALLELISM
    reader_sum = 0;
    reader_sum = reader_sum + 1;
    assert(reader_sum != 4);
#endif

    waiting_readers[observed_turn]--;

    goto again
}
