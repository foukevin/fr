#ifndef ERROR_H
#define ERROR_H

void error(const char *err, ...);
void warning(const char *err, ...);
void die(const char *err, ...);

void setprogname(const char *progname);
#define ERROR_INIT \
	do { \
		if (argv[0] != NULL) \
			setprogname(argv[0]); \
	} while(0)


#endif /* ERROR_H */
