#include "SerScan.h"
#include "omp.h"

#define EXATRA 100
#define MIN(a,b) (((a)<(b))?(a):(b))

void cmd_parse(int argc, char *argv[], params_t *par);
void Scan_Serial_Seq(params_t * par);
void WriteOut(params_t * par);
void cleanup(params_t *par);
void OMP_Scan(params_t *par);

void OMP_Sscan(params_t *par);
void OMP_Sscan_Init(params_t *par);

int main(int argc, char *argv[]) {
	params_t par;
	double timer_scan,timer_serial;
	gk_clearwctimer(par.timer_global);
	gk_clearwctimer(par.timer_1);
	gk_clearwctimer(par.timer_2);
	gk_clearwctimer(par.timer_3);
	gk_clearwctimer(par.timer_4);
	gk_startwctimer(par.timer_global);
	int k;
	printf("\nScan - OMP_Scan\n");
	gk_startwctimer(par.timer_4);
	cmd_parse(argc, argv, &par);
	gk_stopwctimer(par.timer_4);

	memcpy(par.a,par.b,(par.nalloc + 1)*sizeof(int));
	OMP_Scan(&par);
	timer_scan=par.timer_2;

	for (k = 0; k < EXATRA; ++k) {
		memcpy(par.a,par.b,(par.nalloc + 1)*sizeof(int));
		OMP_Scan(&par);
		timer_scan=MIN(timer_scan,par.timer_2);
	}
	par.timer_2=timer_scan;

	Scan_Serial_Seq(&par);
	timer_serial=par.timer_3;
	for (k = 0; k < EXATRA; ++k) {
		Scan_Serial_Seq(&par);
		timer_serial=MIN(timer_serial,par.timer_3);
	}
	par.timer_3=timer_serial;

	for (k=0;k<par.nlines;++k){
		if (par.c[k]!=par.d[k]){
			printf("ERROR occur: k=%d, c=%d, d=%d\n",k,par.c[k],par.d[k]);

		}
	}


	WriteOut(&par);

	for (k = 0; k < EXATRA; ++k) {
		OMP_Sscan(&par);
	}


	gk_stopwctimer(par.timer_global);

	printf("  wclock         (sec): \t%.8lf\n",
			gk_getwctimer(par.timer_global));
	printf("  timer4  Init   (sec): \t%.8lf\n", gk_getwctimer(par.timer_4));
	printf("  timer3  Serial (sec) on %d runs: \t%.8lf\n", EXATRA,
			gk_getwctimer(par.timer_3) );
	printf("  timer2  Scan   (sec) on %d runs: \t%.8lf\n", EXATRA,
			gk_getwctimer(par.timer_2) );
	printf("  timer1  Sscan  (sec) on %d runs: \t%.8lf\n", EXATRA,
			gk_getwctimer(par.timer_1) );
	cleanup(&par);
	return 0;
}


void OMP_Sscan_Init(params_t *par) {

	if (par->f != NULL ) {
		fprintf(stderr, "ERROR: Flag array pointer is not NULL\n");
		exit(5);
	}
	par->f = (char*) calloc((1 + par->nalloc), sizeof(char));
	if (par->f == NULL ) {
		fprintf(stderr, "ERROR: Fail to create flag array\n");
		exit(6);
	}
	if (par->fb != NULL ) {
		fprintf(stderr, "ERROR: Flag array pointer is not NULL\n");
		exit(5);
	}
	par->fb = (char*) malloc((1 + par->nalloc) * sizeof(char));
	if (par->fb == NULL ) {
		fprintf(stderr, "ERROR: Fail to create flag backup array\n");
		exit(6);
	}
	par->f[0] = 1;
	int k, i, ct = 0;
	srand(time(NULL ));
	printf("Sscan_INIT, generating %d entries with flag 1\n",
			(int) floor(sqrt(par->nlines)));
	while (ct < (int) floor(sqrt(par->nlines))) {
		int r = rand();
		i = rand() % par->nlines;
		if (par->f[i] == 0) {
			par->f[i] = 1;
			++ct;
		}

	}

	memcpy(par->fb, par->f, (1 + par->nalloc) * (sizeof(char)));
}

void OMP_Sscan(params_t *par) {
	omp_set_num_threads(par->nthreads);
	int nlevels = (int) ceil(log(par->nlines) / M_LN2);
	int d, k, t;
	int levelstep2d = 1, levelstep2d1;

	/* ****************** UP SWEEP ******************************/
	gk_startwctimer(par->timer_1);
	for (d = 1; d < par->nlevels; ++d) {
		levelstep2d1 = levelstep2d * 2;
#pragma omp parallel for shared(par,levelstep2d,levelstep2d1) \
		schedule(static)
		for (k = 0; k < par->nalloc; k = k + levelstep2d1) {
			if (!par->f[k + levelstep2d1 - 1])
				par->a[k + levelstep2d1 - 1] = par->a[k + levelstep2d - 1]
						+ par->a[k + levelstep2d1 - 1];
			par->f[k + levelstep2d1 - 1] = par->f[k + levelstep2d - 1]
					| par->f[k + levelstep2d1 - 1];
		}
		levelstep2d = levelstep2d * 2;
	}

	/* ****************** DOWN SWEEP ******************************/
	par->a[par->nalloc - 1] = 0;

	levelstep2d = par->nalloc / 2;

	for (d = par->nlevels - 1; d >= 0; --d) {
		levelstep2d1 = levelstep2d * 2;
#pragma omp parallel for shared(par,levelstep2d,levelstep2d1) \
		schedule(static)
		for (k = 0; k < par->nalloc; k = k + levelstep2d1) {
			t = par->a[k + levelstep2d - 1];
			par->a[k + levelstep2d - 1] = par->a[k + levelstep2d1 - 1];

			if (par->fb[k + levelstep2d]) {
				par->a[k + levelstep2d1 - 1] = 0;
			} else if (par->f[k + levelstep2d - 1]) {
				par->a[k + levelstep2d1 - 1] = t;
			} else {
				par->a[k + levelstep2d1 - 1] = t + par->a[k + levelstep2d1 - 1];
			}
			par->f[k + levelstep2d - 1] = 0;
		}
		levelstep2d = levelstep2d / 2;
	}

#pragma omp parallel for shared(par,levelstep2d,levelstep2d1) \
		schedule(static)
	for (k = 0; k < par->nlines; k = k + 1) {
		par->a[k] = par->b[k] + par->a[k];
	}
	gk_stopwctimer(par->timer_1);
	par->c = par->a;

}

void OMP_Scan(params_t *par) {
	omp_set_num_threads(par->nthreads);
	int nlevels = (int) ceil(log(par->nlines) / M_LN2);
	int d, k, t;
	int levelstep2d = 1, levelstep2d1;

	/* ****************** UP SWEEP ******************************/
	gk_startwctimer(par->timer_2);
	for (d = 1; d < par->nlevels; ++d) {
		levelstep2d1 = levelstep2d * 2;
#pragma omp parallel for shared(par,levelstep2d,levelstep2d1) \
		schedule(static)
		for (k = 0; k < par->nalloc; k = k + levelstep2d1) {
			par->a[k + levelstep2d1 - 1] = par->a[k + levelstep2d - 1]
					+ par->a[k + levelstep2d1 - 1];
		}
		levelstep2d = levelstep2d * 2;
	}
	/* ****************** DOWN SWEEP ******************************/
	par->a[par->nalloc - 1] = 0;
	levelstep2d = par->nalloc / 2;
	for (d = par->nlevels - 1; d >= 0; --d) {
		levelstep2d1 = levelstep2d * 2;
#pragma omp parallel for shared(par,levelstep2d,levelstep2d1) \
		schedule(static)
		for (k = 0; k < par->nalloc; k = k + levelstep2d1) {
			t = par->a[k + levelstep2d - 1];
			par->a[k + levelstep2d - 1] = par->a[k + levelstep2d1 - 1];
			par->a[k + levelstep2d1 - 1] = t + par->a[k + levelstep2d1 - 1];
		}
		levelstep2d = levelstep2d / 2;
	}
	gk_stopwctimer(par->timer_2);
	par->a[par->nlines] = par->a[par->nlines - 1] + par->b[par->nlines - 1];
	par->c = &(par->a[1]);
}

void cmd_parse(int argc, char *argv[], params_t *par) {

	par->nthreads = strtol(argv[1], NULL, 10);
	par->infile = argv[2];
	size_t nlines, i;
	int x;
	FILE * fin;
	if (argc==3)
	printf("%d argc: nth=%s,  infile=%s,  \t", argc,
			argv[1], argv[2]);
	else if (argc==4)
		printf("%d argc: nth=%s,  infile=%s,  outfile=%d\t", argc,
					argv[1], argv[2],argv[3]);
	else if (argc<=2)
		fprintf(stderr,"Wrong number of arguments. argc=%d",argc);
	else
		printf("%d argc: nth=%s,  infile=%s,  outfile=%d, more arguments ignored\t", argc,
							argv[1], argv[2],argv[3]);

	if ((fin = fopen(par->infile, "r")) == NULL ) {
		fprintf(stderr, "ERROR: Failed to open '%s' for reading\n",
				par->infile);
		exit(1);
	}
	if (fscanf(fin, "%zu\n", &nlines) != 1) {
		fprintf(stderr, "ERROR: Failed to read first line of '%s'\n",
				par->infile);
		fclose(fin);
		exit(2);
	}
	par->nlines = nlines;
	printf("nlines=%d\n",(int)par->nlines);
	par->nlevels = (int) ceil(log(par->nlines) / M_LN2);
	par->nalloc = (int) (pow(2, par->nlevels));
	par->a = (int*) calloc((1 + par->nalloc), sizeof(int));
	par->b = (int*) calloc((1 + par->nalloc), sizeof(int));
	par->d = NULL;
	if (!par->a) {
		fprintf(stderr, "Fail to allocate memory, a is a null %p\n", par->a);
	}
	if (!par->b) {
		fprintf(stderr, "Fail to allocate memory, b is a null %p\n", par->a);
	}
	for (i = 0; i < nlines; ++i) {
		if (fscanf(fin, "%d\n", &x) != 1) {
			fprintf(stderr,
					"ERROR: Failed to read integer from line %zu/%zu in '%s'\n",
					i, nlines, par->infile);
			fclose(fin);
			exit(3);
		}
		par->b[i] = x;
	}
	fclose(fin);

	if (argc == 3) {
		par->outfile = (char*) malloc(sizeof(char) * 256);
		strcpy(par->outfile, "output.txt\0");
	} else {
		par->outfile = argv[4];
	}
	memcpy(par->a, par->b, (1 + par->nalloc) * (sizeof(int)));

	/* For Sscan Init */
	par->f = NULL;
	par->fb = NULL;
	OMP_Sscan_Init(par);
}

void Scan_Serial_Seq(params_t * par) {
	int i;
	gk_startwctimer(par->timer_3);
	par->d = (int*) malloc(sizeof(int) * par->nlines);
	par->d[0] = par->b[0];
	for (i = 1; i < par->nlines; ++i) {
		par->d[i] = par->d[i - 1] + par->b[i];
	}
	gk_stopwctimer(par->timer_3);
}

void WriteOut(params_t * par) {
	FILE * fout;
	printf("\nWriteout nlines=%d\n", (int)par->nlines);
	if ((fout = fopen(par->outfile, "w")) == NULL ) {
		fprintf(stderr, "ERROR: Failed to open '%s' for writing\n",
				par->outfile);
		exit(1);
	}
	int i;
	for (i = 0; i < par->nlines; ++i) {
//		fprintf(fout, "%d\n", par->c[i]);
	}
	fclose(fout);
}

void cleanup(params_t *par) {
	free((void*) par->a);
	free((void*) par->b);
	if (!par->f) {
		free((void*) par->f);
		free((void*) par->fb);
	}
}
