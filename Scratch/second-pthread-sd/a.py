import os, re, string, operator, datetime
from datetime import date

numthread=8

def run_once(n,inf_name,outdir):
	numthread=n
	inputfilename=inf_name
	arglist=' -nnbrs=20 -minsim=.3 '+inputfilename
	inputdatarawname=os.path.splitext(os.path.basename(inputfilename))[0]
	# signle thread running here


	# multiple thread
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


#print datetime.datetime.now().strftime('%b-%d-%I%M%p-%G')
timestring=datetime.datetime.now().strftime('%m%d_%H%M')
print timestring
outputdir='output'+timestring+'/'
if not os.path.exists(outputdir):
    os.makedirs(outputdir)
# datapath='/export/scratch/csci5451_s13/hw2_data/'
datapath='/home/xuxxx625/p/ahw2/hw2_data/'

filelist=os.listdir(datapath)
filelist=filelist[-1:]
filelist=['sports.mat']
matfilelist=[datapath+file for file in filelist if file.endswith(".mat")]
matfilelist=sorted(matfilelist,key=os.path.getsize)
print matfilelist

for f in matfilelist:
	print "\n==============================================\n",f
	run_once(numthread, f,outputdir)
