# Sean Peters
# 26/3/2014
# Run all libslock benchmarks found in "Everything you wanted to know about sync... "
# on the Freescale Sabre IMX6 (ARM)

p=0     # delay (cycles) between lock attempts
a=100   # duration (cycles) of lock hold

# Runs each libslock atomic benchmark

results="../atomic_bench.res"
cd libslock
rm -f $results
touch $results

echo "Throughput of different atomic operations on a single memory location" >> $results
echo "=====================================================================" >> $results

# -b 1 only count successful atomic operations
for threads in 1 2 3 4; do
    echo "threads: $threads" >> $results
    ./atomic_bench_cas -n $threads -b 1 -p $p >> $results
    ./atomic_bench_tas -n $threads -b 1 -p $p >> $results
    ./atomic_bench_fai -n $threads -b 1 -p $p >> $results
    ./atomic_bench_swap -n $threads -b 1 -p $p >> $results
done

cd -
echo "=====================================================================" >> $results

# Runs each libslock uncontested latency benchmark

results="../uncontested_lat.res"
cd libslock
rm -f $results
touch $results

echo "                Uncontested lock aquisition latency                  " >> $results
echo "=====================================================================" >> $results

./uncontended_mcss >> $results
./uncontended_ttass >> $results
./uncontended_tickets >> $results

cd -
echo "=====================================================================" >> $results

# Stress tests MCS, TTAS, and ticket lock algorithms with only one lock

results="../high_contention.res"
cd libslock
rm -f $results
touch $results

echo "     Throughput of different lock algorithms using a single lock     " >> $results
echo "=====================================================================" >> $results

for threads in 1 2 3 4; do
    echo "threads: $threads" >> $results
    ./stress_test_mcss -l 1 -n $threads -p $p -a $a >> $results
    ./stress_test_ttass -l 1 -n $threads -p $p -a $a >> $results
    ./stress_test_tickets -l 1 -n $threads -p $p -a $a >> $results
done

cd -
echo "=====================================================================" >> $results

# Stress tests MCS, TTAS, and ticket lock algorithms with 512 locks

results="../low_contention.res"
cd libslock
rm -f $results
touch $results

echo "       Throughput of different lock algorithms using 512 locks       " >> $results
echo "=====================================================================" >> $results

for threads in 1 2 3 4; do
    echo "threads: $threads" >> $results
    ./stress_test_mcss -l 512 -n $threads -p $p -a $a >> $results
    ./stress_test_ttass -l 512 -n $threads -p $p -a $a >> $results
    ./stress_test_tickets -l 512 -n $threads -p $p -a $a >> $results
done

cd -
echo "=====================================================================" >> $results

# Throughput and scalability of locks depending on the number of locks.

results="../scalability.res"
cd libslock
rm -f $results
touch $results

echo "Throughput and scalability of locks depending on the number of locks " >> $results
echo "=====================================================================" >> $results

for locks in 4 16 32 128; do
    for threads in 1 2 3 4; do
        echo "locks: $locks, threads: $threads" >> $results
        ./stress_test_mcss -l $locks -n $threads -p $p -a $a >> $results
        ./stress_test_ttass -l $locks -n $threads -p $p -a $a >> $results
        ./stress_test_tickets -l $locks -n $threads -p $p -a $a >> $results
    done
done

cd -
echo "=====================================================================" >> $results
