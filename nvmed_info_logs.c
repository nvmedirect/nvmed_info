#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "nvme_hdr.h"
#include "nvmed.h"
#include "lib_nvmed.h"
#include "nvmed_info.h"


struct nvmed_info_cmd logs_cmds[] = {
	{"get", 1, "GET LOG PAGES", nvmed_info_get_logs},
	{NULL, 0, NULL, NULL}
};

struct log_pages {
	int	logid;
	int cns;
	char *logname;
	int (*cmd_fn)(NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 result);
};

static struct log_pages logs[] = {
	{LOG_ERROR_INFO,					0, "Error Information",			nvmed_info_logs_error},				
	{LOG_SMART_INFO,	 				0, "SMART/Health Information",	nvmed_info_logs_smart},		
	{LOG_FIRMWARE_SLOT_INFO,			0, "Firmware Slot Information",	nvmed_info_logs_firmware},	
	/* optional
	{LOG_CHANGED_NAMESPACE_LIST,		0, "Changed Namespace List",	nvmed_info_logs_namespace},
	{LOG_COMMAND_EFFECTS,				0, "Command Effects Log",		nvmed_info_logs_command},	
	*/
	{0,									0, NULL,						NULL}
};

int nvmed_info_logs (NVMED *nvmed, char **cmd_args)
{
	struct nvmed_info_cmd *c;

	if (cmd_args[0] == NULL)
		return nvmed_info_get_logs(nvmed, NULL);

	c = cmd_lookup(logs_cmds, cmd_args[0]);
	if (c)
		return c->cmd_fn(nvmed, &cmd_args[1]);
	else {
		nvmed_info_logs_help(cmd_args[0]);
		return -1;
	}
}

int nvmed_info_logs_help (char *s)
{
	return cmd_help(s, "LOG PAGES subcommands", logs_cmds);
}

int nvmed_info_get_logs_issue (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 *result)
{
	struct nvme_admin_cmd cmd;
	int rc;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = nvme_admin_get_log_page;
	// TODO: Depending on ctrl->lpa, nsid may be set...
	cmd.nsid = 0xffffffff;
	if (p) {
		cmd.addr = (__u64) htole64(p);
		cmd.data_len = htole32(len);
	}
	// The spec says cdw10 should be ((len / 4) << 16 | logid)
	// However, for XS1715, NUMD should not be larger than 0x7f
	cmd.cdw10 = htole32(((0x7f) << 16) | logid);

	rc = nvmed_info_admin_command(nvmed, &cmd);
	*result = cmd.result;
	return rc;
}

int nvmed_info_get_logs (NVMED *nvmed, char **cmd_args)
{
	int rc;
	__u8 *p;
	__u32 res;
	int nsid = 1;
	struct log_pages *f;
	
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
	P ("GET LOG PAGES\n");
	P ("Log Pages  Values      Description\n");
	P ("---------  ----------  -----------\n");

	f = logs;
	while (f->logname) {
		rc = nvmed_info_get_logs_issue(nvmed, f->logid, f->cns? nsid : 0, p, PAGE_SIZE, &res);
		if (rc < 0) {
			P ("%02x-------  ----N/A---  %s\n", f->logid, f->logname);
			f++;
			continue;
		}
		else {
			P ("%02x-------  0x%08x  %s", f->logid, res, f->logname);
			if (f->cns)
				P (" (Namespace ID: %d)\n", nsid);
			else
				P ("\n");
		}

		f->cmd_fn (nvmed, f->logid, nsid, p, PAGE_SIZE, res);
		P ("\n");
		f++;
	}
	P ("\n\n");

	nvmed_put_buffer(p);
	return 0;
}

int nvmed_info_logs_error (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 res)
{
	__u32 _v;

	PV (0, 7, "Error Count", "");
	PH2 (8);	P ("Submission Queue ID: 0x%x\n", F(0,15));
	PH2 (10);	P ("Command ID: 0x%x\n", F(0,15));
	PH2 (12);	P ("Status Field: 0x%x\n", F(0,15));
	PH2 (14);	P ("Parameter Error Location:\n");
				P ("%26c  Byte in Command that contained the error: %d\n", SP, F(0,7));
				P ("%26c  Bit in Command that contained the error:  %d\n", SP, F(8,10));
	PV (16, 23, "LBA", "");
	PH4 (24);	P ("Namespace: %d\n", F(0,31));
	PH1 (28);	P ("Vendor Specific Information Available: 0x%02x\n", F(0,7));
	PV (32, 39, "Command Specific Information", "");
	return 0;
}
int nvmed_info_logs_smart (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 res)
{
	int i;
	__u32 _v;

	PH1 (0);	P ("Critical Warning: %s\n", F(0,7)? "" : "None");
	if (F(0,0))
		P ("%26c  The available spare space has fallen below the threshold\n", SP);
	if (F(1,1))
		P ("%26c  A temperature is above (or below) an over (or under) temperature threshold\n", SP);
	if (F(2,2))
		P ("%26c  The NVM subsystem reliability has been degraded due to errors\n", SP);
	if (F(3,3))
		P ("%26c  The media has been palce in read only mode\n", SP);
	if (F(4,4))
		P ("%26c  The volatile memory backup device has failed\n", SP);

	PH2 (1); 	P ("Composite Temperature: %d\n", F(0,15));
	PH1 (3);	P ("Available Space: %d%%\n", F(0,7));
	PH1 (4);	P ("Available Spare Threshold: %d%%\n", F(0,7));
	PH1 (5);	P ("Percent Used: %d%%\n", F(0,7));
	PV (32, 47, "Data Units Read", "(x1000 512 bytes)");
	PV (48, 63, "Data Units Written", "(x1000 512 bytes)");
	PV (64, 79, "Host Read Commands", "");
	PV (80, 95, "Host Write Commands", "");
	PV (96, 111, "Controller Busy Time", "(mins)");
	PV (112, 127, "Power Cycles", "");
	PV (128, 143, "Power On Hours", "(hours)");
	PV (144, 159, "Unsafe Shutdowns", "");
	PV (160, 175, "Media and Data Integrity Errors", "");
	PV (176, 191, "Number of Error Information Log Entries", "");

	PH4 (192);	P ("Warning Composite Temperature Time: %d (min)\n", F(0,31));
	PH4 (196); 	P ("Critical Composite Temperature Time: %d (min)\n", F(0,31));
	for (i = 0; i < 8; i++) {
		PH2 (200+i*2);	P ("Temperature Sensor %d: %d\n", i, F(0,15));
	}

	return 0;
}
int nvmed_info_logs_firmware (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 res)
{
	__u32 _v;

	PH1 (0);	P ("Active Firmware Info (AFI):\n");
				P ("%26c  Slot to be activated at the next controller reset: %d\n", SP, F(4,6));
				P ("%26c  Slot the actively running firmware revision was loaded: %d\n", SP, F(0,2));

	PS (8, 15, "Firmware Revision for Slot 1 (FRS1)");
	PS (16, 23, "Firmware Revision for Slot 2 (FRS2)");
	PS (24, 31, "Firmware Revision for Slot 3 (FRS3)");
	PS (32, 39, "Firmware Revision for Slot 4 (FRS4)");
	PS (40, 47, "Firmware Revision for Slot 5 (FRS5)");
	PS (48, 55, "Firmware Revision for Slot 6 (FRS6)");
	PS (56, 63, "Firmware Revision for Slot 7 (FRS7)");

	return 0;
}

int nvmed_info_logs_namespace (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 res)
{
	P("NAMESPACE\n");
	return 0;
}


int nvmed_info_logs_command (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 res)
{
	P("COMMAND\n");
	return 0;
}
