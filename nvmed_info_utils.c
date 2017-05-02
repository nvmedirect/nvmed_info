#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "nvme_hdr.h"
#include "nvmed.h"
#include "lib_nvmed.h"
#include "nvmed_info.h"

#define BYTES_PER_LINE	(16)

char *nvme_sc[] = {
	/* 00h */	"Successful Completion",							
	/* 01h */	"Invalid Command Opcode",								
	/* 02h */	"Invalid Field in Command",
	/* 03h */	"Command ID Conflict",
	/* 04h */	"Data Transfer Error",
	/* 05h */	"Commands Aborted due to Power Loss Notification",
	/* 06h */	"Internal Error",
	/* 07h */	"Command Abort Requested",
	/* 08h */	"Command Aborted due to SQ Deletion",
	/* 09h */	"Command Aborted due to Failed Fused Command",
	/* 0Ah */	"Command Aborted due to Missing Fused Command",
	/* 0Bh */	"Invalid Namespace or Format",
	/* 0Ch */	"Command Sequence Error",
	/* 0Dh */	"Invalid SGL Segment Descriptor",
	/* 0Eh */	"Invalid Number of SQL Descriptors",
	/* 0Fh */	"Data SGL Length Invalid",
	/* 10h */	"Metadata SGL Lengh Invalid",
	/* 11h */	"SGL Descriptor Type Invalid",
	/* 12h */	"Invalid Use of Controller Memory Buffer",
	/* 13h */	"PRP Offset Invalid",
	/* 14h */	"Atomic Write Unit Exceeded",
};



int nvmed_info_admin_command (NVMED *nvmed, struct nvme_admin_cmd *cmd)
{
	int rc;

	rc = ioctl (dev_fd, NVME_IOCTL_ADMIN_CMD, cmd);
	if (rc < 0) {
		printf("ioctl() failed, rc = %d.\n", rc);
		return -1;
	}
	else if (rc > 0) {
		if (rc < (int) (sizeof(nvme_sc)/sizeof(char *)))
			printf ("NVMe Error %d (Opcode %02x): %s\n", rc, cmd->opcode, nvme_sc[rc]);
		else
			printf ("NVMe Error %d (Opcode %02x)\n", rc, cmd->opcode);
		return -1;
	}

	return 0;
}


struct nvmed_info_cmd *cmd_lookup (struct nvmed_info_cmd *list, char *str)
{
	struct nvmed_info_cmd *c = list;

	if (c == NULL || str == NULL)
		return NULL;

	while (c->cmd_name) {
		if (!strncmp(str, c->cmd_name, c->len))
			return c;
		c++;
	}
	return NULL;
}

int cmd_help (char *invalid_cmd, char *cmd_name, struct nvmed_info_cmd *c)
{
	if (invalid_cmd)
		printf("Invalid command \"%s\"\n", invalid_cmd);

	printf("%s\n", cmd_name);
	while (c->cmd_name) {
		printf("\t%-12s\t%s\n", c->cmd_name, c->cmd_help);
		c++;
	}
	return -1;
}

#if 0
void *alloc_buffer (int bytes)
{
	void *p;

	bytes = ((bytes - 1) & ~PAGESIZE) + PAGESIZE;

	p = mmap (0, bytes, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (p == NULL)
	{
		perror ("mmap");
		return p;
	}

	memset (p, 0, bytes);
	return p;
}

int dealloc_buffer (void *p, int bytes)
{
	return munmap (p, bytes);
}
#endif



void print_bytes (__u8 *p, int n)
{
	int i, j;
	int col;

	for (i = 0; i < n; i += BYTES_PER_LINE)
	{
		col = ((n - i) >= BYTES_PER_LINE)? BYTES_PER_LINE : (n - i);

		printf ("[%04x] %04d: ", i, i);
		for (j = 0; j < col; j++)
			printf ("%02x ", p[i+j]);
		for (j = col; j < BYTES_PER_LINE; j++)
			printf ("   ");

		printf ("   ");
		for (j = 0; j < col; j++)
			printf ("%1c", (isprint (p[i+j]))? p[i+j] : '.');
		for (j = col; j < BYTES_PER_LINE; j++)
			printf (" ");
		printf ("\n");
	}
}


void print_something (enum print_format format, __u8 *p, int offset, int end, char *title, char *unit)
{
	int i, j;
	int col;
	char s[80];
	__u64 value;

	for (i = offset; i < end; i += 4)
	{
		if (i == offset)
			printf ("%04d:%04d  ", offset, end);
		else
			printf ("           ");

		col = ((end + 1 - i) < 4)? (end + 1 - i) : 4;
		for (j = 0; j < col; j++)
			printf ("%02x ", p[i+j]);
		for (j = col; j < 4; j++)
			printf ("  ");
		printf (" ");
		switch (format)
		{
			case FORMAT_STRING:
				if (i == offset)
				{
					strncpy (s, (char *) &p[offset], end + 1 - offset);
					s[end + 1 - offset] = '\0';
					printf ("%s: %s", title, s);
				}
				break;

			case FORMAT_ID:
				if (i == offset)
					printf ("%s:", title);
				if (((i == offset) && (end + 1 - offset) <= 4) || (i == offset + 4))
				{
					for (j = end; j >= offset; j--)
					{
						printf ("%02x", p[j]);
						if (j != offset)
							printf ("-");
					}
				}
				break;

			case FORMAT_VALUE:
				if (i == offset)
				{
					value = 0;
					for (j = end; j >= offset; j--)
					value = (value << 8) + p[j];
					printf ("%s: %llu %s", title, value, unit);
				}
				break;

			default:
				printf ("UNKNOWN FORMAT");
		}
		printf ("\n");
	}
}



