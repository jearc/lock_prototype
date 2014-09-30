from subprocess import check_output
from numpy import mean, std
locks = ["clh","spinlock","mcs","array","ticket","ttas","ttas_simp","rw","rw_fair","rw_scalable_fair"]
# locks = ["ttas","rw","rw_fair"]
# (locks,lock holding time, pause time between aquisitionss)
configs =[(1,0,0),(1,300,300),(1,300,1250),(1,300,5000)]
#configs =[(1,100,1000),(1,0,0),(1,10,1000),(1,250,1000),(1,1000,1000),(1,10000,10000)]
num_samples = 10 

for (l,h,p) in configs: 
    print ("locks: " + str(l) + " acquire time: " + str(h) + " pause time: " +
		str(p))
    fname = "x86l"+str(l)+"a"+str(h)+"p"+str(p)+"_delay.dat"
    file = open(fname,'w')
    legend = ["threads"]
    for t in [1,2,3,4]:
	results =[t]
        for lock in locks:
	    samples = []
	    i = 0

	    dummy_run = check_output(["libslock/stress_test_" + lock + "s", "-p " + str(p)
			, "-a " + str(h), "-l " + str(l), "-n " + str(t), "-d 1000"])
	    while i < num_samples:
                i = i + 1
		output = check_output(["libslock/stress_test_" + lock + "s", "-p " + 
			str(p), "-a " + str(h), "-l " + str(l), "-n " + str(t), "-d 10000", "-c 4"])
		print (lock + " tmp: " + output)
		samples = samples+[float(output)]
	    smean = mean(samples)
            if t == 1:
	        legend = legend + [lock]
	    sstd = std(samples,ddof=1)
	    results = results + [smean, sstd/smean]
	    print (lock + ":" + str(smean)+ "," + str(sstd/smean))
        if t == 1:
            print str(legend)
    	print ("results: " + str(results))
        for v in results:
  	    file.write(str(v) + " ")
        file.write("\n")
    file.close()
