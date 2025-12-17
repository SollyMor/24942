#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>

#define PATH_MAX_SIZE 1024

extern char **environ;

//change ulimit

void print_help(const char *progname) {
    printf("Usage: %s [options]\n", progname);
    printf("Options:\n");
    printf("  -i           Print real/effective UID and GID\n");
    printf("  -s           Make process group leader\n");
    printf("  -p           Print PID, PPID, and process group ID\n");
    printf("  -u           Print ulimit value (RLIMIT_FSIZE)\n");
    printf("  -U <value>   Set new ulimit (RLIMIT_FSIZE)\n");
    printf("  -c           Print core file size limit\n");
    printf("  -C <value>   Set core file size limit\n");
    printf("  -d           Print current working directory\n");
    printf("  -v           Print all environment variables\n");
    printf("  -V name=val  Set or update environment variable\n");
}

// "-i-p-d" -> ["-i","-p","-d"]
int preprocess_args(int *argc, char ***argv) {
    int new_argc = 0;
    char **new_argv = malloc(sizeof(char*) * 256);
    if (!new_argv) return -1;

    for (int i = 0; i < *argc; i++) {
        if ((*argv)[i][0] == '-' && strchr((*argv)[i]+1, '-')) {

            char *tok = strtok((*argv)[i], "-");
            while (tok) {
                char *opt = malloc(strlen(tok) + 2);
                sprintf(opt, "-%s", tok);
                new_argv[new_argc++] = opt;
                tok = strtok(NULL, "-");
            }
        } else {
            new_argv[new_argc++] = (*argv)[i];
        }
    }

    new_argv[new_argc] = NULL;
    *argc = new_argc;
    *argv = new_argv;
    return 0;
}



int main(int argc, char *argv[]) {
	
	if (argc == 1) {
		print_help(argv[0]);
		return 0;
	}

	preprocess_args(&argc, &argv);

	char options[] = "ispuU:cC:dvV:";	
	int c;
	char *U_ptr, *C_ptr, *V_ptr, *env_name, *env_val;
	char cwd[PATH_MAX_SIZE];
	struct rlimit rlp;
	
	while ((c = getopt(argc, argv, options)) != EOF) {
		switch (c) {
		case 'i':
			printf("uid: %u\neuid: %u\ngid: %u\negid: %u\n",
				getuid(), geteuid(), getgid(), getegid());
			break;

		case 's':
			if (setpgid(0, 0) == 0)
				printf("Current process is set as a group leader.\n");
			break;

		case 'p':
			printf("pid: %d\nppid: %d\npgrp: %d\n",
				getpid(), getppid(), getpgrp());
			break;

		case 'u':
        	printf("Max child processes per user: %ld\n", sysconf(_SC_CHILD_MAX));
			break;

		case 'U': {
			U_ptr = optarg;
			char *endptr;
			long val = strtol(U_ptr, &endptr, 10);
			if (*endptr != '\0' || val < 0) {
				printf("Invalid value for -U: %s\n", U_ptr);
				break;
			}
			getrlimit(RLIMIT_FSIZE, &rlp);
			rlp.rlim_cur = (rlim_t)val;
			setrlimit(RLIMIT_FSIZE, &rlp);
			printf("Ulimit has changed\n");
			break;
		}

		case 'c':
			getrlimit(RLIMIT_CORE, &rlp);
			printf("Core file limit: %llu\n", (unsigned long long)rlp.rlim_cur);
			break;

		case 'C': {
			C_ptr = optarg;
			char *endptr;
			long val = strtol(C_ptr, &endptr, 10);
			if (*endptr != '\0' || val < 0) {
				printf("Invalid value for -C: %s\n", U_ptr);
				break;
			}
			getrlimit(RLIMIT_CORE, &rlp);
			rlp.rlim_cur = (rlim_t)val;
			setrlimit(RLIMIT_CORE, &rlp);
			printf("Core file limit has changed\n");
			break;
		}

		case 'd':
			getcwd(cwd, PATH_MAX_SIZE);
			printf("%s\n", cwd);
			break;

		case 'v':
			for (char **env_var = environ; *env_var != NULL; env_var++) {
				printf("%s\n", *env_var);
			}
			break;

		case 'V':
			V_ptr = optarg;
			env_name = strtok(V_ptr, "=");
			env_val = strtok(NULL, "=");
			if (!env_name || !env_val) {
				fprintf(stderr, "Invalid format for -V. Use name=value\n");
				break;
			}
			if (setenv(env_name, env_val, 1) == 0) {
				printf("Variable %s is set to %s\n", env_name, env_val);
			}
			break;

		case 'h':
			print_help(argv[0]);
			break;

		case '?':
			fprintf(stderr, "Unknown option: -%c\n", optopt);
			print_help(argv[0]);
			break;
		}
	}

	return 0;
}
