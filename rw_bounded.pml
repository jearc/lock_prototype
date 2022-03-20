#define NUM_WRITERS 3
#define NUM_READERS 2
#define CHECK_READER_PARALLELISM 0

byte ncrit;             // nr of writer procs in critical section
int reader_sum;

bool turn;
bool completed_turn;
int waiting_readers[2];

bool writer_locked[NUM_WRITERS + 1];
int writer_tail;

active [NUM_WRITERS] proctype writer()
{
    bool observed_turn;
    int pred = 0;

    int my_index = _pid + 1;
    int my_pred_index = 0;

again:
    writer_locked[my_index] = 1;
    atomic {
        pred = writer_tail;
        writer_tail = my_index;
    }
    (!writer_locked[pred]);
    my_pred_index = pred;

    atomic {
        turn = !turn;
        observed_turn = turn;
    }
    
    (waiting_readers[!observed_turn] == 0);

    ncrit++;
    assert(ncrit == 1);
    ncrit--;

    completed_turn = observed_turn;

    writer_locked[my_index] = 0;
    my_index = my_pred_index;

    goto again;
}

active [NUM_READERS] proctype reader()
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
    assert(reader_sum != NUM_READERS);
#endif

    waiting_readers[observed_turn]--;

    goto again
}
