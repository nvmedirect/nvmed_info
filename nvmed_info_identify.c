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

	if (cmd_args[0])
	{
		nsid = atoi(cmd_args[0]);
		if (nsid <= 0)
		{
			printf ("Invalid namespace ID %d\n", nsid);
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
	cmd.opcode = 0x06; //nvme_admin_identify;
	cmd.nsid = htole32(nsid);
	cmd.addr = (__u64) htole64(p);
	cmd.data_len = htole32(PAGE_SIZE);
	cmd.cdw10 = htole32(cns);

	rc = nvmed_info_admin_command(nvmed, &cmd);
	return rc;
}


void nvmed_info_identify_parse_controller (__u8 *p)
{
	PRINT_NVMED_INFO;
	P ("IDENTIFY Controller\n");
	P ("Bytes      Values       Description\n");
	P ("---------  -----------  -----------\n");
	P ("\n[Controller Capabilities and Features]\n");
	PH2 (0);	P ("PCI Vendor ID (VID): 0x%x\n", U16(0));
	PH2 (2);	P ("PCI Subsystem Vendor ID (SSVID): 0x%x\n", U16(2));
	PS (04, 23, "Serial Number (SN)");
	PS (24, 63, "Model Number (MN)");
	PS (64, 71, "Firmware Revision (FR)");
	PH1 (72);	P ("Recommended Arbitration Burst (RAB): %d\n", p[72]);
	PH3 (73);	P ("IEEE OUI Identifier (IEEE): 0x%02x%02x%02x\n", p[75], p[74], p[73]);
	PH1 (76);	P ("Controller Multi-Path I/O and Namespace Sharing Capabilities (CMIC):\n");
				P ("%26c  %s, %s, %s\n", ' ',
					ISSET_BIT2(76)? "SR-IOV Virtual Function" : "PCI Function",
					ISSET_BIT1(76)? "2+ controllers" : "single controller",
					ISSET_BIT0(76)? "2+ PCIe ports" : "single PCIe port");
	PH1 (77);	P ("Maximum Data Transfer Size (MDTS): %d %s\n", p[77],
					(p[77])? "" : "(No restriction)");
	PH2 (78);	P ("Controller ID (CNTLID): 0x%x\n", U16(78));
	PH4 (80);	P ("Version (VER): 0x%x\n", U32(80));
	PH4 (84);	P ("RTD3 Resume Latency (RTD3R): 0x%x\n", U32(84));
	PH4 (88);	P ("RTD3 Entry Latency (RTD3E): 0x%x\n", U32(88));
	PH4 (92);	P ("Optional Asynchronous Events Supported (OAES):\n");
				P ("%26c  Supports sending Namespace Attribute Changed event: %s\n", ' ',
					ISSET_BIT0(93)? "Yes" : "No");

	P ("\n[Admin Command Set Attributes & Optional Controller Capabilities]\n");
	PH2 (256);	P ("Optional Admin Command Support (OACS):\n");
				P ("%26c  Supports Namespace Management and Namespace Attachment commands: %s\n", ' ',
					ISSET_BIT3(256)? "Yes" : "No");
				P ("%26c  Supports Firmware Commit and Firmware Image Download commands: %s\n", ' ',
					ISSET_BIT2(256)? "Yes" : "No");
				P ("%26c  Supports Format NVM command: %s\n", ' ',
					ISSET_BIT1(256)? "Yes" : "No");
				P ("%26c  Supports Security Send and Security Receive commands: %s\n", ' ',
					ISSET_BIT0(256)? "Yes" : "No");
	PH1 (258);	P ("Abort Command Limit (ACL): %d\n", p[258]);
	PH1 (259);	P ("Asynchronous Event Request Limit (AERL): %d\n", p[259]);
	PH1 (260);	P ("Firmware Updates (FRMW)\n");
				P ("%26c  Supports firmware activation without a reset: %s\n", ' ',
					ISSET_BIT4(260)? "Yes" : "No");
				P ("%26c  The number of firmware slots: %d\n", ' ', (p[260] >> 1) & 0x7);
				P ("%26c  First firmware slot: %s\n", ' ', 
					ISSET_BIT0(260)? "read only" : "read/write");
	PH1 (261);	P ("Log Page Attributes (LPA)\n");
				P ("%26c  Supports the Command Effects log page: %s\n", ' ',
					ISSET_BIT1(261)? "Yes" : "No");
				P ("%26c  Supports the SMART / Health information log page / namespace: %s\n", ' ',
					ISSET_BIT0(261)? "Yes" : "No");
	PH1 (262);	P ("Error Log Page Entries (ELPE): %d\n", p[262]);
	PH1 (263);	P ("Number of Power States Supported (NPSS): %d\n", p[263]);
	PH1 (264);	P ("Admin Vendor Specific Command Configuration (AVSCC): %s\n",
					ISSET_BIT0(264)? "Standard format" : "Vendor specific format");
	PH1 (265);	P ("Autonomous Power State Transition Attributes (APSTA):\n");
				P ("%26c  Supports autonomous power state transitions: %s\n", ' ',
					ISSET_BIT0(265)? "Yes" : "No");
	PH2 (266);	P ("Warning Composite Temperature Threshold (WCTEMP): %x\n", U16(266));
	PH2 (268);	P ("Critical Composite Temperature Threshold (CCTEMP): %x\n", U16(268));
	PH2 (270);	P ("Maximum Time for Firmware Activation (MTFA): %x\n", U16(270));
	PH4 (272);	P ("Host Memory Buffer Preferred Size (HMPRE): %d (in 4KB units)\n", U32(272));
	PH4 (276);	P ("Host Memory Buffer Minimum Size (HMMIN): %d (in 4KB units)\n\n", U32(276));
	PV (280, 295, "Total NVM Capacity (TNVMCAP)", "bytes");
	PV (296, 311, "Unallocated NVM Capacity (UNVMCAP)", "bytes");
	PH4 (312);	P ("Replay Protected Memory Block Support (RPMBS): %s\n",
					(p[312] & 0x7)? "supported" : "not supported");
			if (p[312] & 0x7)
			{
				P ("%26c  Access Size: %d (in 512B units)\n", ' ', p[315]);
				P ("%26c  Total Szie: %d (in 128KB units)\n", ' ', p[314]);
				P ("%26c  Authentication Method: %s\n", ' ',
					((p[312] >> 3) & 0x07)? "Reserved" : "HMAC SHA-256");
				P ("%26c  Number of RPMB Units: %d\n", ' ', p[312] & 0x7);
			}
	
	P ("\n[NVM Command Set Attributes]\n");
	PH1 (512);	P ("Submission Queue Entry Size (SQES):\n");
				P ("%26c  Maximum Submission Queue entry size: %d\n", ' ', pow2((p[512] >> 4) & 0x0f));
				P ("%26c  Required Submission Queue entry size: %d\n", ' ', pow2(p[512] & 0x0f));
	PH1 (513);	P ("Completion Queue Entry Size (CQES):\n");
				P ("%26c  Maximum Completion Queue entry size: %d\n", ' ', pow2((p[513] >> 4) & 0x0f));
				P ("%26c  Required Completion Queue entry size: %d\n", ' ', pow2(p[513] & 0x0f));
	PH4 (516);	P ("Number of Namespaces (NN): %d\n", U32(516));
	PH2 (520);	P ("Optional NVM Command Support (ONCS):\n");
				P ("%26c  Supports reservations: %s\n", ' ',
					ISSET_BIT5(520)? "Yes" : "No");
				P ("%26c  Supports Save/Select field in the Set/Get Features command: %s\n", ' ',
					ISSET_BIT4(520)? "Yes" : "No");
				P ("%26c  Supports Write Zeroes command: %s\n", ' ',
					ISSET_BIT3(520)? "Yes" : "No");
				P ("%26c  Supports Dataset Management command: %s\n", ' ',
					ISSET_BIT2(520)? "Yes" : "No");
				P ("%26c  Supports Write Uncorrectable command: %s\n", ' ',
					ISSET_BIT1(520)? "Yes" : "No");
				P ("%26c  Supports Compare command: %s\n", ' ',
					ISSET_BIT0(520)? "Yes" : "No");
	PH2 (522);	P ("Fused Operation Support (FUSES)\n");
				P ("%26c  Supports Compare and Write fused operation: %s\n", ' ',
					ISSET_BIT0(522)? "Yes" : "No");
	PH1 (524);	P ("Format NVM Attributes (FNA):\n");
				P ("%26c  Supports cryptographic erase: %s\n", ' ',
					ISSET_BIT2(524)? "Yes" : "No");
				P ("%26c  Does cryptographic erase and user data erase apply to all namespaces?: %s\n", ' ',
					ISSET_BIT1(524)? "Yes" : "No");
				P ("%26c  Does the format operation apply to all namespaces?: %s\n", ' ',
					ISSET_BIT0(524)? "Yes" : "No");
	PH1 (525);	P ("Volatile Write Cache (VWC)\n");
				P ("%26c  Is a volatile write cache present?: %s\n", ' ',
					ISSET_BIT0(525)? "Yes" : "No");
	PH2 (526);	P ("Atomic Write Unit Normal (AWUN): %d blocks\n", U16(526));
	PH2 (528);	P ("Atomic Write Unit Power Fail (AWUPF): %d blocks\n", U16(528));
	PH1 (530);	P ("NVM Vendor Specific Command Configuration (NVSCC): %s\n",
					ISSET_BIT0(530)? "Standard format" : "Vendor specific format");
	PH2 (532);	P ("Atomic Compare & Write Unit (ACWU): %d blocks\n", U16(532));
	PH4 (536);	P ("SGL Support (SGLS): %s\n",
					ISSET_BIT0(536)? "Yes" : "No");
			if (ISSET_BIT0(536))
			{
				P ("%26c  Supports SGL length larger than the data size: %s\n", ' ', 
					ISSET_BIT2(538)? "Yes" : "No");
				P ("%26c  Supports use of a byte aligned contiguous physical buffer of metadata: %s\n", ' ',
					ISSET_BIT1(538)? "Yes" : "No");
				P ("%26c  Supports the SGL Bit Bucket descriptor: %s\n", ' ',
					ISSET_BIT0(538)? "Yes" : "No");
			}

	P ("\n\n");
}

void nvmed_info_identify_parse_namespace (__u8 *p, int nsid)
{
	int i;

	PRINT_NVMED_INFO;
	P ("IDENTIFY Namespace %d\n", nsid);
	P ("Bytes      Values       Description\n");
	P ("---------  -----------  -----------\n");
	
	PV (0, 7, "Namespace Size (NSZE)", "bytes");
	PV (8, 15, "Namespace Capacity (NCAP)", "blocks");
	PV (16, 23, "Namespace Utilization (NUSE)", "blocks");
	PH1 (24);	P ("Namespace Features (NSFEAT)\n");
				P ("%26c  Supports Deallocated or Unwritten Logical Block error: %s\n", 
					' ', YN_BIT2(24));
				P ("%26c  Supports NAWUN, NAWUPF, and NACWU fields: %s\n", 
					' ', YN_BIT1(24));
				P ("%26c  Supports thin provisioning: %s\n", 
					' ', YN_BIT0(24));
	PH1 (25);	P ("Number of LBA Formats (NLBAF): %d\n", p[25]);
	PH1 (26);	P ("Formatted LBA Size (FLBAS):\n");
				P ("%26c  Metadata transferred at the end of the data LBA: %s\n", 
					' ', YN_BIT4(26));
	PH1 (27);	P ("Metadata Capabilties (MC):\n");
				P ("%26c  Supports the metadata being transferred as part of a separate buffer: %s\n", 
					' ', YN_BIT1(27));
				P ("%26c  Supports the metadata being transferred as part of an extended data LBA: %s\n", 
					' ', YN_BIT0(27));
	PH1 (28);	P ("End-to-end Data Protection Capabilties (DPC)\n");
				P ("%26c  Supports protection information transferred as the last eight bytes of metadata: %s\n", 
					' ' , YN_BIT4(28));
				P ("%26c  Supports protection information transferred as the first eight bytes of metadata: %s\n", 
					' ' , YN_BIT3(28));
				P ("%26c  Supports Protection Information Type 3: %s\n", ' ', YN_BIT2(28));
				P ("%26c  Supports Protection Information Type 2: %s\n", ' ', YN_BIT1(28));
				P ("%26c  Supports Protection Information Type 1: %s\n", ' ', YN_BIT0(28));

	PH1 (29);	P ("End-to-end Data Protection Type Settings (DPS):\n");
				P ("%26c  Protection information: at the %s eight bytes of metadata\n", ' ',
					ISSET_BIT3(29)? "first" : "last");
				P ("%26c  Protection information: %s\n", ' ',
					((p[29] & 7) == 0)? "Not enabled" : 
					((p[29] & 7) == 1)? "Type 1" : 
					((p[29] & 7) == 2)? "Type 2" :
					((p[29] & 7) == 3)? "Type 3" : "Reserved");
	PH1 (30);	P ("Namespace Multi-path I/O and Namespace Sharing Capabilities (NMIC):\n");
				P ("%26c  Namespace may be accessible by two or more controllers?: %s\n",
					' ', YN_BIT0(30));
	PH1 (31);	P ("Reservation Capabilities (RESCAP)\n");
				P ("%26c  Supports Exclusive Access - All Registrants: %s\n", 
					' ', YN_BIT6(31));
				P ("%26c  Supports Write Exclusive Access - All Registrants: %s\n", 
					' ', YN_BIT6(31));
				P ("%26c  Supports Exclusive Access - Registrants Only: %s\n", 
					' ', YN_BIT4(31));
				P ("%26c  Supports Write Exclusive Access - Registrants Only: %s\n", 
					' ', YN_BIT3(31));
				P ("%26c  Supports Exclusive Access: %s\n", 
					' ', YN_BIT2(31));
				P ("%26c  Supports Write Exclusive Access: %s\n", 
					' ', YN_BIT1(31));
				P ("%26c  Supports Persist Through Power Loss capability: %s\n", 
					' ', YN_BIT0(31));
	PH1 (32);	P ("Format Progress Indicator (FPI):\n");
				P ("%26c  Supports the Format Proress Indicator: %s\n", ' ', YN_BIT7(32));
				P ("%26c  Percentage of the namespace that remains to be formatted: %d\n",
					' ', p[32] & 0x7f);
	PH2 (34);	P ("Namespace Atomic Write Unit Normal (NAWUN): %d blocks\n", U16(34));
	PH2 (36);	P ("Namespace Atomic Write Unit Power Fail (NAWUPF): %d blocks\n", U16(36));
	PH2 (38);	P ("Namespace Atomic Write Unit Compare & Write Unit (NACWU): %d blocks\n", U16(38));
	PH2 (40);	P ("Namespace Atomic Boundary Size Normal (NABSN): %d blocks\n", U16(40));
	PH2 (42);	P ("Namespace Atomic Boundary Offset (NABO): %d\n", U16(42));
	PH2 (44);	P ("Namespace Atomic Boundary Size Power Fail (NABSPF): %d\n", U16(44));

	PV (48, 63, "NVM Capacity (NVMCAP)", "bytes");
	PI (104, 119, "Namespace Globally Unique Identifier (NGUID)");
	PI (120, 127, "IEEE Extended Unique Identifier (EUI64)");


	for (i = 128; i <192; i += 4)
	{
		char *rp[] = {"Best", "Better", "Good", "Degraded"};

		if (U32(i) == 0) 
			break;					// no more LBA Format

		PH4 (i);	P ("LBA Format %d Support (LBAF%d):\n", (i - 128) / 4, (i - 128) / 4);
				P ("%26c  Relative Performance (RP): %s performance\n", 
					' ', rp[p[i+3] & 0x3]);
				P ("%26c  LBA Data Size (LBADS): %d bytes\n", ' ', pow2(p[i+2]));
				P ("%26c  Metadata Size: %d bytes\n", ' ', U16(i));
	}

	P ("\n\n");
}


