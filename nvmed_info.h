#ifndef _NVMED_INFO_H
#define _NVMED_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#define NVMED_INFO_VERSION	"0.9"
#define NVME_SPEC_VERSION	"1.2.1"

struct nvmed_info_cmd {
	const char *cmd_name;					// subcommand name
	int len;								// the number of unique characters
	const char *cmd_help;					// help message
	int (*cmd_fn)(NVMED *nvmed, char **cmd_args);	// subfunction to handle the subcommand
};

struct pci_info {
	int fd;
	int len;
	int type;
	void *regs;
};

struct nvme_passthru_cmd {
	    __u8    opcode;
	    __u8    flags;
	    __u16   rsvd1;
	    __u32   nsid;
	    __u32   cdw2;
	    __u32   cdw3;
	    __u64   metadata;
	    __u64   addr;
	    __u32   metadata_len;
	    __u32   data_len;
	    __u32   cdw10;
	    __u32   cdw11;
	    __u32   cdw12;
	    __u32   cdw13;
	    __u32   cdw14;
	    __u32   cdw15;
	    __u32   timeout_ms;
	    __u32   result;
};

#define nvme_admin_cmd nvme_passthru_cmd
#define NVME_IOCTL_ADMIN_CMD	_IOWR('N', 0x41, struct nvme_admin_cmd)

//extern char *nvme_sc[];
extern int dev_fd;

#define pow2(x)		(1 << (x))

// CNS values for IDENTIFY command (Figure 86, p.96)
#define CNS_NAMESPACE	0
#define CNS_CONTROLLER	1

#define FEATURE_SEL_CURRENT     (0)
#define FEATURE_SEL_DEFAULT     (1 << 8)
#define FEATURE_SEL_SAVED       (2 << 8)
#define FEATURE_SEL_SUPPORTED   (3 << 8)

#define FEATURE_ARBITRATION                     (0x01)
#define FEATURE_POWER_MANAGEMENT                (0x02)
#define FEATURE_LBA_RANGE_TYPE                  (0x03)
#define FEATURE_TEMPERATURE_THRESHOLD           (0x04)
#define FEATURE_ERROR_RECOVERY                  (0x05)
#define FEATURE_VOLATILE_WRITE_CACHE            (0x06)
#define FEATURE_NUMBER_OF_QUEUES                (0x07)
#define FEATURE_INTERRUPT_COALESCING            (0x08)
#define FEATURE_INTERRUPT_VECTOR_CONFIG         (0x09)
#define FEATURE_WRITE_ATOMICITY_NORMAL          (0x0a)
#define FEATURE_ASYNC_EVENT_CONFIG              (0x0b)
#define FEATURE_AUTO_POWER_STATE_TRANSITION     (0x0c)
#define FEATURE_HOST_MEMORY_BUFFER              (0x0d)
#define FEATURE_KEEP_ALIVE_TIMER				(0x0f)
#define FEATURE_SW_PROGRESS_MARKER              (0x80)
#define FEATURE_HOST_IDENTIFIER                 (0x81)
#define FEATURE_RESERVATION_NOTI_MASK			(0x82)
#define FEATURE_RESERVATION_PERSISTENCE         (0x83)

#define LOG_ERROR_INFO                          (0x01)
#define LOG_SMART_INFO                          (0x02)
#define LOG_FIRMWARE_SLOT_INFO                  (0x03)
#define LOG_CHANGED_NAMESPACE_LIST              (0x04)
#define LOG_COMMAND_EFFECTS                     (0x05)

#define PCI_CAP_NEXT(id)	(((id) >> 8) & 0xff)
#define PCI_CAP_CID(id)		((id) & 0xff)

#define PCI_PMCAP_CID		(0x01)
#define PCI_MSICAP_CID		(0x05)
#define	PCI_MSIXCAP_CID		(0x11)
#define PCI_PXCAP_CID		(0x10)
#define PCI_AERCAP_CID		(0x0001)						// This has a 32-bit header

#define PCI_CAP_OFFSET		52

#define PCI_FILE_COPY		0
#define PCI_FILE_MMAP		1

// Admin command set (1.2 spec, p52)
// 
// 00:03 CDW0	Command Dword 0
//		 b00-07 Opc		Opcode
//		 b08-09 FUSE	Fused Operation
//				00b		Normal operation
//				01b		Fused operation, first command
//				10b		Fused operation, second command
//				11b		Reserved
//		 b10-13 Reserved
//		 b14-15 PSDT	PRP or SGL for Data Transfer
//				00b		PRPs are used
//				01b		SGLs are used
//				10b		SGLs are used
//				11b		Reserved
//		 b16-31 CID		Command Identifier
// 04:07 NSID	Namespace Identifier (CDW1)
// 08:15 Reserved (CDW2-3)
// 16:23 MPTR	Metadata Pointer (CDW4-5)
// 24:31 PRP1	PRP Entry 1 (CDW6-7)
// 32:39 PRP2	PRP Entry 2 (CDW8-9)
// 40:43 CDW10
// 44:47 CDW11
// 48:51 CDW12
// 52:55 CDW13
// 56:59 CDW14
// 60:63 CDW15



// print utilities
#define U8(x)			((__u8) *((__u8 *) &p[x]))
#define U16(x)			((__u16) *((__u16 *) &p[x]))
#define U32(x)			((__u32) *((__u32 *) &p[x]))
#define U64(x)			((__u64) *((__u64 *) &p[x]))


#define PH1(offset) \
	_v = U8(offset); printf ("%04d       %02x           ", offset, p[offset]);

#define PH2(offset) \
	_v = U16(offset); printf ("%04d:%04d  %02x %02x        ", offset, offset+1, p[offset], p[offset+1]);

#define PH3(offset) \
	_v = U32(offset) & 0x00ffffff; \
	printf ("%04d:%04d  %02x %02x %02x     ", offset, offset+2, p[offset], p[offset+1], p[offset+2]);

#define PH4(offset) \
	_v = U32(offset); printf ("%04d:%04d  %02x %02x %02x %02x  ", offset, offset+3,  \
			p[offset], p[offset+1], p[offset+2], p[offset+3]);

#define F(start,end)	(__u32) ((end-start==31)? (_v) : (((_v) >> (start)) & ((1 << (end - start + 1)) - 1)))
#define YN(start)		(F(start,start)? "Yes" : "No")

#if 0
#define PF4(id) \
	printf ("    %02x     %08x     ", id, res);
#endif

#if 0
#define ISSET_BIT0(x)	((p[x]) & (__u8) 0x01)
#define ISSET_BIT1(x)	((p[x]) & (__u8) 0x02)
#define ISSET_BIT2(x)	((p[x]) & (__u8) 0x04)
#define ISSET_BIT3(x)	((p[x]) & (__u8) 0x08)
#define ISSET_BIT4(x)	((p[x]) & (__u8) 0x10)
#define ISSET_BIT5(x)	((p[x]) & (__u8) 0x20)
#define ISSET_BIT6(x)	((p[x]) & (__u8) 0x40)
#define ISSET_BIT7(x)	((p[x]) & (__u8) 0x80)

#define YN_BIT0(x)		(ISSET_BIT0(x)? "Yes" : "No")
#define YN_BIT1(x)		(ISSET_BIT1(x)? "Yes" : "No")
#define YN_BIT2(x)		(ISSET_BIT2(x)? "Yes" : "No")
#define YN_BIT3(x)		(ISSET_BIT3(x)? "Yes" : "No")
#define YN_BIT4(x)		(ISSET_BIT4(x)? "Yes" : "No")
#define YN_BIT5(x)		(ISSET_BIT5(x)? "Yes" : "No")
#define YN_BIT6(x)		(ISSET_BIT6(x)? "Yes" : "No")
#define YN_BIT7(x)		(ISSET_BIT7(x)? "Yes" : "No")
#endif


#define P	printf
#define SP	' '
#define S	printf("%26c", ' ')
#define PRINT_NVMED_INFO	printf("nvmed_info version " NVMED_INFO_VERSION \
								" (Compliant to NVMe Spec. " NVME_SPEC_VERSION ")\n\n")

enum print_format { FORMAT_STRING, FORMAT_ID, FORMAT_VALUE };

#define PS(offset, end, title)	print_something(FORMAT_STRING, p, offset, end, title, NULL);
#define PI(offset, end, title)	print_something(FORMAT_ID, p, offset, end, title, NULL);
#define PV(offset, end, title, unit)	print_something(FORMAT_VALUE, p, offset, end, title, unit);



// Function prototypes
extern struct nvmed_info_cmd *cmd_lookup (struct nvmed_info_cmd *list, char *str);
extern int cmd_help (char *invalid_cmd, char *cmd_name, struct nvmed_info_cmd *c);
extern int nvmed_info_admin_command (NVMED *nvmed, struct nvme_admin_cmd *cmd);
extern int nvmed_info_usage (char *arg0, char *invalid_cmd);
extern int nvmed_info_all (NVMED *nvmed, char **cmd_args);
extern int nvmed_info_identify (NVMED *nvmed, char **cmd_args);
extern int nvmed_info_identify_help (char *s);
extern int nvmed_info_identify_controller (NVMED *nvmed, char **cmd_args);
extern int nvmed_info_identify_namespace (NVMED *nvmed, char **cmd_args);
extern int nvmed_info_identify_issue (NVMED *nvmed, int cns, int nsid, __u8 *p);
extern void nvmed_info_identify_parse_controller (__u8 *p);
extern void nvmed_info_identify_parse_namespace (__u8 *p, int nsid);
extern int nvmed_info_features (NVMED *nvmed, char **cmd_args);
extern int nvmed_info_features_help (char *s);
extern int nvmed_info_get_features_issue (NVMED *nvmed, int fid, int nsid, __u8 *p, int len, __u32 *result);
extern int nvmed_info_get_features (NVMED *nvmed, char **cmd_args);
extern void print_something (enum print_format format, __u8 *p, int offset, int len, char *title, char *unit);
extern int nvmed_info_logs (NVMED *nvmed, char **cmd_args);
extern int nvmed_info_logs_help (char *s);
extern int nvmed_info_get_logs_issue (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 *result);
extern int nvmed_info_get_logs (NVMED *nvmed, char **cmd_args);
extern int nvmed_info_logs_error (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 result);
extern int nvmed_info_logs_smart (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 result);
extern int nvmed_info_logs_firmware (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 result);
extern int nvmed_info_logs_namespace (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 result);
extern int nvmed_info_logs_command (NVMED *nvmed, int logid, int nsid, __u8 *p, int len, __u32 result);
extern int nvmed_info_pci (NVMED *nvmed, char **cmd_args);
extern int nvmed_info_pci_config (NVMED *nvmed, char **cmd_args);
extern int nvmed_info_pci_nvme (NVMED *nvmed, char **cmd_args);
extern int nvmed_info_pci_help (char *s);
extern int nvmed_info_pci_open (NVMED *nvmed, char *name, int type, struct pci_info *pci);
extern int nvmed_info_pci_close (struct pci_info *pci);
extern void nvmed_info_pci_parse_config (NVMED *nvmed, struct pci_info *pci);
extern void nvmed_info_pci_parse_caps (NVMED *nvmed, struct pci_info *pci);
extern void nvmed_info_pci_parse_pmcap (NVMED *nvmed, struct pci_info *pci, int offset);
extern void nvmed_info_pci_parse_msicap (NVMED *nvmed, struct pci_info *pci, int offset);
extern void nvmed_info_pci_parse_msixcap (NVMED *nvmed, struct pci_info *pci, int offset);
extern void nvmed_info_pci_parse_pxcap (NVMED *nvmed, struct pci_info *pci, int offset);
extern void nvmed_info_pci_parse_nvme (NVMED *nvmed, struct pci_info *pci);
extern void print_bytes (__u8 *p, int len);

#endif /* _NVMED_INFO_H */
