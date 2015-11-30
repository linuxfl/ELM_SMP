import os
#user and sample information
USER = "fangling"
SAMPLE_FILE_DIR = "/home/fangling/ELM_SMP/sample"
SOURCE_SAMPLE_DIR = "../../TrainingDataSet/mv"
HOSTFILE = "../hostfile"
METHOD = "node_type" #method is 'process_type' or 'node_type'

numberofprocess = int(input("please input the number of the process:"))
fileline = int(input("please input the number line of the source data file:"))
eachline = int(fileline/numberofprocess)
numberofnode = 0
hostdict = {}
hostlist = []
hpmap = []
strline = ""

class getoutofloop(Exception): pass
    
#get the host information
fp_host = open(HOSTFILE,"r")
strline = fp_host.readline()
while strline:
    pos = strline.find(":")
    host = strline[0:pos]
    if strline[(len(strline)-1)] == '\n':
        np = strline[(pos+1):(len(strline)-1)]
    else:
        np = strline[(pos+1):len(strline)]
    hostdict[host] = np
    hostlist.append(host)
    strline = fp_host.readline()
fp_host.close()

#ceate the map of the node and process
#init the the map of host and process
for i in range(0,int(len(hostlist))):
    l = []
    hpmap.append(l)
    
rank = 0
filter_round = 0
try:
    while True:
        for i in range(0,int(len(hostlist))):
            j = 0
            while j < int(hostdict[hostlist[i]]):
                if rank >= numberofprocess:
					if j == 0:
						i = i - 1
					raise getoutofloop()
                j += 1
                hpmap[i].append(rank)
                rank += 1
	filter_round = 1
except getoutofloop:
    pass

#get the number of arranging node
if filter_round == 1:
	numberofnode = int(len(hostlist))
elif filter_round == 0:
	numberofnode = i+1
else:
	print("error flag value!")
print(numberofnode)

#clean up the sample direction
for i in range(0,int(len(hostlist))):
    command = "ssh "+hostlist[i]+" rm "+SAMPLE_FILE_DIR+"/* -rf"
    print(command)
    #os.system(command)
    

#read data file and split it by number of process
fp_data = open(SOURCE_SAMPLE_DIR,"r")
if METHOD == "process_type":
	for i in range(0,numberofprocess):
		fp = open(str(i),"w")
		line = fp_data.readline()
		count = 0
		while True:
			fp.write(line)
			count += 1
			if count > eachline:
				break
			line = fp_data.readline()
		fp.close()
	fp_data.close()
	#scp the file to other host
	for i in range(0,numberofnode):
		for j in hpmap[i]:
			#example :scp 1 fangling@node1:/home/fangling/xxxx/
			command = "scp "+ str(j)+" "+USER+"@"+str(hostlist[i])+":"+SAMPLE_FILE_DIR
			print(command)
			#os.system(command)
elif METHOD == "node_type":
	for i in range(0,numberofnode):
		fp = open(str(i),"w")
		line = fp_data.readline()
		count = 0
		nodeline = eachline * int(hostdict[hostlist[i]])
		while True:
			fp.write(line)
			count += 1
			if count > nodeline:
				break
			line = fp_data.readline()
		fp.close()
	fp_data.close()
	#scp the file to other host
	for i in range(0,numberofnode):
		#example :scp 1 fangling@node1:/home/fangling/xxxx/
		command = "scp "+ str(i)+" "+USER+"@"+str(hostlist[i])+":"+SAMPLE_FILE_DIR
		print(command)
		#os.system(command)
else:
	print("no support distribution type!")
	
#delete the every file in local
for i in range(0,numberofprocess):
    command = "rm "+str(i)+" -rf"
    os.system(command)
print("success...")
