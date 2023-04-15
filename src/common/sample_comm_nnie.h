#ifndef __SAMPLE_COMM_NNIE_H__
#define __SAMPLE_COMM_NNIE_H__

#include "hi_nnie.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


/*16Byte align*/
#define SAMPLE_SVP_NNIE_ALIGN_16 16
#define SAMPLE_SVP_NNIE_ALIGN16(u32Num) ((u32Num + SAMPLE_SVP_NNIE_ALIGN_16-1) / SAMPLE_SVP_NNIE_ALIGN_16*SAMPLE_SVP_NNIE_ALIGN_16)
/*32Byte align*/
#define SAMPLE_SVP_NNIE_ALIGN_32 32
#define SAMPLE_SVP_NNIE_ALIGN32(u32Num) ((u32Num + SAMPLE_SVP_NNIE_ALIGN_32-1) / SAMPLE_SVP_NNIE_ALIGN_32*SAMPLE_SVP_NNIE_ALIGN_32)

#define SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(Type,Addr) (Type*)(HI_UL)(Addr)
#define SAMPLE_SVP_COORDI_NUM                     4        /*num of coordinates*/
#define SAMPLE_SVP_PROPOSAL_WIDTH                 6        /*the width of each proposal array*/
#define SAMPLE_SVP_QUANT_BASE                     4096     /*the basic quantity*/
#define SAMPLE_SVP_NNIE_MAX_SOFTWARE_MEM_NUM      4
#define SAMPLE_SVP_NNIE_SSD_REPORT_NODE_NUM       12
#define SAMPLE_SVP_NNIE_SSD_PRIORBOX_NUM          6
#define SAMPLE_SVP_NNIE_SSD_SOFTMAX_NUM           6
#define SAMPLE_SVP_NNIE_SSD_ASPECT_RATIO_NUM      6
#define SAMPLE_SVP_NNIE_YOLOV1_WIDTH_GRID_NUM     7
#define SAMPLE_SVP_NNIE_YOLOV1_HEIGHT_GRID_NUM    7
#define SAMPLE_SVP_NNIE_EACH_SEG_STEP_ADDR_NUM    2
#define SAMPLE_SVP_NNIE_MAX_CLASS_NUM             30
#define SAMPLE_SVP_NNIE_MAX_ROI_NUM_OF_CLASS      50
#define SAMPLE_SVP_NNIE_REPORT_NAME_LENGTH        64

#define SAMPLE_SVP_NIE_PERF_STAT_DEF_VAR()
#define SAMPLE_SVP_NIE_PERF_STAT_DEF_FRM_VAR()
#define SAMPLE_SVP_NNIE_PERF_STAT_BEGIN_LOOP()
#define SAMPLE_SVP_NNIE_PERF_STAT_END_LOOP()
#define SAMPLE_SVP_NNIE_PERF_STAT_BEGIN()
#define SAMPLE_SVP_NNIE_PERF_STAT_END()
#define SAMPLE_SVP_NNIE_PERF_STAT_TIME_DIFF()

#define SAMPLE_SVP_NNIE_PERF_STAT_OP_FORWARD_CLREAR()
#define SAMPLE_SVP_NNIE_PERF_STAT_OP_FORWARD_SRC_FLUSH_TIME()
#define SAMPLE_SVP_NNIE_PERF_STAT_OP_FORWARD_PRE_DST_FLUSH_TIME()
#define SAMPLE_SVP_NNIE_PERF_STAT_OP_FORWARD_AFTER_DST_FLUSH_TIME()
#define SAMPLE_SVP_NNIE_PERF_STAT_OP_FORWARD_OP_TIME()


typedef struct hiSAMPLE_SVP_NNIE_MODEL_S
{
    SVP_NNIE_MODEL_S    stModel;
    SVP_MEM_INFO_S      stModelBuf;//store Model file
}SAMPLE_SVP_NNIE_MODEL_S;


typedef struct hiSAMPLE_SVP_NNIE_SEG_DATA_S
{
	SVP_SRC_BLOB_S astSrc[SVP_NNIE_MAX_INPUT_NUM];
	SVP_DST_BLOB_S astDst[SVP_NNIE_MAX_OUTPUT_NUM];
}SAMPLE_SVP_NNIE_SEG_DATA_S;


typedef struct hiSAMPLE_SVP_NNIE_PARAM_S
{
    SVP_NNIE_MODEL_S*    pstModel;
    HI_U32 u32TmpBufSize;
    HI_U32 au32TaskBufSize[SVP_NNIE_MAX_NET_SEG_NUM];
    SVP_MEM_INFO_S      stTaskBuf;
	SVP_MEM_INFO_S      stTmpBuf;
    SVP_MEM_INFO_S      stStepBuf;//store Lstm step info
    SAMPLE_SVP_NNIE_SEG_DATA_S astSegData[SVP_NNIE_MAX_NET_SEG_NUM];        // @each seg's input and output blob
    SVP_NNIE_FORWARD_CTRL_S astForwardCtrl[SVP_NNIE_MAX_NET_SEG_NUM];
	SVP_NNIE_FORWARD_WITHBBOX_CTRL_S astForwardWithBboxCtrl[SVP_NNIE_MAX_NET_SEG_NUM];
}SAMPLE_SVP_NNIE_PARAM_S;


typedef struct hiSAMPLE_SVP_NNIE_CFG_S
{
    HI_CHAR *pszPic;
    HI_U32 u32MaxInputNum;
    HI_U32 u32MaxRoiNum;
    HI_U64 au64StepVirAddr[SAMPLE_SVP_NNIE_EACH_SEG_STEP_ADDR_NUM*SVP_NNIE_MAX_NET_SEG_NUM];//virtual addr of LSTM's or RNN's step buffer
	SVP_NNIE_ID_E	aenNnieCoreId[SVP_NNIE_MAX_NET_SEG_NUM];
}SAMPLE_SVP_NNIE_CFG_S;


typedef struct hiSAMPLE_SVP_NNIE_YOLOV3_SOFTWARE_PARAM_S
{
    HI_U32 u32OriImHeight;
    HI_U32 u32OriImWidth;
    HI_U32 u32BboxNumEachGrid;
    HI_U32 u32ClassNum;
    HI_U32 au32GridNumHeight[3];
    HI_U32 au32GridNumWidth[3];
    HI_U32 u32NmsThresh;
    HI_U32 u32ConfThresh;
    HI_U32 u32MaxRoiNum;
    HI_FLOAT af32Bias[3][6];
    SVP_MEM_INFO_S stGetResultTmpBuf;
    SVP_DST_BLOB_S stClassRoiNum;
    SVP_DST_BLOB_S stDstRoi;
    SVP_DST_BLOB_S stDstScore;
}SAMPLE_SVP_NNIE_YOLOV3_SOFTWARE_PARAM_S;


typedef struct hiSAMPLE_SVP_NNIE_BLOB_SIZE_S
{
	HI_U32 au32SrcSize[SVP_NNIE_MAX_INPUT_NUM];
	HI_U32 au32DstSize[SVP_NNIE_MAX_OUTPUT_NUM];
}SAMPLE_SVP_NNIE_BLOB_SIZE_S;


typedef struct hiSAMPLE_SVP_NNIE_DATA_INDEX_S
{
	HI_U32 u32SegIdx;
	HI_U32 u32NodeIdx;
}SAMPLE_SVP_NNIE_DATA_INDEX_S;

/*this struct is used to indicate the input data from which seg's input or report node*/
typedef SAMPLE_SVP_NNIE_DATA_INDEX_S  SAMPLE_SVP_NNIE_INPUT_DATA_INDEX_S;
/*this struct is used to indicate which seg will be executed*/
typedef SAMPLE_SVP_NNIE_DATA_INDEX_S  SAMPLE_SVP_NNIE_PROCESS_SEG_INDEX_S;


HI_S32 SAMPLE_COMM_SVP_NNIE_LoadModel(HI_CHAR * pszModelFile, SAMPLE_SVP_NNIE_MODEL_S *pstNnieModel);

HI_S32 SAMPLE_SVP_NNIE_ParamInit(SAMPLE_SVP_NNIE_CFG_S *pstNnieCfg, SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam);

/*
*Fulsh cached
*/
HI_S32 SAMPLE_COMM_SVP_FlushCache(HI_U64 u64PhyAddr, HI_VOID *pvVirAddr, HI_U32 u32Size);

 /*****************************************************************************
 *   Prototype    : SAMPLE_COMM_SVP_NNIE_ParamDeinit
 *   Description  : Deinit NNIE parameters
 *   Input        : SAMPLE_SVP_NNIE_PARAM_S        *pstNnieParam     NNIE Parameter
 *                  SAMPLE_SVP_NNIE_SOFTWARE_MEM_S *pstSoftWareMem   software mem
 *
 *
 *
 *
 *   Output       :
 *   Return Value :  HI_S32,HI_SUCCESS:Success,Other:failure
 *   Spec         :
 *   Calls        :
 *   Called By    :
 *   History:
 *
 *       1.  Date         : 2017-11-20
 *           Author       :
 *           Modification : Create
 *
 *****************************************************************************/
HI_S32 SAMPLE_COMM_SVP_NNIE_ParamDeinit(SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam);


#endif      // __SAMPLE_COMM_NNIE_H__
