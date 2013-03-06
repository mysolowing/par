import os, re, string, operator, datetime
from datetime import date

def run_once(n,inf_name,outdir):
	numthread=n
	inputfilename=inf_name
	arglist=' -nnbrs=50 -minsim=.3 '+inputfilename
	inputdatarawname=os.path.splitext(os.path.basename(inputfilename))[0]
	# signle thread running here
	outputfilename_serial=outdir+inputdatarawname+'_s.nbrs'
	outputfilename_serial_reorder=outdir+inputdatarawname+'_rs.nbrs'
	logfilename_serial=outdir+inputdatarawname+'_s.log'
	os.system('touch '+logfilename_serial)
	cmd_s='\n./sd_serial '+arglist+' '+outputfilename_serial +' >> '+logfilename_serial
	print cmd_s
	os.system(cmd_s)
	os.system('cat '+logfilename_serial)

	os.system('touch '+outputfilename_serial_reorder)
	cmd_sort_s='sort -n -k 1 -k 2 '+outputfilename_serial+ '>> '+outputfilename_serial_reorder
	print cmd_sort_s
	os.system(cmd_sort_s)

	multiple thread
	source_sdname='sd.c'
	print "\nThread num %d. \t Data file %s. Rawname %s\n"%(numthread,inputfilename,inputdatarawname)
	numthread_w=r'#define NTHREADS '+str(numthread)
	print numthread_w
	find_nthreads=re.compile('#define NTHREADS .*')
	source_code_fin=open(source_sdname,'r')
	m=re.sub(find_nthreads,numthread_w,source_code_fin.read())
	source_code_fin.close()
	source_code_fout=open('sd.c','w')
	source_code_fout.write(m)
	source_code_fout.close()
	os.system('make clean')
	os.system('make')

	outputfilename_parallel=outdir+inputdatarawname+'_p.nbrs'
	outputfilename_parallel_reorder=outdir+inputdatarawname+'_rp.nbrs'
	logfilename_parallel=outdir+inputdatarawname+'_p.log'

	os.system('touch '+logfilename_parallel)
	cmd_p='\n./sd '+arglist+' '+outputfilename_parallel+' >> '+logfilename_parallel
	print cmd_p
	os.system(cmd_p)
	os.system('cat '+logfilename_parallel)

	os.system('touch '+outputfilename_parallel_reorder)
	cmd_sort_p='sort -n -k 1 -k 2 '+outputfilename_parallel+ '>> '+outputfilename_parallel_reorder
	print cmd_sort_p
	os.system(cmd_sort_p)

	# Compare and show difference
	difffilename=outdir+inputdatarawname+'_diff.nbrs'
	cmd_diff='diff '+outputfilename_serial_reorder+' '+ outputfilename_parallel_reorder+'> '+difffilename
	print cmd_diff
	os.system(cmd_diff)

	print ('head -n 20 '+difffilename)
	os.system('head -n 20 '+difffilename)

#print datetime.datetime.now().strftime('%b-%d-%I%M%p-%G')
timestring=datetime.datetime.now().strftime('%m%d_%H%M')
print timestring
outputdir='output'+timestring+'/'
if not os.path.exists(outputdir):
    os.makedirs(outputdir)
datapath='/export/scratch/csci5451_s13/hw2_data/'
# datapath='/home/xuxxx625/p/ahw2/hw2_data/'
numthread=8
filelist=os.listdir(datapath)
#filelist=filelist[-1:]
matfilelist=[datapath+file for file in filelist if file.endswith(".mat")]
matfilelist=sorted(matfilelist,key=os.path.getsize)
print matfilelist

for f in matfilelist:
	print "\n==============================================\n",f
	run_once(numthread, f,outputdir)
