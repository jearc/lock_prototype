# Sean Peters
# 26/3/2014
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

echo "=====================================================================" >> $results
