# Sean Peters
# 26/3/2014
# Runs each libslock atomic benchmark

results="../atomic_bench.res"
cd libslock
rm -f $results
touch $results

echo "Throughput of different atomic operations on a single memory location" >> $results
echo "=====================================================================" >> $results

for threads in 1 2 3 4; do
    echo "threads: $threads" >> $results
    ./atomic_bench_cas -n $threads >> $results
    ./atomic_bench_tas -n $threads >> $results
    ./atomic_bench_fai -n $threads >> $results
    ./atomic_bench_swap -n $threads >> $results
done

echo "=====================================================================" >> $results
