# Sean Peters
# 26/3/2014
# Stress tests MCS, TTAS, and ticket lock algorithms with 512 locks

results="../low_contention.res"
cd libslock
rm -f $results
touch $results

echo "       Throughput of different lock algorithms using 512 locks       " >> $results
echo "=====================================================================" >> $results

for threads in 1 2 3 4; do
    echo "threads: $threads" >> $results
    ./stress_test_mcss -l 512 -n $threads >> $results
    ./stress_test_ttass -l 512 -n $threads >> $results
    ./stress_test_tickets -l 512 -n $threads >> $results
done

echo "=====================================================================" >> $results
