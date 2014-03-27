# Sean Peters
# 26/3/2014
# Stress tests MCS, TTAS, and ticket lock algorithms with only one lock

results="../high_contention.res"
cd libslock
rm -f $results
touch $results

echo "     Throughput of different lock algorithms using a single lock     " >> $results
echo "=====================================================================" >> $results

for threads in 1 2 3 4; do
    echo "threads: $threads" >> $results
    ./stress_one_mcss -l 1 -n $threads >> $results
    ./stress_one_ttass -l 1 -n $threads >> $results
    ./stress_one_tickets -l 1 -n $threads >> $results
done

echo "=====================================================================" >> $results
