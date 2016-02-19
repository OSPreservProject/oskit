/* ATI Mach64 Driver for SVGALIB - (Driver (C) 1996 Asad F. Hanif)
   Ye olde defines
*/


/* Mach64 io and memory mapped registers */
/* ripped from ati sdk */
/* NON-GUI IO MAPPED Registers */

#define ioCRTC_H_TOTAL_DISP     0x02EC
#define ioCRTC_H_SYNC_STRT_WID  0x06EC
#define ioCRTC_V_TOTAL_DISP     0x0AEC
#define ioCRTC_V_SYNC_STRT_WID  0x0EEC
#define ioCRTC_VLINE_CRNT_VLINE 0x12EC
#define ioCRTC_OFF_PITCH        0x16EC
#define ioCRTC_INT_CNTL         0x1AEC
#define ioCRTC_GEN_CNTL         0x1EEC

#define ioOVR_CLR               0x22EC
#define ioOVR_WID_LEFT_RIGHT    0x26EC
#define ioOVR_WID_TOP_BOTTOM    0x2AEC

#define ioCUR_CLR0              0x2EEC
#define ioCUR_CLR1              0x32EC
#define ioCUR_OFFSET            0x36EC
#define ioCUR_HORZ_VERT_POSN    0x3AEC
#define ioCUR_HORZ_VERT_OFF     0x3EEC

#define ioSCRATCH_REG0          0x42EC
#define ioSCRATCH_REG1          0x46EC

#define ioCLOCK_SEL_CNTL        0x4AEC

#define ioBUS_CNTL              0x4EEC

#define ioMEM_CNTL              0x52EC
#define ioMEM_VGA_WP_SEL        0x56EC
#define ioMEM_VGA_RP_SEL        0x5AEC

#define ioDAC_REGS              0x5EEC
#define ioDAC_CNTL              0x62EC

#define ioGEN_TEST_CNTL         0x66EC

#define ioCONFIG_CNTL           0x6AEC
#define ioCONFIG_CHIP_ID        0x6EEC
#define ioCONFIG_STAT0          0x72EC
#define ioCONFIG_STAT1          0x76EC


/* NON-GUI MEMORY MAPPED Registers - expressed in BYTE offsets */

#define mCRTC_H_TOTAL_DISP       0x0000  
#define mCRTC_H_SYNC_STRT_WID    0x0004  
#define mCRTC_V_TOTAL_DISP       0x0008  
#define mCRTC_V_SYNC_STRT_WID    0x000C  
#define mCRTC_VLINE_CRNT_VLINE   0x0010  
#define mCRTC_OFF_PITCH          0x0014  
#define mCRTC_INT_CNTL           0x0018  
#define mCRTC_GEN_CNTL           0x001C  

#define mOVR_CLR                 0x0040  
#define mOVR_WID_LEFT_RIGHT      0x0044 
#define mOVR_WID_TOP_BOTTOM      0x0048  

#define mCUR_CLR0                0x0060  
#define mCUR_CLR1                0x0064  
#define mCUR_OFFSET              0x0068  
#define mCUR_HORZ_VERT_POSN      0x006C  
#define mCUR_HORZ_VERT_OFF       0x0070  

#define mSCRATCH_REG0            0x0080  
#define mSCRATCH_REG1            0x0084  

#define mCLOCK_SEL_CNTL          0x0090  

#define mBUS_CNTL                0x00A0  

#define mMEM_CNTL                0x00B0 

#define mMEM_VGA_WP_SEL          0x00B4  
#define mMEM_VGA_RP_SEL          0x00B8  

#define mDAC_REGS                0x00C0  
#define mDAC_CNTL                0x00C4  

#define mGEN_TEST_CNTL           0x00D0  

#define mCONFIG_CHIP_ID          0x00E0  
#define mCONFIG_STAT0            0x00E4  
#define mCONFIG_STAT1            0x00E8  


/* GUI MEMORY MAPPED Registers */

#define mDST_OFF_PITCH           0x0100  
#define mDST_X                   0x0104  
#define mDST_Y                   0x0108  
#define mDST_Y_X                 0x010C  
#define mDST_WIDTH               0x0110  
#define mDST_HEIGHT              0x0114  
#define mDST_HEIGHT_WIDTH        0x0118  
#define mDST_X_WIDTH             0x011C  
#define mDST_BRES_LNTH           0x0120 
#define mDST_BRES_ERR            0x0124  
#define mDST_BRES_INC            0x0128  
#define mDST_BRES_DEC            0x012C  
#define mDST_CNTL                0x0130  

#define mSRC_OFF_PITCH           0x0180  
#define mSRC_X                   0x0184  
#define mSRC_Y                   0x0188  
#define mSRC_Y_X                 0x018C  
#define mSRC_WIDTH1              0x0190  
#define mSRC_HEIGHT1             0x0194  
#define mSRC_HEIGHT1_WIDTH1      0x0198  
#define mSRC_X_START             0x019C 
#define mSRC_Y_START             0x01A0  
#define mSRC_Y_X_START           0x01A4 
#define mSRC_WIDTH2              0x01A8  
#define mSRC_HEIGHT2             0x01AC  
#define mSRC_HEIGHT2_WIDTH2      0x01B0  
#define mSRC_CNTL                0x01B4  

#define mHOST_DATA0              0x0200  
#define mHOST_DATA1              0x0204 
#define mHOST_DATA2              0x0208  
#define mHOST_DATA3              0x020C  
#define mHOST_DATA4              0x0210  
#define mHOST_DATA5              0x0214  
#define mHOST_DATA6              0x0218  
#define mHOST_DATA7              0x021C  
#define mHOST_DATA8              0x0220  
#define mHOST_DATA9              0x0224 
#define mHOST_DATAA              0x0228  
#define mHOST_DATAB              0x022C  
#define mHOST_DATAC              0x0230  
#define mHOST_DATAD              0x0234 
#define mHOST_DATAE              0x0238 
#define mHOST_DATAF              0x023C  
#define mHOST_CNTL               0x0240 

#define mPAT_REG0                0x0280 
#define mPAT_REG1                0x0284 
#define mPAT_CNTL                0x0288 

#define mSC_LEFT                 0x02A0  
#define mSC_RIGHT                0x02A4  
#define mSC_LEFT_RIGHT           0x02A8  
#define mSC_TOP                  0x02AC 
#define mSC_BOTTOM               0x02B0 
#define mSC_TOP_BOTTOM           0x02B4 

#define mDP_BKGD_CLR             0x02C0  
#define mDP_FRGD_CLR             0x02C4  
#define mDP_WRITE_MASK           0x02C8 
#define mDP_CHAIN_MASK           0x02CC 
#define mDP_PIX_WIDTH            0x02D0  
#define mDP_MIX                  0x02D4 
#define mDP_SRC                  0x02D8  

#define mCLR_CMP_CLR             0x0300  
#define mCLR_CMP_MASK            0x0304 
#define mCLR_CMP_CNTL            0x0308

#define mFIFO_STAT               0x0310

#define mCONTEXT_MASK            0x0320
#define mCONTEXT_LOAD_CNTL       0x0324

#define mGUI_TRAJ_CNTL           0x0330  
#define mGUI_STAT                0x0338 

/*

Shortened register names 

*/

#define	IOBASE_ADDRESS			0x02ec
#define IOB				IOBASE_ADDRESS

#define	CRTC_H_TOTAL			( IOB + 0x0000 )
#define CRTC_H_DISP			( IOB + 0x0002 )

#define CRTC_H_SYNC_STRT		( IOB + 0x0400 )
#define CRTC_H_SYNC_DLY			( IOB + 0x0401 )
#define CRTC_H_SYNC_WID			( IOB + 0x0402 )

#define CRTC_V_TOTAL			( IOB + 0x0800 )
#define CRTC_V_DISP			( IOB + 0x0802 )

#define CRTC_V_SYNC_STRT		( IOB + 0x0c00 )
#define CRTC_V_SYNC_WID			( IOB + 0x0c02 )

#define CRTC_OFFSET			( IOB + 0x1400 )
#define CRTC_PITCH 			( IOB + 0x1402 )

#define CRTC_INT_CNTL			( IOB + 0x1800 )

#define CRTC_GEN_CNTL			( IOB + 0x1c00 )
#define CRTC_DBL_SCAN_EN		0x01
#define CRTC_INTERLACE_EN		0x02
#define CRTC_HSYNC_DIS			0x04
#define CRTC_VSYNC_DIS			0x08
#define CRTC_CSYNC_EN			0x10
#define CRTC_PIX_BY_2_EN		0x20
#define CRTC_BLANK			0x40

#define CRTC_PIX_WIDTH			( IOB + 0x1c01 )
#define PIX_WIDTH8			0x02

#define CRTC_FIFO			( IOB + 0x1c02 )

#define CRTC_EXT_DISP			( IOB + 0x1c03 )
#define CRTC_EXT_DISP_EN		0x01
#define CRTC_EXT_EN			0x02

#define OVR_CLR_8			( IOB + 0x2000 )
#define OVR_CLR_B			( IOB + 0x2001 )
#define OVR_CLR_G			( IOB + 0x2002 )
#define OVR_CLR_R			( IOB + 0x2003 )

#define OVR_WID_LEFT			( IOB + 0x2400 )
#define OVR_WID_RIGHT			( IOB + 0x2402 )
#define OVR_WID_TOP			( IOB + 0x2800 )
#define OVR_WID_BOTTOM			( IOB + 0x2802 )

#define SCRATCH_REG0			( IOB + 0x4000 )
#define TEST_MEM_ADDR			( IOB + 0x4000 )
#define SCRATCH_REG1			( IOB + 0x4400 )
#define TEST_MEM_DATA			( IOB + 0x4400 )

#define CLOCK_CNTL			( IOB + 0x4800 )
#define FS2_bit				0x04
#define FS3_bit				0x08
#define CLOCK_SEL			0x0f
#define CLOCK_DIV			0x30
#define CLOCK_DIV1			0x00
#define CLOCK_DIV2			0x10
#define CLOCK_DIV4			0x20
#define CLOCK_STROBE			0x40
#define CLOCK_SERIAL_DATA		0x80

#define BUS_CNTL1_16			( IOB + 0x4c00 )

#define BUS_CNTL2_16			( IOB + 0x4c02 )	

#define MEM_CNTL8			( IOB + 0x5000 )

#define MEM_VGA_WP_SEL			( IOB + 0x5400 )
#define MEM_VGA_RP_SEL			( IOB + 0x5800 )

#define MEM_BNDRY			( IOB + 0x5002 )
#define MEM_BNDRY_256K			0x01
#define MEM_BNDRY_512K			0x02
#define MEM_BNDRY_1M			0x03
#define MEM_BNDRY_EN			0x04

#define DAC_W_INDEX			( IOB + 0x5c00 )
#define DAC_DATA			( IOB + 0x5c01 )
#define DAC_MASK			( IOB + 0x5c02 )
#define DAC_R_INDEX			( IOB + 0x5c03 )
#define DAC8_W_INDEX			( DAC_W_INDEX & 0xff )
#define DAC8_DATA			( DAC_DATA & 0xff )
#define DAC8_MASK			( DAC_MASK & 0xff )
#define DAC8_R_INDEX			( DAC_R_INDEX & 0xff )

#define DAC_CNTL1			( IOB + 0x6000 )
#define DAC_EXT_SEL_RS2			0x01
#define DAC_EXT_SEL_RS3			0x02
#define DAC_CNTL2			( IOB + 0x6001 )
#define DAC_8BIT_EN			0x01
#define DAC_PIX_DLY_MASK		0x06
#define DAC_PIX_DLY_0NS			0x00
#define DAC_PIX_DLY_2NS			0x02
#define DAC_PIX_DLY_4NS			0x04
#define DAC_BLANK_ADJ_MASK		0x18
#define DAC_BLANK_ADJ_0			0x00
#define DAC_BLANK_ADJ_1			0x08
#define DAC_BLANK_ADJ_2			0x10

#define DAC_TYPE_			( IOB + 0x6002 )

#define GEN_TEST_CNTL			( IOB + 0x6400 )

#define CONFIG_CNTL			( IOB + 0x6800 )

#define CONFIG_CHIP_ID			( IOB + 0x6c00 )

#define CONFIG_STAT			( IOB + 0x7000 )

#define GEN_TEST_CNTL			( IOB + 0x6400 )

#define GEN_TEST_CNTL2			( IOB + 0x6402 )

#define PIXEL_DELAY_ON			0x0800

#define PITCH_INFO			(SCRATCH_REG1 + 2)
#define DAC_VGA_MODE			0x80
#define SET_VGA_MODE			0x02
#define PITCH_INFO_PSIZE		0xc0
#define PITCH_INFO_DAC			0x10
#define PITCH_INFO_MUX2			0x20
#define PITCH_INFO_TIOCLK		0x08
#define PITCH_INFO_DAC0			0x01

#define	VGA_MODE			1
#define ACCELERATOR_MODE		0

#define DOTCLK_TOLERANCE		25

#define REPORT
#define DEBUG
#define BAILOUT

#define TRUE		1
#define FALSE		0

#define	ATIPORT		0x1ce
#define	ATIOFF		0x80
#define	ATISEL(reg)	(ATIOFF+reg)

#define M64_BIOS_SIZE	0x8000
#define M64_CLOCK_MAX	0x0f

#define BPP_4		1
#define BPP_8		2
#define BPP_15		3
#define BPP_16		4
#define BPP_24		5
#define BPP_32		6

#define VRAM_MASK	0x66

#define C_ISC2595	1
#define C_STG1703	2
#define C_CH8398	3
#define C_BEDROCK	4
#define C_ATT20C408	5
#define C_RGB514	6

#define D_ATI68860	5

#define	SAY(x)	fprintf(stderr,#x"\n");fflush(stderr);
#define DUMP(x)	for (DUMPINDEX=0;DUMPINDEX<32;DUMPINDEX++) \
			DUMPOUT[31-DUMPINDEX]=(((x)>>DUMPINDEX)&0x1)? \
				'1':'0'; \
		DUMPOUT[32]='\0'; \
		printf("%s\n",DUMPOUT);

typedef unsigned char UB;
typedef unsigned short US;
typedef unsigned long UL;

typedef struct {
	UB h_disp;		/* horizontal res in chars */
	UB dacmask;		/* supported dacs */
	UB ram_req;		/* ram required */
	UB max_dot_clk;		/* max dot clock */
	UB color_depth;		/* mac color depth */
	UB FILLER;		/* make even #bytes */
} M64_MaxColorDepth;

typedef struct {
	UB ClockType;		/* clock type */
	US MinFreq;		/* min clock freq */
	US MaxFreq;		/* max clock freq */
	US RefFreq;		/* reference freq */
	US RefDivdr;		/* reference freq divider */
	US N_adj;		/* adjust */
	US Dram_Mem_Clk;
	US Vram_Mem_Clk;
	US Coproc_Mem_Clk;
	UB CX_Clk;
	UB VGA_Clk;
	UB Mem_Clk_Entry;
	UB SClk_Entry;
	US ClkFreqTable[M64_CLOCK_MAX*2];	/* clock frequency table */
	UB DefaultDivdr;
	UB DefaultFS2;
	UB CX_DMcycle;
	UB VGA_DMcycle;
	UB CX_VMcycle;
	UB VGA_VMcycle;
} M64_ClockInfo;

typedef struct {
	US Chip_Type;		/* ASIC Chip type */
	UB Chip_Class;		/* ASIC Chip class */
	UB Chip_Rev;		/* ASIC Chip Revision */
	UB Chip_ID_Index;	/* Name offset */
	UB Cfg_Bus_type;	/* Bus type */
	UB Cfg_Ram_type;	/* Ram type */
	UB Mem_Size;		/* Memory installed */
	UB Dac_Type;		/* Dac type */
	UB Dac_Subtype;		/* Dac sub type */
} M64_StartUpInfo;


static UB *M64_Chip_Name[]={"GX-C","GX-D","GX-E","GX-F","GX-?",
			"CX","CX-?",
			"CT","ET",
			"?"};

static UB *M64_Bus_Name[]={"ISA","EISA","","","","","VLB","PCI"};
static UB *M64_Dac_Name[]={"","TVP3020, RGB514","ATI68875","BT476/BT478",
			"BT481, ATT490, ATT491","ATI68860/ATI68880",
			"STG1700","SC15021, STF1702"};
static UB *M64_Ram_Name[]={"DRAM (256Kx4)","VRAM (256x4,x8,x16)",
			"VRAM (256Kx16 short shift register)",
			"DRAM (256Kx16)","Graphics DRAM (256Kx16)",
			"Enhanced VRAM (256Kx4,x8,x16)",
			"Enhanced VRAM (256Kx16 short shift register)",
			""};
static US M64_Mem_Size[]={512,1024,2048,4096,6144,8192,0,0};
static UB *M64_Clock_Name[]={"","ISC2595","STG1703","CH8398",
			"BEDROCK","ATT20C408","RGB514"};

static UB M64_BPP[]={-1,4,8,15,16,24,32,-1};

/* 

Some misc. structures 

*/

typedef struct {
		US	VRes;
		US	HRes;
		UB	max_cdepth;
		UB	req_cdepth;
		US	fifo_vl;

		US	pdot_clock;
		UB	refresh_mask;
		UB	vmode;
		US	misc;
		
		UB	h_total;
		UB	h_disp;
		UB	h_sync_strt;
		UB	h_sync_wid;

		US	v_total;
		US	v_disp;
		US	v_sync_strt;
		UB	v_sync_wid;

		UB	clock_cntl;
		US	dot_clock;
		
		UB	h_overscan_a;
		UB	h_overscan_b;

		UB	v_overscan_a;
		UB	v_overscan_b;

		UB	overscan_8;
		UB	overscan_B;
		UB	overscan_G;
		UB	overscan_R;
} CRTC_Table;

#define MAX_CRTC_TABLE	1

static CRTC_Table CRTC_tinfo[MAX_CRTC_TABLE]={
	/* 640x480 60 Hz NI */
	{	0,	0,	0,	0,	0,	
		0,	60,	0x12,	PIXEL_DELAY_ON,
/*		0x63,	0x4f,	0x52,	0x2c,
		0x020c,	0x01df,	0x01ea,	0x22,
*/		0x64,	0x50,	0x53,	0x02,
		0x20d,	0x1e0,	0x1eb,	0x02,
		0x00,	2518,
		0x00,	0x00,	0x00,	0x00,
		0x00,	0x00,	0x00,	0x00
	}
};

	
