#include <sys/types.h>
#include <sys/sysctl.h>

#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int verbose;

void
usage(void)
{
	fprintf(stderr, "usage: %s [-h] [-v] [-p pid | -n process_name] -e string\n",
	        getprogname());
}

void
debug_print(const char *message)
{
	if (verbose)
		fprintf(stderr, "%s: %s\n", getprogname(), message);
}


int
pid_is_in(int pid, struct kinfo_proc **kinfo, int entries)
{
	int i;
	for (i = 0; i < entries; i++) {
		if (kinfo[i]->p_pid == pid)
			return 1;
	}
	return 0;
}

int
pname_is_in(char *pname, struct kinfo_proc **kinfo, int entries)
{
	int i;
	for (i = 0; i < entries; i++) {
		if (strcmp(kinfo[i]->p_comm, pname) == 0)
			return 1;
	}
	return 0;
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
	int found = 0;
	char *pname = NULL, *string = NULL;
	char errbuf[_POSIX2_LINE_MAX];

	kvm_t *kd;
	struct kinfo_proc **kinfo;

	verbose = 0;

	if (pledge("stdio ps", NULL) == -1)
		err(1, "pledge");

	// argument parsing...
	while((ch = getopt(argc, argv, "e:hn:p:v")) != -1)
		switch(ch) {
		case 'e':
			string = optarg;
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
			usage();
			return 0;
		default:
			usage();
			exit(1);
		}

	// argument checking...
	// show usage if neither a pid nor a pname are given
	if (!(pname != NULL || pid != 0)) {
		usage();
		exit(1);
	}
	// show usage if no output string is given
	if (string == NULL) {
		usage();
		exit(1);
	}

	// initialise virtual memory access
	kd = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, errbuf);
	if (kd == NULL)
		errx(1, "%s", errbuf);

	// get initial process list
	kinfo = get_proc_list(kd, &entries);

	// if a process name is given use that function, otherwise use pid function
	while (
		(pname != NULL) ?
		pname_is_in(pname, kinfo, entries) :
		pid_is_in(pid, kinfo, entries)
	) {
		found = 1;
		kinfo = get_proc_list(kd, &entries);
		debug_print("waiting...");
		sleep(1);
	}

	if (found) {
		debug_print("process died.");
		printf("%s\n", string);
	} else {
		errx(1, "process not in process list.");
	}

	return 0;
}
