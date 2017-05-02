#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "nvme_hdr.h"
#include "nvmed.h"
#include "lib_nvmed.h"
#include "nvmed_info.h"


struct nvmed_info_cmd features_cmds[] = {
	{"get", 1, "GET FEATURES", nvmed_info_get_features},
	{NULL, 0, NULL, NULL}
};

struct feature_set {
	int	fid;
	int cns;
	int datalen;		// whether data page is need to be attached
	char *fname;
};

static struct feature_set features[] = {
	{FEATURE_ARBITRATION,					0, 	0,		"Arbitration"},				
	{FEATURE_POWER_MANAGEMENT, 				0,  0,		"Power Management"},		
	{FEATURE_LBA_RANGE_TYPE,				1, 	4096,	"LBA Range Type"},	
	{FEATURE_TEMPERATURE_THRESHOLD,			0, 	0,		"Temperature Threshold"},
	{FEATURE_ERROR_RECOVERY,				0, 	0,		"Error Recovery"},	
	{FEATURE_VOLATILE_WRITE_CACHE,			0, 	0,		"Volatile Write Cache"},
	{FEATURE_NUMBER_OF_QUEUES,				0, 	0,		"Number of Queues"},
	{FEATURE_INTERRUPT_COALESCING,			0, 	0,		"Interrupt Coalescing"},
	{FEATURE_INTERRUPT_VECTOR_CONFIG,		0, 	0,		"Interrupt Vector Configuration"},
	{FEATURE_WRITE_ATOMICITY_NORMAL,		0, 	0,		"Write Atomicity Normal"},
	{FEATURE_ASYNC_EVENT_CONFIG,			0, 	0,		"Asynchronous Event Configuration"},
	{FEATURE_AUTO_POWER_STATE_TRANSITION,	0, 	256,	"Autonomous Power State Transition"},
	{FEATURE_HOST_MEMORY_BUFFER,			0, 	4096,	"Host Memory Buffer"},
	{FEATURE_KEEP_ALIVE_TIMER,				0,	0,		"Keep Alive Timer"},
	{FEATURE_SW_PROGRESS_MARKER,			0, 	0,		"Software Progress Marker"},
	{FEATURE_HOST_IDENTIFIER,				0, 	4096,	"Host Identifier"},
	{FEATURE_RESERVATION_NOTI_MASK,			0, 	0,		"Reservation Notification Mask"},
	{FEATURE_RESERVATION_PERSISTENCE,		0, 	0,		"Reservation Persistence"},
	{0,										0, 	0,		NULL}
};



int nvmed_info_features (NVMED *nvmed, char **cmd_args)
{
	struct nvmed_info_cmd *c;

	if (cmd_args[0] == NULL)
		return nvmed_info_get_features(nvmed, NULL);

	c = cmd_lookup(features_cmds, cmd_args[0]);
	if (c)
		return c->cmd_fn(nvmed, &cmd_args[1]);
	else {
		nvmed_info_features_help(cmd_args[0]);
		return -1;
	}
}

int nvmed_info_features_help (char *s)
{
	return cmd_help(s, "FEATURES subcommands", features_cmds);
}

int nvmed_info_get_features_issue (NVMED *nvmed, int fid, int nsid, __u8 *p, int len, __u32 *result)
{
	struct nvme_admin_cmd cmd;
	int rc;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = nvme_admin_get_features;
	if (nsid)
		cmd.nsid = htole32(nsid);
	if (p) {
		cmd.addr = (__u64) htole64(p);
		cmd.data_len = htole32(len);
	}
	cmd.cdw10 = htole32(fid);

	rc = nvmed_info_admin_command(nvmed, &cmd);
	*result = cmd.result;
	return rc;
}

int nvmed_info_get_features (NVMED *nvmed, char **cmd_args)
{
	int rc;
	__u32 res;
	int nsid = 1;
	struct feature_set *f;
	__u8 *p;
	__u32 _v;

	p = (__u8 *) nvmed_get_buffer(nvmed, 1);
	if (p == NULL) {
		printf("Memory allocation failed.\n");
		return -1;
	}

	if (cmd_args && cmd_args[0]) {
		nsid = atoi(cmd_args[0]);
		if (nsid <= 0) {
			printf("Invalid namespace ID %d\n", nsid);
			return -1;
		}
	}

	PRINT_NVMED_INFO;
	P ("GET FEATURES\n");
	P ("Feature    Values      Description\n");
	P ("---------  ----------  -----------\n");

	f = features;
	while (f->fname) {
		rc = nvmed_info_get_features_issue(nvmed, f->fid, f->cns? nsid : 0, 
				f->datalen? p : NULL, f->datalen, &res);
		if (rc < 0) {
			P ("    %02x     ----N/A---  %s\n", f->fid, f->fname);
			f++;
			continue;
		}
		else {
			printf ("\n    %02x     0x%08x  %s", f->fid, res, f->fname);
			if (f->cns)
				P (" (Namespace ID: %d)\n", nsid);
			else
				P ("\n");
		}

		_v = res;
		switch (f->fid) {
			case FEATURE_ARBITRATION:	/* Arbitration */
				P ("%24c  High Priority Weight (HPW): %u\n", SP, F(24,31));
				P ("%24c  Medium Priority Weight (MPW): %u\n", SP, F(16,23));
				P ("%24c  Low Priority Weight (MPW): %u\n", SP, F(8,15));
				P ("%24c  Arbitration Burst (AB): ", SP);
				if (F(0,2) == 7)
					P ("No limit\n");
				else
					P ("%u\n", 1 << F(0,2));
				break;

			case FEATURE_POWER_MANAGEMENT:	/* Power Management */
				P ("%24c  Workload Hint (WH): %u\n", SP, F(5,7));
				P ("%24c  Power State (PS): %u\n", SP, F(0,4));
				break;

			case FEATURE_LBA_RANGE_TYPE: {	/* LBA Range Type */
				int i, entries;
				static char *lbatype[] = {"Reserved", "Filesystem", "RAID", "Cache", "Page/swap file"};
				entries = F(0,5);
				P ("%24c  Number of LBA Ranges (NUM): %u\n", SP, entries);
				for (i = 0; i < entries; i++) {
					P ("LBA Range Type %d\n", i);
						PH1 (i*64+0);	P ("Type (Type): %s\n", 
											(F(0,7) <= 0x04)? lbatype[F(0,7)] :
											(F(0,7) <= 0x7f)? lbatype[0] : "Vendor Specific");
						PH1 (i*64+1);	P ("Attributes:\n");
											P ("%24c  The LBA range may %s overwritten\n", 
												SP, F(0,0)? "be" : "not be");
											P ("%24c  The LBA range should be %s the OS\n", 
												SP, F(1,1)? "hidden from" : "visible to");
						PV (i*64+16, i*64+23, "Starting LBA (SLBA)", "block");
						PV (i*64+24, i*64+31, "Number of Logical Blocks (NLB)", "blocks");
						PI (i*64+32, i*64+47, "Unique Identifier (GUID)");
				}
				}
				break;

			case FEATURE_TEMPERATURE_THRESHOLD:	/* Temperature Threshold */
				P ("%24c  Threshold Type Select (THSEL): %s\n", SP,
						(F(20,21) == 0)? "Over temperature threshold" :
						(F(20,21) == 1)? "Under temperature threshold" : "Reserved");
				P ("%24c  Threshold Temperature Select (TMPSEL): ", SP);
				switch (F(16,19))
				{
					case 0:
						P ("Composite temperature\n");
						break;
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8:
						P ("Temperature sensor %d\n", F(16,19));
						break;
					default:
						printf ("Reserved\n");
				}
				P ("%24c  Temperature Threshold (TMPTH): %u\n", SP, F(0,15));
				break;

			case FEATURE_ERROR_RECOVERY:	/* Error Recovery */
				P ("%24c  Deallocated or Unwritten Logical Block Error Enable (DULBE): %s\n", SP, YN(16));
				P ("%24c  Time Limited Error Recovery (TLER): %u (100msec)\n", SP, F(0,15));
				break;

			case FEATURE_VOLATILE_WRITE_CACHE:	/* Volatile Write Cache */
				P ("%24c  Volatile Write Cache Enable (WCE): %s\n", SP, YN(0));
				break;

			case FEATURE_NUMBER_OF_QUEUES:	/* Number of Queues */
				P ("%24c  Number of I/O Completion Queues Allocated (NCQA): %u\n", SP, F(16,31)+1);
				P ("%24c  Number of I/O Submission Queues Allocated (NSQA): %u\n", SP, F(0,15)+1);
				break;

			case FEATURE_INTERRUPT_COALESCING:	/* Interrupt Coalescing */
				P ("%24c  Aggregation Time: ", SP);
				if (F(8,15))
					printf ("%u (100 msec)\n", F(8,15));
				else 
					printf ("No delay\n");
				P ("%24c  Aggreation Threshold (THR): %u (entries)\n", SP, F(0,7));
				break;

			case FEATURE_INTERRUPT_VECTOR_CONFIG:	/* Interrupt Vector Configuration */
				P ("%24c  Coalescing Disable (CD): %s\n", SP, YN(16));
				P ("%24c  Interrupt Vector (IV): 0x%02x\n", SP, F(0,15));
				break;

			case FEATURE_WRITE_ATOMICITY_NORMAL:	/* Write Atomicity Normal */
				P ("%24c  Disable Normal (DN): %s\n", SP, YN(0));
				break;

			case FEATURE_ASYNC_EVENT_CONFIG:	/* Asynchronous Event Configuration */
				P ("%24c  Firmware Activation Notices: %s\n", SP, YN(9));
				P ("%24c  Namespace Attribute Notices: %s\n", SP, YN(8));
				P ("%24c  SMART / Health Critical Warnings: 0x%02x\n", SP, F(0,7));
				break;

			case FEATURE_AUTO_POWER_STATE_TRANSITION: {	/* Autonomous Power State Transition */
				int i;

				P ("%24c  Autonomous Power State Transition Enable (APSTE): %s\n", SP, YN(0));

				for (i = 0; i < 256; i += 8) {
					if (U64(p[i]) == 0) 
						break;

					P ("Power State %d\n", i/8);
						PH1 (i*8+0);	P ("Idle Transition Power State (ITPS): %u\n", F(3,7));
						PV (i*8+1, i*8+3, "Idle Time Prior to Transition (ITPT):", "(msec)");
				}
			  	} 
				break;

			case FEATURE_HOST_MEMORY_BUFFER: {	/* Host Memory Buffer */
				P ("%24c  Memory Return (MR): %s\n", SP, YN(1));
				P ("%24c  Enable Host Memory (EHM): %s\n", SP, YN(0));
				P ("Host Memory Buffer Attributes\n");
					PH4 (0);	P ("Host Memory Buffer Size (HSIZE): %u (pages)\n", F(0,31));
					PH4 (4);	P ("Host Memory Descriptor List Address Lower (HMDLAL): 0x%08x\n", F(0,31));
					PH4 (8);	P ("Host Memory Descriptor List Address Upper (HMDLAU): 0x%08x\n", F(0,31));
					PH4 (12);	P ("Host Memory Descriptor List Entry Count (HMDLEC): 0x%d\n", F(0,31));
				}
				break;

			case FEATURE_KEEP_ALIVE_TIMER: 		/* Keep Alive Timer */
				P ("%24c  Keep Alive Timeout (KATO): %u (msec)\n", SP, F(0,31));
				break;

			case FEATURE_SW_PROGRESS_MARKER:	/* Software Progress Marker */
				P ("%24c  Pre-boot Software Load Count (PBSLC): %u\n", SP, F(0,7));
				break;

			case FEATURE_HOST_IDENTIFIER:	/* Host Identifier */
				PI (0, 16, "Host Identifier (HOSTID)");
				break;

			case FEATURE_RESERVATION_NOTI_MASK:	/* Reservation Notification Mask */
				P ("%24c  Mask Reservation Preempted Notification (RESPRE): %s\n", SP, YN(3));
				P ("%24c  Mask Reservation Released Notification (RESREL): %s\n", SP, YN(2));
				P ("%24c  Mask Registration Preempted Notification (REGPRE): %s\n", SP, YN(1));
				break;

			case FEATURE_RESERVATION_PERSISTENCE:	/* Reservation Persistence */
				P ("%24c  Persist Through Power Loss (PTPL): %s\n", SP, YN(0));
				break;

			default:
				P ("%24c  Unknown Feature ID\n", SP);
		}
		f++;
	}
	P ("\n\n");

	nvmed_put_buffer(p);
	return 0;
}

