from subprocess import check_output
from numpy import mean, std
locks = ["spinlock","mcs","array","ticket","ttas","ttas_simp"]
# (locks,lock holding time, pause time between aquisitionss)
configs =[(1,0,0),(1,10,1000),(1,250,1000),(1,1000,1000)]
num_samples = 3 

for (l,h,p) in configs: 
    print ("locks: " + str(l) + " acquire time: " + str(h) + " pause time: " +
		str(p))
    legend = ["threads"]
    for t in [1,2,3,4]:
	results =[t]
        for lock in locks:
	    samples = []
	    i = 0

	    dummy_run = check_output(["libslock/stress_test_" + lock + "s", "-p " + str(p)
			, "-a " + str(h), "-l " + str(l), "-n " + str(t)])
	    while i < num_samples:
                i = i + 1
		output = check_output(["libslock/stress_test_" + lock + "s", "-p " + 
			str(p), "-a " + str(h), "-l " + str(l), "-n " + str(t)])
		print (lock + " tmp: " + output)
		samples = samples+[float(output)]
	    smean = mean(samples)
            if t == 1:
	        legend = legend + [lock]
	    results = results + [smean]
	    sstd = std(samples,ddof=1)
	    print (lock + ":" + str(smean)+ "," + str(sstd/smean))
        if t == 1:
            print str(legend)
    	print ("results: " + str(results))
