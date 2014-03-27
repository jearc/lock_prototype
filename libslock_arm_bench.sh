# Sean Peters
# 26/3/2014
# Run all libslock benchmarks found in "Everything you wanted to know about sync... "
# on the Freescale Sabre IMX6 (ARM)

# Runs each libslock atomic benchmark

results="../atomic_bench.res"
cd libslock
touch $results

echo "Throughput of different atomic operations on a single memory location" >> $results
echo "=====================================================================" >> $results

for threads in 1 2 3 4; do
    echo "threads: $threads" >> $results
    ./atomic_bench_cas -t $threads >> $results
    ./atomic_bench_tas -t $threads >> $results
    ./atomic_bench_fai -t $threads >> $results
    ./atomic_bench_swap -t $threads >> $results
done

echo "=====================================================================" >> $results

# Runs each libslock uncontested latency benchmark

results="../uncontested_lat.res"
cd libslock
touch $results

echo "                Uncontested lock aquisition latency                  " >> $results
echo "=====================================================================" >> $results

./uncontended_mcss >> $results
./uncontended_ttass >> $results
./uncontended_tickets >> $results

echo "=====================================================================" >> $results

# Stress tests MCS, TTAS, and ticket lock algorithms with only one lock

results="../stress_one.res"
cd libslock
touch $results

echo "     Throughput of different lock algorithms using a single lock     " >> $results
echo "=====================================================================" >> $results

for threads in 1 2 3 4; do
    echo "threads: $threads" >> $results
    ./stress_one_mcss -l 1 >> $results
    ./stress_one_ttass -l 1>> $results
    ./stress_one_tickets -l 1>> $results
done

echo "=====================================================================" >> $results

# Stress tests MCS, TTAS, and ticket lock algorithms with 512 locks

results="../stress_test.res"
cd libslock
touch $results

echo "       Throughput of different lock algorithms using 512 locks       " >> $results
echo "=====================================================================" >> $results

for threads in 1 2 3 4; do
    echo "threads: $threads" >> $results
    ./stress_test_mcss -l 512 >> $results
    ./stress_test_ttass -l 512 >> $results
    ./stress_test_tickets -l 512 >> $results
done

echo "=====================================================================" >> $results

# Throughput and scalability of locks depending on the number of locks.

results="../scalability.res"
cd libslock
touch $results

echo "Throughput and scalability of locks depending on the number of locks " >> $results
echo "=====================================================================" >> $results

for locks in 4 16 32 128; do
    for threads in 1 2 3 4; do
        echo "locks: $locks, threads: $threads" >> $results
        ./stress_test_mcss -l $locks -t $threads >> $results
        ./stress_test_ttass -l $locks -t $threads >> $results
        ./stress_test_tickets -l $locks -t $threads >> $results
    done
done

echo "=====================================================================" >> $results
