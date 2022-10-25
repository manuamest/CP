#ifndef __OPTIONS_H__
#define __OPTIONS_H__

struct options {
	int num_threads;
	int num_accounts;
	int iterations;
	int delay;
};

int read_options(int argc, char **argv, struct options *opt);


#endif
