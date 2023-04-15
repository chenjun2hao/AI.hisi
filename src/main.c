#include <sys/time.h>
#include "hi_type.h"
#include "mpi_nnie.h"

#include "sample_comm_svp.h"
#include "sample_comm_nnie.h"

static SAMPLE_SVP_NNIE_MODEL_S centerfaceModel = {0};
static SAMPLE_SVP_NNIE_PARAM_S centerfaceNnieParam = {0};
static SAMPLE_SVP_NNIE_YOLOV3_SOFTWARE_PARAM_S s_stYolov3SoftwareParam = {0};


/******************************************************************************
* function : Yolov3 Deinit
******************************************************************************/
static HI_S32 SAMPLE_SVP_NNIE_Yolov3_Deinit(SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam,
    SAMPLE_SVP_NNIE_YOLOV3_SOFTWARE_PARAM_S* pstSoftWareParam,SAMPLE_SVP_NNIE_MODEL_S *pstNnieModel)
{
    HI_S32 s32Ret = HI_SUCCESS;
    /*hardware deinit*/
    if(pstNnieParam!=NULL)
    {
        s32Ret = SAMPLE_COMM_SVP_NNIE_ParamDeinit(pstNnieParam);
        SAMPLE_SVP_CHECK_EXPR_TRACE(HI_SUCCESS != s32Ret,SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error,SAMPLE_COMM_SVP_NNIE_ParamDeinit failed!\n");
    }

    return s32Ret;
}


static HI_S32 SAMPLE_SVP_NNIE_Yolov3_ParamInit(SAMPLE_SVP_NNIE_CFG_S* pstNnieCfg,
    SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam, SAMPLE_SVP_NNIE_YOLOV3_SOFTWARE_PARAM_S* pstSoftWareParam)
{
    HI_S32 s32Ret = HI_SUCCESS;

    /*check*/
    SAMPLE_SVP_CHECK_EXPR_RET((NULL == pstNnieCfg || NULL == pstNnieParam),HI_ERR_SVP_NNIE_ILLEGAL_PARAM,
        SAMPLE_SVP_ERR_LEVEL_ERROR,"Error,pstNnieCfg and pstNnieParam can't be NULL!\n");
    SAMPLE_SVP_CHECK_EXPR_RET((NULL == pstNnieParam->pstModel),HI_ERR_SVP_NNIE_ILLEGAL_PARAM,
        SAMPLE_SVP_ERR_LEVEL_ERROR,"Error,pstNnieParam->pstModel can't be NULL!\n");

	/*NNIE parameter initialization */
	s32Ret = SAMPLE_SVP_NNIE_ParamInit(pstNnieCfg, pstNnieParam);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, INIT_FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
                              "Error(%#x),SAMPLE_COMM_SVP_NNIE_ParamInit failed!\n",s32Ret);
    
    return s32Ret;      // 不返回还报“段错误”
    
INIT_FAIL_0:
    s32Ret = SAMPLE_SVP_NNIE_Yolov3_Deinit(pstNnieParam, pstSoftWareParam, NULL);
    SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret,s32Ret,SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error(%#x),SAMPLE_SVP_NNIE_Yolov3_Deinit failed!\n",s32Ret);
    return HI_FAILURE;


}



/******************************************************************************
* function : Fill Src Data
******************************************************************************/
static HI_S32 SAMPLE_SVP_NNIE_FillSrcData(SAMPLE_SVP_NNIE_CFG_S* pstNnieCfg,
    SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam, SAMPLE_SVP_NNIE_INPUT_DATA_INDEX_S* pstInputDataIdx)
{
    FILE* fp = NULL;
    HI_U32 i =0, j = 0, n = 0;
    HI_U32 u32Height = 0, u32Width = 0, u32Chn = 0, u32Stride = 0, u32Dim = 0;
    HI_U32 u32VarSize = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U8* pu8PicAddr = NULL;
    HI_U32*pu32StepAddr = NULL;
    HI_U32 u32SegIdx = pstInputDataIdx->u32SegIdx;
    HI_U32 u32NodeIdx = pstInputDataIdx->u32NodeIdx;
    HI_U32 u32TotalStepNum = 0;


    /*open file*/
    if (NULL != pstNnieCfg->pszPic)
    {
        fp = fopen(pstNnieCfg->pszPic, "rb");
        SAMPLE_SVP_CHECK_EXPR_RET(NULL == fp, HI_INVALID_VALUE, SAMPLE_SVP_ERR_LEVEL_ERROR,
            "Error, open file failed!\n");
    }

    /*get data size*/
    if(SVP_BLOB_TYPE_U8 <= pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].enType &&
        SVP_BLOB_TYPE_YVU422SP >= pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].enType)
    {
        u32VarSize = sizeof(HI_U8);     // yolov3是uint8
    }
    else
    {
        u32VarSize = sizeof(HI_U32);
    }

    /*fill src data*/
    if(SVP_BLOB_TYPE_SEQ_S32 == pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].enType)
    {
        u32Dim = pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].unShape.stSeq.u32Dim;
        u32Stride = pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u32Stride;
        pu32StepAddr = SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_U32,pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].unShape.stSeq.u64VirAddrStep);
        pu8PicAddr = SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_U8,pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u64VirAddr);
        for(n = 0; n < pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u32Num; n++)
        {
            for(i = 0;i < *(pu32StepAddr+n); i++)
            {
                s32Ret = fread(pu8PicAddr,u32Dim*u32VarSize,1,fp);
                SAMPLE_SVP_CHECK_EXPR_GOTO(1 != s32Ret,FAIL,SAMPLE_SVP_ERR_LEVEL_ERROR,"Error,Read image file failed!\n");
                pu8PicAddr += u32Stride;
            }
            u32TotalStepNum += *(pu32StepAddr+n);
        }
        SAMPLE_COMM_SVP_FlushCache(pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u64PhyAddr,
            SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_VOID,pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u64VirAddr),
            u32TotalStepNum*u32Stride);
    }
    else
    {
        u32Height = pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].unShape.stWhc.u32Height;
        u32Width = pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].unShape.stWhc.u32Width;
        u32Chn = pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].unShape.stWhc.u32Chn;
        u32Stride = pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u32Stride;
        
        SAMPLE_SVP_TRACE_INFO("输入blob的信息 h:%d | w:%d | c:%d | stride:%d \n", u32Height, u32Width, u32Chn, u32Stride);              // 模型编译时的输入信息
        SAMPLE_SVP_TRACE_INFO("输入blob的数据类型: %d \n", pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].enType);
        SAMPLE_SVP_TRACE_INFO("输入blob, seg idx: %d, node idx: %d \n", u32SegIdx, u32NodeIdx);

        pu8PicAddr = SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_U8, pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u64VirAddr);      // 获得内存地址

        if(SVP_BLOB_TYPE_YVU420SP== pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].enType)
        {
            for(n = 0; n < pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u32Num; n++)
            {
                for(i = 0; i < u32Chn*u32Height/2; i++)
                {
                    s32Ret = fread(pu8PicAddr,u32Width*u32VarSize,1,fp);
                    SAMPLE_SVP_CHECK_EXPR_GOTO(1 != s32Ret,FAIL,SAMPLE_SVP_ERR_LEVEL_ERROR,"Error,Read image file failed!\n");
                    pu8PicAddr += u32Stride;
                }
            }
        }
        else if(SVP_BLOB_TYPE_YVU422SP== pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].enType)
        {
            for(n = 0; n < pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u32Num; n++)
            {
                for(i = 0; i < u32Height*2; i++)
                {
                    s32Ret = fread(pu8PicAddr,u32Width*u32VarSize,1,fp);
                    SAMPLE_SVP_CHECK_EXPR_GOTO(1 != s32Ret,FAIL,SAMPLE_SVP_ERR_LEVEL_ERROR,"Error,Read image file failed!\n");
                    pu8PicAddr += u32Stride;
                }
            }
        }
        else
        {
            SAMPLE_SVP_TRACE_INFO("开始数据读取 \n");
            for(n = 0; n < pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u32Num; n++)          // 设置的为1
            {
                for(i = 0; i < u32Chn; i++)
                {
                    for(j = 0; j < u32Height; j++)
                    {
                        s32Ret = fread(pu8PicAddr, u32Width * u32VarSize, 1, fp);
                        SAMPLE_SVP_CHECK_EXPR_GOTO(1 != s32Ret, FAIL, SAMPLE_SVP_ERR_LEVEL_ERROR,"Error,Read image file failed!\n");
                        pu8PicAddr += u32Stride;
                    }
                }
            }
        }
        SAMPLE_SVP_TRACE_INFO("完成数据输入 \n");

        SAMPLE_COMM_SVP_FlushCache(pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u64PhyAddr,
            SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_VOID,pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u64VirAddr),
            pstNnieParam->astSegData[u32SegIdx].astSrc[u32NodeIdx].u32Num * u32Chn * u32Height * u32Stride);        // 物理地址和虚拟地址刷新
    }

    fclose(fp);
    return HI_SUCCESS;
FAIL:

    fclose(fp);
    return HI_FAILURE;
}


/******************************************************************************
* function : NNIE Forward
******************************************************************************/
static HI_S32 SAMPLE_SVP_NNIE_Forward(SAMPLE_SVP_NNIE_PARAM_S *pstNnieParam,
    SAMPLE_SVP_NNIE_INPUT_DATA_INDEX_S* pstInputDataIdx,
    SAMPLE_SVP_NNIE_PROCESS_SEG_INDEX_S* pstProcSegIdx,
    HI_BOOL bInstant)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 i = 0, j = 0;
    HI_BOOL bFinish = HI_FALSE;
    SVP_NNIE_HANDLE hSvpNnieHandle = 0;
    HI_U32 u32TotalStepNum = 0;
    SAMPLE_SVP_NIE_PERF_STAT_DEF_VAR();

    SAMPLE_SVP_NNIE_PERF_STAT_OP_FORWARD_CLREAR();

    // 清空task buffer, outblob1, outblob2, outblob3的虚拟内存
    SAMPLE_COMM_SVP_FlushCache(pstNnieParam->astForwardCtrl[pstProcSegIdx->u32SegIdx].stTskBuf.u64PhyAddr,
                                SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_VOID, pstNnieParam->astForwardCtrl[pstProcSegIdx->u32SegIdx].stTskBuf.u64VirAddr),
                                pstNnieParam->astForwardCtrl[pstProcSegIdx->u32SegIdx].stTskBuf.u32Size);

    SAMPLE_SVP_NNIE_PERF_STAT_BEGIN();
    for(i = 0; i < pstNnieParam->astForwardCtrl[pstProcSegIdx->u32SegIdx].u32DstNum; i++)
    {
        if(SVP_BLOB_TYPE_SEQ_S32 == pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].enType)
        {
            for(j = 0; j < pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u32Num; j++)
            {
                u32TotalStepNum += *(SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_U32,pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].unShape.stSeq.u64VirAddrStep)+j);
            }
            SAMPLE_COMM_SVP_FlushCache(pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u64PhyAddr,
                SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_VOID,pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u64VirAddr),
                u32TotalStepNum*pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u32Stride);

        }
        else
        {
            SAMPLE_COMM_SVP_FlushCache(pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u64PhyAddr,
                SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_VOID, pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u64VirAddr),
                pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u32Num*
                pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].unShape.stWhc.u32Chn*
                pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].unShape.stWhc.u32Height*
                pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u32Stride);
        }
    }
    SAMPLE_SVP_NNIE_PERF_STAT_END();
    SAMPLE_SVP_NNIE_PERF_STAT_OP_FORWARD_PRE_DST_FLUSH_TIME();

    /*set input blob according to node name*/
    if(pstInputDataIdx->u32SegIdx != pstProcSegIdx->u32SegIdx)
    {
        SAMPLE_SVP_TRACE_INFO("靠node name做输入 \n");
        for(i = 0; i < pstNnieParam->pstModel->astSeg[pstProcSegIdx->u32SegIdx].u16SrcNum; i++)
        {
            for(j = 0; j < pstNnieParam->pstModel->astSeg[pstInputDataIdx->u32SegIdx].u16DstNum; j++)
            {
                if(0 == strncmp(pstNnieParam->pstModel->astSeg[pstInputDataIdx->u32SegIdx].astDstNode[j].szName,
                    pstNnieParam->pstModel->astSeg[pstProcSegIdx->u32SegIdx].astSrcNode[i].szName,
                    SVP_NNIE_NODE_NAME_LEN))
                {
                    pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astSrc[i] =
                        pstNnieParam->astSegData[pstInputDataIdx->u32SegIdx].astDst[j];
                    break;
                }
            }
            SAMPLE_SVP_CHECK_EXPR_RET((j == pstNnieParam->pstModel->astSeg[pstInputDataIdx->u32SegIdx].u16DstNum),
                HI_FAILURE,SAMPLE_SVP_ERR_LEVEL_ERROR,"Error,can't find %d-th seg's %d-th src blob!\n",
                pstProcSegIdx->u32SegIdx,i);
        }
    }

    /*NNIE_Forward, 系统函数*/
    SAMPLE_SVP_NNIE_PERF_STAT_BEGIN();
    s32Ret = HI_MPI_SVP_NNIE_Forward(&hSvpNnieHandle,
                                    pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astSrc,
                                    pstNnieParam->pstModel, 
                                    pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst,
                                    &pstNnieParam->astForwardCtrl[pstProcSegIdx->u32SegIdx],
                                    bInstant);
    SAMPLE_SVP_CHECK_EXPR_RET(HI_SUCCESS != s32Ret, s32Ret, SAMPLE_SVP_ERR_LEVEL_ERROR,
                            "Error,HI_MPI_SVP_NNIE_Forward failed!\n");

    if(bInstant)
    {
        /*Wait NNIE finish*/
        while(HI_ERR_SVP_NNIE_QUERY_TIMEOUT == 
             (s32Ret = HI_MPI_SVP_NNIE_Query(pstNnieParam->astForwardCtrl[pstProcSegIdx->u32SegIdx].enNnieId,hSvpNnieHandle, &bFinish, HI_TRUE)) )
        {
            usleep(100);
            SAMPLE_SVP_TRACE(SAMPLE_SVP_ERR_LEVEL_INFO, "HI_MPI_SVP_NNIE_Query Query timeout!\n");
        }
    }
    SAMPLE_SVP_NNIE_PERF_STAT_END();
    SAMPLE_SVP_NNIE_PERF_STAT_OP_FORWARD_OP_TIME();
    u32TotalStepNum = 0;

    SAMPLE_SVP_NNIE_PERF_STAT_BEGIN();
    for(i = 0; i < pstNnieParam->astForwardCtrl[pstProcSegIdx->u32SegIdx].u32DstNum; i++)
    {
        if(SVP_BLOB_TYPE_SEQ_S32 == pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].enType)
        {
            for(j = 0; j < pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u32Num; j++)
            {
                u32TotalStepNum += *(SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_U32,pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].unShape.stSeq.u64VirAddrStep)+j);
            }
            SAMPLE_COMM_SVP_FlushCache(pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u64PhyAddr,
                SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_VOID,pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u64VirAddr),
                u32TotalStepNum*pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u32Stride);

        }
        else
        {
            SAMPLE_SVP_TRACE_INFO("nnie推理之后,输出blob idx: %d | 类型: %d \n", i, pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].enType);

            SAMPLE_COMM_SVP_FlushCache(pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u64PhyAddr,
                                        SAMPLE_SVP_NNIE_CONVERT_64BIT_ADDR(HI_VOID,pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u64VirAddr),
                                        pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u32Num*
                                        pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].unShape.stWhc.u32Chn*
                                        pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].unShape.stWhc.u32Height*
                                        pstNnieParam->astSegData[pstProcSegIdx->u32SegIdx].astDst[i].u32Stride);        // 将结果从cache，转到内存中，物理地址可能使cache，虚拟地址可能是内存
        }
    }
    SAMPLE_SVP_NNIE_PERF_STAT_END();
    SAMPLE_SVP_NNIE_PERF_STAT_OP_FORWARD_AFTER_DST_FLUSH_TIME();

    return s32Ret;
}


long long Time_use(struct timeval start, struct timeval end)
{
    long long time_ = (end.tv_sec - start.tv_sec) * 1000000.0 + end.tv_usec - start.tv_usec;
    return time_;
}


void Centerface(void)
{   
    struct timeval begin, end;

    /*Sys init*/
    SAMPLE_COMM_SVP_CheckSysInit();

    //// load model
    HI_S32 s32Ret;
    HI_CHAR *pcSrcFile = "./nnie_image/rgb_planar/dog_bike_car_416x416.bgr";
    HI_CHAR *model_file = "./nnie_model/detection/yolov3_face_small_inst.wk";
    s32Ret = SAMPLE_COMM_SVP_NNIE_LoadModel(model_file, &centerfaceModel);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
                                "Error,SAMPLE_COMM_SVP_NNIE_LoadModel failed!\n");

    //// parameter init
    SAMPLE_SVP_NNIE_CFG_S   stNnieCfg = {0};
    stNnieCfg.pszPic= pcSrcFile;
    stNnieCfg.u32MaxInputNum = 1;                   //max input image num in each batch
    stNnieCfg.u32MaxRoiNum = 0;
    stNnieCfg.aenNnieCoreId[0] = SVP_NNIE_ID_0;     //set NNIE core
    centerfaceNnieParam.pstModel = &centerfaceModel.stModel;
    s32Ret = SAMPLE_SVP_NNIE_Yolov3_ParamInit(&stNnieCfg, &centerfaceNnieParam, &s_stYolov3SoftwareParam);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
                                "Error,SAMPLE_SVP_NNIE_Yolov3_ParamInit failed!\n");

    //// fill input
    SAMPLE_SVP_NNIE_INPUT_DATA_INDEX_S stInputDataIdx = {0};
    stInputDataIdx.u32SegIdx = 0;
    stInputDataIdx.u32NodeIdx = 0;
    s32Ret = SAMPLE_SVP_NNIE_FillSrcData(&stNnieCfg, &centerfaceNnieParam, &stInputDataIdx);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
                                    "Error,SAMPLE_SVP_NNIE_FillSrcData failed!\n");
    
    //// nnie forward
    SAMPLE_SVP_NNIE_PROCESS_SEG_INDEX_S stProcSegIdx = {0};
    stProcSegIdx.u32SegIdx = 0;
    gettimeofday(&begin, NULL);
    s32Ret = SAMPLE_SVP_NNIE_Forward(&centerfaceNnieParam, &stInputDataIdx, &stProcSegIdx, HI_TRUE);
    SAMPLE_SVP_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL_0, SAMPLE_SVP_ERR_LEVEL_ERROR,
                                    "Error,SAMPLE_SVP_NNIE_FillSrcData failed!\n");
    gettimeofday(&end, NULL);
    SAMPLE_SVP_TRACE_INFO("nnie forward time:%lld us \n", Time_use(begin, end) );

    //// decode


FAIL_0:
    // SAMPLE_SVP_NNIE_Yolov3_Deinit(&s_stYolov3NnieParam,&s_stYolov3SoftwareParam,&s_stYolov3Model);
    SAMPLE_COMM_SVP_CheckSysExit();

}



int main(int argc, char *argv[])
{
    switch (*argv[1])
    {
    case '0':
        Centerface();
        break;
    
    default:
        break;
    }
}