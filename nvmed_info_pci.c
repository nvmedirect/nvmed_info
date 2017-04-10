#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "nvme_hdr.h"
#include "nvmed.h"
#include "lib_nvmed.h"
#include "nvmed_info.h"


#define PCI_FILE_COPY	0
#define PCI_FILE_MMAP	1

struct nvmed_info_cmd pci_cmds[] = {
	{"config", 1, "PCIe Config Space", nvmed_info_pci_config},
	{NULL, 0, NULL, NULL}
};

int nvmed_info_pci (NVMED *nvmed, char **cmd_args)
{
	struct nvmed_info_cmd *c;

	if (cmd_args[0] == NULL)
		return nvmed_info_pci_config(nvmed, NULL);

	c = cmd_lookup(pci_cmds, cmd_args[0]);
	if (c)
		return c->cmd_fn (nvmed, &cmd_args[1]);
	else {
		nvmed_info_pci_help(cmd_args[0]);
		return -1;
	}
}

int nvmed_info_pci_help (char *s)
{
	return cmd_help(s, "PCIe Registers", pci_cmds);
}

int nvmed_info_pci_open (NVMED *nvmed, char *name, int type, struct pci_info *pci)
{
	char *sysfs_path;
	char *p;
	int rc;
	struct stat st;

	if (name == NULL || pci == NULL)
		return -1;

	// "admin" replaced with "sysfs/name"; +1 for '/', +1 for null
	sysfs_path = (char *) malloc(strlen(nvmed->ns_path) + strlen(name) + 2);
	strcpy (sysfs_path, nvmed->ns_path);
	p = strstr(sysfs_path, "admin");
	if (p == NULL) {
		printf("Wrong path: %s\n", nvmed->ns_path);
		goto abort;
	}
	strcpy(p, "sysfs/");
	strcat(p, name);			


	rc = stat(sysfs_path, &st);
	if (rc < 0 || st.st_size <= 0) {
		printf("invalid stat() for %s\n", sysfs_path);
		goto abort;
	}

	memset((char *) pci, 0, sizeof(*pci));
	pci->len = st.st_size;
	pci->fd = open(sysfs_path, O_RDWR | O_SYNC);
	pci->type = type;
	if (pci->fd < 0) {
		printf("open() failed for %s\n", sysfs_path);
		goto abort;
	}

	switch (pci->type) {
		case PCI_FILE_COPY:
			pci->regs = malloc(pci->len);
			if (read (pci->fd, (char *) pci->regs, pci->len) != pci->len) {
				printf("read() failed for %s\n", sysfs_path);
				goto abort;
			}
			break;

		case PCI_FILE_MMAP:
			pci->regs = mmap(0, pci->len, PROT_READ | PROT_WRITE, MAP_SHARED, pci->fd, 0);
			if (pci->regs == (void *) -1) {
				printf("mmap() failed for %s\n", sysfs_path);
				goto abort;
			}
			break;
		default:
			printf("invalid type %d\n", pci->type);
			goto abort;
	}
	return 0;

abort:
	if (pci->fd)
		close(pci->fd);
	free(sysfs_path);
	return -1;
}

int nvmed_info_pci_close (struct pci_info *pci)
{
	if (pci == NULL)
		return -1;

	if (pci->type == PCI_FILE_COPY) 
		free(pci->regs);
	else if (pci->type == PCI_FILE_MMAP)
		munmap(pci->regs, pci->len);
	if (pci->fd >= 0)
		close(pci->fd);
	return 0;
}


int nvmed_info_pci_config (NVMED *nvmed, char **cmd_args)
{
	int rc;
	struct pci_info pci;
	//__u8 *p;

	rc = nvmed_info_pci_open(nvmed, "config", PCI_FILE_COPY, &pci);
	if (rc < 0)
		return -1;

	nvmed_info_pci_parse_config(nvmed, &pci);

	nvmed_info_pci_close(&pci);
	return 0;
}


void nvmed_info_pci_parse_config (NVMED *nvmed, struct pci_info *pci)
{
	__u8 *p;
	__u32 _v;
	
	if (pci == NULL || pci->regs == NULL)
		return;

	p = (__u8 *) pci->regs;

	PRINT_NVMED_INFO;
	P ("PCIe Config Registers\n");
    P ("Bytes      Values       Description\n");
	P ("---------  -----------  -----------\n");
	P ("\n[PCI Header]\n");
	PH4 (0);	P ("Identifiers (ID):\n");
				P ("%26c Device ID (VID): 0x%04x\n", SP, F(16,31));
				P ("%26c Vendor ID (VID): 0x%04x\n", SP, F(0,15));

	PH2 (4);	P ("Command (CMD):\n");
				P ("%26c Interrupt Disable (ID): %s\n", SP, YN(10));
			  	P ("%26c SERR# Enable (SEE): %s\n", SP, YN(8));
				P ("%26c Parity Error Response Enable (PEE): %s\n", SP, YN(6));
				P ("%26c Bus Master Enable (BME): %s\n", SP, YN(2));
				P ("%26c Memory Space Enable (MSE): %s\n", SP, YN(1));
				P ("%26c I/O Space Enable (IOSE): %s\n", SP, YN(0));

	PH2 (6);	P ("Device Status (STS):\n");
				P ("%26c Detected Parity Error (DPE): %s\n", SP, YN(15));
				P ("%26c Signaled System Error (SSE): %s\n", SP, YN(14));
				P ("%26c Received Master-Abort (RMA): %s\n", SP, YN(13));
				P ("%26c Received Target-Abort (RTA): %s\n", SP, YN(12));
				P ("%26c Master Data Parity Error Detected (DPD): %s\n", SP, YN(8));
				P ("%26c Capabilities List (CL): %s\n", SP, YN(4));
				P ("%26c Interrupt Status (IS): %s\n", SP, YN(3));

	PH1 (8);	P ("Revision ID (RID): %02x\n", F(0,7));

	PH3 (9);	P ("Class Code (CC):\n");
				P ("%26c Base Class Code (BCC): %02x\n", SP, F(16,23));
				P ("%26c Sub Class Code (SCC): %02x\n", SP, F(8,15));
				P ("%26c Programming Interface (PI): %02x\n", SP, F(0,7));

	PH1 (12)	P ("Cache Line Size (CLS): %d\n", F(0,7));

	//PH1 (13);	P ("Master Latency Timer (MLT): %d clocks\n", U8(13));

	PH1 (14);	P ("Header Type (HTYPE):\n");
				P ("%26c Multi-Function Device (MFD): %s\n", SP, YN(7));
				P ("%26c Header Layout (HL): %d\n", SP, F(0,6));

	PH1 (15);	P ("Built In Self Test (BIST): [Optional] %s\n", F(0,7)? "" : "Not supported");
			if (F(0,7)) {
				P ("%26c BIST Capable (BC): %s\n", SP, YN(7));
				P ("%26c Start BIST (SB): %s\n", SP, YN(6));
				P ("%26c Completion Code (CC): %d\n", SP, F(0,3));
			}

	PH4 (16); 	P ("Memory Register Base Address Lower 32-bits (MLBAR, BAR0):\n");
				P ("%26c Base Address (BA): 0x%08x\n", SP, F(14,31) << 14);
				P ("%26c Prefetchable (PF): %s\n", SP, YN(3));
				P ("%26c Type (TP): %d\n", SP, F(1,2));
				P ("%26c Resource Type Indicator (RTE): %s\n", SP, YN(0));
	
	PH4 (20);	P ("Memory Register Base Address Upper 32-bits (MUBAR, BAR1): 0x%08x\n", F(0,31));

	PH4 (24);	P ("Index/Data Pair Register Base Address (IDBAR, BAR2): [Optional] %s\n",
					F(0,31)? "" : "Not supported");
			if (F(0,31)) {
				P ("%26c Base Address (BA): 0x%08x\n", SP, F(3,31) << 3);
				P ("%26c Resource Type Indicator (RTE): %s\n", SP, YN(0));
			}

	PH4 (44);	P ("Sub System Identifiers (SS)\n");
				P ("%26c Subsystem ID (SSID): 0x%04x\n", SP, F(16,31));
				P ("%26c Subsystem Vendor ID (SSVID): 0x%04x\n", SP, F(0,15));

	PH4 (48);	P ("Expansion ROM: [Optional] %s\n", F(0,31)? "" : "Not supported");
			if (F(0,31)) 
				P ("%26c ROM Base Address (RBA): 0x%08x\n", SP, F(0,31));

	PH1 (52);	P ("Capabilities Pointer (CAP):\n");
				P ("%26c Capability Pointer (CP): 0x%02x\n", SP, F(0,7));

	PH2 (60);	P ("Interrupt Information (INTR):\n");
				P ("%26c Interrupt Pin (IPIN): %d\n", SP, F(8,15));
				P ("%26c Interrupt Line (ILINE): %d\n", SP, F(0,7));

	nvmed_info_pci_parse_caps (nvmed, pci);

}


#define PCI_CAP_NEXT(id)	(((id) >> 8) & 0xff)
#define PCI_CAP_CID(id)		((id) & 0xff)

#define PCI_PMCAP_CID		(0x01)
#define PCI_MSICAP_CID		(0x05)
#define	PCI_MSIXCAP_CID		(0x11)
#define PCI_PXCAP_CID		(0x10)
#define PCI_AERCAP_CID		(0x0001)						// This has a 32-bit header

#define PCI_CAP_OFFSET		52


struct pci_cap {
	int capid;
	void (*parse_fn)(NVMED *nvmed, struct pci_info *pci, int offset);;
};

struct pci_cap caps[] = {
	{PCI_PMCAP_CID,		nvmed_info_pci_parse_pmcap},
	{PCI_MSICAP_CID, 	nvmed_info_pci_parse_msicap},
	{PCI_MSIXCAP_CID, 	nvmed_info_pci_parse_msixcap},
	{PCI_PXCAP_CID,		nvmed_info_pci_parse_pxcap},
	//{PCI_AERCAP_CID,	nvmed_info_pci_parse_aercap},	
	{0,					NULL}
};


void nvmed_info_pci_parse_caps (NVMED *nvmed, struct pci_info *pci)
{
	int offset;
	struct pci_cap *c;
	__u16 id;
	__u8 *p = (__u8 *) pci->regs;

	offset = (int) U8(PCI_CAP_OFFSET);
	while (offset > 0) {
		id = U16(offset);
		c = caps;
		while (c->capid) {
			if (c->capid == PCI_CAP_CID(id)) {
				c->parse_fn(nvmed, pci, offset);
				offset = PCI_CAP_NEXT(id);
				break;
			}
			c++;
		}
		if (c->capid == 0) {
			printf ("Unknown Capability ID 0x%02x, skipped\n", PCI_CAP_CID(id));
			offset = PCI_CAP_NEXT(id);
		}
	}

}

void nvmed_info_pci_parse_pmcap (NVMED *nvmed, struct pci_info *pci, int offset)
{
	__u8 *p = (__u8 *) pci->regs + offset;
	__u32 _v;
	static char *ps[] = { "D0", "D1", "D2", "D3_HOT" };

	P ("\n[PCI Power Management Capabilities] @ offset 0x%02x\n", offset);

	PH2 (0);	P ("PCI Power Management Capability ID (PID):\n");
				P ("%26c Next Capability (NEXT): 0x%02x\n", SP, F(8,15));
				P ("%26c Cap ID (CID): 0x%02x\n", SP, F(0,7));

	PH2 (2);	P ("PCI Power Management Capabilities (PC):\n");
				P ("%26c Device Specific Initialization (DSI): %s\n", SP, YN(5));
				P ("%26c PME Clock (PMEC): %d\n", SP, F(3,3));
				P ("%26c Version (VS): %d\n", SP, F(0,2));

	PH2 (4);	P ("PCI Power Management Control and Status (PMCS):\n");
				P ("%26c PME Status (PMES): %d\n", SP, F(15,15));
				P ("%26c Data Scale (DSC): %d\n", SP, F(13,14));
				P ("%26c Data Select (DSE): %d\n", SP, F(9,12));
				P ("%26c PME Enable (PMEE): %d\n", SP, F(8,8));
				P ("%26c No Soft Reset (NSFRST): %d\n", SP, F(3,3));
				P ("%26c Power Stats (PS): %s state\n", SP, ps[F(0,1)]);
}


void nvmed_info_pci_parse_msicap (NVMED *nvmed, struct pci_info *pci, int offset)
{
	__u8 *p = (__u8 *) pci->regs + offset;
	__u32 _v;

	P ("\n[Message Signaled Interrupt Capabilities] @ offset 0x%02x [Optional]\n", offset);

	PH2 (0);	P ("Message Signaled Interrupt Identifiers (MID):\n");
				P ("%26c Next Pointer (NEXT): 0x%02x\n", SP, F(8,15));
				P ("%26c Capability ID (CID): 0x%02x\n", SP, F(0,7));

	PH2 (2);	P ("Message Signaled Interrupt Message Control (MC):\n");
				P ("%26c Per-Vector Masking Capable (PVM): %s\n", SP, YN(8));
				P ("%26c 64bit Address Capable (C64): %s\n", SP, YN(7));
				P ("%26c Multiple Message Enable (MME): %d\n", SP, F(4,6));
				P ("%26c Multiple Message Capable (MMC): %d\n", SP, F(1,3));
				P ("%26c MSI Enable (MSIE): %s\n", SP, YN(0));

	PH4 (4);	P ("Message Signaled Interrupt Message Address (MA): 0x%08x\n", F(2,31) << 2);

	PH4 (8);	P ("Message Signaled Interrupt Upper Address (MUA): 0x%08x\n", F(0,31));

	PH2 (12);	P ("Message Signaled Interrupt Message Data (MD): 0x%04x\n", F(0,15));
	PH4 (16);	P ("Message Signaled Interrupt Mask Bits (MMASK): 0x%08x\n", F(0,31));
	PH4 (20);	P ("Message Signaled Interrupt Pending Bits (MPEND): 0x%08x\n", F(0,31));
}


void nvmed_info_pci_parse_msixcap (NVMED *nvmed, struct pci_info *pci, int offset)
{
	__u8 *p = (__u8 *) pci->regs + offset;
	__u32 _v;
	static char *bir[] = {"0x10", "N/A", "N/A", "Reserved", "0x20", "0x24", "Reserved", "Reserved"};

	P ("\n[MSI-X Capabilities] @ offset 0x%02x [Optional]\n", offset);

	PH2 (0);	P ("MSI-X Identifiers (MXID):\n");
				P ("%26c Next Pointer (NEXT): 0x%02x\n", SP, F(8,15));
				P ("%26c Capability ID (CID): 0x%02x\n", SP, F(0,7));

	PH2 (2);	P ("MSI-X Message Control (MXC):\n");
				P ("%26c MSI-X Enable (MXE): %s\n", SP, YN(15));
				P ("%26c Function Mask (FM): %s\n", SP, YN(14));
				P ("%26c Table Size (TS): %d\n", SP, F(0,10));

	PH4 (4);	P ("MSI-X Table Offset / Table BIR (MTAB):\n");
				P ("%26c Table Offset (TO): 0x%08x\n", SP, F(3,31) << 3);
				P ("%26c Table BIR (TBIR): %s\n", SP, bir[F(0,2)]);

	PH4 (8);	P ("MSI-X PBA Offset / PBA BIR (MPBA):\n");
				P ("%26c PBA Offset (PBAO): 0x%08x\n", SP, F(3,31) << 3);
				P ("%26c PBA BIR (PBIR): %s\n", SP, bir[F(0,2)]);

}
void nvmed_info_pci_parse_pxcap (NVMED *nvmed, struct pci_info *pci, int offset)
{
	__u8 *p = (__u8 *) pci->regs + offset;
	__u32 _v;

	P ("\n[PCI Express Capabilities] @ offset 0x%02x\n", offset);

	PH2 (0);	P ("PCI Express Capability ID (PXID):\n");
				P ("%26c Next Pointer (NEXT): 0x%02x\n", SP, F(8,15));
				P ("%26c Capability ID (CID): 0x%02x\n", SP, F(0,7));

	PH2 (2);	P ("PCI Express Capabilities (PXCAP):\n");
				P ("%26c Interrupt Message Number (IMN): %d\n", SP, F(9,13));
				P ("%26c Device/Port Type (DPT): %d\n", SP, F(4,7));
				P ("%26c Capability Version (VER): %d\n", SP, F(0,3));
	
	PH4 (4);	P ("PCI Express Device Capabilities (PXDCAP):\n");
				P ("%26c Function Level Reset Capability (FLRC): %s\n", SP, YN(28));
				P ("%26c Captured Slot Power Limit Scale (CSPLS): %d\n", SP, F(26,27));
				P ("%26c Captured Slot Power Limit Value (CSPLV): %d\n", SP, F(18,25));
				P ("%26c Role-based Error Reporting (RER): %s\n", SP, YN(15));
				P ("%26c Endpoint L1 Acceptable Latency (L1L): %d\n", SP, F(9,11));
				P ("%26c Endpoint L0s Acceptable Latency (L0SL): %d\n", SP, F(6,8));
				P ("%26c Extended Tag Field Supported (ETFS): %s\n", SP, YN(5));
				P ("%26c Phantom Functions Supported (PFS): %d\n", SP, F(3,4));
				P ("%26c Max_Payload_Size Supported (MPS): %d\n", SP, F(0,2));

	PH2 (8);	P ("PCI Express Device Control (PXDC):\n");
				P ("%26c Initiate Function level Reset (IFLR): %s\n", SP, YN(15));
				P ("%26c Max_Read_Request Size (MRRS): %d\n", SP, F(12,14));
				P ("%26c Enable No Snoop (ENS): %s\n", SP, YN(11));
				P ("%26c AUX Power PM Enable (APPME): %s\n", SP, YN(10));
				P ("%26c Phantom Functions Enable (PFE): %s\n", SP, YN(9));
				P ("%26c Extended Tag Enable (ETE): %s\n", SP, YN(8));
				P ("%26c Max_Payload_Size (MPS): %d\n", SP, F(5,7));
				P ("%26c Enable Relaxed Ordering (ERO): %s\n", SP, YN(4));
				P ("%26c Unsupported Request Reporting Enable (URRE): %s\n", SP, YN(3));
				P ("%26c Fatal Error Reporting Enable (FERE): %s\n", SP, YN(2));
				P ("%26c Non-Fatal Error Reporting Enable (NFERE): %s\n", SP, YN(1));
				P ("%26c Correctable Error Reporting Enable (CERE): %s\n", SP, YN(0));
	
	PH2 (10);	P ("PCI Express Device Status (PXDS):\n");
				P ("%26c Transactions Pending (TP): %s\n", SP, YN(5));
				P ("%26c AUX Power Detected (APD): %s\n", SP, YN(4));
				P ("%26c Unsupported Request Detected (URD): %s\n", SP, YN(3));
				P ("%26c Fatal Error Detected (FED): %s\n", SP, YN(2));
				P ("%26c Non-Fatal Error Detected (NFED): %s\n", SP, YN(1));
				P ("%26c Correctable Error Detected (CED): %s\n", SP, YN(0));

	PH4 (12);	P ("PCI Express Link Capabilities (PXLCAP):\n");
				P ("%26c Port Number (PN): 0x%02x\n", SP, F(24,31));
				P ("%26c ASPM Optionality Compliance (AOC): %s\n", SP, YN(22));
				P ("%26c Clock Power Management (CPM): %s\n", SP, YN(18));
				P ("%26c L1 Exit Latency (L1EL): %d\n", SP, F(15,17));
				P ("%26c L0s Exit Latency (L0SEL): %d\n", SP, F(12,14));
				P ("%26c Active State Power Management Support (ASPMS): %d\n", SP, F(10,11));
				P ("%26c Maximum Link Width (MLW): %d lanes\n", SP, F(4,9));
				P ("%26c Supported Link Speeds (SLS): Gen %d\n", SP, F(0,3));

	PH2 (16);	P ("PCI Express Link Control (PXLC):\n");
				P ("%26c Hardware Autonomous Width Disable (HAWD): %s\n", SP, YN(9));
				P ("%26c Enable Clock Power Management (ECPM): %s\n", SP, YN(8));
				P ("%26c Extended Synch (ES): %s\n", SP, YN(7));
				P ("%26c Common Clock Configuration (CCC): %s\n", SP, YN(6));
				P ("%26c Read Completion Boundary (RCB): %d\n", SP, F(3,3));
				P ("%26c Active State Power Management Control (ASPMC): %d\n", SP, F(0,1));

	PH2 (18);	P ("PCI Express Link Status (PXLS):\n");
				P ("%26c Slot Clock Configuration (SCC): %d\n", SP, F(12,12));
				P ("%26c Negotiated Link Width (NLW): %d lanes\n", SP, F(4,9));
				P ("%26c Current Link Speed (CLS): Gen %d\n", SP,F(0,3));

	static char *tph[] = {"None", "TPH Completer Supported", "Reserved", "Both TPH and Extended TPH Completer Supported"};

	PH4 (36);	P ("PCI Express Device Capabilities 2 (PXDCAP2):\n");
				P ("%26c Max End-End TLP Prefixes (MEETP): %d\n", SP, F(22,23));
				P ("%26c End-End TLP Prefix Supported (EETPS): %s\n", SP, YN(21));
				P ("%26c Extended Fmt Field Supported (EFFS): %s\n", SP, YN(20));
				P ("%26c OBFF Supported (OBFFS): %d\n", SP, F(18,19));
				P ("%26c TPH Completer Supported (TPHCS): %s\n", SP, tph[F(12,13)]);
				P ("%26c Latency Tolerance Reporting Supported (LTRS): %s\n", SP, YN(11));
				P ("%26c 128-bit CAS Completer Supported (128CCS): %s\n", SP, YN(9));
				P ("%26c 64-bit AtomicOp Completer Supported (64AOCS): %s\n", SP, YN(8));
				P ("%26c 32-bit AtomicOp Completer Supported (32AOCS): %s\n", SP, YN(7));
				P ("%26c Completion Timeout Disable Supported (CTDS): %s\n", SP, YN(4));
				P ("%26c Completion Timeout Ranges Supported (CTRS): %d\n", SP, F(0,3));

	PH4 (40);	P ("PCI Express Device Control 2 (PXDC2):\n");
				P ("%26c OBFF Enable (OBFFE): %d\n", SP, F(13,14));
				P ("%26c Latency Tolerance Reporting Mechanism Enable (LTRME): %s\n", SP, YN(10));
				P ("%26c Completion Timeout Disable (CTD): %s\n", SP, YN(4));
				P ("%26c Completion Timeout Value (CTV): %d\n", SP, F(0,3));

}




#if 0

int nlib_pcibar_print (struct nvme_pcibar *regs)
{
	char *cc_shn[] = {
		"No notification; no effect",
		"Normal shutdown notification",
		"Abrupt shutdwon notification",
		"Reserved" };
	char *csts_shst[] = {
		"Normal operation",
		"Shutdown processing occurring",
		"Shutdown processing complete",
		"Reserved" };
	char *cmbsz_szu[] = {
		"4 KB", "64 KB", "1 MB", "16 MB", "256 MB", "4 GB", "64 GB" };


	P ("NVMe Controller Registers\n");
	P ("-------------------------\n");
	PH4 (regs, cap.d.l);	P ("Controller Capabilities (CAP)\n");
	PH4 (regs, cap.d.h);	
				   P ("  Memory Page Size Maximum (MPSMAX): %u bytes\n",
						pow2 (12 + NVME_CAP_MPSMAX(regs->cap.q)));
				S; P ("Memory Page Size Minimum (MPSMIN): %u bytes\n",
						pow2 (12 + NVME_CAP_MPSMIN(regs->cap.q)));
				S; P ("Command Sets Supported (CSS): %s\n",
						(NVME_CAP_CSS(regs->cap.q) & 1)? "NVM command set" : "");
				S; P ("NVM Subsystem Reset Supported (NSSRS): %s\n",
						NVME_CAP_NSSRS(regs->cap.q)? YES : NO);
				S; P ("Doorbell Stride (DSTRD): %u bytes\n",
						pow2 (2 + NVME_CAP_DSTRD(regs->cap.q)));
				S; P ("Timeout (TO): %u * 500 ms\n",
						NVME_CAP_TO(regs->cap.q));
				S; P ("Arbitration Mechanism Supported (AMS): %s\n", 
						(NVME_CAP_AMS(regs->cap.q) == 0)? "Round Robin" :
						(NVME_CAP_AMS(regs->cap.q) == 1)? "Weighted Round Robin with Urgent Priority Class" :
						(NVME_CAP_AMS(regs->cap.q) == 2)? "Vendor Specific" :
						"WRR + VS");
				S; P ("Contiguous Queues Required (CQR): %s\n",
						NVME_CAP_CQR(regs->cap.q)? YES : NO);
				S; P ("Maximum Queue Entries Supported (MQES): %u\n",
						NVME_CAP_MQES(regs->cap.q));
	PH4 (regs, vs);	P ("Version (VS): %u.%u\n", 
						NVME_VS_MJR(regs->vs), NVME_VS_MNR(regs->vs));
	PH4 (regs, intms);	P ("Interrupt Mask Set (IVMS): 0x%04x\n", 
						regs->intms);
	PH4 (regs, intmc);	P ("Interrupt Vector Mask Clear (IVMC): 0x%04x\n",
						regs->intmc);
	PH4 (regs, cc);	P ("Controller Configuration (CC):\n");
				S; P ("I/O Completion Queue Entry Size (IOCQES): %u bytes\n",
						pow2 (NVME_CC_IOCQES(regs->cc)));
				S; P ("I/O Submission Queue Entry Size (IOSQES): %u bytes\n",
						pow2 (NVME_CC_IOSQES(regs->cc)));
				S; P ("Shutdown Notification (SHN): %s\n",
						cc_shn[NVME_CC_SHN(regs->cc)]);
				S; P ("Arbitration Mechanism Selected (AMS): %s\n",
						NVME_CC_AMS(regs->cc) == 0? "Round Robin" :
						NVME_CC_AMS(regs->cc) == 1? "Weighted Round Robin with Urgent Priority Class" :
						NVME_CC_AMS(regs->cc) == 7? "Vendor Specific" :
						"Reserved");
				S; P ("Memory Page Size (MPS): %u bytes\n",
						pow2 (12 + NVME_CC_MPS(regs->cc)));
				S; P ("I/O Command Set Selected (CSS): %s\n",
						NVME_CC_CSS(regs->cc) == 0? "NVM Command Set" :
						"Reserved");
				S; P ("Enable (EN): %s\n", 
						NVME_CC_EN(regs->cc)? YES : NO);
	PH4 (regs, csts);	P ("Controller Status (CSTS):\n");	
				S; P ("Processing Paused (PP): %s\n",
						NVME_CSTS_PP(regs->csts)? YES : NO);
				S; P ("NVM Subsystem Reset Occurred (NSSRO): %s\n",
						NVME_CSTS_NSSRO(regs->csts)? YES : NO);
				S; P ("Shutdown status (SHST): %s\n",
						csts_shst[NVME_CSTS_SHST(regs->csts)]);
				S; P ("Controller Fatal Status (CFS): %s\n",
						NVME_CSTS_CFS(regs->csts)? YES : NO);
				S; P ("Ready (RDY): %s\n",
						NVME_CSTS_RDY(regs->csts)? YES : NO);
	PH4 (regs, nssr);	P ("NVM Subsystem Reset (NSSR)\n");
	PH4 (regs, aqa);		P ("Admin Queue Attributes (AQA):\n");
				S; P ("Admin Completion Queue Size (ACQS): %u entries\n",
						NVME_AQA_ACQS(regs->aqa));
				S; P ("Admin Submission Queue Size (ASQS): %u entries\n",
						NVME_AQA_ASQS(regs->aqa));
	PH4 (regs, asq.d.l);	P ("Admin Submission Queue Base (ASQB): 0x%08llx\n",
						regs->asq.q);
	PH4 (regs, asq.d.h);	P ("\n");
	PH4 (regs, acq.d.l);	P ("Admin Completion Queue Base (ACQB): 0x%08llx\n",
						regs->acq.q);
	PH4 (regs, acq.d.h);	P ("\n");
	PH4 (regs, cmbloc);	P ("Controller Memory Buffer Location (CMBLOC):\n");
				S; P ("Base Indicator Register (BIR): %u\n",
						NVME_CMBLOC_BIR(regs->cmbloc));
				S; P ("Offset (OFST): %u (in CMBSZ)\n",
						NVME_CMBLOC_OFST(regs->cmbloc));
	PH4 (regs, cmbsz);	P ("Controller Memory Buffer Size (CMBSZ):\n");
				S; P ("Size (SZ): %u (in SZU)\n", 
						NVME_CMBSZ_SZ(regs->cmbsz));
				S; P ("Size Units (SZU): %s\n", (NVME_CMBSZ_SZU(regs->cmbsz) < 7)?
						cmbsz_szu[NVME_CMBSZ_SZU(regs->cmbsz)] : "Reserved");
				S; P ("Write Data Support (WDS): %s\n",
						NVME_CMBSZ_WDS(regs->cmbsz)? YES : NO);
				S; P ("Read Data Support (RDS): %s\n",
						NVME_CMBSZ_RDS(regs->cmbsz)? YES : NO);
				S; P ("PRP/SGL List Support (LISTS): PRP/SGL Lists in %s\n",
						NVME_CMBSZ_LISTS(regs->cmbsz)? "Controller Memory Buffer":
						"Host Memory");
				S; P ("Completion Queue Support (CQS): CQs in %s\n",
						NVME_CMBSZ_CQS(regs->cmbsz)? "Controller Memory Buffer" :
						"Host Memory");
				S; P ("Submission Queue Support (SQS): SQs in %s\n",
						NVME_CMBSZ_SQS(regs->cmbsz)? "Controller Memory Buffer" :
						"Host Memory");

}
#endif
