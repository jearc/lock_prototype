/* Constants defining how many writer and reader processes will be run
   in the simulation. Increasing the number of processes increases the
   state space which means the model will take longer to check. Six
   processes is the limit to how many I could feasibly run on my
   machine (checking takes about half an hour). I believe there are
   techniques and features to optimise this in Spin, but I'm not too
   familiar with them.
 */
#define NUM_WRITERS 3
#define NUM_READERS 3

/* This section contains the LTL formulae that will be checked by
   Spin. Each block beginning with 'ltl' defines one such
   formula. These formulae are checked individually. Preprocessor
   macros are used to abstract certain details and make the formulae
   more clear.
 */

/* The syntax proc_name[proc_index] is supported, but rather
   inconveniently the proc_index refers to the global list of
   processes as opposed to the processes of the specific type of
   process. Hence with three writers and three readers, the writers
   will be enumerated as processes 0, 1, 2 while the readers will be
   processes 3, 4, 5. These macros allow us instead to think of
   writers 0, 1, 2 and readers 0, 1, 2.
 */
#define writer_proc(n) writer[n]
#define reader_proc(n) reader[NUM_WRITERS + n]

/* Whether writer n is in its critical section. */
#define w_cs(n) (writer_proc(n)@cs)
/* Whether any writer is in its critical section. */
#define some_writer (w_cs(0) || w_cs(1) || w_cs(2))
/* Whether no writer is in its critical section. */
#define no_writer (!some_writer)
/* Whether reader n is currently trying to obtain the lock. */
#define r_try(n) (reader_proc(n)@try)
/* Whether reader n is in its critical section. */
#define r_cs(n) (reader_proc(n)@cs)
/* Whether any reader is in its critical section. */
#define some_reader (r_cs(0) || r_cs(1) || r_cs(2))
/* Whether all readers are in their critical sections simultaneously. */
#define all_readers (r_cs(0) && r_cs(1) && r_cs(2))

/* Formula representing mutual exclusion of writer processes. It must
   always hold that if writer 0 is in its critical section, then
   neither writer 1 nor writer 2 are in their critical sections,
   etc.
 */
ltl w_mutex { [] (  (w_cs(0) -> !(w_cs(1) || w_cs(2)))
                 && (w_cs(1) -> !(w_cs(0) || w_cs(2)))
                 && (w_cs(2) -> !(w_cs(0) || w_cs(1)))) };

/* Not yet implemented formula representing writer fairness, i.e. that
   if writer a tries to attain the lock before writer b tries to
   attain the lock, then writer a will attain the lock before writer
   b.
 */
ltl w_fair { true };

/* Formula representing mutual exclusion between writer processes and
   reader processes. It must always hold that if a writer is in its
   critical section, then no reader is in its critical section, and
   vice versa.
 */
ltl rw_mutex { [] !(some_writer && some_reader) };

/* Formula representing liveness of writer processes. It must always
   hold that all writers will eventually enter their critical
   sections.
 */
ltl w_liveness { [] ((<> w_cs(0)) && (<> w_cs(1)) && (<> w_cs(2))) };

/* Formula representing liveness of reader processes. It must always
   hold that all readers will eventually enter their critical
   sections.
 */
ltl r_liveness { [] ((<> r_cs(0)) && (<> r_cs(1)) && (<> r_cs(2))) };

/* Formula representing NON-parallelism of readers. That is, this
   formula asserts that it will never be the case that all readers are
   in their critical section simultaneously. Of course, this is the
   negation of what we want to prove, i.e. that it is possible for all
   readers to be in their critical sections simultaneously, as it is a
   readers-writer lock. In order to use this formula, we check it and
   if Spin produces a counterexample, we consider it successful,
   i.e. existence of parallelism is proved.
 */
ltl r_nonparallelism { [] !all_readers };

/* Formula representing the 'boundedness' property: that a reader
   trying to enter its critical section need only wait on at most one
   writer. The macro reader_n_bounded(n) whether this holds for a
   reader n, and the ltl formula is a conjunction of such for all
   readers. Inside reader_n_bounded we have a enhancement of the
   following formula for one-bounded overtaking with two processes,
   taken from section 5.9.4 of the textbook Principles of the Spin
   Model Checker (2008) by Mordechai Ben-Ari.

       p_try -> (!q_cs U (q_cs U (!q_cs U p_cs)))

   We enhance the above formula from bounding how long p needs to wait
   on another process q to bounding how long a single reader process
   needs to wait on all writer processes, i.e. from a single process
   interacting on a single other process, to a single process
   interacting on three other processes.
*/
#define reader_n_bounded(n)                                           \
    (r_try(n) -> (no_writer U (  (w_cs(0) U (no_writer U r_cs(n)))    \
                              || (w_cs(1) U (no_writer U r_cs(n)))    \
                              || (w_cs(2) U (no_writer U r_cs(n))))))
ltl bounded { [] (  reader_n_bounded(0)
                 && reader_n_bounded(1)
                 && reader_n_bounded(2)) };


/* This section contains the algorithm that is run by the model
   checker. It consists of global variables, a writer process and a
   reader process.

   The algorithm maintains a CLH queue for writer processes, and then
   contains extra state for alternating lock acquisitions between
   writers and readers. This extra machinery deals only with the head
   of the writer queue; the rest of the writers only deal with the CLH
   queue. Given a constant stream of new writers and readers trying to
   acquire the lock, the lock will alternate between individual
   writers and batches of readers. This alternation is captured by the
   bools representing 'turns', current_writer_turn and
   completed_writer_turn (while they use a bool type, they do not
   represent truth; it may be more useful to think of these turns as
   integers mod 2) and the size 2 array reader_cohorts.

   Readers interacting with the lock are divided into two cohorts:
   those who own the lock and those who are waiting on the current
   head of the writer queue. When the head of the writer queue tries
   to acquire the lock, it waits until the cohort of readers who own
   the lock decreases to zero in number. When a reader tries to
   acquire the lock, it checks the current writer turn and waits until
   this turn is completed--in this way only waiting on at most one
   writer.
 */

bool current_writer_turn;
bool completed_writer_turn;
byte reader_cohorts[2];

bool writer_locked[NUM_WRITERS + 1];
byte writer_tail_queue_index;

active [NUM_WRITERS] proctype writer()
{
    byte queue_index = _pid + 1;
    byte pred_queue_index = 0;

    do ::
        writer_locked[queue_index] = 1;
        atomic {
            pred_queue_index = writer_tail_queue_index;
            writer_tail_queue_index = queue_index;
        }
        (!writer_locked[pred_queue_index]);

        current_writer_turn = !current_writer_turn;
        (reader_cohorts[!current_writer_turn] == 0);

cs:     completed_writer_turn = current_writer_turn;

        writer_locked[queue_index] = 0;
        queue_index = pred_queue_index;
    od
}

active [NUM_READERS] proctype reader()
{
    bool observed_writer_turn;

    do ::
        reader_cohorts[0]++;
        reader_cohorts[1]++;
        observed_writer_turn = current_writer_turn;
        reader_cohorts[!observed_writer_turn]--;

try:    (completed_writer_turn == observed_writer_turn);

cs:     reader_cohorts[observed_writer_turn]--;
    od
}
