#include "sample_comm_nnie.h"
#include "sample_comm_svp.h"
#include "mpi_nnie.h"
#include "mpi_sys.h"


HI_S32 SAMPLE_COMM_SVP_NNIE_LoadModel(HI_CHAR * pszModelFile,
    SAMPLE_SVP_NNIE_MODEL_S *pstNnieModel)
{
	HI_S32 s32Ret = HI_INVALID_VALUE;
	HI_U64 u64PhyAddr = 0;
	HI_U8 *pu8VirAddr = NULL;
	HI_SL slFileSize = 0;

	/*Get model file size*/
	FILE *fp=fopen(pszModelFile,"rb");
	SAMPLE_SVP_CHECK_EXPR_RET(NULL == fp,s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR, "Error, open model file failed!\n");
    s32Ret = fseek(fp,0L,SEEK_END);
	SAMPLE_SVP_CHECK_EXPR_GOTO(-1 == s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,"Error, fseek failed!\n");
    slFileSize = ftell(fp);
	SAMPLE_SVP_CHECK_EXPR_GOTO(slFileSize <= 0, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,"Error, ftell failed!\n");
	s32Ret = fseek(fp,0L,SEEK_SET);
	SAMPLE_SVP_CHECK_EXPR_GOTO(-1 == s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,"Error, fseek failed!\n");

	/*malloc model file mem*/
	s32Ret = SAMPLE_COMM_SVP_MallocMem("SAMPLE_NNIE_MODEL", NULL, (HI_U64*)&u64PhyAddr, (void**)&pu8VirAddr, slFileSize);
	SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret,FAIL_0,SAMPLE_SVP_ERR_LEVEL_ERROR,
                "Error(%#x),Malloc memory failed!\n",s32Ret);

	pstNnieModel->stModelBuf.u32Size = (HI_U32)slFileSize;
	pstNnieModel->stModelBuf.u64PhyAddr = u64PhyAddr;
	pstNnieModel->stModelBuf.u64VirAddr = (HI_U64)(HI_UL)pu8VirAddr;

    s32Ret = fread(pu8VirAddr,slFileSize,1,fp);
	SAMPLE_SVP_CHECK_EXPR_GOTO(1 != s32Ret,FAIL_1,SAMPLE_SVP_ERR_LEVEL_ERROR,
		"Error,read model file failed!\n");

	/*load model*/
	s32Ret = HI_MPI_SVP_NNIE_LoadModel(&pstNnieModel->stModelBuf, &pstNnieModel->stModel);
	SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret,FAIL_1,SAMPLE_SVP_ERR_LEVEL_ERROR,
		"Error,HI_MPI_SVP_NNIE_LoadModel failed!\n");

	fclose(fp);
    return HI_SUCCESS;

FAIL_1:
    SAMPLE_SVP_MMZ_FREE(pstNnieModel->stModelBuf.u64PhyAddr,pstNnieModel->stModelBuf.u64VirAddr);
    pstNnieModel->stModelBuf.u32Size  = 0;

FAIL_0:
    if (NULL != fp)
    {
        fclose(fp);
    }

	return HI_FAILURE;
}


static HI_S32 SAMPLE_SVP_NNIE_FillForwardInfo(SAMPLE_SVP_NNIE_CFG_S *pstNnieCfg,
    SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam)
{
	HI_U32 i = 0, j = 0;
	HI_U32 u32Offset = 0;
	HI_U32 u32Num = 0;

	printf("net seg num:%d \n", pstNnieParam->pstModel->u32NetSegNum);

    // 整个网络切分的stage
	for(i = 0; i < pstNnieParam->pstModel->u32NetSegNum; i++)
	{
        /*fill forwardCtrl info*/
		if(SVP_NNIE_NET_TYPE_ROI == pstNnieParam->pstModel->astSeg[i].enNetType)
		{
			pstNnieParam->astForwardWithBboxCtrl[i].enNnieId = pstNnieCfg->aenNnieCoreId[i];
			pstNnieParam->astForwardWithBboxCtrl[i].u32SrcNum = pstNnieParam->pstModel->astSeg[i].u16SrcNum;
			pstNnieParam->astForwardWithBboxCtrl[i].u32DstNum = pstNnieParam->pstModel->astSeg[i].u16DstNum;
			pstNnieParam->astForwardWithBboxCtrl[i].u32ProposalNum = 1;
			pstNnieParam->astForwardWithBboxCtrl[i].u32NetSegId = i;
			pstNnieParam->astForwardWithBboxCtrl[i].stTmpBuf = pstNnieParam->stTmpBuf;
			pstNnieParam->astForwardWithBboxCtrl[i].stTskBuf.u64PhyAddr= pstNnieParam->stTaskBuf.u64PhyAddr+u32Offset;
			pstNnieParam->astForwardWithBboxCtrl[i].stTskBuf.u64VirAddr= pstNnieParam->stTaskBuf.u64VirAddr+u32Offset;
			pstNnieParam->astForwardWithBboxCtrl[i].stTskBuf.u32Size= pstNnieParam->au32TaskBufSize[i];
		}
		else if(SVP_NNIE_NET_TYPE_CNN == pstNnieParam->pstModel->astSeg[i].enNetType ||
            SVP_NNIE_NET_TYPE_RECURRENT== pstNnieParam->pstModel->astSeg[i].enNetType)
		{
            SAMPLE_SVP_TRACE_INFO("网络是cnn type!\n");
            SAMPLE_SVP_TRACE_INFO("输入blob num: %d \n", pstNnieParam->pstModel->astSeg[i].u16SrcNum);
            SAMPLE_SVP_TRACE_INFO("输出blob num: %d \n", pstNnieParam->pstModel->astSeg[i].u16DstNum);

			pstNnieParam->astForwardCtrl[i].enNnieId = pstNnieCfg->aenNnieCoreId[i];
			pstNnieParam->astForwardCtrl[i].u32SrcNum = pstNnieParam->pstModel->astSeg[i].u16SrcNum;        // yolov3:1，输入blob数量
			pstNnieParam->astForwardCtrl[i].u32DstNum = pstNnieParam->pstModel->astSeg[i].u16DstNum;        // yolov3:3，输出blob数量
			pstNnieParam->astForwardCtrl[i].u32NetSegId = i;
			pstNnieParam->astForwardCtrl[i].stTmpBuf = pstNnieParam->stTmpBuf;
			pstNnieParam->astForwardCtrl[i].stTskBuf.u64PhyAddr= pstNnieParam->stTaskBuf.u64PhyAddr+u32Offset;
			pstNnieParam->astForwardCtrl[i].stTskBuf.u64VirAddr= pstNnieParam->stTaskBuf.u64VirAddr+u32Offset;
			pstNnieParam->astForwardCtrl[i].stTskBuf.u32Size= pstNnieParam->au32TaskBufSize[i];
		}
		u32Offset += pstNnieParam->au32TaskBufSize[i];

		SAMPLE_SVP_TRACE_INFO("[fill forward info] tash buffer size:%d \n", pstNnieParam->au32TaskBufSize[i]);      // yolov3:0
		SAMPLE_SVP_TRACE_INFO("[fill forward info] task phy address:%lld \n", pstNnieParam->stTaskBuf.u64PhyAddr);  // yolov3:0
		SAMPLE_SVP_TRACE_INFO("[fill forward info] task vir address:%lld \n", pstNnieParam->stTaskBuf.u64VirAddr);


        /*fill src blob info, 输入blob信息*/
		for(j = 0; j < pstNnieParam->pstModel->astSeg[i].u16SrcNum; j++)
	    {
            /*Recurrent blob*/
            if(SVP_BLOB_TYPE_SEQ_S32 == pstNnieParam->pstModel->astSeg[i].astSrcNode[j].enType)
            {
                pstNnieParam->astSegData[i].astSrc[j].enType = pstNnieParam->pstModel->astSeg[i].astSrcNode[j].enType;
                pstNnieParam->astSegData[i].astSrc[j].unShape.stSeq.u32Dim = pstNnieParam->pstModel->astSeg[i].astSrcNode[j].unShape.u32Dim;
                pstNnieParam->astSegData[i].astSrc[j].u32Num = pstNnieCfg->u32MaxInputNum;
                pstNnieParam->astSegData[i].astSrc[j].unShape.stSeq.u64VirAddrStep = pstNnieCfg->au64StepVirAddr[i*SAMPLE_SVP_NNIE_EACH_SEG_STEP_ADDR_NUM];
            }
            else
            {
    		    pstNnieParam->astSegData[i].astSrc[j].enType = pstNnieParam->pstModel->astSeg[i].astSrcNode[j].enType;
    	        pstNnieParam->astSegData[i].astSrc[j].unShape.stWhc.u32Chn = pstNnieParam->pstModel->astSeg[i].astSrcNode[j].unShape.stWhc.u32Chn;
    	        pstNnieParam->astSegData[i].astSrc[j].unShape.stWhc.u32Height = pstNnieParam->pstModel->astSeg[i].astSrcNode[j].unShape.stWhc.u32Height;
    	        pstNnieParam->astSegData[i].astSrc[j].unShape.stWhc.u32Width = pstNnieParam->pstModel->astSeg[i].astSrcNode[j].unShape.stWhc.u32Width;
    	        pstNnieParam->astSegData[i].astSrc[j].u32Num = pstNnieCfg->u32MaxInputNum;
				SAMPLE_SVP_TRACE_INFO("输入blob的形状, c:%d, h:%d, w:%d \n",
                                    pstNnieParam->pstModel->astSeg[i].astSrcNode[j].unShape.stWhc.u32Chn,
                                    pstNnieParam->pstModel->astSeg[i].astSrcNode[j].unShape.stWhc.u32Height,
                                    pstNnieParam->pstModel->astSeg[i].astSrcNode[j].unShape.stWhc.u32Width);
                SAMPLE_SVP_TRACE_INFO("输入blob的类型: %d \n", pstNnieParam->pstModel->astSeg[i].astSrcNode[j].enType);
            }
	    }


        /*fill dst blob info*/
		if(SVP_NNIE_NET_TYPE_ROI == pstNnieParam->pstModel->astSeg[i].enNetType)
		{
			u32Num = pstNnieCfg->u32MaxRoiNum * pstNnieCfg->u32MaxInputNum;
		}
		else
		{
			u32Num = pstNnieCfg->u32MaxInputNum;        // 输入的batch
		}

		for(j = 0; j < pstNnieParam->pstModel->astSeg[i].u16DstNum; j++)
		{
            if(SVP_BLOB_TYPE_SEQ_S32 == pstNnieParam->pstModel->astSeg[i].astDstNode[j].enType)
            {
    			pstNnieParam->astSegData[i].astDst[j].enType = pstNnieParam->pstModel->astSeg[i].astDstNode[j].enType;
    			pstNnieParam->astSegData[i].astDst[j].unShape.stSeq.u32Dim = pstNnieParam->pstModel->astSeg[i].astDstNode[j].unShape.u32Dim;
                pstNnieParam->astSegData[i].astDst[j].u32Num = u32Num;
                pstNnieParam->astSegData[i].astDst[j].unShape.stSeq.u64VirAddrStep =pstNnieCfg->au64StepVirAddr[i*SAMPLE_SVP_NNIE_EACH_SEG_STEP_ADDR_NUM+1];
            }
            else
            {
    		    pstNnieParam->astSegData[i].astDst[j].enType = pstNnieParam->pstModel->astSeg[i].astDstNode[j].enType;
    		    pstNnieParam->astSegData[i].astDst[j].unShape.stWhc.u32Chn = pstNnieParam->pstModel->astSeg[i].astDstNode[j].unShape.stWhc.u32Chn;
    		    pstNnieParam->astSegData[i].astDst[j].unShape.stWhc.u32Height = pstNnieParam->pstModel->astSeg[i].astDstNode[j].unShape.stWhc.u32Height;
    		    pstNnieParam->astSegData[i].astDst[j].unShape.stWhc.u32Width = pstNnieParam->pstModel->astSeg[i].astDstNode[j].unShape.stWhc.u32Width;
    		    pstNnieParam->astSegData[i].astDst[j].u32Num = u32Num;
				SAMPLE_SVP_TRACE_INFO("整体网络切分的:%d | 输出blob idx:%d, c:%d, h:%d, w:%d , 输出类型：%d \n", 
						i, 
						j, 
						pstNnieParam->pstModel->astSeg[i].astDstNode[j].unShape.stWhc.u32Chn,
						pstNnieParam->pstModel->astSeg[i].astDstNode[j].unShape.stWhc.u32Height,
						pstNnieParam->pstModel->astSeg[i].astDstNode[j].unShape.stWhc.u32Width,
                        pstNnieParam->pstModel->astSeg[i].astDstNode[j].enType);
            }
		}
	}
	return HI_SUCCESS;
}


/*****************************************************************************
*   Prototype    : SAMPLE_SVP_NNIE_GetBlobMemSize
*   Description  : Get blob mem size
*   Input        : SVP_NNIE_NODE_S astNnieNode[]   NNIE Node
*                  HI_U32          u32NodeNum      Node num
*                  HI_U32          astBlob[]       blob struct
*                  HI_U32          u32Align        stride align type
*                  HI_U32          *pu32TotalSize  Total size
*                  HI_U32          au32BlobSize[]  blob size
*
*
*
*
*   Output       :
*   Return Value : VOID
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
static void SAMPLE_SVP_NNIE_GetBlobMemSize(SVP_NNIE_NODE_S astNnieNode[], HI_U32 u32NodeNum,
	HI_U32 u32TotalStep,SVP_BLOB_S astBlob[], HI_U32 u32Align, HI_U32* pu32TotalSize,HI_U32 au32BlobSize[])
{
	HI_U32 i = 0;
	HI_U32 u32Size = 0;
	HI_U32 u32Stride = 0;

    SAMPLE_SVP_TRACE_INFO("求 %d 个 blob 的buffer \n", u32NodeNum);

    //// yolov3,输入是uint8，输出是int32
	for(i = 0; i < u32NodeNum; i++)
	{
		SAMPLE_SVP_TRACE_INFO("blob type:%d \n", astNnieNode[i].enType);
		if(SVP_BLOB_TYPE_S32== astNnieNode[i].enType || SVP_BLOB_TYPE_VEC_S32== astNnieNode[i].enType ||
            SVP_BLOB_TYPE_SEQ_S32== astNnieNode[i].enType)
		{
			u32Size = sizeof(HI_U32);
		}
		else
		{
			u32Size = sizeof(HI_U8);
		}

        if(SVP_BLOB_TYPE_SEQ_S32 == astNnieNode[i].enType)      // 一般不是这种数据排列
        {
            SAMPLE_SVP_TRACE_INFO("一般不是这种数据排布 \n");
            if(SAMPLE_SVP_NNIE_ALIGN_16 == u32Align)
    		{
    			u32Stride = SAMPLE_SVP_NNIE_ALIGN16(astNnieNode[i].unShape.u32Dim*u32Size);
    		}
    		else
    		{
    			u32Stride = SAMPLE_SVP_NNIE_ALIGN32(astNnieNode[i].unShape.u32Dim*u32Size);
    		}
            au32BlobSize[i] = u32TotalStep*u32Stride;
        }
        else
        {
            if(SAMPLE_SVP_NNIE_ALIGN_16 == u32Align)
    		{
                SAMPLE_SVP_TRACE_INFO("通常是w方向的16对齐 \n");
    			u32Stride = SAMPLE_SVP_NNIE_ALIGN16(astNnieNode[i].unShape.stWhc.u32Width * u32Size);
    		}
    		else
    		{
    			u32Stride = SAMPLE_SVP_NNIE_ALIGN32(astNnieNode[i].unShape.stWhc.u32Width * u32Size);
    		}
    		au32BlobSize[i] = astBlob[i].u32Num * u32Stride * astNnieNode[i].unShape.stWhc.u32Height *
    			astNnieNode[i].unShape.stWhc.u32Chn;

			SAMPLE_SVP_TRACE_INFO("字节数:%d | u32num:%d | stride:%d | height:%d | channel:%d | blob size:%d \n", u32Size, astBlob[i].u32Num, 
					u32Stride, 
					astNnieNode[i].unShape.stWhc.u32Height, 
					astNnieNode[i].unShape.stWhc.u32Chn,
					au32BlobSize[i]);
        }
		*pu32TotalSize += au32BlobSize[i];
	    astBlob[i].u32Stride = u32Stride;
	}
}


/*****************************************************************************
*   Prototype    : SAMPLE_SVP_NNIE_GetTaskAndBlobBufSize
*   Description  : Get taskinfo and blob memory size
*   Input        : SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam     NNIE parameter
* 	                HI_U32                  *pu32TaskInfoSize Task info size
*                  HI_U32                  *pu32TmpBufSize    Tmp buffer size
*                  SAMPLE_SVP_NNIE_BLOB_SIZE_S  astBlobSize[] each seg input and output blob mem size
*                  HI_U32                  *pu32TotalSize     Total mem size
*
*
*   Output       :
*   Return Value : HI_S32,HI_SUCCESS:Success,Other:failure
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
static HI_S32 SAMPLE_SVP_NNIE_GetTaskAndBlobBufSize(SAMPLE_SVP_NNIE_CFG_S *pstNnieCfg,
    SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam,HI_U32*pu32TotalTaskBufSize, HI_U32*pu32TmpBufSize,
    SAMPLE_SVP_NNIE_BLOB_SIZE_S astBlobSize[], HI_U32*pu32TotalSize)
{
	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 i = 0, j = 0;
    HI_U32 u32TotalStep = 0;

	/*Get each seg's task buf size*/
	printf("before | task 0, tashbufsize:%d \n", pstNnieParam->au32TaskBufSize[0]);
	printf("before | temp buffer size:%d \n", *pu32TmpBufSize);
	printf("before | input num:%d, roinum:%d, segnum:%d \n", pstNnieCfg->u32MaxInputNum, pstNnieCfg->u32MaxRoiNum, pstNnieParam->pstModel->u32NetSegNum);

    // 系统函数，传入模型buffer，获得tskbuffsize
	s32Ret = HI_MPI_SVP_NNIE_GetTskBufSize(pstNnieCfg->u32MaxInputNum, 
											pstNnieCfg->u32MaxRoiNum,
											pstNnieParam->pstModel, 
											pstNnieParam->au32TaskBufSize,          // 传出
											pstNnieParam->pstModel->u32NetSegNum);
	SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret,s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
                                        "Error,HI_MPI_SVP_NNIE_GetTaskSize failed!\n");

	// 所需要的内存 = task ctrl buffer + model temp buffer + input blob + out blob
    /*Get total task buf size*/
	*pu32TotalTaskBufSize = 0;
	for(i = 0; i < pstNnieParam->pstModel->u32NetSegNum; i++)
	{
		*pu32TotalTaskBufSize += pstNnieParam->au32TaskBufSize[i];
        SAMPLE_SVP_TRACE_INFO("网络stage: %d, task buff: %d \n", i, pstNnieParam->au32TaskBufSize[i]);
	}

	/*Get tmp buf size*/
	*pu32TmpBufSize = pstNnieParam->pstModel->u32TmpBufSize;			// 前向时的temp buffer，模型编译时生成的
	*pu32TotalSize += *pu32TotalTaskBufSize + *pu32TmpBufSize;			// + task buffer, 固定需要的

	SAMPLE_SVP_TRACE_INFO("模型编译指定的blob buffer size:%d \n", *pu32TmpBufSize);

	/*calculate Blob mem size*/
	for(i = 0; i < pstNnieParam->pstModel->u32NetSegNum; i++)
	{
        if(SVP_NNIE_NET_TYPE_RECURRENT == pstNnieParam->pstModel->astSeg[i].enNetType)
        {
            SAMPLE_SVP_TRACE_INFO("网络是rnn结构");
            for(j = 0; j < pstNnieParam->astSegData[i].astSrc[0].u32Num; j++)
            {
                u32TotalStep += *((HI_S32*)(HI_UL)pstNnieParam->astSegData[i].astSrc[0].unShape.stSeq.u64VirAddrStep+j);
            }
        }
		/*the first seg's Src Blob mem size, other seg's src blobs from the output blobs of
		those segs before it or from software output results*/
		if(i == 0)
		{
			SAMPLE_SVP_NNIE_GetBlobMemSize(&(pstNnieParam->pstModel->astSeg[i].astSrcNode[0]),
				pstNnieParam->pstModel->astSeg[i].u16SrcNum, u32TotalStep, &(pstNnieParam->astSegData[i].astSrc[0]),
				SAMPLE_SVP_NNIE_ALIGN_16, pu32TotalSize, &(astBlobSize[i].au32SrcSize[0]));		// + 输入tensor buffer size
		}

		/*Get each seg's Dst Blob mem size*/
		SAMPLE_SVP_NNIE_GetBlobMemSize(&(pstNnieParam->pstModel->astSeg[i].astDstNode[0]),
			pstNnieParam->pstModel->astSeg[i].u16DstNum, u32TotalStep, &(pstNnieParam->astSegData[i].astDst[0]),
			SAMPLE_SVP_NNIE_ALIGN_16, pu32TotalSize, &(astBlobSize[i].au32DstSize[0]));			// + 输出tensor buffer size
	}
	return s32Ret;
}


/*
*Malloc memory with cached
*/
HI_S32 SAMPLE_COMM_SVP_MallocCached(HI_CHAR *pszMmb, HI_CHAR *pszZone,HI_U64 *pu64PhyAddr, HI_VOID **ppvVirAddr, HI_U32 u32Size)
{
	HI_S32 s32Ret = HI_SUCCESS;
	s32Ret = HI_MPI_SYS_MmzAlloc_Cached(pu64PhyAddr, ppvVirAddr, pszMmb, pszZone, u32Size);

	return s32Ret;
}


/*
*Fulsh cached
*/
HI_S32 SAMPLE_COMM_SVP_FlushCache(HI_U64 u64PhyAddr, HI_VOID *pvVirAddr, HI_U32 u32Size)
{
	HI_S32 s32Ret = HI_SUCCESS;
	s32Ret = HI_MPI_SYS_MmzFlushCache(u64PhyAddr, pvVirAddr,u32Size);
	return s32Ret;
}


HI_S32 SAMPLE_SVP_NNIE_ParamInit(SAMPLE_SVP_NNIE_CFG_S *pstNnieCfg,
    SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam)
{
	HI_U32 i = 0, j = 0;
	HI_U32 u32TotalSize = 0;
	HI_U32 u32TotalTaskBufSize = 0;
	HI_U32 u32TmpBufSize = 0;
	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 u32Offset = 0;
	HI_U64 u64PhyAddr = 0;
	HI_U8 *pu8VirAddr = NULL;
	SAMPLE_SVP_NNIE_BLOB_SIZE_S astBlobSize[SVP_NNIE_MAX_NET_SEG_NUM] = {0};

	/*fill forward info*/
	s32Ret = SAMPLE_SVP_NNIE_FillForwardInfo(pstNnieCfg, pstNnieParam);
	SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret,s32Ret,SAMPLE_SVP_ERR_LEVEL_ERROR,
		"Error,SAMPLE_SVP_NNIE_FillForwardCtrl failed!\n");

	/*Get taskInfo and Blob mem size*/
	s32Ret = SAMPLE_SVP_NNIE_GetTaskAndBlobBufSize(pstNnieCfg, pstNnieParam, &u32TotalTaskBufSize, &u32TmpBufSize, astBlobSize, &u32TotalSize);
	SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret,s32Ret,SAMPLE_SVP_ERR_LEVEL_ERROR,
		"Error,SAMPLE_SVP_NNIE_GetTaskAndBlobBufSize failed!\n");

	/*Malloc mem*/
	s32Ret = SAMPLE_COMM_SVP_MallocCached("SAMPLE_NNIE_TASK", NULL, (HI_U64*)&u64PhyAddr, (void**)&pu8VirAddr, u32TotalSize);
	SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret,s32Ret,SAMPLE_SVP_ERR_LEVEL_ERROR,
		"Error,Malloc memory failed!\n");
	memset(pu8VirAddr, 0, u32TotalSize);
	SAMPLE_COMM_SVP_FlushCache(u64PhyAddr, (void*)pu8VirAddr, u32TotalSize);		// 刷新cache中的内容到内存，并使cashe里的内容无效

	SAMPLE_SVP_TRACE_INFO("申请总内存 phy address:%lld | vir address:%lld \n", u64PhyAddr, (HI_U64)(HI_UL)pu8VirAddr);
	SAMPLE_SVP_TRACE_INFO("申请的task内存大小:%d \n", u32TotalTaskBufSize);

	// @所需要的内存 = task ctrl buffer + model temp buffer + input blob + out blob
	/*fill taskinfo mem addr*/
	pstNnieParam->stTaskBuf.u32Size = u32TotalTaskBufSize;			// task, temp, input tensor, output tensor
	pstNnieParam->stTaskBuf.u64PhyAddr = u64PhyAddr;
	pstNnieParam->stTaskBuf.u64VirAddr = (HI_U64)(HI_UL)pu8VirAddr;

	/*fill Tmp mem addr*/
	pstNnieParam->stTmpBuf.u32Size = u32TmpBufSize;
	pstNnieParam->stTmpBuf.u64PhyAddr = u64PhyAddr + u32TotalTaskBufSize;
	pstNnieParam->stTmpBuf.u64VirAddr = (HI_U64)(HI_UL)pu8VirAddr + u32TotalTaskBufSize;

	/*fill forward ctrl addr*/
	for(i = 0; i < pstNnieParam->pstModel->u32NetSegNum; i++)
	{
		if(SVP_NNIE_NET_TYPE_ROI == pstNnieParam->pstModel->astSeg[i].enNetType)
		{
			pstNnieParam->astForwardWithBboxCtrl[i].stTmpBuf = pstNnieParam->stTmpBuf;
			pstNnieParam->astForwardWithBboxCtrl[i].stTskBuf.u64PhyAddr= pstNnieParam->stTaskBuf.u64PhyAddr+u32Offset;
			pstNnieParam->astForwardWithBboxCtrl[i].stTskBuf.u64VirAddr= pstNnieParam->stTaskBuf.u64VirAddr+u32Offset;
			pstNnieParam->astForwardWithBboxCtrl[i].stTskBuf.u32Size= pstNnieParam->au32TaskBufSize[i];
		}
		else if(SVP_NNIE_NET_TYPE_CNN == pstNnieParam->pstModel->astSeg[i].enNetType ||
            SVP_NNIE_NET_TYPE_RECURRENT == pstNnieParam->pstModel->astSeg[i].enNetType)
		{


			pstNnieParam->astForwardCtrl[i].stTmpBuf = pstNnieParam->stTmpBuf;
			pstNnieParam->astForwardCtrl[i].stTskBuf.u64PhyAddr= pstNnieParam->stTaskBuf.u64PhyAddr + u32Offset;
			pstNnieParam->astForwardCtrl[i].stTskBuf.u64VirAddr= pstNnieParam->stTaskBuf.u64VirAddr + u32Offset;
			pstNnieParam->astForwardCtrl[i].stTskBuf.u32Size= pstNnieParam->au32TaskBufSize[i];
		}
		u32Offset += pstNnieParam->au32TaskBufSize[i];
		SAMPLE_SVP_TRACE_INFO("task ctrl buffer size:%d \n ", pstNnieParam->au32TaskBufSize[i]);
	}

	/*fill each blob's mem addr*/
	u64PhyAddr = u64PhyAddr + u32TotalTaskBufSize + u32TmpBufSize;
	pu8VirAddr = pu8VirAddr + u32TotalTaskBufSize + u32TmpBufSize;
	for(i = 0; i < pstNnieParam->pstModel->u32NetSegNum; i++)
	{
		/*first seg has src blobs, other seg's src blobs from the output blobs of
		those segs before it or from software output results*/
		if(0 == i)
		{
			for(j = 0; j < pstNnieParam->pstModel->astSeg[i].u16SrcNum; j++)
			{
				if(j!=0)
				{
					u64PhyAddr += astBlobSize[i].au32SrcSize[j-1];
					pu8VirAddr += astBlobSize[i].au32SrcSize[j-1];
				}
				pstNnieParam->astSegData[i].astSrc[j].u64PhyAddr = u64PhyAddr;
				pstNnieParam->astSegData[i].astSrc[j].u64VirAddr = (HI_U64)(HI_UL)pu8VirAddr;

				SAMPLE_SVP_TRACE_INFO("输入blob idx: %d | phyaddr:%lld | viraddr:%p \n", j, u64PhyAddr, pu8VirAddr);
			}
			u64PhyAddr += astBlobSize[i].au32SrcSize[j-1];
			pu8VirAddr += astBlobSize[i].au32SrcSize[j-1];
		}

		/*fill the mem addrs of each seg's output blobs*/
		for(j = 0; j < pstNnieParam->pstModel->astSeg[i].u16DstNum; j++)
		{
			if(j!=0)
			{
				u64PhyAddr += astBlobSize[i].au32DstSize[j-1];
				pu8VirAddr += astBlobSize[i].au32DstSize[j-1];
			}
			pstNnieParam->astSegData[i].astDst[j].u64PhyAddr = u64PhyAddr;
			pstNnieParam->astSegData[i].astDst[j].u64VirAddr = (HI_U64)(HI_UL)pu8VirAddr;

			SAMPLE_SVP_TRACE_INFO("输出blob idx: %d | phyaddr:%lld | viraddr:%lld \n", j, u64PhyAddr, (HI_U64)(HI_UL)pu8VirAddr);
		}
		u64PhyAddr += astBlobSize[i].au32DstSize[j-1];
		pu8VirAddr += astBlobSize[i].au32DstSize[j-1];
	}

    pu8VirAddr = SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_U8, pstNnieParam->astSegData[0].astSrc[0].u64VirAddr);
    SAMPLE_SVP_TRACE_INFO("输入blob的地址: %p \n", pu8VirAddr);
    memset(pu8VirAddr, 0, 100);
    SAMPLE_SVP_TRACE_INFO("输入测试通过 \n");

	return s32Ret;
}



HI_S32 SAMPLE_COMM_SVP_NNIE_ParamDeinit(SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam)
{
	SAMPLE_SVP_CHECK_EXPR_RET(NULL == pstNnieParam,HI_INVALID_VALUE,SAMPLE_SVP_ERR_LEVEL_ERROR,
		"Error, pstNnieParam can't be NULL!\n");

	if(0!=pstNnieParam->stTaskBuf.u64PhyAddr && 0!=pstNnieParam->stTaskBuf.u64VirAddr)
	{
		SAMPLE_SVP_MMZ_FREE(pstNnieParam->stTaskBuf.u64PhyAddr,pstNnieParam->stTaskBuf.u64VirAddr);
		pstNnieParam->stTaskBuf.u64PhyAddr = 0;
		pstNnieParam->stTaskBuf.u64VirAddr = 0;
	}

    if(0!=pstNnieParam->stStepBuf.u64PhyAddr && 0!=pstNnieParam->stStepBuf.u64VirAddr)
	{
		SAMPLE_SVP_MMZ_FREE(pstNnieParam->stStepBuf.u64PhyAddr,pstNnieParam->stStepBuf.u64VirAddr);
		pstNnieParam->stStepBuf.u64PhyAddr = 0;
		pstNnieParam->stStepBuf.u64VirAddr = 0;
	}
	return HI_SUCCESS;
}
