# Sean Peters
# 26/3/2014
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
        ./stress_test_mcss -l $locks -n $threads >> $results
        ./stress_test_ttass -l $locks -n $threads >> $results
        ./stress_test_tickets -l $locks -n $threads >> $results
    done
done

echo "=====================================================================" >> $results
