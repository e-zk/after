#include <sys/types.h>
#include <sys/sysctl.h>

#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

char *progname;
int verbose;

void
usage(char *progname)
{
	printf("usage: %s [-h] [-v] [-p pid | -n process_name] -e string\n",
                progname);
}

void
debug_print(const char *message)
{
	if (verbose == 1)
		fprintf(stderr, "%s: %s\n", progname, message);
}


int
pid_is_in(int pid, struct kinfo_proc **kinfo, int entries)
{
	int i;
	for (i = 0; i < entries; i++) {
		if (kinfo[i]->p_pid == pid)
			return 0;
	}
	return -1;
}

int
pname_is_in(char *pname, struct kinfo_proc **kinfo, int entries)
{
	int i;
	for (i = 0; i < entries; i++) {
		if (strcmp(kinfo[i]->p_comm, pname) == 0)
			return 0;
	}
	return -1;
}

struct kinfo_proc **
get_proc_list(kvm_t *kd, int *entries)
{
	int i;
	struct kinfo_proc *kp, **kinfo;

	// get proc list
	kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, sizeof(*kp), entries);
	if (kp == NULL)
		errx(1, "%s", kvm_geterr(kd));

	// this may not be necessary since we are not sorting our array at any point
	if ((kinfo = reallocarray(NULL, *entries, sizeof(*kinfo))) == NULL)
		err(1, "failed to allocate memory for proc pointers");
	for (i = 0; i < *entries; i++)
		kinfo[i] = &kp[i];

	return kinfo;
}

int
main(int argc, char *argv[])
{
	int i, ch, entries;
	int pid = 0;
	char *pname = NULL, *cmd = NULL;
	char errbuf[_POSIX2_LINE_MAX];

	kvm_t *kd;
	struct kinfo_proc **kinfo;

	verbose = 0;
	progname = argv[0];

	// argument parsing...
	while((ch = getopt(argc, argv, "e:hn:p:v")) != -1)
		switch(ch) {
		case 'e':
			cmd = optarg;
			break;
		case 'n':
			pname = optarg;
			break;
		case 'p':
			sscanf(optarg, "%d", &pid);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			usage(progname);
			return 0;
		default:
			usage(progname);
			exit(1);
		}

	// argument checking...
	// show usage if neither a pid nor a pname are given
	if (!(pname != NULL || pid != 0)) {
		usage(progname);
		exit(1);
	}
	// show usage if no output string is given
	if (cmd == NULL) {
		usage(progname);
		exit(1);
	}


	// initialise virtual memory access
	kd = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, errbuf);
	if (kd == NULL)
		errx(1, "%s", errbuf);

	// get initial process list
	kinfo = get_proc_list(kd, &entries);

	// if a process name is given use that, otherwise use pid
	if (pname != NULL) {
		while(pname_is_in(pname, kinfo, entries) == 0) {
			kinfo = get_proc_list(kd, &entries);
			debug_print("waiting...");
			sleep(1);
		}
	}
	else {
		while(pid_is_in(pid, kinfo, entries) == 0) {
			kinfo = get_proc_list(kd, &entries);
			debug_print("waiting...");
			sleep(1);
		}
	}


	debug_print("process not in process list (died?).");
	printf("%s\n", cmd);

	return 0;
}
