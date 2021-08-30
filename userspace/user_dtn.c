static const char *__doc__ = "Tuning Module Userspace program\n"
        " - Finding xdp_stats_map via --dev name info\n"
	" - Has a Tuning Module counterpart in the kernel\n";

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <bits/stdint-uintn.h>
#include <bits/types.h>
#include <ctype.h>
#include <getopt.h>

int fTalkToKernel(int fd);

/* start of bpf stuff  ****/
#ifndef PATH_MAX
#define PATH_MAX    4096
#endif

const char *pin_basedir =  "/sys/fs/bpf";

#include <locale.h>
#include <time.h>

#include <bpf/bpf.h>
#include "../../libbpf/src/libbpf.h"
/* Lesson#1: this prog does not need to #include <bpf/libbpf.h> as it only uses
 * the simple bpf-syscall wrappers, defined in libbpf #include<bpf/bpf.h>
 */

#include <net/if.h>
#include <linux/if_link.h> /* depend on kernel-headers installed */

#include "../common/common_params.h"
#include "../common/common_user_bpf_xdp.h"
#include "common_kern_user.h"
#include "bpf_util.h" /* bpf_num_possible_cpus */

static int read_buffer_sample(void *ctx, void *data, size_t len) {
  struct event *evt = (struct event *)data;
  printf("%lld ::: %s\n", evt->numb, evt->filename);
  return 0;
}

static const struct option_wrapper long_options[] = {
    {{"help",        no_argument,       NULL, 'h' },
     "Show help", false},

    {{"dev",         required_argument, NULL, 'd' },
     "Operate on device <ifname>", "<ifname>", true},

    {{"quiet",       no_argument,       NULL, 'q' },
     "Quiet mode (no output)"},

    {{0, 0, NULL,  0 }}
};

int fDoRunBpfCollection(int argc, char **argv, int kernel_fd) 
{

    struct bpf_map_info map_expect = { 0 };
    struct bpf_map_info info = { 0 };
    char pin_dir[PATH_MAX];
    int buffer_map_fd;
    int interval = 2;
    struct ring_buffer *rb;
    int len, err;

    struct config cfg = {
        .ifindex   = -1,
        .do_unload = false,
    };

    /* Cmdline options can change progsec */
    parse_cmdline_args(argc, argv, long_options, &cfg, __doc__);

	/* Required option */
    if (cfg.ifindex == -1) {
        fprintf(stderr, "ERR: required option --dev missing\n\n");
        usage(argv[0], __doc__, long_options, (argc == 1));
        return EXIT_FAIL_OPTION;
    }

	/* Use the --dev name as subdir for finding pinned maps */
    len = snprintf(pin_dir, PATH_MAX, "%s/%s", pin_basedir, cfg.ifname);
    if (len < 0) {
        fprintf(stderr, "ERR: creating pin dirname\n");
        return EXIT_FAIL_OPTION;
    }

	buffer_map_fd = open_bpf_map_file(pin_dir, "int_ring_buffer_me", &info);
    if (buffer_map_fd < 0) {
        return EXIT_FAIL_BPF;
    }

    /* check map info, e.g. datarec is expected size */
    map_expect.max_entries = 1 << 14;
    err = check_map_fd_info(&info, &map_expect);
    if (err) {
        fprintf(stderr, "ERR: map via FD not compatible\n");
        return err;
    }

    if (verbose) {
        printf("\nCollecting stats from BPF map\n");
        printf(" - BPF map (bpf_map_type:%d) id:%d name:%s"
               " max_entries:%d\n",
               info.type, info.id, info.name, info.max_entries
               );
    }

    rb = ring_buffer__new(buffer_map_fd, read_buffer_sample, NULL, NULL);

    if (!rb)
    {
	    printf ("can't create ring buffer struct****\n");
		    return -1;
    }
	while (1) {
			ring_buffer__consume(rb);
			fTalkToKernel(kernel_fd);
			sleep(interval);
	}
    	
}


/* End of bpf stuff ****/

/* This works with tuning Module */

FILE * tunDefSysCfgPtr = 0;
FILE * tunLogPtr = 0;
void fDoSystemtuning(void);

#define htcp 		0
#define fq_codel 	1
char *aStringval[] ={"htcp", "fq_codel"};

typedef struct {
    char * setting;
    uint32_t  minimum;
    uint32_t xDefault; //if default is 0, then default and max are nops
    uint32_t maximum;
}host_tuning_vals_t;

/* 
 * Suggestion for net.ipv4.tcp_mem...
 *
 * for tcp_mem, set it to twice the maximum value for tcp_[rw]mem multiplied by  * the maximum number of running network applications divided by 4096 bytes per  * page.
 * Increase rmem_max and wmem_max so they are at least as large as the third 
 * values of tcp_rmem and tcp_wmem.
 */
#define TUNING_NUMS	7
/* Must change TUNING_NUMS if adding more to the array below */

host_tuning_vals_t aTuningNumsToUse[TUNING_NUMS] = {
    {"net.core.rmem_max",   			67108864,       	0,      	0},
    {"net.core.wmem_max",   			67108864,       	0,      	0},
    {"net.ipv4.tcp_rmem",       			4096,       87380,   33554432},
    {"net.ipv4.tcp_wmem",       			4096,       65536,   33554432},
    {"net.ipv4.tcp_mtu_probing",			   1,       	0,      	0},
    {"net.ipv4.tcp_congestion_control",	    htcp, 			0, 			0}, //uses #defines to help
    {"net.core.default_qdisc",		    fq_codel, 			0, 			0}, //uses #defines
};

void fDoSystemTuning(void)
{

	char *line = NULL;
    size_t len = 0;
    ssize_t nread;
	char *q, *r, *p = 0;
	char setting[256];
	char value[256];
	int count, intvalue, found = 0;
		
	fprintf(tunLogPtr,"\n\t\t\t***Start of Default System Tuning***\n");
	fprintf(tunLogPtr,"\t\t\t***------------------------------***\n");

	tunDefSysCfgPtr = fopen("/tmp/default_sysctl_config","r");
	if (!tunDefSysCfgPtr)
	{
		fprintf(tunLogPtr,"Could not open Tuning Module default system config file, exiting...\n");
		fclose(tunLogPtr);
		exit(-2);
	}

	fprintf(tunLogPtr, "Tuning Module default system configuration file opened***\n");
	fflush(tunLogPtr);

    while ((nread = getline(&line, &len, tunDefSysCfgPtr)) != -1) {
    	//fprintf(tunLogPtr,"Retrieved line of length %zu:\n", nread);
		//fprintf(tunLogPtr,"&%s&",line);
		p = line;
		q = strchr(line,' '); //search for space	
		len = (q-p) + 1;
		strncpy(setting,p,len);
		if (setting[len-1] == ' ')
			setting[--len] = 0;
		else
			setting[len] = 0;

		fprintf(tunLogPtr,"\nsetting is ***%s***\n",setting);
		/* compare with known list now */
		for (count = 0; count < TUNING_NUMS; count++)
		{
			if (strcmp(aTuningNumsToUse[count].setting, setting) == 0) //found
			{
				q++;// move it up past the space
				q = strchr(q,' '); //search for next space	
				q++; // move to the beginning of 1st (maybe only) number
				r = strchr(q,'\n'); //search for newline
				len = (r-q) + 1;
				strncpy(value,q,len);
				value[--len] = 0;
	
				if(isdigit(value[0]))
				{
					intvalue = atoi(value);
					if(intvalue <= aTuningNumsToUse[count].minimum)
					{
						if (aTuningNumsToUse[count].xDefault == 0) //only one value
						{
							fprintf(tunLogPtr,"Current config value for *%s* is *%s* which is less than the minimum recommendation...\n",setting, value);	
							fprintf(tunLogPtr,"You should change to the recommended setting of *%d* for *%s*.\n",aTuningNumsToUse[count].minimum, setting);
						}
						else
							{//has min, default and max values
								//more work needed			
								fprintf(tunLogPtr,"Current config value for *XXXX***%s* is *%s*...\n",setting, value);	
								fprintf(tunLogPtr,"You should change to the recommended setting of *%s* to *%d\t%d\t%d*.\n",setting, aTuningNumsToUse[count].minimum, aTuningNumsToUse[count].xDefault, aTuningNumsToUse[count].maximum);
							}
					}
				}	
				else
					{ //must be a string
						if (strcmp(value, aStringval[aTuningNumsToUse[count].minimum]) != 0)
						{
							fprintf(tunLogPtr,"Current config value for *%s* is *%s* which is not the same as the recommendation...\n",setting, value);	
							fprintf(tunLogPtr,"You should change to the recommended setting of *%s* for *%s*.\n",aStringval[aTuningNumsToUse[count].minimum], setting);
						}
						else
							{
								fprintf(tunLogPtr,"Current config value for *%s* is *%s* is the same as the recommendation...\n",setting, value);	
							}
							
					}

				found = 1;
				break;
			}
		}

		if (!found)
			fprintf(tunLogPtr,"ERR*** Could not find the following setting **%s**\n", setting);
		
	}
	
	fprintf(tunLogPtr, "\n***Closing Tuning Module default system configuration file***\n");
	fclose(tunDefSysCfgPtr);

	fprintf(tunLogPtr,"\n\t\t\t***End of Default System Tuning***\n");
	fprintf(tunLogPtr,"\t\t\t***----------------------------***\n\n");

	if(line)
		free(line);

	return;
}

int fTalkToKernel(int fd)
{
	int result = 0;
	char aMessage[512];
	strcpy(aMessage,"This is a message...");
	result = write(fd,aMessage,strlen(aMessage));
	if (result < 0)
		fprintf(tunLogPtr,"There was an error writing***\n");
	else
		fprintf(tunLogPtr,"***GoodW**, message written to kernel module = ***%s***\n", aMessage);

	memset(aMessage,0,512);
	result = read(fd,aMessage,512);
	if (result < 0)
		fprintf(tunLogPtr,"There was an error readin***\n");
	else
		fprintf(tunLogPtr,"***GoodR**, message read = ***%s***\n", aMessage);

	fflush(tunLogPtr);
	return result;
}

int main(int argc, char **argv) 
{

	char *pDevName = "/dev/tuningMod";
	int fd, err;

	tunLogPtr = fopen("/tmp/tuningLog","w");
	if (!tunLogPtr)
	{
		printf("Could not open tuning Logfile, exiting...\n");
		exit(-1);
	}
	fprintf(tunLogPtr, "tuning Log opened***\n");

	fprintf(tunLogPtr, "Running gdv.s - Shell script to Get default system valuesh***\n");

	system("sh ./gdv.sh");

	fDoSystemTuning();
	
	fd = open(pDevName, O_RDWR,0);

	if (fd > 0)
		err = fTalkToKernel(fd);
	else
	{
		fprintf(tunLogPtr,"***Error opening kernel device, errno = %dn***\n",errno);
		fprintf(tunLogPtr, "Closing tuning Log and exiting***\n");
		fclose(tunLogPtr);
		exit(-8);
	}
			
	fflush(tunLogPtr);

	err = fDoRunBpfCollection(argc, argv, fd);
	if (err)
	{
		fprintf(tunLogPtr, "***Err %d while starting up bpf Collector***\n",err);
	}


	if (fd > 0)
		close(fd);

	fprintf(tunLogPtr, "Closing tuning Log***\n");
	fclose(tunLogPtr);

return 0;
}

