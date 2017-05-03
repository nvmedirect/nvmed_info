#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "nvme_hdr.h"
#include "nvmed.h"
#include "lib_nvmed.h"
#include "nvmed_info.h"


struct nvmed_info_cmd identify_cmds[] = {
	{"controller", 1, "IDENTIFY Controller", nvmed_info_identify_controller},
	{"namespace", 1, "IDENTIFY Namespace", nvmed_info_identify_namespace},
	{NULL, 0, NULL, NULL}
};

int nvmed_info_identify (NVMED *nvmed, char **cmd_args)
{
	struct nvmed_info_cmd *c;

	if (cmd_args[0] == NULL)
		return nvmed_info_identify_controller(nvmed, NULL);

	c = cmd_lookup(identify_cmds, cmd_args[0]);
	if (c)
		return c->cmd_fn (nvmed, &cmd_args[1]);
	else {
		nvmed_info_identify_help(cmd_args[0]);
		return -1;
	}
}

int nvmed_info_identify_help (char *s)
{
	return cmd_help(s, "IDENTIFY subcommands", identify_cmds);
}


int nvmed_info_identify_controller (NVMED *nvmed, char **cmd_args)
{
	int rc;
	__u8 *p;
	
	p = (__u8 *) nvmed_get_buffer(nvmed, 1);
	if (p == NULL) {
		printf("Memory allocation failed.\n");
		return -1;
	}

	rc = nvmed_info_identify_issue(nvmed, CNS_CONTROLLER, 0, p);
	if (rc < 0)
		return rc;

	nvmed_info_identify_parse_controller(p);
	nvmed_put_buffer(p);
	return 0;
}

int nvmed_info_identify_namespace (NVMED *nvmed, char **cmd_args)
{
	int rc;
	int nsid = 1;
	__u8 *p;

	p = (__u8 *) nvmed_get_buffer(nvmed, 1);
	if (p == NULL) {
		printf("Memory allocation failed.\n");
		return -1;
	}

	if (cmd_args && cmd_args[0]) {
		nsid = atoi(cmd_args[0]);
		if (nsid <= 0)
		{
			printf("Invalid namespace ID %d\n", nsid);
			return -1;
		}
	}

	rc = nvmed_info_identify_issue(nvmed, CNS_NAMESPACE, nsid, p);
	if (rc < 0)
		return rc;

	nvmed_info_identify_parse_namespace(p, nsid);
	nvmed_put_buffer(p);
	return 0;
}

int nvmed_info_identify_issue (NVMED *nvmed, int cns, int nsid, __u8 *p)
{
	struct nvme_admin_cmd cmd;
	int rc;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = nvme_admin_identify;
	cmd.nsid = htole32(nsid);
	cmd.addr = (__u64) htole64(p);
	cmd.data_len = htole32(PAGE_SIZE);
	cmd.cdw10 = htole32(cns);

	rc = nvmed_info_admin_command(nvmed, &cmd);
	return rc;
}


void nvmed_info_identify_parse_controller (__u8 *p)
{
	__u32 _v;
	static char *ps[] = {"Not reported", "0.0001 W", "0.01 W", "Reserved"};

	PRINT_NVMED_INFO;
	P ("IDENTIFY Controller\n");
	P ("Bytes      Values       Description\n");
	P ("---------  -----------  -----------\n");
	P ("\n[Controller Capabilities and Features]\n");
	PH2 (0);	P ("PCI Vendor ID (VID): 0x%x\n", F(0,15));
	PH2 (2);	P ("PCI Subsystem Vendor ID (SSVID): 0x%x\n", F(0,15));
	PS (04, 23, "Serial Number (SN)");
	PS (24, 63, "Model Number (MN)");
	PS (64, 71, "Firmware Revision (FR)");
	PH1 (72);	P ("Recommended Arbitration Burst (RAB): %d\n", F(0,7));
	PH3 (73);	P ("IEEE OUI Identifier (IEEE): 0x%06x\n", F(0,23)); 
	PH1 (76);	P ("Controller Multi-Path I/O and Namespace Sharing Capabilities (CMIC):\n");
				P ("%26c  %s, %s, %s\n", SP,
					F(2,2)? "SR-IOV Virtual Function" : "PCI Function",
					F(1,1)? "2+ controllers" : "Single controller",
					F(0,0)? "2+ ports" : "Single port");
	PH1 (77);	P ("Maximum Data Transfer Size (MDTS): %d %s\n", F(0,7),
					(F(0,7))? "" : "(No restriction)");
	PH2 (78);	P ("Controller ID (CNTLID): 0x%x\n", F(0,15));
	PH4 (80);	P ("Version (VER): 0x%x\n", F(0,31));
	PH4 (84);	P ("RTD3 Resume Latency (RTD3R): 0x%x\n", F(0,31));
	PH4 (88);	P ("RTD3 Entry Latency (RTD3E): 0x%x\n", F(0,31));
	PH4 (92);	P ("Optional Asynchronous Events Supported (OAES):\n");
				P ("%26c  Supports sending Namespace Attribute Notices: %s\n", SP, YN(9));
				P ("%26c  Supports sending Firmware Activation Notices: %s\n", SP, YN(8));
	PH4 (96);	P ("Controller Attributes (CTRATT):\n");
				P ("%26c  Supports a 128-bit Host Identifier: %s\n", SP, YN(0));

	P ("\n[Admin Command Set Attributes & Optional Controller Capabilities]\n");
	PH2 (256);	P ("Optional Admin Command Support (OACS):\n");
				P ("%26c  Supports Namespace Management and Namespace Attachment commands: %s\n", SP, YN(3));
				P ("%26c  Supports Firmware Commit and Firmware Image Download commands: %s\n", SP, YN(2));
				P ("%26c  Supports Format NVM command: %s\n", SP, YN(1));
				P ("%26c  Supports Security Send and Security Receive commands: %s\n", SP, YN(0));
	PH1 (258);	P ("Abort Command Limit (ACL): %d\n", F(0,7));
	PH1 (259);	P ("Asynchronous Event Request Limit (AERL): %d\n", F(0,7));
	PH1 (260);	P ("Firmware Updates (FRMW)\n");
				P ("%26c  Supports firmware activation without a reset: %s\n", SP, YN(4));
				P ("%26c  The number of firmware slots: %d\n", SP, F(1,3));
				P ("%26c  First firmware slot: %s\n", SP, F(0,0)? "read only" : "read/write");
	PH1 (261);	P ("Log Page Attributes (LPA)\n");
				P ("%26c  Supports extended data for Get Log page: %s\n", SP, YN(2));
				P ("%26c  Supports the Command Effects log page: %s\n", SP, YN(1));
				P ("%26c  Supports the SMART / Health information log page per namespace: %s\n", SP, YN(0));
	PH1 (262);	P ("Error Log Page Entries (ELPE): %d\n", F(0,7));
	PH1 (263);	P ("Number of Power States Supported (NPSS): %d\n", F(0,7));
	PH1 (264);	P ("Admin Vendor Specific Command Configuration (AVSCC): %s\n",
					F(0,0)? "Standard format" : "Vendor specific format");
	PH1 (265);	P ("Autonomous Power State Transition Attributes (APSTA):\n");
				P ("%26c  Supports autonomous power state transitions: %s\n", SP, YN(0));
	PH2 (266);	P ("Warning Composite Temperature Threshold (WCTEMP): %x\n", F(0,15));
	PH2 (268);	P ("Critical Composite Temperature Threshold (CCTEMP): %x\n", F(0,15));
	PH2 (270);	P ("Maximum Time for Firmware Activation (MTFA): %x\n", F(0,15));
	PH4 (272);	P ("Host Memory Buffer Preferred Size (HMPRE): %d (in 4KB units)\n", F(0,31));
	PH4 (276);	P ("Host Memory Buffer Minimum Size (HMMIN): %d (in 4KB units)\n\n", F(0,31));
	PV (280, 295, "Total NVM Capacity (TNVMCAP)", "bytes");
	PV (296, 311, "Unallocated NVM Capacity (UNVMCAP)", "bytes");
	PH4 (312);	P ("Replay Protected Memory Block Support (RPMBS): %s\n", 
					F(0,2)? "supported" : "not supported");
			if (F(0,2))
			{
				P ("%26c  Access Size: %d (in 512B units)\n", SP, F(24,31));
				P ("%26c  Total Szie: %d (in 128KB units)\n", SP, F(16,23));
				P ("%26c  Authentication Method: %s\n", SP, F(3,5)? "Reserved" : "HMAC SHA-256");
				P ("%26c  Number of RPMB Units: %d\n", SP, F(0,2));
			}
	PH2 (320);	P ("Keep Alive Support (KAS): %d (in 100ms)\n", F(0,16));
	
	P ("\n[NVM Command Set Attributes]\n");
	PH1 (512);	P ("Submission Queue Entry Size (SQES):\n");
				P ("%26c  Maximum Submission Queue entry size: %d\n", SP, pow2(F(4,7)));
				P ("%26c  Required Submission Queue entry size: %d\n", SP, pow2(F(0,3)));;
	PH1 (513);	P ("Completion Queue Entry Size (CQES):\n");
				P ("%26c  Maximum Completion Queue entry size: %d\n", SP, pow2(F(4,7)));
				P ("%26c  Required Completion Queue entry size: %d\n", SP, pow2(F(0,3)));
	PH2 (514);	P ("Maximum Outstanding Commands (MAXCMD): %d\n", F(0,15));
	PH4 (516);	P ("Number of Namespaces (NN): %d\n", F(0,31));
	PH2 (520);	P ("Optional NVM Command Support (ONCS):\n");
				P ("%26c  Supports reservations: %s\n", SP, YN(5));
				P ("%26c  Supports Save/Select field in the Set/Get Features command: %s\n", SP, YN(4));
				P ("%26c  Supports Write Zeroes command: %s\n", SP, YN(3));
				P ("%26c  Supports Dataset Management command: %s\n", SP, YN(2));
				P ("%26c  Supports Write Uncorrectable command: %s\n", SP, YN(1));
				P ("%26c  Supports Compare command: %s\n", SP, YN(0));
	PH2 (522);	P ("Fused Operation Support (FUSES)\n");
				P ("%26c  Supports Compare and Write fused operation: %s\n", SP, YN(0));
	PH1 (524);	P ("Format NVM Attributes (FNA):\n");
				P ("%26c  Supports cryptographic erase: %s\n", SP, YN(2));
				P ("%26c  Does cryptographic erase and user data erase apply to all namespaces?: %s\n", SP, YN(1));
				P ("%26c  Does the format operation apply to all namespaces?: %s\n", SP, YN(0));
	PH1 (525);	P ("Volatile Write Cache (VWC)\n");
				P ("%26c  A volatile write cache present: %s\n", SP, YN(0));
	PH2 (526);	P ("Atomic Write Unit Normal (AWUN): %d blocks\n", F(0,15));
	PH2 (528);	P ("Atomic Write Unit Power Fail (AWUPF): %d blocks\n", F(0,15));
	PH1 (530);	P ("NVM Vendor Specific Command Configuration (NVSCC): %s\n",
					F(0,0)? "Standard format" : "Vendor specific format");
	PH2 (532);	P ("Atomic Compare & Write Unit (ACWU): %d blocks\n", F(0,15));
	PH4 (536);	P ("SGL Support (SGLS): %s\n", YN(0));
			if (F(0,0))
			{
				P ("%26c  Supports the Address field specifying an offset: %s\n", SP, YN(20));
				P ("%26c  Supports the Metadata Pointer (MPTR) containing an SGL Descriptor: %s\n", SP, YN(19));
				P ("%26c  Supports the SGL length larger than the amount of data to be transferred: %s\n", SP, YN(18));
				P ("%26c  Supports a byte aligned contiguous physical buffer of metadata: %s\n", SP, YN(17));
				P ("%26c  Supports the SGL Bit Bucket descriptor: %s\n", SP, YN(16));
				P ("%26c  Supports the Keyed SGL Data Block descriptor: %s\n", SP, YN(2));
			}

#define PSD0	2048	// Offset for PSD0

	P ("\n[Power State Descriptor 0 (PSD0)]\n");
	PH2 (PSD0+0);	P ("Maximum Power (MP): %d (in MXPS)\n", F(0,16));
	PH1 (PSD0+3);	P ("Max Power Scale (MXPS) / Non-Operational Staet (NOPS):\n");
						P ("%26c Max Power Scale (MXPS): %s (W)\n", SP, F(0,0)? "0.0001" : "0.01");
						P ("%26c The controller %s I/O commands in this state\n", SP,
							F(1,1)? "DOES NOT PROCESS" : "PROCESSES");
	PV (PSD0+4, PSD0+7, "Entry Latency (ENLAT)", "(microseconds)");
	PV (PSD0+8, PSD0+11, "Exit Latency (EXLAT)", "(microseconds)");
	PH1 (PSD0+12);	P ("Relative Read Throughput (RRT): %d\n", F(0,4));
	PH1 (PSD0+13);	P ("Relative Read Latency (RRL): %d\n", F(0,4));
	PH1 (PSD0+14);	P ("Relative Write Throughput (RWT): %d\n", F(0,4));
	PH1 (PSD0+15);	P ("Relative Write Latency (RWL): %d\n", F(0,4));
	PH2 (PSD0+16);	P ("Idle Power (IDLP): %d (in MXPS)\n", F(0,15));
	PH1 (PSD0+18);	P ("Idle Power Scale (IPS): %s\n", ps[F(6,7)]);
	PH2 (PSD0+20);	P ("Active Power (ACTP): %d (in APS)\n", F(0,15));
	PH1 (PSD0+22);	P ("Active Power Workload (APW) / Active Power Scale (APS)\n");
						P ("%26c Active Power Workload (APW): %d\n", SP, F(0,2));
						P ("%26c Active Power Scale (APS): %s\n", SP, ps[F(6,7)]);

	P ("\n\n");
}

void nvmed_info_identify_parse_namespace (__u8 *p, int nsid)
{
	int i;
	__u32 _v;
	static char *pi[] = {"Not enabled", "Type 1", "Type 2", "Type 3"};

	PRINT_NVMED_INFO;
	P ("IDENTIFY Namespace %d\n", nsid);
	P ("Bytes      Values       Description\n");
	P ("---------  -----------  -----------\n");
	
	PV (0, 7, "Namespace Size (NSZE)", "blocks");
	PV (8, 15, "Namespace Capacity (NCAP)", "blocks");
	PV (16, 23, "Namespace Utilization (NUSE)", "blocks");
	PH1 (24);	P ("Namespace Features (NSFEAT)\n");
				P ("%26c  Supports Deallocated or Unwritten Logical Block error: %s\n", SP, YN(2));
				P ("%26c  Supports NAWUN, NAWUPF, and NACWU fields: %s\n", SP, YN(1));
				P ("%26c  Supports thin provisioning: %s\n", SP, YN(0));
	PH1 (25);	P ("Number of LBA Formats (NLBAF): %d\n", F(0,7));
	PH1 (26);	P ("Formatted LBA Size (FLBAS):\n");
				P ("%26c  Metadata transferred at the end of the data LBA: %s\n", SP, YN(4));
				P ("%26c  Supported LBA formats: %d\n", SP, F(0,3));
	PH1 (27);	P ("Metadata Capabilties (MC):\n");
				P ("%26c  Supports the metadata being transferred as part of a separate buffer: %s\n", 
					SP, YN(1));
				P ("%26c  Supports the metadata being transferred as part of an extended data LBA: %s\n", 
					SP, YN(0));
	PH1 (28);	P ("End-to-end Data Protection Capabilties (DPC)\n");
				P ("%26c  Supports protection information transferred as the last eight bytes of metadata: %s\n", 
					SP , YN(4));
				P ("%26c  Supports protection information transferred as the first eight bytes of metadata: %s\n", 
					SP , YN(3));
				P ("%26c  Supports Protection Information Type 3: %s\n", SP, YN(2));
				P ("%26c  Supports Protection Information Type 2: %s\n", SP, YN(1));
				P ("%26c  Supports Protection Information Type 1: %s\n", SP, YN(0));

	PH1 (29);	P ("End-to-end Data Protection Type Settings (DPS):\n");
				P ("%26c  Protection information: at the %s eight bytes of metadata\n", SP, F(3,3)? "FIRST" : "LAST");
				P ("%26c  Protection information: %s\n", SP, (F(0,2) > 3)? "Reserved" : pi[F(0,2)]);
	PH1 (30);	P ("Namespace Multi-path I/O and Namespace Sharing Capabilities (NMIC):\n");
				P ("%26c  Namespace may be accessible by two or more controllers?: %s\n", SP, YN(0));
	PH1 (31);	P ("Reservation Capabilities (RESCAP)\n");
				P ("%26c  Supports Exclusive Access - All Registrants: %s\n", SP, YN(6));
				P ("%26c  Supports Write Exclusive Access - All Registrants: %s\n", SP, YN(5));
				P ("%26c  Supports Exclusive Access - Registrants Only: %s\n", SP, YN(4));
				P ("%26c  Supports Write Exclusive Access - Registrants Only: %s\n", SP, YN(3));
				P ("%26c  Supports Exclusive Access: %s\n", SP, YN(2));
				P ("%26c  Supports Write Exclusive Access: %s\n", SP, YN(1));
				P ("%26c  Supports Persist Through Power Loss capability: %s\n", SP, YN(0));
	PH1 (32);	P ("Format Progress Indicator (FPI):\n");
				P ("%26c  Supports the Format Proress Indicator: %s\n", SP, YN(7));
				P ("%26c  Percentage of the namespace that remains to be formatted: %d\n", SP, F(0,6));
	PH2 (34);	P ("Namespace Atomic Write Unit Normal (NAWUN): %d blocks\n", F(0,15));
	PH2 (36);	P ("Namespace Atomic Write Unit Power Fail (NAWUPF): %d blocks\n", F(0,15));
	PH2 (38);	P ("Namespace Atomic Write Unit Compare & Write Unit (NACWU): %d blocks\n", F(0,15));
	PH2 (40);	P ("Namespace Atomic Boundary Size Normal (NABSN): %d blocks\n", F(0,15));
	PH2 (42);	P ("Namespace Atomic Boundary Offset (NABO): %d\n", F(0,15));
	PH2 (44);	P ("Namespace Atomic Boundary Size Power Fail (NABSPF): %d\n", F(0,15));
	PV (48, 63, "NVM Capacity (NVMCAP)", "bytes");
	PI (104, 119, "Namespace Globally Unique Identifier (NGUID)");
	PI (120, 127, "IEEE Extended Unique Identifier (EUI64)");


	for (i = 128; i <192; i += 4)
	{
		static char *rp[] = {"Best", "Better", "Good", "Degraded"};

		if (U32(i) == 0) 
			break;					// no more LBA Format

		PH4 (i);	P ("LBA Format %d Support (LBAF%d):\n", (i - 128) / 4, (i - 128) / 4);
				P ("%26c  Relative Performance (RP): %s performance\n", SP, rp[F(24,25)]);
				P ("%26c  LBA Data Size (LBADS): %d bytes\n", SP, pow2(F(16,23)));
				P ("%26c  Metadata Size: %d bytes\n", SP, F(0,15));
	}

	P ("\n\n");
}


