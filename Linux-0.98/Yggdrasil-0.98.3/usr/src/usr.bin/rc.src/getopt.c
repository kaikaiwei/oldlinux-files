/* getopt routine courtesy of David Sanderson */
#include "rc.h"
#include "utils.h"
#include "getopt.h"

int     opterr = 1;
int     optind = 1;
int     optopt;
char    *optarg;
 
int getopt(int argc, char **argv, char *opts) {
        static int sp = 1;
        int c;
        char *cp;
 
	if (optind == 0) /* reset getopt() */
		optind = sp = 1;

        if (sp == 1)
                if(optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0') {
                        return -1;
                } else if (strcmp(argv[optind], "--") == 0) {
                        optind++;
                        return -1;
                }
        optopt = c = argv[optind][sp];
        if (c == ':' || (cp=strchr(opts, c)) == 0) {
                fprint(2, "%s: illegal option -- %c\n", argv[0], c);
                if (argv[optind][++sp] == '\0') {
                        optind++;
                        sp = 1;
                }
                return '?';
        }
        if (*++cp == ':') {
                if(argv[optind][sp+1] != '\0') {
                        optarg = &argv[optind++][sp+1];
                } else if(++optind >= argc) {
                        fprint(2, "%s: option requires an argument -- %c\n", argv[0], c);
                        sp = 1;
                        return '?';
                } else
                        optarg = argv[optind++];
                sp = 1;
        } else {
                if (argv[optind][++sp] == '\0') {
                        sp = 1;
                        optind++;
                }
                optarg = 0;
        }
        return c;
}
