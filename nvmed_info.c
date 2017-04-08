#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "nvme_hdr.h"
#include "nvmed.h"
#include "lib_nvmed.h"
#include "nvmed_info.h"


struct nvmed_info_cmd main_cmds[] = {
	{"identify", 1, "IDENTIFY Command", nvmed_info_identify},
	{"features", 1, "FEATURES Command", nvmed_info_features},
	{NULL, 0, NULL, NULL}
};


int main (int argc, char **argv)
{
	NVMED	*nvmed;
	char	*dev_path;
	struct nvmed_info_cmd *c;

	if (argc < 2)
	{
		return nvmed_info_usage(argv[0], NULL);
		return -1;
	}

	dev_path = argv[1];
	nvmed = nvmed_open(dev_path, 0);
	if (nvmed == NULL) {
		printf("%s: Cannot open the NVMe device \"%s\"\n", argv[0], dev_path);
		return -1;
	}

	if (argc == 2) 
		main_cmds[0].cmd_fn(nvmed, &argv[2]);
	else {
		c = cmd_lookup(main_cmds, argv[2]);
		if (c == NULL)
			return nvmed_info_usage(argv[1], argv[2]);
		c->cmd_fn(nvmed, &argv[3]);
	}
	
	nvmed_close(nvmed);
	return 0;

}


int nvmed_info_usage (char *arg0, char *invalid_cmd)
{
	struct nvmed_info_cmd *c = main_cmds;
	
	if (invalid_cmd)
		printf("%s: Invalid command \"%s\"\n", arg0, invalid_cmd);

	PRINT_NVMED_INFO;
	printf("Usage: %s <device_path> <command> <args> ...\n", arg0);
	while (c->cmd_name) {
		printf("\t%-12s\t%s\n", c->cmd_name, c->cmd_help);
		c++;
	}

	return -1;
}
