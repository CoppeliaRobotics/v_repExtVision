#include "simExtVision.h"
#include "scriptFunctionData.h"
#include "simLib.h"
#include <iostream>
#include "visionTransfCont.h"
#include "visionVelodyneHDL64ECont.h"
#include "visionVelodyneVPL16Cont.h"
#include "imageProcess.h"
#include "7Vector.h"
#include <map>
#include <algorithm>

#ifdef _WIN32
    #ifdef QT_COMPIL
        #include <direct.h>
    #else
        #include <shlwapi.h>
        #pragma comment(lib, "Shlwapi.lib")
    #endif
#endif /* _WIN32 */
#if defined (__linux) || defined (__APPLE__)
    #include <unistd.h>
#endif /* __linux || __APPLE__ */

#define CONCAT(x,y,z) x y z
#define strConCat(x,y,z)    CONCAT(x,y,z)

static LIBRARY simLib;
static CVisionTransfCont* visionTransfContainer;
static CVisionVelodyneHDL64ECont* visionVelodyneHDL64EContainer;
static CVisionVelodyneVPL16Cont* visionVelodyneVPL16Container;


// Following few for backward compatibility:
#define LUA_HANDLESPHERICAL_COMMANDOLD_PLUGIN "simExtVision_handleSpherical@Vision"
#define LUA_HANDLESPHERICAL_COMMANDOLD "simExtVision_handleSpherical"
#define LUA_HANDLEANAGLYPHSTEREO_COMMANDOLD_PLUGIN "simExtVision_handleAnaglyphStereo@Vision"
#define LUA_HANDLEANAGLYPHSTEREO_COMMANDOLD "simExtVision_handleAnaglyphStereo"
#define LUA_CREATEVELODYNEHDL64E_COMMANDOLD_PLUGIN "simExtVision_createVelodyneHDL64E@Vision"
#define LUA_CREATEVELODYNEHDL64E_COMMANDOLD "simExtVision_createVelodyneHDL64E"
#define LUA_DESTROYVELODYNEHDL64E_COMMANDOLD_PLUGIN "simExtVision_destroyVelodyneHDL64E@Vision"
#define LUA_DESTROYVELODYNEHDL64E_COMMANDOLD "simExtVision_destroyVelodyneHDL64E"
#define LUA_HANDLEVELODYNEHDL64E_COMMANDOLD_PLUGIN "simExtVision_handleVelodyneHDL64E@Vision"
#define LUA_HANDLEVELODYNEHDL64E_COMMANDOLD "simExtVision_handleVelodyneHDL64E"
#define LUA_CREATEVELODYNEVPL16_COMMANDOLD_PLUGIN "simExtVision_createVelodyneVPL16@Vision"
#define LUA_CREATEVELODYNEVPL16_COMMANDOLD "simExtVision_createVelodyneVPL16"
#define LUA_DESTROYVELODYNEVPL16_COMMANDOLD_PLUGIN "simExtVision_destroyVelodyneVPL16@Vision"
#define LUA_DESTROYVELODYNEVPL16_COMMANDOLD "simExtVision_destroyVelodyneVPL16"
#define LUA_HANDLEVELODYNEVPL16_COMMANDOLD_PLUGIN "simExtVision_handleVelodyneVPL16@Vision"
#define LUA_HANDLEVELODYNEVPL16_COMMANDOLD "simExtVision_handleVelodyneVPL16"


// --------------------------------------------------------------------------------------
// simVision.handleSpherical
// --------------------------------------------------------------------------------------
#define LUA_HANDLESPHERICAL_COMMAND_PLUGIN "simVision.handleSpherical@Vision"
#define LUA_HANDLESPHERICAL_COMMAND "simVision.handleSpherical"

const int inArgs_HANDLESPHERICAL[]={
    5,
    sim_script_arg_int32,0,
    sim_script_arg_int32|sim_script_arg_table,6,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_int32,0,
};

void LUA_HANDLESPHERICAL_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    int result=-1;
    if (D.readDataFromStack(p->stackID,inArgs_HANDLESPHERICAL,inArgs_HANDLESPHERICAL[0]-1,LUA_HANDLESPHERICAL_COMMAND)) // last arg is optional
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int passiveVisionSensorHandle=inData->at(0).int32Data[0];
        int activeVisionSensorHandes[6];
        for (int i=0;i<6;i++)
            activeVisionSensorHandes[i]=inData->at(1).int32Data[i];
        float horizontalAngle=inData->at(2).floatData[0];
        float verticalAngle=inData->at(3).floatData[0];
        int passiveVisionSensor2Handle=-1;
        if (inData->size()>=5)
            passiveVisionSensor2Handle=inData->at(4).int32Data[0]; // for the depth map
        int h=passiveVisionSensorHandle;
        if (h==-1)
            h=passiveVisionSensor2Handle;
        CVisionTransf* obj=visionTransfContainer->getVisionTransfFromReferencePassiveVisionSensor(h);
        if (obj!=NULL)
        {
            if (!obj->isSame(activeVisionSensorHandes,horizontalAngle,verticalAngle,passiveVisionSensorHandle,passiveVisionSensor2Handle))
            {
                visionTransfContainer->removeObject(h);
                obj=NULL;
            }
        }
        if (obj==NULL)
        {
            obj=new CVisionTransf(passiveVisionSensorHandle,activeVisionSensorHandes,horizontalAngle,verticalAngle,passiveVisionSensor2Handle);
            visionTransfContainer->addObject(obj);
        }

        if (obj->doAllObjectsExistAndAreVisionSensors())
        {
            if (obj->isActiveVisionSensorResolutionCorrect())
            {
                if (obj->areActiveVisionSensorsExplicitelyHandled())
                {
                    if (!obj->areRGBAndDepthVisionSensorResolutionsCorrect())
                        simSetLastError(LUA_HANDLESPHERICAL_COMMAND,"Invalid vision sensor resolution for the RGB or depth component."); // output an error
                    else
                    {
                        obj->handleObject();
                        result=1; // success
                    }
                }
                else
                    simSetLastError(LUA_HANDLESPHERICAL_COMMAND,"Active vision sensors should be explicitely handled."); // output an error
            }
            else
                simSetLastError(LUA_HANDLESPHERICAL_COMMAND,"Invalid vision sensor resolutions."); // output an error
        }
        else
            simSetLastError(LUA_HANDLESPHERICAL_COMMAND,"Invalid handles, or handles are not vision sensor handles."); // output an error

        if (result==-1)
            visionTransfContainer->removeObject(h);

    }
    D.pushOutData(CScriptFunctionDataItem(result));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.handleAnaglyphStereo
// --------------------------------------------------------------------------------------
#define LUA_HANDLEANAGLYPHSTEREO_COMMAND_PLUGIN "simVision.handleAnaglyphStereo@Vision"
#define LUA_HANDLEANAGLYPHSTEREO_COMMAND "simVision.handleAnaglyphStereo"

const int inArgs_HANDLEANAGLYPHSTEREO[]={
    3,
    sim_script_arg_int32,0,
    sim_script_arg_int32|sim_script_arg_table,2,
    sim_script_arg_float|sim_script_arg_table,6,
};

void LUA_HANDLEANAGLYPHSTEREO_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    int result=-1;
    if (D.readDataFromStack(p->stackID,inArgs_HANDLEANAGLYPHSTEREO,inArgs_HANDLEANAGLYPHSTEREO[0]-1,LUA_HANDLEANAGLYPHSTEREO_COMMAND)) // -1 because last arg is optional
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int passiveVisionSensorHande=inData->at(0).int32Data[0];
        int leftSensorHandle=inData->at(1).int32Data[0];
        int rightSensorHandle=inData->at(1).int32Data[1];
        // Check the object types:
        bool existAndAreVisionSensors=true;
        if (simGetObjectType(passiveVisionSensorHande)!=sim_object_visionsensor_type)
            existAndAreVisionSensors=false;
        if (simGetObjectType(leftSensorHandle)!=sim_object_visionsensor_type)
            existAndAreVisionSensors=false;
        if (simGetObjectType(rightSensorHandle)!=sim_object_visionsensor_type)
            existAndAreVisionSensors=false;
        if (existAndAreVisionSensors)
        { // check the sensor resolutions:
            int r[2];
            simGetVisionSensorResolution(passiveVisionSensorHande,r);
            int rl[2];
            simGetVisionSensorResolution(leftSensorHandle,rl);
            int rr[2];
            simGetVisionSensorResolution(rightSensorHandle,rr);
            if ((r[0]==rl[0])&&(r[0]==rr[0])&&(r[1]==rl[1])&&(r[1]==rr[1]))
            { // check if the sensors are explicitely handled:
                int e=simGetExplicitHandling(passiveVisionSensorHande);
                int el=simGetExplicitHandling(leftSensorHandle);
                int er=simGetExplicitHandling(rightSensorHandle);
                if ((e&el&er&1)==1)
                {
                    float leftAndRightColors[6]={1.0f,0.0f,0.0f,0.0f,1.0f,1.0f}; // default
                    if (inData->size()>2)
                    { // we have the optional argument
                        for (int i=0;i<6;i++)
                            leftAndRightColors[i]=inData->at(2).floatData[i];
                    }
                    simHandleVisionSensor(leftSensorHandle,NULL,NULL);
                    float* leftImage=simGetVisionSensorImage(leftSensorHandle);
                    simHandleVisionSensor(rightSensorHandle,NULL,NULL);
                    float* rightImage=simGetVisionSensorImage(rightSensorHandle);
                    for (int i=0;i<r[0]*r[1];i++)
                    {
                        float il=(leftImage[3*i+0]+leftImage[3*i+1]+leftImage[3*i+2])/3.0f;
                        float ir=(rightImage[3*i+0]+rightImage[3*i+1]+rightImage[3*i+2])/3.0f;
                        leftImage[3*i+0]=il*leftAndRightColors[0]+ir*leftAndRightColors[3];
                        leftImage[3*i+1]=il*leftAndRightColors[1]+ir*leftAndRightColors[4];
                        leftImage[3*i+2]=il*leftAndRightColors[2]+ir*leftAndRightColors[5];
                    }
                    simSetVisionSensorImage(passiveVisionSensorHande,leftImage);
                    simReleaseBuffer((char*)leftImage);
                    simReleaseBuffer((char*)rightImage);
                    result=1;
                }
                else
                    simSetLastError(LUA_HANDLEANAGLYPHSTEREO_COMMAND,"Vision sensors should be explicitely handled."); // output an error
            }
            else
                simSetLastError(LUA_HANDLEANAGLYPHSTEREO_COMMAND,"Invalid vision sensor resolutions."); // output an error
        }
        else
            simSetLastError(LUA_HANDLEANAGLYPHSTEREO_COMMAND,"Invalid handles, or handles are not vision sensor handles."); // output an error
    }
    D.pushOutData(CScriptFunctionDataItem(result));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.createVelodyneHDL64E
// --------------------------------------------------------------------------------------
#define LUA_CREATEVELODYNEHDL64E_COMMAND_PLUGIN "simVision.createVelodyneHDL64E@Vision"
#define LUA_CREATEVELODYNEHDL64E_COMMAND "simVision.createVelodyneHDL64E"

const int inArgs_CREATEVELODYNEHDL64E[]={
    7,
    sim_script_arg_int32|sim_script_arg_table,4,
    sim_script_arg_float,0,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
    sim_script_arg_float|sim_script_arg_table,2,
    sim_script_arg_float,0,
    sim_script_arg_int32,0,
};

void LUA_CREATEVELODYNEHDL64E_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    int velodyneHandle=-1;
    if (D.readDataFromStack(p->stackID,inArgs_CREATEVELODYNEHDL64E,inArgs_CREATEVELODYNEHDL64E[0]-5,LUA_CREATEVELODYNEHDL64E_COMMAND)) // -5 because the last 5 arguments are optional
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int options=0;
        float pointSize=2.0f;
        float scalingFactor=1.0f;
        float coloringDistances[2]={1,5};
        int visionSensorHandes[4];
        for (int i=0;i<4;i++)
            visionSensorHandes[i]=inData->at(0).int32Data[i];
        float frequency=inData->at(1).floatData[0];
        int pointCloudHandle=-1;
        if (inData->size()>2)
        { // we have the optional 'options' argument:
            options=inData->at(2).int32Data[0];
        }
        if (inData->size()>3)
        { // we have the optional 'pointSize' argument:
            pointSize=inData->at(3).floatData[0];   
        }
        if (inData->size()>4)
        { // we have the optional 'coloringDistance' argument:
            coloringDistances[0]=inData->at(4).floatData[0];
            coloringDistances[1]=inData->at(4).floatData[1];
        }
        if (inData->size()>5)
        { // we have the optional 'displayScalingFactor' argument:
            scalingFactor=inData->at(5).floatData[0];
        }
        if (inData->size()>6)
        { // we have the optional 'pointCloudHandle' argument:
            pointCloudHandle=inData->at(6).int32Data[0];
        }
        CVisionVelodyneHDL64E* obj=new CVisionVelodyneHDL64E(visionSensorHandes,frequency,options,pointSize,coloringDistances,scalingFactor,pointCloudHandle);
        visionVelodyneHDL64EContainer->addObject(obj);
        if (obj->doAllObjectsExistAndAreVisionSensors())
        {
            if (obj->areVisionSensorsExplicitelyHandled())
                velodyneHandle=obj->getVelodyneHandle(); // success
            else
                simSetLastError(LUA_CREATEVELODYNEHDL64E_COMMAND,"Vision sensors should be explicitely handled."); // output an error
        }
        else
            simSetLastError(LUA_CREATEVELODYNEHDL64E_COMMAND,"Invalid handles, or handles are not vision sensor handles."); // output an error

        if (velodyneHandle==-1)
            visionVelodyneHDL64EContainer->removeObject(obj->getVelodyneHandle());
    }
    D.pushOutData(CScriptFunctionDataItem(velodyneHandle));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.destroyVelodyneHDL64E
// --------------------------------------------------------------------------------------
#define LUA_DESTROYVELODYNEHDL64E_COMMAND_PLUGIN "simVision.destroyVelodyneHDL64E@Vision"
#define LUA_DESTROYVELODYNEHDL64E_COMMAND "simVision.destroyVelodyneHDL64E"

const int inArgs_DESTROYVELODYNEHDL64E[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_DESTROYVELODYNEHDL64E_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    int result=-1;
    if (D.readDataFromStack(p->stackID,inArgs_DESTROYVELODYNEHDL64E,inArgs_DESTROYVELODYNEHDL64E[0],LUA_DESTROYVELODYNEHDL64E_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=inData->at(0).int32Data[0];
        CVisionVelodyneHDL64E* obj=visionVelodyneHDL64EContainer->getObject(handle);
        if (obj!=NULL)
        {
            visionVelodyneHDL64EContainer->removeObject(obj->getVelodyneHandle());
            result=1;
        }
        else
            simSetLastError(LUA_DESTROYVELODYNEHDL64E_COMMAND,"Invalid handle."); // output an error
    }
    D.pushOutData(CScriptFunctionDataItem(result));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.handleVelodyneHDL64E
// --------------------------------------------------------------------------------------
#define LUA_HANDLEVELODYNEHDL64E_COMMAND_PLUGIN "simVision.handleVelodyneHDL64E@Vision"
#define LUA_HANDLEVELODYNEHDL64E_COMMAND "simVision.handleVelodyneHDL64E"

const int inArgs_HANDLEVELODYNEHDL64E[]={
    2,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
};

void LUA_HANDLEVELODYNEHDL64E_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    std::vector<float> pts;
    bool result=false;
    bool codedString=false;
    if (D.readDataFromStack(p->stackID,inArgs_HANDLEVELODYNEHDL64E,inArgs_HANDLEVELODYNEHDL64E[0],LUA_HANDLEVELODYNEHDL64E_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=inData->at(0).int32Data[0];
        bool absCoords=false;
        if (handle>=0)
        {
            codedString=((handle&sim_handleflag_codedstring)!=0);
            absCoords=((handle&sim_handleflag_abscoords)!=0);
            handle=handle&0x000fffff;
        }
        float dt=inData->at(1).floatData[0];
        CVisionVelodyneHDL64E* obj=visionVelodyneHDL64EContainer->getObject(handle);
        if (obj!=NULL)
            result=obj->handle(dt,pts,absCoords);
        else
            simSetLastError(LUA_HANDLEVELODYNEHDL64E_COMMAND,"Invalid handle."); // output an error
    }
    if (result)
    {
        if (codedString)
        {
            if (pts.size()>0)
                D.pushOutData(CScriptFunctionDataItem((char*)&pts[0],pts.size()*sizeof(float)));
            else
                D.pushOutData(CScriptFunctionDataItem(nullptr,0));
        }
        else
            D.pushOutData(CScriptFunctionDataItem(pts));
    }
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.createVelodyneVPL16
// --------------------------------------------------------------------------------------
#define LUA_CREATEVELODYNEVPL16_COMMAND_PLUGIN "simVision.createVelodyneVPL16@Vision"
#define LUA_CREATEVELODYNEVPL16_COMMAND "simVision.createVelodyneVPL16"

const int inArgs_CREATEVELODYNEVPL16[]={
    7,
    sim_script_arg_int32|sim_script_arg_table,4,
    sim_script_arg_float,0,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
    sim_script_arg_float|sim_script_arg_table,2,
    sim_script_arg_float,0,
    sim_script_arg_int32,0,
};

void LUA_CREATEVELODYNEVPL16_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    int velodyneHandle=-1;
    if (D.readDataFromStack(p->stackID,inArgs_CREATEVELODYNEVPL16,inArgs_CREATEVELODYNEVPL16[0]-5,LUA_CREATEVELODYNEVPL16_COMMAND)) // -5 because the last 5 arguments are optional
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int options=0;
        float pointSize=2.0f;
        float scalingFactor=1.0f;
        float coloringDistances[2]={1,5};
        int visionSensorHandes[4];
        for (int i=0;i<4;i++)
            visionSensorHandes[i]=inData->at(0).int32Data[i];
        float frequency=inData->at(1).floatData[0];
        int pointCloudHandle=-1;
        if (inData->size()>2)
        { // we have the optional 'options' argument:
            options=inData->at(2).int32Data[0];
        }
        if (inData->size()>3)
        { // we have the optional 'pointSize' argument:
            pointSize=inData->at(3).floatData[0];   
        }
        if (inData->size()>4)
        { // we have the optional 'coloringDistance' argument:
            coloringDistances[0]=inData->at(4).floatData[0];
            coloringDistances[1]=inData->at(4).floatData[1];
        }
        if (inData->size()>5)
        { // we have the optional 'displayScalingFactor' argument:
            scalingFactor=inData->at(5).floatData[0];
        }
        if (inData->size()>6)
        { // we have the optional 'pointCloudHandle' argument:
            pointCloudHandle=inData->at(6).int32Data[0];
        }
        CVisionVelodyneVPL16* obj=new CVisionVelodyneVPL16(visionSensorHandes,frequency,options,pointSize,coloringDistances,scalingFactor,pointCloudHandle);
        visionVelodyneVPL16Container->addObject(obj);
        if (obj->doAllObjectsExistAndAreVisionSensors())
        {
            if (obj->areVisionSensorsExplicitelyHandled())
                velodyneHandle=obj->getVelodyneHandle(); // success
            else
                simSetLastError(LUA_CREATEVELODYNEVPL16_COMMAND,"Vision sensors should be explicitely handled."); // output an error
        }
        else
            simSetLastError(LUA_CREATEVELODYNEVPL16_COMMAND,"Invalid handles, or handles are not vision sensor handles."); // output an error

        if (velodyneHandle==-1)
            visionVelodyneVPL16Container->removeObject(obj->getVelodyneHandle());
    }
    D.pushOutData(CScriptFunctionDataItem(velodyneHandle));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.destroyVelodyneVPL16
// --------------------------------------------------------------------------------------
#define LUA_DESTROYVELODYNEVPL16_COMMAND_PLUGIN "simVision.destroyVelodyneVPL16@Vision"
#define LUA_DESTROYVELODYNEVPL16_COMMAND "simVision.destroyVelodyneVPL16"

const int inArgs_DESTROYVELODYNEVPL16[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_DESTROYVELODYNEVPL16_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    int result=-1;
    if (D.readDataFromStack(p->stackID,inArgs_DESTROYVELODYNEVPL16,inArgs_DESTROYVELODYNEVPL16[0],LUA_DESTROYVELODYNEVPL16_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=inData->at(0).int32Data[0];
        CVisionVelodyneVPL16* obj=visionVelodyneVPL16Container->getObject(handle);
        if (obj!=NULL)
        {
            visionVelodyneVPL16Container->removeObject(obj->getVelodyneHandle());
            result=1;
        }
        else
            simSetLastError(LUA_DESTROYVELODYNEVPL16_COMMAND,"Invalid handle."); // output an error
    }
    D.pushOutData(CScriptFunctionDataItem(result));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.handleVelodyneVPL16
// --------------------------------------------------------------------------------------
#define LUA_HANDLEVELODYNEVPL16_COMMAND_PLUGIN "simVision.handleVelodyneVPL16@Vision"
#define LUA_HANDLEVELODYNEVPL16_COMMAND "simVision.handleVelodyneVPL16"

const int inArgs_HANDLEVELODYNEVPL16[]={
    2,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
};

void LUA_HANDLEVELODYNEVPL16_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    std::vector<float> pts;
    bool result=false;
    bool codedString=false;
    if (D.readDataFromStack(p->stackID,inArgs_HANDLEVELODYNEVPL16,inArgs_HANDLEVELODYNEVPL16[0],LUA_HANDLEVELODYNEVPL16_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=inData->at(0).int32Data[0];
        bool absCoords=false;
        if (handle>=0)
        {
            codedString=((handle&sim_handleflag_codedstring)!=0);
            absCoords=((handle&sim_handleflag_abscoords)!=0);
            handle=handle&0x000fffff;
        }
        float dt=inData->at(1).floatData[0];
        CVisionVelodyneVPL16* obj=visionVelodyneVPL16Container->getObject(handle);
        if (obj!=NULL)
            result=obj->handle(dt,pts,absCoords);
        else
            simSetLastError(LUA_HANDLEVELODYNEVPL16_COMMAND,"Invalid handle."); // output an error
    }
    if (result)
    {
        if (codedString)
        {
            if (pts.size()>0)
                D.pushOutData(CScriptFunctionDataItem((char*)&pts[0],pts.size()*sizeof(float)));
            else
                D.pushOutData(CScriptFunctionDataItem(nullptr,0));
        }
        else
            D.pushOutData(CScriptFunctionDataItem(pts));
    }
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

class CVisionSensorData
{
    public:
    CVisionSensorData()
    {
        workImg=nullptr;
        buff1Img=nullptr;
        buff2Img=nullptr;
    }
    ~CVisionSensorData()
    {
        if (workImg!=nullptr)
            delete[] workImg;
        if (buff1Img!=nullptr)
            delete[] buff1Img;
        if (buff2Img!=nullptr)
            delete[] buff2Img;
    }
    int resolution[2];
    float* workImg;
    float* buff1Img;
    float* buff2Img;
};

std::map<int,CVisionSensorData*> _imgData;

CVisionSensorData* getImgData(int handle)
{
    std::map<int,CVisionSensorData*>::iterator it=_imgData.find(handle);
    if (it!=_imgData.end())
        return(it->second);
    return(nullptr);
}

void removeImgData(int handle)
{
    std::map<int,CVisionSensorData*>::iterator it=_imgData.find(handle);
    if (it!=_imgData.end())
    {
        delete it->second;
        _imgData.erase(it);
    }
}

void setImgData(int handle,CVisionSensorData* imgData)
{
    removeImgData(handle);
    _imgData[handle]=imgData;
}

int getVisionSensorHandle(int handle,int attachedObj)
{
    if (handle==sim_handle_self)
        handle=attachedObj;
    return(handle);
}

// --------------------------------------------------------------------------------------
// simVision.sensorImgToWorkImg
// --------------------------------------------------------------------------------------
#define LUA_SENSORIMGTOWORKIMG_COMMAND_PLUGIN "simVision.sensorImgToWorkImg@Vision"
#define LUA_SENSORIMGTOWORKIMG_COMMAND "simVision.sensorImgToWorkImg"

const int inArgs_SENSORIMGTOWORKIMG[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_SENSORIMGTOWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SENSORIMGTOWORKIMG,inArgs_SENSORIMGTOWORKIMG[0],LUA_SENSORIMGTOWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float* img=simGetVisionSensorImage(handle);
        if (img!=nullptr)
        {
            int res[2];
            simGetVisionSensorResolution(handle,res);
            CVisionSensorData* imgData=getImgData(handle);
            if (imgData==nullptr)
            {
                imgData=new CVisionSensorData();
                imgData->resolution[0]=res[0];
                imgData->resolution[1]=res[1];
                setImgData(handle,imgData);
            }
            if ( (imgData->resolution[0]!=res[0])||(imgData->resolution[1]!=res[1]) )
            {
                if (imgData->workImg!=nullptr)
                {
                    delete[] imgData->workImg;
                    imgData->workImg=new float[res[0]*res[1]*3];
                }
                if (imgData->buff1Img!=nullptr)
                {
                    delete[] imgData->buff1Img;
                    imgData->buff1Img=new float[res[0]*res[1]*3];
                }
                if (imgData->buff2Img!=nullptr)
                {
                    delete[] imgData->buff2Img;
                    imgData->buff2Img=new float[res[0]*res[1]*3];
                }
                imgData->resolution[0]=res[0];
                imgData->resolution[1]=res[1];
            }
            if (imgData->workImg==nullptr)
                imgData->workImg=new float[res[0]*res[1]*3];
            for (size_t i=0;i<res[0]*res[1]*3;i++)
                imgData->workImg[i]=img[i];
            simReleaseBuffer((char*)img);
        }
        else
            simSetLastError(LUA_SENSORIMGTOWORKIMG_COMMAND,"Invalid handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.sensorDepthMapToWorkImg
// --------------------------------------------------------------------------------------
#define LUA_SENSORDEPTHMAPTOWORKIMG_COMMAND_PLUGIN "simVision.sensorDepthMapToWorkImg@Vision"
#define LUA_SENSORDEPTHMAPTOWORKIMG_COMMAND "simVision.sensorDepthMapToWorkImg"

const int inArgs_SENSORDEPTHMAPTOWORKIMG[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_SENSORDEPTHMAPTOWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SENSORDEPTHMAPTOWORKIMG,inArgs_SENSORDEPTHMAPTOWORKIMG[0],LUA_SENSORDEPTHMAPTOWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float* img=simGetVisionSensorDepthBuffer(handle);
        if (img!=nullptr)
        {
            int res[2];
            simGetVisionSensorResolution(handle,res);
            CVisionSensorData* imgData=getImgData(handle);
            if (imgData==nullptr)
            {
                imgData=new CVisionSensorData();
                imgData->resolution[0]=res[0];
                imgData->resolution[1]=res[1];
                setImgData(handle,imgData);
            }
            if ( (imgData->resolution[0]!=res[0])||(imgData->resolution[1]!=res[1]) )
            {
                if (imgData->workImg!=nullptr)
                {
                    delete[] imgData->workImg;
                    imgData->workImg=new float[res[0]*res[1]*3];
                }
                if (imgData->buff1Img!=nullptr)
                {
                    delete[] imgData->buff1Img;
                    imgData->buff1Img=new float[res[0]*res[1]*3];
                }
                if (imgData->buff2Img!=nullptr)
                {
                    delete[] imgData->buff2Img;
                    imgData->buff2Img=new float[res[0]*res[1]*3];
                }
                imgData->resolution[0]=res[0];
                imgData->resolution[1]=res[1];
            }
            if (imgData->workImg==nullptr)
                imgData->workImg=new float[res[0]*res[1]*3];
            for (size_t i=0;i<res[0]*res[1];i++)
            {
                imgData->workImg[3*i+0]=img[i];
                imgData->workImg[3*i+1]=img[i];
                imgData->workImg[3*i+2]=img[i];
            }
            simReleaseBuffer((char*)img);
        }
        else
            simSetLastError(LUA_SENSORDEPTHMAPTOWORKIMG_COMMAND,"Invalid handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.workImgToSensorImg
// --------------------------------------------------------------------------------------
#define LUA_WORKIMGTOSENSORIMG_COMMAND_PLUGIN "simVision.workImgToSensorImg@Vision"
#define LUA_WORKIMGTOSENSORIMG_COMMAND "simVision.workImgToSensorImg"

const int inArgs_WORKIMGTOSENSORIMG[]={
    2,
    sim_script_arg_int32,0,
    sim_script_arg_bool,0,
};

void LUA_WORKIMGTOSENSORIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_WORKIMGTOSENSORIMG,inArgs_WORKIMGTOSENSORIMG[0]-1,LUA_WORKIMGTOSENSORIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        bool removeImg=true;
        if ( (inData->size()>1)&&(inData->at(1).boolData.size()==1) )
            removeImg=inData->at(1).boolData[0];
        if (imgData!=nullptr)
        {
            int res[2];
            simGetVisionSensorResolution(handle,res);
            if ( (imgData->resolution[0]==res[0])&&(imgData->resolution[1]==res[1]) )
                simSetVisionSensorImage(handle,imgData->workImg);
            else
                simSetLastError(LUA_SENSORIMGTOWORKIMG_COMMAND,"Resolution mismatch.");
            if ( (imgData->buff1Img==nullptr)&&(imgData->buff2Img==nullptr)&&removeImg )
                removeImgData(handle); // otherwise, we have to explicitely free the image data with simVision.releaseBuffers
        }
        else
            simSetLastError(LUA_WORKIMGTOSENSORIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.workImgToSensorDepthMap
// --------------------------------------------------------------------------------------
#define LUA_WORKIMGTOSENSORDEPTHMAP_COMMAND_PLUGIN "simVision.workImgToSensorDepthMap@Vision"
#define LUA_WORKIMGTOSENSORDEPTHMAP_COMMAND "simVision.workImgToSensorDepthMap"

const int inArgs_WORKIMGTOSENSORDEPTHMAP[]={
    2,
    sim_script_arg_int32,0,
    sim_script_arg_bool,0,
};

void LUA_WORKIMGTOSENSORDEPTHMAP_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_WORKIMGTOSENSORDEPTHMAP,inArgs_WORKIMGTOSENSORDEPTHMAP[0]-1,LUA_WORKIMGTOSENSORDEPTHMAP_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        bool removeImg=true;
        if ( (inData->size()>1)&&(inData->at(1).boolData.size()==1) )
            removeImg=inData->at(1).boolData[0];
        if (imgData!=nullptr)
        {
            int res[2];
            simGetVisionSensorResolution(handle,res);
            if ( (imgData->resolution[0]==res[0])&&(imgData->resolution[1]==res[1]) )
            {
                float* tmpDepthMap=new float[res[0]*res[1]];
                for (size_t i=0;i<res[0]*res[1];i++)
                    tmpDepthMap[i]=(imgData->workImg[3*i+0]+imgData->workImg[3*i+1]+imgData->workImg[3*i+2])/3.0f;
                simSetVisionSensorImage(handle|sim_handleflag_depthbuffer,tmpDepthMap);
                delete[] tmpDepthMap;
            }
            else
                simSetLastError(LUA_WORKIMGTOSENSORDEPTHMAP_COMMAND,"Resolution mismatch.");
            if ( (imgData->buff1Img==nullptr)&&(imgData->buff2Img==nullptr)&&removeImg )
                removeImgData(handle); // otherwise, we have to explicitely free the image data with simVision.releaseBuffers
        }
        else
            simSetLastError(LUA_WORKIMGTOSENSORDEPTHMAP_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.workImgToBuffer1
// --------------------------------------------------------------------------------------
#define LUA_WORKIMGTOBUFFER1_COMMAND_PLUGIN "simVision.workImgToBuffer1@Vision"
#define LUA_WORKIMGTOBUFFER1_COMMAND "simVision.workImgToBuffer1"

const int inArgs_WORKIMGTOBUFFER1[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_WORKIMGTOBUFFER1_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_WORKIMGTOBUFFER1,inArgs_WORKIMGTOBUFFER1[0],LUA_WORKIMGTOBUFFER1_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff1Img==nullptr)
                imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (size_t i=0;i<imgData->resolution[0]*imgData->resolution[1]*3;i++)
                imgData->buff1Img[i]=imgData->workImg[i];
        }
        else
            simSetLastError(LUA_WORKIMGTOBUFFER1_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.workImgToBuffer2
// --------------------------------------------------------------------------------------
#define LUA_WORKIMGTOBUFFER2_COMMAND_PLUGIN "simVision.workImgToBuffer2@Vision"
#define LUA_WORKIMGTOBUFFER2_COMMAND "simVision.workImgToBuffer2"

const int inArgs_WORKIMGTOBUFFER2[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_WORKIMGTOBUFFER2_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_WORKIMGTOBUFFER2,inArgs_WORKIMGTOBUFFER2[0],LUA_WORKIMGTOBUFFER2_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff2Img==nullptr)
                imgData->buff2Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (size_t i=0;i<imgData->resolution[0]*imgData->resolution[1]*3;i++)
                imgData->buff2Img[i]=imgData->workImg[i];
        }
        else
            simSetLastError(LUA_WORKIMGTOBUFFER2_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.swapBuffers
// --------------------------------------------------------------------------------------
#define LUA_SWAPBUFFERS_COMMAND_PLUGIN "simVision.swapBuffers@Vision"
#define LUA_SWAPBUFFERS_COMMAND "simVision.swapBuffers"

const int inArgs_SWAPBUFFERS[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_SWAPBUFFERS_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SWAPBUFFERS,inArgs_SWAPBUFFERS[0],LUA_SWAPBUFFERS_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff1Img==nullptr)
                imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            if (imgData->buff2Img==nullptr)
                imgData->buff2Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            float* tmp=imgData->buff1Img;
            imgData->buff1Img=imgData->buff2Img;
            imgData->buff2Img=tmp;
        }
        else
            simSetLastError(LUA_SWAPBUFFERS_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.buffer1ToWorkImg
// --------------------------------------------------------------------------------------
#define LUA_BUFFER1TOWORKIMG_COMMAND_PLUGIN "simVision.buffer1ToWorkImg@Vision"
#define LUA_BUFFER1TOWORKIMG_COMMAND "simVision.buffer1ToWorkImg"

const int inArgs_BUFFER1TOWORKIMG[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_BUFFER1TOWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_BUFFER1TOWORKIMG,inArgs_BUFFER1TOWORKIMG[0],LUA_BUFFER1TOWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff1Img==nullptr)
                imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (size_t i=0;i<imgData->resolution[0]*imgData->resolution[1]*3;i++)
                imgData->workImg[i]=imgData->buff1Img[i];
        }
        else
            simSetLastError(LUA_BUFFER1TOWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.buffer2ToWorkImg
// --------------------------------------------------------------------------------------
#define LUA_BUFFER2TOWORKIMG_COMMAND_PLUGIN "simVision.buffer2ToWorkImg@Vision"
#define LUA_BUFFER2TOWORKIMG_COMMAND "simVision.buffer2ToWorkImg"

const int inArgs_BUFFER2TOWORKIMG[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_BUFFER2TOWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_BUFFER2TOWORKIMG,inArgs_BUFFER2TOWORKIMG[0],LUA_BUFFER2TOWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff2Img==nullptr)
                imgData->buff2Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (size_t i=0;i<imgData->resolution[0]*imgData->resolution[1]*3;i++)
                imgData->workImg[i]=imgData->buff2Img[i];
        }
        else
            simSetLastError(LUA_BUFFER2TOWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.swapWorkImgWithBuffer1
// --------------------------------------------------------------------------------------
#define LUA_SWAPWORKIMGWITHBUFFER1_COMMAND_PLUGIN "simVision.swapWorkImgWithBuffer1@Vision"
#define LUA_SWAPWORKIMGWITHBUFFER1_COMMAND "simVision.swapWorkImgWithBuffer1"

const int inArgs_SWAPWORKIMGWITHBUFFER1[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_SWAPWORKIMGWITHBUFFER1_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SWAPWORKIMGWITHBUFFER1,inArgs_SWAPWORKIMGWITHBUFFER1[0],LUA_SWAPWORKIMGWITHBUFFER1_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff1Img==nullptr)
                imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (size_t i=0;i<imgData->resolution[0]*imgData->resolution[1]*3;i++)
            {
                float tmp=imgData->workImg[i];
                imgData->workImg[i]=imgData->buff1Img[i];
                imgData->buff1Img[i]=tmp;
            }
        }
        else
            simSetLastError(LUA_SWAPWORKIMGWITHBUFFER1_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.addWorkImgToBuffer1
// --------------------------------------------------------------------------------------
#define LUA_ADDWORKIMGTOBUFFER1_COMMAND_PLUGIN "simVision.addWorkImgToBuffer1@Vision"
#define LUA_ADDWORKIMGTOBUFFER1_COMMAND "simVision.addWorkImgToBuffer1"

const int inArgs_ADDWORKIMGTOBUFFER1[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_ADDWORKIMGTOBUFFER1_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_ADDWORKIMGTOBUFFER1,inArgs_ADDWORKIMGTOBUFFER1[0],LUA_ADDWORKIMGTOBUFFER1_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff1Img==nullptr)
                imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (size_t i=0;i<imgData->resolution[0]*imgData->resolution[1]*3;i++)
            {
                imgData->buff1Img[i]+=imgData->workImg[i];
                if (imgData->buff1Img[i]>1.0f)
                    imgData->buff1Img[i]=1.0f;
            }
        }
        else
            simSetLastError(LUA_ADDWORKIMGTOBUFFER1_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.subtractWorkImgFromBuffer1
// --------------------------------------------------------------------------------------
#define LUA_SUBTRACTWORKIMGFROMBUFFER1_COMMAND_PLUGIN "simVision.subtractWorkImgFromBuffer1@Vision"
#define LUA_SUBTRACTWORKIMGFROMBUFFER1_COMMAND "simVision.subtractWorkImgFromBuffer1"

const int inArgs_SUBTRACTWORKIMGFROMBUFFER1[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_SUBTRACTWORKIMGFROMBUFFER1_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SUBTRACTWORKIMGFROMBUFFER1,inArgs_SUBTRACTWORKIMGFROMBUFFER1[0],LUA_SUBTRACTWORKIMGFROMBUFFER1_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff1Img==nullptr)
                imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (size_t i=0;i<imgData->resolution[0]*imgData->resolution[1]*3;i++)
            {
                imgData->buff1Img[i]-=imgData->workImg[i];
                if (imgData->buff1Img[i]<0.0f)
                    imgData->buff1Img[i]=0.0f;
            }
        }
        else
            simSetLastError(LUA_SUBTRACTWORKIMGFROMBUFFER1_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.addBuffer1ToWorkImg
// --------------------------------------------------------------------------------------
#define LUA_ADDBUFFER1TOWORKIMG_COMMAND_PLUGIN "simVision.addBuffer1ToWorkImg@Vision"
#define LUA_ADDBUFFER1TOWORKIMG_COMMAND "simVision.addBuffer1ToWorkImg"

const int inArgs_ADDBUFFER1TOWORKIMG[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_ADDBUFFER1TOWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_ADDBUFFER1TOWORKIMG,inArgs_ADDBUFFER1TOWORKIMG[0],LUA_ADDBUFFER1TOWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff1Img==nullptr)
                imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (size_t i=0;i<imgData->resolution[0]*imgData->resolution[1]*3;i++)
            {
                imgData->workImg[i]+=imgData->buff1Img[i];
                if (imgData->workImg[i]>1.0f)
                    imgData->workImg[i]=1.0f;
            }
        }
        else
            simSetLastError(LUA_ADDBUFFER1TOWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.subtractBuffer1FromWorkImg
// --------------------------------------------------------------------------------------
#define LUA_SUBTRACTBUFFER1FROMWORKIMG_COMMAND_PLUGIN "simVision.subtractBuffer1FromWorkImg@Vision"
#define LUA_SUBTRACTBUFFER1FROMWORKIMG_COMMAND "simVision.subtractBuffer1FromWorkImg"

const int inArgs_SUBTRACTBUFFER1FROMWORKIMG[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_SUBTRACTBUFFER1FROMWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SUBTRACTBUFFER1FROMWORKIMG,inArgs_SUBTRACTBUFFER1FROMWORKIMG[0],LUA_SUBTRACTBUFFER1FROMWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff1Img==nullptr)
                imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (size_t i=0;i<imgData->resolution[0]*imgData->resolution[1]*3;i++)
            {
                imgData->workImg[i]-=imgData->buff1Img[i];
                if (imgData->workImg[i]<0.0f)
                    imgData->workImg[i]=0.0f;
            }
        }
        else
            simSetLastError(LUA_SUBTRACTBUFFER1FROMWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.multiplyWorkImgWithBuffer1
// --------------------------------------------------------------------------------------
#define LUA_MULTIPLYWORKIMGWITHBUFFER1_COMMAND_PLUGIN "simVision.multiplyWorkImgWithBuffer1@Vision"
#define LUA_MULTIPLYWORKIMGWITHBUFFER1_COMMAND "simVision.multiplyWorkImgWithBuffer1"

const int inArgs_MULTIPLYWORKIMGWITHBUFFER1[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_MULTIPLYWORKIMGWITHBUFFER1_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_MULTIPLYWORKIMGWITHBUFFER1,inArgs_MULTIPLYWORKIMGWITHBUFFER1[0],LUA_MULTIPLYWORKIMGWITHBUFFER1_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (imgData->buff1Img==nullptr)
                imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (size_t i=0;i<imgData->resolution[0]*imgData->resolution[1]*3;i++)
            {
                imgData->workImg[i]*=imgData->buff1Img[i];
                if (imgData->workImg[i]>1.0f)
                    imgData->workImg[i]=1.0f;
            }
        }
        else
            simSetLastError(LUA_MULTIPLYWORKIMGWITHBUFFER1_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.horizontalFlipWorkImg
// --------------------------------------------------------------------------------------
#define LUA_HORIZONTALFLIPWORKIMG_COMMAND_PLUGIN "simVision.horizontalFlipWorkImg@Vision"
#define LUA_HORIZONTALFLIPWORKIMG_COMMAND "simVision.horizontalFlipWorkImg"

const int inArgs_HORIZONTALFLIPWORKIMG[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_HORIZONTALFLIPWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_HORIZONTALFLIPWORKIMG,inArgs_HORIZONTALFLIPWORKIMG[0],LUA_HORIZONTALFLIPWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            float tmp;
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            for (int i=0;i<sizeX/2;i++)
            {
                for (int j=0;j<sizeY;j++)
                {
                    for (int k=0;k<3;k++)
                    {
                        tmp=imgData->workImg[3*(i+j*sizeX)+k];
                        imgData->workImg[3*(i+j*sizeX)+k]=imgData->workImg[3*((sizeX-1-i)+j*sizeX)+k];
                        imgData->workImg[3*((sizeX-1-i)+j*sizeX)+k]=tmp;
                    }
                }
            }
        }
        else
            simSetLastError(LUA_HORIZONTALFLIPWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.verticalFlipWorkImg
// --------------------------------------------------------------------------------------
#define LUA_VERTICALFLIPWORKIMG_COMMAND_PLUGIN "simVision.verticalFlipWorkImg@Vision"
#define LUA_VERTICALFLIPWORKIMG_COMMAND "simVision.verticalFlipWorkImg"

const int inArgs_VERTICALFLIPWORKIMG[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_VERTICALFLIPWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_VERTICALFLIPWORKIMG,inArgs_VERTICALFLIPWORKIMG[0],LUA_VERTICALFLIPWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            float tmp;
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            for (int i=0;i<sizeX;i++)
            {
                for (int j=0;j<sizeY/2;j++)
                {
                    for (int k=0;k<3;k++)
                    {
                        tmp=imgData->workImg[3*(i+j*sizeX)+k];
                        imgData->workImg[3*(i+j*sizeX)+k]=imgData->workImg[3*(i+(sizeY-1-j)*sizeX)+k];
                        imgData->workImg[3*(i+(sizeY-1-j)*sizeX)+k]=tmp;
                    }
                }
            }
        }
        else
            simSetLastError(LUA_VERTICALFLIPWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.uniformImgToWorkImg
// --------------------------------------------------------------------------------------
#define LUA_UNIFORMIMGTOWORKIMG_COMMAND_PLUGIN "simVision.uniformImgToWorkImg@Vision"
#define LUA_UNIFORMIMGTOWORKIMG_COMMAND "simVision.uniformImgToWorkImg"

const int inArgs_UNIFORMIMGTOWORKIMG[]={
    2,
    sim_script_arg_int32,0,
    sim_script_arg_float|sim_lua_arg_table,3,
};

void LUA_UNIFORMIMGTOWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_UNIFORMIMGTOWORKIMG,inArgs_UNIFORMIMGTOWORKIMG[0],LUA_UNIFORMIMGTOWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float* p=&(inData->at(1).floatData[0]);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            for (int i=0;i<imgData->resolution[0]*imgData->resolution[1];i++)
            {
                imgData->workImg[3*i+0]=p[0];
                imgData->workImg[3*i+1]=p[1];
                imgData->workImg[3*i+2]=p[2];
            }
        }
        else
            simSetLastError(LUA_UNIFORMIMGTOWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.normalizeWorkImg
// --------------------------------------------------------------------------------------
#define LUA_NORMALIZEWORKIMG_COMMAND_PLUGIN "simVision.normalizeWorkImg@Vision"
#define LUA_NORMALIZEWORKIMG_COMMAND "simVision.normalizeWorkImg"

const int inArgs_NORMALIZEWORKIMG[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_NORMALIZEWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_NORMALIZEWORKIMG,inArgs_NORMALIZEWORKIMG[0],LUA_NORMALIZEWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int s=imgData->resolution[0]*imgData->resolution[1]*3;
            float maxCol=0.0f;
            float minCol=1.0f;
            for (int i=0;i<s;i++)
            {
                if (imgData->workImg[i]>maxCol)
                    maxCol=imgData->workImg[i];
                if (imgData->workImg[i]<minCol)
                    minCol=imgData->workImg[i];
            }
            if (maxCol-minCol!=0.0f)
            {
                float mul=1.0f/(maxCol-minCol);
                for (int i=0;i<s;i++)
                    imgData->workImg[i]=(imgData->workImg[i]-minCol)*mul;
            }
        }
        else
            simSetLastError(LUA_NORMALIZEWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.colorSegmentationOnWorkImg
// --------------------------------------------------------------------------------------
#define LUA_COLORSEGMENTATIONONWORKIMG_COMMAND_PLUGIN "simVision.colorSegmentationOnWorkImg@Vision"
#define LUA_COLORSEGMENTATIONONWORKIMG_COMMAND "simVision.colorSegmentationOnWorkImg"

const int inArgs_COLORSEGMENTATIONONWORKIMG[]={
    2,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
};

void LUA_COLORSEGMENTATIONONWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_COLORSEGMENTATIONONWORKIMG,inArgs_COLORSEGMENTATIONONWORKIMG[0],LUA_COLORSEGMENTATIONONWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float p=inData->at(1).floatData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int s=imgData->resolution[0]*imgData->resolution[1];
            std::vector<float> goodColors;
            float squaredDistance=p*p;
            for (int i=0;i<s;i++)
            {
                bool found=false;
                for (size_t j=0;j<goodColors.size()/3;j++)
                {
                    float r=imgData->workImg[3*i+0]-goodColors[3*j+0];
                    float g=imgData->workImg[3*i+1]-goodColors[3*j+1];
                    float b=imgData->workImg[3*i+2]-goodColors[3*j+2];
                    float d=r*r+g*g+b*b;
                    if (d<squaredDistance)
                    {
                        found=true;
                        imgData->workImg[3*i+0]=goodColors[3*j+0];
                        imgData->workImg[3*i+1]=goodColors[3*j+1];
                        imgData->workImg[3*i+2]=goodColors[3*j+2];
                        break;
                    }
                }
                if (!found)
                {
                    goodColors.push_back(imgData->workImg[3*i+0]);
                    goodColors.push_back(imgData->workImg[3*i+1]);
                    goodColors.push_back(imgData->workImg[3*i+2]);
                }
            }
        }
        else
            simSetLastError(LUA_COLORSEGMENTATIONONWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

void colorFromIntensity(float intensity,int colorTable,float col[3])
{
    if (intensity>1.0f)
        intensity=1.0f;
    if (intensity<0.0f)
        intensity=0.0f;
    const float c[12]={0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,1.0f,0.0f,0.0f,1.0f,1.0f,0.0f};
    int d=int(intensity*3);
    if (d>2)
        d=2;
    float r=(intensity-float(d)/3.0f)*3.0f;
    col[0]=c[3*d+0]*(1.0f-r)+c[3*(d+1)+0]*r;
    col[1]=c[3*d+1]*(1.0f-r)+c[3*(d+1)+1]*r;
    col[2]=c[3*d+2]*(1.0f-r)+c[3*(d+1)+2]*r;
}

// --------------------------------------------------------------------------------------
// simVision.intensityScaleOnWorkImg
// --------------------------------------------------------------------------------------
#define LUA_INTENSITYSCALEONWORKIMG_COMMAND_PLUGIN "simVision.intensityScaleOnWorkImg@Vision"
#define LUA_INTENSITYSCALEONWORKIMG_COMMAND "simVision.intensityScaleOnWorkImg"

const int inArgs_INTENSITYSCALEONWORKIMG[]={
    4,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_bool,0,
};

void LUA_INTENSITYSCALEONWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_INTENSITYSCALEONWORKIMG,inArgs_INTENSITYSCALEONWORKIMG[0],LUA_INTENSITYSCALEONWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float start=inData->at(1).floatData[0];
        float end=inData->at(2).floatData[0];
        bool greyScale=inData->at(3).boolData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            float b=start;
            float a=end-b;
            // intensity first transformed like: intensity=a*intensity+b
            int s=imgData->resolution[0]*imgData->resolution[1];
            float intensity;
            float col[3];
            for (int i=0;i<s;i++)
            {
                intensity=(imgData->workImg[3*i+0]+imgData->workImg[3*i+1]+imgData->workImg[3*i+2])/3.0f;
                intensity=a*intensity+b;
                if (greyScale)
                { // grey scale
                    imgData->workImg[3*i+0]=intensity;
                    imgData->workImg[3*i+1]=intensity;
                    imgData->workImg[3*i+2]=intensity;
                }
                else
                { // intensity scale
                    colorFromIntensity(intensity,0,col);
                    imgData->workImg[3*i+0]=col[0];
                    imgData->workImg[3*i+1]=col[1];
                    imgData->workImg[3*i+2]=col[2];
                }
            }
        }
        else
            simSetLastError(LUA_INTENSITYSCALEONWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

void rgbToHsl(float rgb[3],float hsl[3])
{
    float r=rgb[0];
    float g=rgb[1];
    float b=rgb[2];
    float h,s,l,delta;
    float cmax=std::max<float>(r,std::max<float>(g,b));
    float cmin=std::min<float>(r,std::min<float>(g,b));
    l=(cmax+cmin)/2.0f;
    if (cmax==cmin)
    {
        s=0.0f;
        h=0.0f;
    }
    else
    {
        if(l<0.5f)
            s=(cmax-cmin)/(cmax+cmin);
        else
            s=(cmax-cmin)/(2.0f-cmax-cmin);
        delta=cmax-cmin;
        if (r==cmax)
            h=(g-b)/delta;
        else
            if (g==cmax)
                h=2.0f+(b-r)/delta;
            else
                h=4.0f+(r-g)/delta;
        h=h/6.0f;
        if (h<0.0f)
            h=h+1.0f;
    }
    hsl[0]=h;
    hsl[1]=s;
    hsl[2]=l;
}

float hueToRgb(float m1,float m2,float h)
{
    if (h<0.0f)
        h=h+1.0f;
    if (h>1.0f)
        h=h-1.0f;
    if (6.0f*h<1.0f)
        return(m1+(m2-m1)*h*6.0f);
    if (2.0f*h<1.0f)
        return(m2);
    if (3.0f*h<2.0f)
        return(m1+(m2-m1)*((2.0f/3.0f)-h)*6.0f);
    return(m1);
}

void hslToRgb(float hsl[3],float rgb[3])
{
    float h=hsl[0];
    float s=hsl[1];
    float l=hsl[2];
    float m1,m2;

    if (s==0.0f)
    {
        rgb[0]=l;
        rgb[1]=l;
        rgb[2]=l;
    }
    else
    {
        if (l<=0.5f)
            m2=l*(1.0f+s);
        else
            m2=l+s-l*s;
        m1=2.0f*l-m2;
        rgb[0]=hueToRgb(m1,m2,h+1.0f/3.0f);
        rgb[1]=hueToRgb(m1,m2,h);
        rgb[2]=hueToRgb(m1,m2,h-1.0f/3.0f);
    }
}


// --------------------------------------------------------------------------------------
// simVision.selectiveColorOnWorkImg
// --------------------------------------------------------------------------------------
#define LUA_SELECTIVECOLORONONWORKIMG_COMMAND_PLUGIN "simVision.selectiveColorOnWorkImg@Vision"
#define LUA_SELECTIVECOLORONONWORKIMG_COMMAND "simVision.selectiveColorOnWorkImg"

const int inArgs_SELECTIVECOLORONONWORKIMG[]={
    6,
    sim_script_arg_int32,0,
    sim_script_arg_float|sim_lua_arg_table,3,
    sim_script_arg_float|sim_lua_arg_table,3,
    sim_script_arg_bool,0,
    sim_script_arg_bool,0,
    sim_script_arg_bool,0,
};

void LUA_SELECTIVECOLORONONWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SELECTIVECOLORONONWORKIMG,inArgs_SELECTIVECOLORONONWORKIMG[0],LUA_SELECTIVECOLORONONWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float* pCol=&(inData->at(1).floatData)[0];
        float* pTol=&(inData->at(2).floatData)[0];
        bool inRgbDim=inData->at(3).boolData[0];
        bool keep=inData->at(4).boolData[0];
        bool toBuffer1=inData->at(5).boolData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (toBuffer1)
            {
                if (imgData->buff1Img==nullptr)
                    imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            }
            int s=imgData->resolution[0]*imgData->resolution[1];
            float col[3];
            float rgb[3];
            float lowTol[3]={pCol[0]-pTol[0],pCol[1]-pTol[1],pCol[2]-pTol[2]};
            float upTol[3]={pCol[0]+pTol[0],pCol[1]+pTol[1],pCol[2]+pTol[2]};
            float lowTolUpHue=1.0f+lowTol[0];
            float upTolLowHue=upTol[0]-1.0f;
            for (int i=0;i<s;i++)
            {
                if (inRgbDim)
                { // rgb dimension
                    col[0]=imgData->workImg[3*i+0];
                    col[1]=imgData->workImg[3*i+1];
                    col[2]=imgData->workImg[3*i+2];
                }
                else
                { // hsl dimension
                    rgb[0]=imgData->workImg[3*i+0];
                    rgb[1]=imgData->workImg[3*i+1];
                    rgb[2]=imgData->workImg[3*i+2];
                    rgbToHsl(rgb,col);
                }
                bool outOfTol;
                if (inRgbDim)
                { // rgb dimension
                    outOfTol=((col[0]>upTol[0])||
                        (col[0]<lowTol[0])||
                        (col[1]>upTol[1])||
                        (col[1]<lowTol[1])||
                        (col[2]>upTol[2])||
                        (col[2]<lowTol[2]));
                }
                else
                { // hsl dimension
                    outOfTol=((col[1]>upTol[1])||
                        (col[1]<lowTol[1])||
                        (col[2]>upTol[2])||
                        (col[2]<lowTol[2]));
                    if ( (!outOfTol)&&((upTol[0]-lowTol[0])<1.0f) )
                    { // Check the Hue value (special handling):
                        outOfTol=( (col[0]>upTol[0])&&(col[0]<lowTolUpHue) )||( (col[0]<lowTol[0])&&(col[0]>upTolLowHue) );
                    }
                }

                if (outOfTol)
                {
                    if (keep)
                    { // color not within tolerance, we remove it
                        if (toBuffer1)
                        { // we copy the removed part to buffer 1
                            imgData->buff1Img[3*i+0]=imgData->workImg[3*i+0];
                            imgData->buff1Img[3*i+1]=imgData->workImg[3*i+1];
                            imgData->buff1Img[3*i+2]=imgData->workImg[3*i+2];
                        }
                        imgData->workImg[3*i+0]=0.0f;
                        imgData->workImg[3*i+1]=0.0f;
                        imgData->workImg[3*i+2]=0.0f;
                    }
                    else
                    { // color within tolerance
                        if (toBuffer1)
                        { // we mark as black in buffer 1 the parts not removed
                            imgData->buff1Img[3*i+0]=0.0f;
                            imgData->buff1Img[3*i+1]=0.0f;
                            imgData->buff1Img[3*i+2]=0.0f;
                        }
                    }
                }
                else
                {
                    if (!keep)
                    { // color not within tolerance, we remove it
                        if (toBuffer1)
                        { // we copy the removed part to buffer 1
                            imgData->buff1Img[3*i+0]=imgData->workImg[3*i+0];
                            imgData->buff1Img[3*i+1]=imgData->workImg[3*i+1];
                            imgData->buff1Img[3*i+2]=imgData->workImg[3*i+2];
                        }
                        imgData->workImg[3*i+0]=0.0f;
                        imgData->workImg[3*i+1]=0.0f;
                        imgData->workImg[3*i+2]=0.0f;
                    }
                    else
                    { // color within tolerance
                        if (toBuffer1)
                        { // we mark as black in buffer 1 the parts not removed
                            imgData->buff1Img[3*i+0]=0.0f;
                            imgData->buff1Img[3*i+1]=0.0f;
                            imgData->buff1Img[3*i+2]=0.0f;
                        }
                    }
                }
            }
        }
        else
            simSetLastError(LUA_SELECTIVECOLORONONWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.scaleAndOffsetWorkImg
// --------------------------------------------------------------------------------------
#define LUA_SCALEANDOFFSETWORKIMG_COMMAND_PLUGIN "simVision.scaleAndOffsetWorkImg@Vision"
#define LUA_SCALEANDOFFSETWORKIMG_COMMAND "simVision.scaleAndOffsetWorkImg"

const int inArgs_SCALEANDOFFSETWORKIMG[]={
    5,
    sim_script_arg_int32,0,
    sim_script_arg_float|sim_lua_arg_table,3,
    sim_script_arg_float|sim_lua_arg_table,3,
    sim_script_arg_float|sim_lua_arg_table,3,
    sim_script_arg_bool,0,
};

void LUA_SCALEANDOFFSETWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SCALEANDOFFSETWORKIMG,inArgs_SCALEANDOFFSETWORKIMG[0],LUA_SCALEANDOFFSETWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float* pCol=&(inData->at(1).floatData)[0];
        float* pScale=&(inData->at(2).floatData)[0];
        float* pOff=&(inData->at(3).floatData)[0];
        bool inRgbDim=inData->at(4).boolData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int s=imgData->resolution[0]*imgData->resolution[1];
            float col[3];
            float rgb[3];
            for (int i=0;i<s;i++)
            {
                if (inRgbDim)
                { // rgb dimension
                    col[0]=imgData->workImg[3*i+0];
                    col[1]=imgData->workImg[3*i+1];
                    col[2]=imgData->workImg[3*i+2];
                }
                else
                { // hsl dimension
                    rgb[0]=imgData->workImg[3*i+0];
                    rgb[1]=imgData->workImg[3*i+1];
                    rgb[2]=imgData->workImg[3*i+2];
                    rgbToHsl(rgb,col);
                }
                col[0]=pOff[0]+(col[0]+pCol[0])*pScale[0];
                col[1]=pOff[1]+(col[1]+pCol[1])*pScale[1];
                col[2]=pOff[2]+(col[2]+pCol[2])*pScale[2];
                if (col[0]<0.0f)
                    col[0]=0.0f;
                if (col[0]>1.0f)
                    col[0]=1.0f;
                if (col[1]<0.0f)
                    col[1]=0.0f;
                if (col[1]>1.0f)
                    col[1]=1.0f;
                if (col[2]<0.0f)
                    col[2]=0.0f;
                if (col[2]>1.0f)
                    col[2]=1.0f;
                if (inRgbDim)
                { // rgb dimension
                    imgData->workImg[3*i+0]=col[0];
                    imgData->workImg[3*i+1]=col[1];
                    imgData->workImg[3*i+2]=col[2];
                }
                else
                { // hsl dimension
                    rgb[0]=imgData->workImg[3*i+0];
                    rgb[1]=imgData->workImg[3*i+1];
                    rgb[2]=imgData->workImg[3*i+2];
                    hslToRgb(col,imgData->workImg+3*i+0);
                }
            }
        }
        else
            simSetLastError(LUA_SCALEANDOFFSETWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

float angleMinusAlpha(float angle,float alpha)
{
    double sinAngle0=sin(double(angle));
    double sinAngle1=sin(double(alpha));
    double cosAngle0=cos(double(angle));
    double cosAngle1=cos(double(alpha));
    double sin_da=sinAngle0*cosAngle1-cosAngle0*sinAngle1;
    double cos_da=cosAngle0*cosAngle1+sinAngle0*sinAngle1;
    double angle_da=atan2(sin_da,cos_da);
    return(float(angle_da));
}

void drawLine(CVisionSensorData* imgData,const float col[3],int x0,int y0,int x1,int y1)
{
    int sizeX=imgData->resolution[0];
    int sizeY=imgData->resolution[1];
    int x=x0;
    int y=y0;
    int dx=x1-x0;
    int dy=y1-y0;
    int incX=1;
    int incY=1;
    if (dx<0)
        incX=-1;
    if (dy<0)
        incY=-1;
    if ( abs(dx)>=abs(dy) )
    {
        int p=abs(dx);
        while(x!=x1)
        {
            p-=abs(dy);
            if ( (x>=0)&&(x<sizeX)&&(y>=0)&&(y<sizeY) )
            {
                imgData->workImg[3*(x+y*sizeX)+0]=col[0];
                imgData->workImg[3*(x+y*sizeX)+1]=col[1];
                imgData->workImg[3*(x+y*sizeX)+2]=col[2];
            }
            if (p<0)
            {
                y+=incY;
                p+=abs(dx);
            }
            x+=incX;
        }
    }
    else
    {
        int p=abs(dy);
        while(y!=y1)
        {
            p-=abs(dx);
            if ( (x>=0)&&(x<sizeX)&&(y>=0)&&(y<sizeY) )
            {
                imgData->workImg[3*(x+y*sizeX)+0]=col[0];
                imgData->workImg[3*(x+y*sizeX)+1]=col[1];
                imgData->workImg[3*(x+y*sizeX)+2]=col[2];
            }
            if (p<0)
            {
                x+=incX;
                p+=abs(dy);
            }
            y+=incY;
        }
    }
}

void drawLines(CVisionSensorData* imgData,const std::vector<int>& overlayLines,const float col[3])
{
    for (size_t i=0;i<overlayLines.size()/4;i++)
        drawLine(imgData,col,overlayLines[4*i+0],overlayLines[4*i+1],overlayLines[4*i+2],overlayLines[4*i+3]);
}

// --------------------------------------------------------------------------------------
// simVision.binaryWorkImg
// --------------------------------------------------------------------------------------
#define LUA_BINARYWORKIMG_COMMAND_PLUGIN "simVision.binaryWorkImg@Vision"
#define LUA_BINARYWORKIMG_COMMAND "simVision.binaryWorkImg"

const int inArgs_BINARYWORKIMG[]={
    13,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_bool,0,
    sim_script_arg_float|sim_script_arg_table,3,
};

void LUA_BINARYWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    bool retVal=false;
    std::vector<float> returnData;
    if (D.readDataFromStack(p->stackID,inArgs_BINARYWORKIMG,inArgs_BINARYWORKIMG[0]-1,LUA_BINARYWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float threshold=inData->at(1).floatData[0];
        float oneProp=inData->at(2).floatData[0];
        float oneTol=inData->at(3).floatData[0];
        float xCenter=inData->at(4).floatData[0];
        float xCenterTol=inData->at(5).floatData[0];
        float yCenter=inData->at(6).floatData[0];
        float yCenterTol=inData->at(7).floatData[0];
        float orient=inData->at(8).floatData[0];
        float orientTol=inData->at(9).floatData[0];
        float roundV=inData->at(10).floatData[0];
        bool triggerEnabled=inData->at(11).boolData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            float col[3]={1.0f,0.0f,1.0f};
            bool overlay=false;
            if ( (inData->size()>=13)&&(inData->at(12).floatData.size()==3) )
            {
                col[0]=inData->at(12).floatData[0];
                col[1]=inData->at(12).floatData[1];
                col[2]=inData->at(12).floatData[2];
                overlay=true;
            }
            std::vector<int> overlayLines;
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            float area=0.0f;
            float proportion=0.0f;
            float cmx=0.0f;
            float cmy=0.0f;
            float angle=0.0f;
            float roundness=1.0f;
            for (int i=0;i<sizeX;i++)
            {
                for (int j=0;j<sizeY;j++)
                {
                    float intensity=(imgData->workImg[3*(i+j*sizeX)+0]+imgData->workImg[3*(i+j*sizeX)+1]+imgData->workImg[3*(i+j*sizeX)+2])/3.0f;
                    if (intensity>=threshold)
                    { // Binary 1
                        imgData->workImg[3*(i+j*sizeX)+0]=1.0f;
                        imgData->workImg[3*(i+j*sizeX)+1]=1.0f;
                        imgData->workImg[3*(i+j*sizeX)+2]=1.0f;
                        area+=1.0f;
                        cmx+=float(i);
                        cmy+=float(j);
                    }
                    else
                    { // Binary 0
                        imgData->workImg[3*(i+j*sizeX)+0]=0.0f;
                        imgData->workImg[3*(i+j*sizeX)+1]=0.0f;
                        imgData->workImg[3*(i+j*sizeX)+2]=0.0f;
                    }
                }
            }
            proportion=area/float(sizeX*sizeY);
            if (area!=0.0f)
            {
                cmx/=area;
                cmy/=area;

                float a=0.0f;
                float b=0.0f;
                float c=0.0f;
                float tmpX,tmpY;
                for (int i=0;i<sizeX;i++)
                {
                    for (int j=0;j<sizeY;j++)
                    {
                        if (imgData->workImg[3*(i+j*sizeX)+0]!=0.0f)
                        { // Binary 1
                            tmpX=float(i)-cmx;
                            tmpY=float(j)-cmy;
                            a+=tmpX*tmpX;
                            b+=tmpX*tmpY;
                            c+=tmpY*tmpY;
                        }
                    }
                }
                b*=2.0f;
                if ((b!=0.0f)||(a!=c))
                {
                    float denom=sqrt(b*b+(a-c)*(a-c));
                    float sin2ThetaMax=-b/denom;
                    float sin2ThetaMin=b/denom;
                    float cos2ThetaMax=-(a-c)/denom;
                    float cos2ThetaMin=(a-c)/denom;
                    float iMax=0.5f*(c+a)-0.5f*(a-c)*cos2ThetaMax-0.5f*b*sin2ThetaMax;
                    float iMin=0.5f*(c+a)-0.5f*(a-c)*cos2ThetaMin-0.5f*b*sin2ThetaMin;
                    roundness=iMin/iMax;
                    float theta=cos2ThetaMin;
                    if (theta>=1.0f)
                        theta=0.0f;
                    else if (theta<=-1.0f)
                        theta=3.14159265f;
                    else
                        theta=acosf(theta);
                    theta*=0.5f;
                    if (sin2ThetaMin<0.0f)
                        theta*=-1.0f;
                    angle=theta;
                    if (overlay)
                    {
                        float rcm[2]={cmx/float(sizeX),cmy/float(sizeY)};
                        float l=0.3f-roundness*0.25f;
                        overlayLines.push_back(int(float(sizeX)*(rcm[0]-cos(theta)*l)));
                        overlayLines.push_back(int(float(sizeY)*(rcm[1]-sin(theta)*l)));
                        overlayLines.push_back(int(float(sizeX)*(rcm[0]+cos(theta)*l)));
                        overlayLines.push_back(int(float(sizeY)*(rcm[1]+sin(theta)*l)));
                    }
                }
                if (overlay)
                { // visualize the CM
                    int rcm[2]={int(cmx+0.5f),int(cmy+0.5f)};
                    overlayLines.push_back(rcm[0]);
                    overlayLines.push_back(rcm[1]-4);
                    overlayLines.push_back(rcm[0]);
                    overlayLines.push_back(rcm[1]+4);
                    overlayLines.push_back(rcm[0]-4);
                    overlayLines.push_back(rcm[1]);
                    overlayLines.push_back(rcm[0]+4);
                    overlayLines.push_back(rcm[1]);
                }
            }
            else
            {
                cmx=0.5f;
                cmy=0.5f;
            }
            returnData.push_back(proportion);
            returnData.push_back(cmx/float(sizeX));
            returnData.push_back(cmy/float(sizeY));
            returnData.push_back(angle);
            returnData.push_back(roundness);
            // Now check if we have to trigger:
            if (triggerEnabled)
            { // we might have to trigger!
                if (fabs(oneProp-proportion)<oneTol)
                { // within proportions
                    if (fabs(xCenter-cmx/float(sizeX))<xCenterTol)
                    { // within cm x-pos
                        if (fabs(yCenter-cmy/float(sizeY))<yCenterTol)
                        { // within cm y-pos
                            float d=fabs(angleMinusAlpha(orient,angle));
                            if (d<orientTol)
                            { // within angular tolerance
                                retVal=(roundness<=roundV);
                            }
                        }
                    }
                }
            }
            if (overlayLines.size()>0)
                drawLines(imgData,overlayLines,col);
        }
        else
            simSetLastError(LUA_BINARYWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(retVal));
    if (returnData.size()>0)
        D.pushOutData(CScriptFunctionDataItem((char*)(&returnData[0]),returnData.size()*sizeof(float))); // packed data is much faster in Lua
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.blobDetectionOnWorkImg
// --------------------------------------------------------------------------------------
#define LUA_BLOBDETECTIONONWORKIMG_COMMAND_PLUGIN "simVision.blobDetectionOnWorkImg@Vision"
#define LUA_BLOBDETECTIONONWORKIMG_COMMAND "simVision.blobDetectionOnWorkImg"

const int inArgs_BLOBDETECTIONONWORKIMG[]={
    5,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
    sim_script_arg_float,0,
    sim_script_arg_bool,0,
    sim_script_arg_float|sim_script_arg_table,3,
};

void LUA_BLOBDETECTIONONWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    bool retVal=false;
    std::vector<float> returnData;
    if (D.readDataFromStack(p->stackID,inArgs_BLOBDETECTIONONWORKIMG,inArgs_BLOBDETECTIONONWORKIMG[0]-1,LUA_BLOBDETECTIONONWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float threshold=inData->at(1).floatData[0];
        float minBlobSize=inData->at(2).floatData[0];
        bool diffColor=inData->at(3).boolData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            float col[3]={1.0f,0.0f,1.0f};
            bool overlay=false;
            if ( (inData->size()>=5)&&(inData->at(4).floatData.size()==3) )
            {
                col[0]=inData->at(4).floatData[0];
                col[1]=inData->at(4).floatData[1];
                col[2]=inData->at(4).floatData[2];
                overlay=true;
            }
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            int s=sizeX*sizeY;
            const float colorsR[12]={1.0f,0.0f,1.0f,0.0f,1.0f,0.0f,1.0f};
            const float colorsG[12]={0.0f,1.0f,1.0f,0.0f,0.0f,1.0f,1.0f};
            const float colorsB[12]={0.0f,0.0f,0.0f,1.0f,1.0f,1.0f,1.0f};

            int* cellAppart=new int[s];
            int currentCellID=0;
            std::vector<std::vector<int>*> cellEquivalencies;
            for (int j=0;j<sizeY;j++)
            {
                for (int i=0;i<sizeX;i++)
                {
                    float intensity=(imgData->workImg[3*(i+j*sizeX)+0]+imgData->workImg[3*(i+j*sizeX)+1]+imgData->workImg[3*(i+j*sizeX)+2])/3.0f;
                    if (intensity>=threshold)
                    { // Binary 1
                        // Check the 4 neighbours:
                        int neighbourCellIDs[4]={99999,99999,99999,99999};
                        if (i>0)
                        {
                            neighbourCellIDs[0]=cellAppart[(i-1)+j*sizeX];
                            if (j>0)
                                neighbourCellIDs[1]=cellAppart[(i-1)+(j-1)*sizeX];
                        }
                        if (j>0)
                            neighbourCellIDs[2]=cellAppart[i+(j-1)*sizeX];
                        if ((i<sizeX-1)&&(j>0))
                            neighbourCellIDs[3]=cellAppart[(i+1)+(j-1)*sizeX];
                        int cellID=neighbourCellIDs[0];
                        if (neighbourCellIDs[1]<cellID)
                            cellID=neighbourCellIDs[1];
                        if (neighbourCellIDs[2]<cellID)
                            cellID=neighbourCellIDs[2];
                        if (neighbourCellIDs[3]<cellID)
                            cellID=neighbourCellIDs[3];
                        if (cellID==99999)
                        {
                            cellID=currentCellID++;
                            cellEquivalencies.push_back(new std::vector<int>);
                            cellEquivalencies[cellEquivalencies.size()-1]->push_back(cellID);
                        }
                        else
                        {
                            for (int k=0;k<4;k++)
                            {
                                if ( (neighbourCellIDs[k]!=99999)&&(neighbourCellIDs[k]!=cellID) )
                                { // Cell is equivalent!
                                    int classCellID=-1;
                                    for (int l=0;l<int(cellEquivalencies.size());l++)
                                    {
                                        for (int m=0;m<int(cellEquivalencies[l]->size());m++)
                                        {
                                            if (cellEquivalencies[l]->at(m)==cellID)
                                            {
                                                classCellID=l;
                                                break;
                                            }
                                        }
                                        if (classCellID!=-1)
                                            break;
                                    }


                                    int classNeighbourCellID=-1;
                                    for (int l=0;l<int(cellEquivalencies.size());l++)
                                    {
                                        for (int m=0;m<int(cellEquivalencies[l]->size());m++)
                                        {
                                            if (cellEquivalencies[l]->at(m)==neighbourCellIDs[k])
                                            {
                                                classNeighbourCellID=l;
                                                break;
                                            }
                                        }
                                        if (classNeighbourCellID!=-1)
                                            break;
                                    }
                                    if (classCellID!=classNeighbourCellID)
                                    { // We have to merge the two classes:
                                        cellEquivalencies[classCellID]->insert(cellEquivalencies[classCellID]->end(),cellEquivalencies[classNeighbourCellID]->begin(),cellEquivalencies[classNeighbourCellID]->end());
                                        delete cellEquivalencies[classNeighbourCellID];
                                        cellEquivalencies.erase(cellEquivalencies.begin()+classNeighbourCellID);
                                    }
                                }
                            }
                        }
                        cellAppart[i+j*sizeX]=cellID;
                    }
                    else
                    { // Binary 0
                        if (diffColor)
                        {
                            imgData->workImg[3*(i+j*sizeX)+0]=0.0f;
                            imgData->workImg[3*(i+j*sizeX)+1]=0.0f;
                            imgData->workImg[3*(i+j*sizeX)+2]=0.0f;
                        }
                        cellAppart[i+j*sizeX]=99999;
                    }
                }
            }
            int* classIDs=new int[currentCellID];
            for (int i=0;i<int(cellEquivalencies.size());i++)
            {
                for (int j=0;j<int(cellEquivalencies[i]->size());j++)
                    classIDs[cellEquivalencies[i]->at(j)]=i;
            }
            std::vector<std::vector<float>*> vertices;
            const int BLOBDATSIZE=6;
            float* blobData=new float[BLOBDATSIZE*currentCellID];
            for (int i=0;i<currentCellID;i++)
            {
                vertices.push_back(new std::vector<float>);
                blobData[BLOBDATSIZE*i+0]=0.0f; // the number of pixels
            }


            for (int j=0;j<sizeY;j++)
            {
                for (int i=0;i<sizeX;i++)
                {
                    int b=cellAppart[i+j*sizeX];
                    if (b!=99999)
                    {
                        float v=0.8f-float(classIDs[b]/7)*0.2f;
                        while (v<0.19f)
                            v+=0.7f;
                        if (diffColor)
                        {
                            imgData->workImg[3*(i+j*sizeX)+0]=colorsR[classIDs[b]%7]*v;
                            imgData->workImg[3*(i+j*sizeX)+1]=colorsG[classIDs[b]%7]*v;
                            imgData->workImg[3*(i+j*sizeX)+2]=colorsB[classIDs[b]%7]*v;
                        }

                        if (    (i==0)||(i==sizeX-1)||(j==0)||(j==sizeY-1)||
                                (cellAppart[(i-1)+j*sizeX]==99999)||(cellAppart[(i+1)+j*sizeX]==99999)||
                                (cellAppart[i+(j-1)*sizeX]==99999)||(cellAppart[i+(j+1)*sizeX]==99999)||
                                (cellAppart[(i-1)+(j-1)*sizeX]==99999)||(cellAppart[(i-1)+(j+1)*sizeX]==99999)||
                                (cellAppart[(i+1)+(j-1)*sizeX]==99999)||(cellAppart[(i+1)+(j+1)*sizeX]==99999) )
                        {
                            vertices[classIDs[b]]->push_back(float(i));
                            vertices[classIDs[b]]->push_back(float(j));
                        }
                        blobData[BLOBDATSIZE*classIDs[b]+0]++;
                    }
                }
            }

            for (int j=0;j<sizeY;j++)
            {
                for (int i=0;i<sizeX;i++)
                {
                    int b=cellAppart[i+j*sizeX];
                    if (b!=99999)
                    {
                        float relSize=blobData[BLOBDATSIZE*classIDs[b]+0]/float(sizeX*sizeY); // relative size of the blob
                        if (relSize<minBlobSize)
                        { // the blob is too small
                            imgData->workImg[3*(i+j*sizeX)+0]=0.0;
                            imgData->workImg[3*(i+j*sizeX)+1]=0.0;
                            imgData->workImg[3*(i+j*sizeX)+2]=0.0;
                        }
                    }
                }
            }

            int blobCount=0;
            returnData.clear();
            returnData.push_back(float(blobCount)); // We have to actualize this at the end!!!
            returnData.push_back(6.0f);
            std::vector<int> overlayLines;
            for (int i=0;i<int(vertices.size());i++)
            { // For each blob...

                if (vertices[i]->size()!=0)
                {
                    float relSize=blobData[BLOBDATSIZE*i+0]/float(sizeX*sizeY); // relative size of the blob
                    if (relSize>=minBlobSize)
                    { // the blob is large enough
                        blobCount++;
                        float bestOrientation[6]={0.0f,99999999999999999999999.0f,0.0f,0.0f,0.0f,0.0f};
                        float previousDa=0.392699081f; // 22.5 degrees
                        for (int j=0;j<4;j++)
                        { // Try 4 orientations..
                            float a=previousDa*float(j);
                            float cosa=cos(a);
                            float sina=sin(a);
                            float minV[2]={99999.0f,99999.0f};
                            float maxV[2]={-99999.0f,-99999.0f};
                            for (int j=0;j<int(vertices[i]->size()/2);j++)
                            {
                                float ox=(*vertices[i])[2*j+0];
                                float oy=(*vertices[i])[2*j+1];
                                float x=ox*cosa-oy*sina;
                                float y=ox*sina+oy*cosa;
                                if (x<minV[0])
                                    minV[0]=x;
                                if (x>maxV[0])
                                    maxV[0]=x;
                                if (y<minV[1])
                                    minV[1]=y;
                                if (y>maxV[1])
                                    maxV[1]=y;
                            }
                            float s=(maxV[0]-minV[0])*(maxV[1]-minV[1]);
                            if (s<bestOrientation[1])
                            {
                                bestOrientation[0]=a;
                                bestOrientation[1]=s;
                                bestOrientation[2]=maxV[0]-minV[0];
                                bestOrientation[3]=maxV[1]-minV[1];
                                float c[2]={(maxV[0]+minV[0])*0.5f,(maxV[1]+minV[1])*0.5f};
                                bestOrientation[4]=c[0]*cos(-a)-c[1]*sin(-a);
                                bestOrientation[5]=c[0]*sin(-a)+c[1]*cos(-a);
                            }
                        }

                        for (int k=0;k<3;k++) // the desired precision here
                        {
                            previousDa/=3.0f;
                            float bestOrientationFromPreviousStep=bestOrientation[0];
                            for (int j=-2;j<=2;j++)
                            { // Try 4 orientations..
                                if (j!=0)
                                {
                                    float a=bestOrientationFromPreviousStep+previousDa*float(j);
                                    float cosa=cos(a);
                                    float sina=sin(a);
                                    float minV[2]={99999.0f,99999.0f};
                                    float maxV[2]={-99999.0f,-99999.0f};
                                    for (int j=0;j<int(vertices[i]->size()/2);j++)
                                    {
                                        float ox=(*vertices[i])[2*j+0];
                                        float oy=(*vertices[i])[2*j+1];
                                        float x=ox*cosa-oy*sina;
                                        float y=ox*sina+oy*cosa;
                                        if (x<minV[0])
                                            minV[0]=x;
                                        if (x>maxV[0])
                                            maxV[0]=x;
                                        if (y<minV[1])
                                            minV[1]=y;
                                        if (y>maxV[1])
                                            maxV[1]=y;
                                    }
                                    float s=(maxV[0]-minV[0])*(maxV[1]-minV[1]);
                                    if (s<bestOrientation[1])
                                    {
                                        bestOrientation[0]=a;
                                        bestOrientation[1]=s;
                                        bestOrientation[2]=maxV[0]-minV[0];
                                        bestOrientation[3]=maxV[1]-minV[1];
                                        float c[2]={(maxV[0]+minV[0])*0.5f,(maxV[1]+minV[1])*0.5f};
                                        bestOrientation[4]=c[0]*cos(-a)-c[1]*sin(-a);
                                        bestOrientation[5]=c[0]*sin(-a)+c[1]*cos(-a);
                                    }
                                }
                            }

                        }

                        bestOrientation[0]=-bestOrientation[0]; // it is inversed!
                        bestOrientation[4]+=0.5f; // b/c of pixel precision
                        bestOrientation[5]+=0.5f; // b/c of pixel precision

                        float c[2]={bestOrientation[4]/sizeX,bestOrientation[5]/sizeY};
                        float v2[2]={bestOrientation[2]*0.5f/sizeX,bestOrientation[3]*0.5f/sizeY};
                        if (overlay)
                        {
                            float cosa=cos(bestOrientation[0]);
                            float sina=sin(bestOrientation[0]);
                            overlayLines.push_back(int(float(sizeX)*(c[0]+v2[0]*cosa-v2[1]*sina)));
                            overlayLines.push_back(int(float(sizeY)*(c[1]+v2[0]*sina+v2[1]*cosa)));
                            overlayLines.push_back(int(float(sizeX)*(c[0]-v2[0]*cosa-v2[1]*sina)));
                            overlayLines.push_back(int(float(sizeY)*(c[1]-v2[0]*sina+v2[1]*cosa)));
                            overlayLines.push_back(int(float(sizeX)*(c[0]-v2[0]*cosa-v2[1]*sina)));
                            overlayLines.push_back(int(float(sizeY)*(c[1]-v2[0]*sina+v2[1]*cosa)));
                            overlayLines.push_back(int(float(sizeX)*(c[0]-v2[0]*cosa+v2[1]*sina)));
                            overlayLines.push_back(int(float(sizeY)*(c[1]-v2[0]*sina-v2[1]*cosa)));
                            overlayLines.push_back(int(float(sizeX)*(c[0]-v2[0]*cosa+v2[1]*sina)));
                            overlayLines.push_back(int(float(sizeY)*(c[1]-v2[0]*sina-v2[1]*cosa)));
                            overlayLines.push_back(int(float(sizeX)*(c[0]+v2[0]*cosa+v2[1]*sina)));
                            overlayLines.push_back(int(float(sizeY)*(c[1]+v2[0]*sina-v2[1]*cosa)));
                            overlayLines.push_back(int(float(sizeX)*(c[0]+v2[0]*cosa+v2[1]*sina)));
                            overlayLines.push_back(int(float(sizeY)*(c[1]+v2[0]*sina-v2[1]*cosa)));
                            overlayLines.push_back(int(float(sizeX)*(c[0]+v2[0]*cosa-v2[1]*sina)));
                            overlayLines.push_back(int(float(sizeY)*(c[1]+v2[0]*sina+v2[1]*cosa)));
                        }
                        returnData.push_back(relSize);
                        returnData.push_back(bestOrientation[0]);
                        returnData.push_back(c[0]);
                        returnData.push_back(c[1]);
                        returnData.push_back(v2[0]*2.0f);
                        returnData.push_back(v2[1]*2.0f);
                    }
                }
            }

            delete[] blobData;
            for (int i=0;i<int(vertices.size());i++)
                delete vertices[i];

            delete[] classIDs;
            for (int i=0;i<int(cellEquivalencies.size());i++)
                delete cellEquivalencies[i];

            delete[] cellAppart;

            if (overlayLines.size()>0)
                drawLines(imgData,overlayLines,col);

            returnData[0]=float(blobCount);
            // the other return data is filled-in here above!
        }
        else
            simSetLastError(LUA_BLOBDETECTIONONWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(retVal));
    if (returnData.size()>0)
        D.pushOutData(CScriptFunctionDataItem((char*)(&returnData[0]),returnData.size()*sizeof(float))); // packed data is much faster in Lua
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.sharpenWorkImg
// --------------------------------------------------------------------------------------
#define LUA_SHARPENWORKIMG_COMMAND_PLUGIN "simVision.sharpenWorkImg@Vision"
#define LUA_SHARPENWORKIMG_COMMAND "simVision.sharpenWorkImg"

const int inArgs_SHARPENWORKIMG[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_SHARPENWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SHARPENWORKIMG,inArgs_SHARPENWORKIMG[0],LUA_SHARPENWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            float m[9]={-0.1111f,-0.1111f,-0.1111f,  -0.1111f,+1.8888f,-0.1111f,  -0.1111f,-0.1111f,-0.1111f};
            float* im2=CImageProcess::createRGBImage(sizeX,sizeY);
            CImageProcess::filter3x3RgbImage(sizeX,sizeY,imgData->workImg,im2,m);
            CImageProcess::copyRGBImage(sizeX,sizeY,im2,imgData->workImg);
            CImageProcess::clampRgbImage(sizeX,sizeY,imgData->workImg,0.0f,1.0f);
            CImageProcess::deleteImage(im2);
        }
        else
            simSetLastError(LUA_SHARPENWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.edgeDetectionOnWorkImg
// --------------------------------------------------------------------------------------
#define LUA_EDGEDETECTIONONWORKIMG_COMMAND_PLUGIN "simVision.edgeDetectionOnWorkImg@Vision"
#define LUA_EDGEDETECTIONONWORKIMG_COMMAND "simVision.edgeDetectionOnWorkImg"

const int inArgs_EDGEDETECTIONONWORKIMG[]={
    2,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
};

void LUA_EDGEDETECTIONONWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_EDGEDETECTIONONWORKIMG,inArgs_EDGEDETECTIONONWORKIMG[0],LUA_EDGEDETECTIONONWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float threshold=inData->at(1).floatData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            float m[9]={-3.0f,-3.0f,-3.0f,  -3.0f,24.0f,-3.0f,  -3.0f,-3.0f,-3.0f};
            float* im2=CImageProcess::createRGBImage(sizeX,sizeY);
            CImageProcess::filter3x3RgbImage(sizeX,sizeY,imgData->workImg,im2,m);
            CImageProcess::copyRGBImage(sizeX,sizeY,im2,imgData->workImg);
            int s=sizeX*sizeY;
            for (int i=0;i<s;i++)
            {
                float intens=(imgData->workImg[3*i+0]+imgData->workImg[3*i+1]+imgData->workImg[3*i+2])/3.0f;
                if (intens>threshold)
                {
                    imgData->workImg[3*i+0]=1.0f;
                    imgData->workImg[3*i+1]=1.0f;
                    imgData->workImg[3*i+2]=1.0f;
                }
                else
                {
                    imgData->workImg[3*i+0]=0.0f;
                    imgData->workImg[3*i+1]=0.0f;
                    imgData->workImg[3*i+2]=0.0f;
                }
            }
            CImageProcess::deleteImage(im2);
        }
        else
            simSetLastError(LUA_EDGEDETECTIONONWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.shiftWorkImg
// --------------------------------------------------------------------------------------
#define LUA_SHIFTWORKIMG_COMMAND_PLUGIN "simVision.shiftWorkImg@Vision"
#define LUA_SHIFTWORKIMG_COMMAND "simVision.shiftWorkImg"

const int inArgs_SHIFTWORKIMG[]={
    3,
    sim_script_arg_int32,0,
    sim_script_arg_float|sim_lua_arg_table,2,
    sim_script_arg_bool,0,
};

void LUA_SHIFTWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_SHIFTWORKIMG,inArgs_SHIFTWORKIMG[0],LUA_SHIFTWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float xShift=inData->at(1).floatData[0];
        float yShift=inData->at(1).floatData[1];
        float wrap=inData->at(2).boolData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            float* im2=CImageProcess::createRGBImage(sizeX,sizeY);
            CImageProcess::copyRGBImage(sizeX,sizeY,imgData->workImg,im2);
            float xShiftT=xShift*float(sizeX);
            float yShiftT=yShift*float(sizeY);
            if (xShiftT>=0.0f)
                xShiftT+=0.5f;
            else
                xShiftT-=0.5f;
            if (yShiftT>=0.0f)
                yShiftT+=0.5f;
            else
                yShiftT-=0.5f;
            int xShift=int(xShiftT);
            int yShift=int(yShiftT);
            if (xShift>sizeX)
                xShift=sizeX;
            if (xShift<-sizeX)
                xShift=-sizeX;
            if (yShift>sizeY)
                yShift=sizeY;
            if (yShift<-sizeY)
                yShift=-sizeY;
            int cpx,cpy;
            int ppx,ppy;
            for (int i=0;i<sizeX;i++)
            {
                cpx=i+xShift;
                ppx=cpx;
                if (ppx>=sizeX)
                    ppx-=sizeX;
                if (ppx<0)
                    ppx+=sizeX;
                for (int j=0;j<sizeY;j++)
                {
                    cpy=j+yShift;
                    ppy=cpy;
                    if (ppy>=sizeY)
                        ppy-=sizeY;
                    if (ppy<0)
                        ppy+=sizeY;
                    if (((cpx<0)||(cpx>=sizeX)||(cpy<0)||(cpy>=sizeY))&&(!wrap))
                    { // we remove that area (black)
                        imgData->workImg[3*(ppx+ppy*sizeX)+0]=0.0f;
                        imgData->workImg[3*(ppx+ppy*sizeX)+1]=0.0f;
                        imgData->workImg[3*(ppx+ppy*sizeX)+2]=0.0f;
                    }
                    else
                    {
                        imgData->workImg[3*(ppx+ppy*sizeX)+0]=im2[3*(i+j*sizeX)+0];
                        imgData->workImg[3*(ppx+ppy*sizeX)+1]=im2[3*(i+j*sizeX)+1];
                        imgData->workImg[3*(ppx+ppy*sizeX)+2]=im2[3*(i+j*sizeX)+2];
                    }
                }
            }
            CImageProcess::deleteImage(im2);
        }
        else
            simSetLastError(LUA_SHIFTWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.circularCutWorkImg
// --------------------------------------------------------------------------------------
#define LUA_CIRCULARCUTWORKIMG_COMMAND_PLUGIN "simVision.circularCutWorkImg@Vision"
#define LUA_CIRCULARCUTWORKIMG_COMMAND "simVision.circularCutWorkImg"

const int inArgs_CIRCULARCUTWORKIMG[]={
    3,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
    sim_script_arg_bool,0,
};

void LUA_CIRCULARCUTWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_CIRCULARCUTWORKIMG,inArgs_CIRCULARCUTWORKIMG[0],LUA_CIRCULARCUTWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float radius=inData->at(1).floatData[0];
        float copyToBuffer1=inData->at(2).boolData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (copyToBuffer1)
            {
                if (imgData->buff1Img==nullptr)
                    imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            }
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            int smallestSize=std::min<int>(sizeX,sizeY);
            float centerX=float(sizeX)/2.0f;
            float centerY=float(sizeY)/2.0f;
            float radSquared=radius*float(smallestSize)*0.5f;
            radSquared*=radSquared;
            float dx,dy;
            for (int i=0;i<sizeX;i++)
            {
                dx=float(i)+0.5f-centerX;
                dx*=dx;
                for (int j=0;j<sizeY;j++)
                {
                    dy=float(j)+0.5f-centerY;
                    dy*=dy;
                    if (dy+dx>radSquared)
                    {
                        if (copyToBuffer1)
                        {
                            imgData->buff1Img[3*(i+j*sizeX)+0]=imgData->workImg[3*(i+j*sizeX)+0];
                            imgData->buff1Img[3*(i+j*sizeX)+1]=imgData->workImg[3*(i+j*sizeX)+1];
                            imgData->buff1Img[3*(i+j*sizeX)+2]=imgData->workImg[3*(i+j*sizeX)+2];
                        }
                        imgData->workImg[3*(i+j*sizeX)+0]=0.0f;
                        imgData->workImg[3*(i+j*sizeX)+1]=0.0f;
                        imgData->workImg[3*(i+j*sizeX)+2]=0.0f;
                    }
                    else
                    {
                        if (copyToBuffer1)
                        {
                            imgData->buff1Img[3*(i+j*sizeX)+0]=0.0f;
                            imgData->buff1Img[3*(i+j*sizeX)+1]=0.0f;
                            imgData->buff1Img[3*(i+j*sizeX)+2]=0.0f;
                        }
                    }
                }
            }
        }
        else
            simSetLastError(LUA_CIRCULARCUTWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.resizeWorkImg
// --------------------------------------------------------------------------------------
#define LUA_RESIZEWORKIMG_COMMAND_PLUGIN "simVision.resizeWorkImg@Vision"
#define LUA_RESIZEWORKIMG_COMMAND "simVision.resizeWorkImg"

const int inArgs_RESIZEWORKIMG[]={
    2,
    sim_script_arg_int32,0,
    sim_script_arg_float|sim_lua_arg_table,2,
};

void LUA_RESIZEWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_RESIZEWORKIMG,inArgs_RESIZEWORKIMG[0],LUA_RESIZEWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float xScale=inData->at(1).floatData[0];
        float yScale=inData->at(1).floatData[1];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            float* im=CImageProcess::createRGBImage(sizeX,sizeY);
            CImageProcess::copyRGBImage(sizeX,sizeY,imgData->workImg,im);
            float* cntIm=CImageProcess::createIntensityImage(sizeX,sizeY);
            int s=sizeX*sizeY;
            for (int i=0;i<s;i++)
            {
                cntIm[i]=0.0f;
                imgData->workImg[3*i+0]=0.0f;
                imgData->workImg[3*i+1]=0.0f;
                imgData->workImg[3*i+2]=0.0f;
            }
            float centerX=float(sizeX)/2.0f;
            float centerY=float(sizeY)/2.0f;
            float dx,dy;
            int npx,npy;
            for (int i=0;i<sizeX;i++)
            {
                dx=(float(i)+0.5f-centerX)/xScale;
                npx=int(centerX+dx);
                if ((npx>=0)&&(npx<sizeX))
                {
                    for (int j=0;j<sizeY;j++)
                    {
                        dy=(float(j)+0.5f-centerY)/yScale;
                        npy=int(centerY+dy);
                        if ((npy>=0)&&(npy<sizeY))
                        {
                            imgData->workImg[3*(i+j*sizeX)+0]+=im[3*(npx+npy*sizeX)+0];
                            imgData->workImg[3*(i+j*sizeX)+1]+=im[3*(npx+npy*sizeX)+1];
                            imgData->workImg[3*(i+j*sizeX)+2]+=im[3*(npx+npy*sizeX)+2];
                            cntIm[i+j*sizeX]+=1.0f;
                        }
                    }
                }
            }
            for (int i=0;i<s;i++)
            {
                if (cntIm[i]>0.1f)
                {
                    imgData->workImg[3*i+0]/=cntIm[i];
                    imgData->workImg[3*i+1]/=cntIm[i];
                    imgData->workImg[3*i+2]/=cntIm[i];
                }
            }
            CImageProcess::deleteImage(cntIm);
            CImageProcess::deleteImage(im);
        }
        else
            simSetLastError(LUA_RESIZEWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.rotateWorkImg
// --------------------------------------------------------------------------------------
#define LUA_ROTATEWORKIMG_COMMAND_PLUGIN "simVision.rotateWorkImg@Vision"
#define LUA_ROTATEWORKIMG_COMMAND "simVision.rotateWorkImg"

const int inArgs_ROTATEWORKIMG[]={
    2,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
};

void LUA_ROTATEWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_ROTATEWORKIMG,inArgs_ROTATEWORKIMG[0],LUA_ROTATEWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float rotAngle=inData->at(1).floatData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            float* im=CImageProcess::createRGBImage(sizeX,sizeY);
            CImageProcess::copyRGBImage(sizeX,sizeY,imgData->workImg,im);
            int ss=sizeX*sizeY*3;
            for (int i=0;i<ss;i++)
                imgData->workImg[i]=0.0f;
            float centerX=float(sizeX)/2.0f;
            float centerY=float(sizeY)/2.0f;
            float dx,dy;
            int npx,npy;
            float dxp0;
            float dxp1;
            float dyp0;
            float dyp1;
            float c=cos(-rotAngle);
            float s=sin(-rotAngle);
            for (int i=0;i<sizeX;i++)
            {
                dx=float(i)+0.5f-centerX;
                dxp0=dx*c;
                dyp0=dx*s;
                for (int j=0;j<sizeY;j++)
                {
                    dy=float(j)+0.5f-centerY;
                    dxp1=-dy*s;
                    dyp1=dy*c;
                    npx=int(centerX+dxp0+dxp1);
                    npy=int(centerY+dyp0+dyp1);
                    if ((npy>=0)&&(npy<sizeY)&&(npx>=0)&&(npx<sizeX))
                    {
                        imgData->workImg[3*(i+j*sizeX)+0]+=im[3*(npx+npy*sizeX)+0];
                        imgData->workImg[3*(i+j*sizeX)+1]+=im[3*(npx+npy*sizeX)+1];
                        imgData->workImg[3*(i+j*sizeX)+2]+=im[3*(npx+npy*sizeX)+2];
                    }
                }
            }
            CImageProcess::deleteImage(im);
        }
        else
            simSetLastError(LUA_ROTATEWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.matrix3x3OnWorkImg
// --------------------------------------------------------------------------------------
#define LUA_MATRIX3X3ONWORKIMG_COMMAND_PLUGIN "simVision.matrix3x3OnWorkImg@Vision"
#define LUA_MATRIX3X3ONWORKIMG_COMMAND "simVision.matrix3x3OnWorkImg"

const int inArgs_MATRIX3X3ONWORKIMG[]={
    4,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
    sim_script_arg_float|sim_lua_arg_table,9,
};

void LUA_MATRIX3X3ONWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_MATRIX3X3ONWORKIMG,inArgs_MATRIX3X3ONWORKIMG[0]-1,LUA_MATRIX3X3ONWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        int passes=inData->at(1).int32Data[0];
        float multiplier=inData->at(2).floatData[0];
        float m[9];
        if ( (inData->size()<4)||(inData->at(3).floatData.size()<9) )
        {
            for (size_t i=0;i<9;i++)
                m[i]=1.0f;
            const float sigma=2.0f;
            float tot=0.0f;
            for (int i=-1;i<2;i++)
            {
                for (int j=-1;j<2;j++)
                {
                    float v=pow(2.7182818f,-(i*i+j*j)/(2.0f*sigma*sigma))/(2.0f*3.14159265f*sigma*sigma);
                    m[1+i+(j+1)*3]=v;
                    tot+=v;
                }
            }
            for (size_t i=0;i<9;i++)
                m[i]*=multiplier/tot;
        }
        else
        {
            for (size_t i=0;i<9;i++)
                m[i]=inData->at(3).floatData[i]*multiplier;
        }
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            float* im2=CImageProcess::createRGBImage(sizeX,sizeY);
            for (int i=0;i<passes/2;i++)
            {
                CImageProcess::filter3x3RgbImage(sizeX,sizeY,imgData->workImg,im2,m);
                CImageProcess::filter3x3RgbImage(sizeX,sizeY,im2,imgData->workImg,m);
            }
            if (passes&1)
            {
                CImageProcess::filter3x3RgbImage(sizeX,sizeY,imgData->workImg,im2,m);
                CImageProcess::copyRGBImage(sizeX,sizeY,im2,imgData->workImg);
            }
            CImageProcess::clampRgbImage(sizeX,sizeY,imgData->workImg,0.0f,1.0f);
            CImageProcess::deleteImage(im2);
        }
        else
            simSetLastError(LUA_MATRIX3X3ONWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.matrix5x5OnWorkImg
// --------------------------------------------------------------------------------------
#define LUA_MATRIX5X5ONWORKIMG_COMMAND_PLUGIN "simVision.matrix5x5OnWorkImg@Vision"
#define LUA_MATRIX5X5ONWORKIMG_COMMAND "simVision.matrix5x5OnWorkImg"

const int inArgs_MATRIX5X5ONWORKIMG[]={
    4,
    sim_script_arg_int32,0,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
    sim_script_arg_float|sim_lua_arg_table,25,
};

void LUA_MATRIX5X5ONWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_MATRIX5X5ONWORKIMG,inArgs_MATRIX5X5ONWORKIMG[0]-1,LUA_MATRIX5X5ONWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        int passes=inData->at(1).int32Data[0];
        float multiplier=inData->at(2).floatData[0];
        float m[25];
        if ( (inData->size()<4)||(inData->at(3).floatData.size()<25) )
        {
            for (size_t i=0;i<25;i++)
                m[i]=1.0f;
            const float sigma=2.0f;
            float tot=0.0f;
            for (int i=-2;i<3;i++)
            {
                for (int j=-2;j<3;j++)
                {
                    float v=pow(2.7182818f,-(i*i+j*j)/(2.0f*sigma*sigma))/(2.0f*3.14159265f*sigma*sigma);
                    m[i+2+(j+2)*5]=v;
                    tot+=v;
                }
            }
            for (size_t i=0;i<25;i++)
                m[i]*=multiplier/tot;
        }
        else
        {
            for (size_t i=0;i<25;i++)
                m[i]=inData->at(3).floatData[i]*multiplier;
        }
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            float* im2=CImageProcess::createRGBImage(sizeX,sizeY);
            for (int i=0;i<passes/2;i++)
            {
                CImageProcess::filter5x5RgbImage(sizeX,sizeY,imgData->workImg,im2,m);
                CImageProcess::filter5x5RgbImage(sizeX,sizeY,im2,imgData->workImg,m);
            }
            if (passes&1)
            {
                CImageProcess::filter5x5RgbImage(sizeX,sizeY,imgData->workImg,im2,m);
                CImageProcess::copyRGBImage(sizeX,sizeY,im2,imgData->workImg);
            }
            CImageProcess::clampRgbImage(sizeX,sizeY,imgData->workImg,0.0f,1.0f);
            CImageProcess::deleteImage(im2);
        }
        else
            simSetLastError(LUA_MATRIX5X5ONWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.rectangularCutWorkImg
// --------------------------------------------------------------------------------------
#define LUA_RECTANGULARCUTWORKIMG_COMMAND_PLUGIN "simVision.rectangularCutWorkImg@Vision"
#define LUA_RECTANGULARCUTWORKIMG_COMMAND "simVision.rectangularCutWorkImg"

const int inArgs_RECTANGULARCUTWORKIMG[]={
    3,
    sim_script_arg_int32,0,
    sim_script_arg_float|sim_lua_arg_table,2,
    sim_script_arg_bool,0,
};

void LUA_RECTANGULARCUTWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    if (D.readDataFromStack(p->stackID,inArgs_RECTANGULARCUTWORKIMG,inArgs_RECTANGULARCUTWORKIMG[0],LUA_RECTANGULARCUTWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float nsizeX=inData->at(1).floatData[0];
        float nsizeY=inData->at(1).floatData[1];
        float copyToBuffer1=inData->at(2).boolData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            if (copyToBuffer1)
            {
                if (imgData->buff1Img==nullptr)
                    imgData->buff1Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            }
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            float centerX=float(sizeX)/2.0f;
            float centerY=float(sizeY)/2.0f;
            float dx,dy;
            float hdx=nsizeX*0.5f;
            float hdy=nsizeY*0.5f;
            for (int i=0;i<sizeX;i++)
            {
                dx=float(i)+0.5f-centerX;
                for (int j=0;j<sizeY;j++)
                {
                    dy=float(j)+0.5f-centerY;
                    if ((fabs(dx)>hdx*float(sizeX))||(fabs(dy)>hdy*float(sizeY)))
                    {
                        if (copyToBuffer1)
                        {
                            imgData->buff1Img[3*(i+j*sizeX)+0]=imgData->workImg[3*(i+j*sizeX)+0];
                            imgData->buff1Img[3*(i+j*sizeX)+1]=imgData->workImg[3*(i+j*sizeX)+1];
                            imgData->buff1Img[3*(i+j*sizeX)+2]=imgData->workImg[3*(i+j*sizeX)+2];
                        }
                        imgData->workImg[3*(i+j*sizeX)+0]=0.0f;
                        imgData->workImg[3*(i+j*sizeX)+1]=0.0f;
                        imgData->workImg[3*(i+j*sizeX)+2]=0.0f;
                    }
                    else
                    {
                        if (copyToBuffer1)
                        {
                            imgData->buff1Img[3*(i+j*sizeX)+0]=0.0f;
                            imgData->buff1Img[3*(i+j*sizeX)+1]=0.0f;
                            imgData->buff1Img[3*(i+j*sizeX)+2]=0.0f;
                        }
                    }
                }
            }
        }
        else
            simSetLastError(LUA_RECTANGULARCUTWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.coordinatesFromWorkImg
// --------------------------------------------------------------------------------------
#define LUA_COORDINATESFROMWORKIMG_COMMAND_PLUGIN "simVision.coordinatesFromWorkImg@Vision"
#define LUA_COORDINATESFROMWORKIMG_COMMAND "simVision.coordinatesFromWorkImg"

const int inArgs_COORDINATESFROMWORKIMG[]={
    3,
    sim_script_arg_int32,0,
    sim_script_arg_int32|sim_lua_arg_table,2,
    sim_script_arg_bool,0,
};

void LUA_COORDINATESFROMWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    std::vector<float> returnData;
    if (D.readDataFromStack(p->stackID,inArgs_COORDINATESFROMWORKIMG,inArgs_COORDINATESFROMWORKIMG[0],LUA_COORDINATESFROMWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int hhandle=inData->at(0).int32Data[0];
        bool absCoords=false;
        if (hhandle>=0)
        {
            absCoords=((hhandle&sim_handleflag_abscoords)!=0);
            hhandle=hhandle&0x000fffff;
        }

        int handle=getVisionSensorHandle(hhandle,p->objectID);
        int ptCntX=inData->at(1).int32Data[0];
        int ptCntY=inData->at(1).int32Data[1];
        bool angularSpace=inData->at(2).boolData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            C7Vector sensorTr;
            simGetObjectPosition(handle,-1,sensorTr.X.data);
            float q[4];
            simGetObjectQuaternion(handle,-1,q);
            sensorTr.Q=C4Vector(q[3],q[0],q[1],q[2]); // CoppeliaSim quaternion, internally: w x y z, at interfaces: x y z w
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];

            int perspectiveOperation;
            simGetObjectInt32Parameter(handle,sim_visionintparam_perspective_operation,&perspectiveOperation);
            float np,fp;
            simGetObjectFloatParameter(handle,sim_visionfloatparam_near_clipping,&np);
            simGetObjectFloatParameter(handle,sim_visionfloatparam_far_clipping,&fp);
            float depthThresh=np;
            float depthRange=fp-depthThresh;
            float farthestValue=fp;
            float xAngle;
            simGetObjectFloatParameter(handle,sim_visionfloatparam_perspective_angle,&xAngle);
            float yAngle=xAngle;
            float ratio=float(sizeX)/float(sizeY);
            if (sizeX>sizeY)
                yAngle=2.0f*(float)atan(tan(xAngle/2.0f)/ratio);
            else
                xAngle=2.0f*(float)atan(tan(xAngle/2.0f)*ratio);
            float xS;
            simGetObjectFloatParameter(handle,sim_visionfloatparam_ortho_size,&xS);
            float yS=xS;
            if (sizeX>sizeY)
                yS=xS/ratio;
            else
                xS=xS*ratio;


            int xPtCnt=ptCntX;
            int yPtCnt=ptCntY;

            returnData.clear();
            returnData.push_back(float(xPtCnt));
            returnData.push_back(float(yPtCnt));


            if (perspectiveOperation!=0)
            {
                float yDist=0.0f;
                float dy=0.0f;
                if (yPtCnt>1)
                {
                    dy=yAngle/float(yPtCnt-1);
                    yDist=-yAngle*0.5f;
                }
                float dx=0.0f;
                if (xPtCnt>1)
                    dx=xAngle/float(xPtCnt-1);

                float xAlpha=0.5f/(tan(xAngle*0.5f));
                float yAlpha=0.5f/(tan(yAngle*0.5f));

                float xBeta=2.0f*tan(xAngle*0.5f);
                float yBeta=2.0f*tan(yAngle*0.5f);

                for (int j=0;j<yPtCnt;j++)
                {
                    float tanYDistTyAlpha;
                    int yRow;
                    if (angularSpace)
                    {
                        tanYDistTyAlpha=tan(yDist)*yAlpha;
                        yRow=int((tanYDistTyAlpha+0.5f)*(sizeY-0.5f));
                    }
                    else
                        yRow=int((0.5f+yDist/yAngle)*(sizeY-0.5f));

                    float xDist=0.0f;
                    if (xPtCnt>1)
                        xDist=-xAngle*0.5f;
                    for (int i=0;i<xPtCnt;i++)
                    {
                        C3Vector v;
                        if (angularSpace)
                        {
                            float tanXDistTxAlpha=tan(xDist)*xAlpha;
                            int xRow=int((0.5f-tanXDistTxAlpha)*(sizeX-0.5f));
                            int indexP=3*(xRow+yRow*sizeX);
                            float intensity=(imgData->workImg[indexP+0]+imgData->workImg[indexP+1]+imgData->workImg[indexP+2])/3.0f;
                            float zDist=depthThresh+intensity*depthRange;
                            v.set(tanXDistTxAlpha*xBeta*zDist,tanYDistTyAlpha*yBeta*zDist,zDist);
                        }
                        else
                        {
                            int xRow=int((0.5f-xDist/xAngle)*(sizeX-0.5f));
                            int indexP=3*(xRow+yRow*sizeX);
                            float intensity=(imgData->workImg[indexP+0]+imgData->workImg[indexP+1]+imgData->workImg[indexP+2])/3.0f;
                            float zDist=depthThresh+intensity*depthRange;
                            v.set(tan(xAngle*0.5f)*xDist/(xAngle*0.5f)*zDist,tan(yAngle*0.5f)*yDist/(yAngle*0.5f)*zDist,zDist);
                        }

                        float l=v.getLength();
                        if (l>farthestValue)
                        {
                            v=(v/l)*farthestValue;
                            if (absCoords)
                                v=sensorTr*v;
                            returnData.push_back(v(0));
                            returnData.push_back(v(1));
                            returnData.push_back(v(2));
                            returnData.push_back(farthestValue);
                        }
                        else
                        {
                            if (absCoords)
                                v=sensorTr*v;
                            returnData.push_back(v(0));
                            returnData.push_back(v(1));
                            returnData.push_back(v(2));
                            returnData.push_back(l);
                        }
                        xDist+=dx;
                    }
                    yDist+=dy;
                }
            }
            else
            {
                float yDist=0.0f;
                float dy=0.0f;
                if (yPtCnt>1)
                {
                    dy=yS/float(yPtCnt-1);
                    yDist=-yS*0.5f;
                }
                float dx=0.0f;
                if (xPtCnt>1)
                    dx=xS/float(xPtCnt-1);

                for (int j=0;j<yPtCnt;j++)
                {
                    int yRow=int(((yDist+yS*0.5f)/yS)*(sizeY-0.5f));

                    float xDist=0.0f;
                    if (xPtCnt>1)
                        xDist=-xS*0.5f;
                    for (int i=0;i<xPtCnt;i++)
                    {
                        int xRow=int((1.0f-((xDist+xS*0.5f)/xS))*(sizeX-0.5f));
                        int indexP=3*(xRow+yRow*sizeX);
                        float intensity=(imgData->workImg[indexP+0]+imgData->workImg[indexP+1]+imgData->workImg[indexP+2])/3.0f;
                        float zDist=depthThresh+intensity*depthRange;
                        if (absCoords)
                        {
                            C3Vector absv(sensorTr*C3Vector(xDist,yDist,zDist));
                            returnData.push_back(absv(0));
                            returnData.push_back(absv(1));
                            returnData.push_back(absv(2));
                        }
                        else
                        {
                            returnData.push_back(xDist);
                            returnData.push_back(yDist);
                            returnData.push_back(zDist);
                        }
                        returnData.push_back(zDist);
                        xDist+=dx;
                    }
                    yDist+=dy;
                }
            }
        }
        else
            simSetLastError(LUA_COORDINATESFROMWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    if (returnData.size()>0)
        D.pushOutData(CScriptFunctionDataItem((char*)(&returnData[0]),returnData.size()*sizeof(float))); // packed data is much faster in Lua
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.changedPixelsOnWorkImg
// --------------------------------------------------------------------------------------
#define LUA_CHANGEDPIXELSONWORKIMG_COMMAND_PLUGIN "simVision.changedPixelsOnWorkImg@Vision"
#define LUA_CHANGEDPIXELSONWORKIMG_COMMAND "simVision.changedPixelsOnWorkImg"

const int inArgs_CHANGEDPIXELSONWORKIMG[]={
    2,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
};

void LUA_CHANGEDPIXELSONWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    std::vector<float> returnData;
    if (D.readDataFromStack(p->stackID,inArgs_CHANGEDPIXELSONWORKIMG,inArgs_CHANGEDPIXELSONWORKIMG[0],LUA_CHANGEDPIXELSONWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=getVisionSensorHandle(inData->at(0).int32Data[0],p->objectID);
        float thresh=inData->at(1).floatData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];
            if (imgData->buff2Img==nullptr)
                imgData->buff2Img=new float[imgData->resolution[0]*imgData->resolution[1]*3];
            for (int j=0;j<sizeY;j++)
            {
                for (int i=0;i<sizeX;i++)
                {
                    float buffIntens=(imgData->buff2Img[3*(i+j*sizeX)+0]+imgData->buff2Img[3*(i+j*sizeX)+1]+imgData->buff2Img[3*(i+j*sizeX)+2])/3.0f;
                    float imgIntens=(imgData->workImg[3*(i+j*sizeX)+0]+imgData->workImg[3*(i+j*sizeX)+1]+imgData->workImg[3*(i+j*sizeX)+2])/3.0f;
                    float diff=imgIntens-buffIntens;
                    if (buffIntens==0.0f)
                        buffIntens=0.001f;
                    if (fabs(diff)/buffIntens>=thresh)
                    {
                        imgData->buff2Img[3*(i+j*sizeX)+0]=imgIntens;
                        imgData->buff2Img[3*(i+j*sizeX)+1]=imgIntens;
                        imgData->buff2Img[3*(i+j*sizeX)+2]=imgIntens;
                        if (diff>0)
                        {
                            imgData->workImg[3*(i+j*sizeX)+0]=1.0f;
                            imgData->workImg[3*(i+j*sizeX)+1]=1.0f;
                            imgData->workImg[3*(i+j*sizeX)+2]=1.0f;
                            returnData.push_back(1.0f);
                        }
                        else
                        {
                            imgData->workImg[3*(i+j*sizeX)+0]=0.0f;
                            imgData->workImg[3*(i+j*sizeX)+1]=0.0f;
                            imgData->workImg[3*(i+j*sizeX)+2]=0.0f;
                            returnData.push_back(-1.0f);
                        }
                        returnData.push_back(float(i));
                        returnData.push_back(float(j));
                    }
                    else
                    {
                        imgData->workImg[3*(i+j*sizeX)+0]=0.5f;
                        imgData->workImg[3*(i+j*sizeX)+1]=0.5f;
                        imgData->workImg[3*(i+j*sizeX)+2]=0.5f;
                    }
                }
            }
        }
        else
            simSetLastError(LUA_CHANGEDPIXELSONWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    if (returnData.size()>0)
        D.pushOutData(CScriptFunctionDataItem((char*)(&returnData[0]),returnData.size()*sizeof(float))); // packed data is much faster in Lua
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simVision.velodyneDataFromWorkImg
// --------------------------------------------------------------------------------------
#define LUA_VELODYNEDATAFROMWORKIMG_COMMAND_PLUGIN "simVision.velodyneDataFromWorkImg@Vision"
#define LUA_VELODYNEDATAFROMWORKIMG_COMMAND "simVision.velodyneDataFromWorkImg"

const int inArgs_VELODYNEDATAFROMWORKIMG[]={
    3,
    sim_script_arg_int32,0,
    sim_script_arg_int32|sim_lua_arg_table,2,
    sim_script_arg_float,0,
};

void LUA_VELODYNEDATAFROMWORKIMG_CALLBACK(SScriptCallBack* p)
{
    CScriptFunctionData D;
    std::vector<float> returnData;
    if (D.readDataFromStack(p->stackID,inArgs_VELODYNEDATAFROMWORKIMG,inArgs_VELODYNEDATAFROMWORKIMG[0],LUA_VELODYNEDATAFROMWORKIMG_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int hhandle=inData->at(0).int32Data[0];
        bool absCoords=false;
        if (hhandle>=0)
        {
            absCoords=((hhandle&sim_handleflag_abscoords)!=0);
            hhandle=hhandle&0x000fffff;
        }
        int handle=getVisionSensorHandle(hhandle,p->objectID);
        int xPtCnt=inData->at(1).int32Data[0];
        int yPtCnt=inData->at(1).int32Data[1];
        float vAngle=inData->at(2).floatData[0];
        CVisionSensorData* imgData=getImgData(handle);
        if (imgData!=nullptr)
        {
            C7Vector sensorTr;
            simGetObjectPosition(handle,-1,sensorTr.X.data);
            float q[4];
            simGetObjectQuaternion(handle,-1,q);
            sensorTr.Q=C4Vector(q[3],q[0],q[1],q[2]); // CoppeliaSim quaternion, internally: w x y z, at interfaces: x y z w

            int sizeX=imgData->resolution[0];
            int sizeY=imgData->resolution[1];

            float np,fp;
            simGetObjectFloatParameter(handle,sim_visionfloatparam_near_clipping,&np);
            simGetObjectFloatParameter(handle,sim_visionfloatparam_far_clipping,&fp);
            float xAngle;
            simGetObjectFloatParameter(handle,sim_visionfloatparam_perspective_angle,&xAngle);

            float depthThresh=np;
            float depthRange=fp-depthThresh;
            float farthestValue=fp;
            float yAngle=xAngle;
            float ratio=float(sizeX)/float(sizeY);
            if (sizeX>sizeY)
                yAngle=2.0f*(float)atan(tan(xAngle/2.0f)/ratio);
            else
                xAngle=2.0f*(float)atan(tan(xAngle/2.0f)*ratio);
            returnData.clear();
            returnData.push_back(float(xPtCnt));
            returnData.push_back(float(yPtCnt));

            //if (sensor->getPerspectiveOperation())
            {
                float dx=0.0f;
                if (xPtCnt>1)
                    dx=xAngle/float(xPtCnt-1);
                float xDist=0.0f;
                if (xPtCnt>1)
                    xDist=-xAngle*0.5f;

                float xAlpha=0.5f/(tan(xAngle*0.5f));

                float xBeta=2.0f*tan(xAngle*0.5f);
                float yBeta=2.0f*tan(yAngle*0.5f);

                for (int j=0;j<xPtCnt;j++)
                {
                    float h=1.0f/cos(xDist);

                    float yDist=0.0f;
                    float dy=0.0f;
                    if (yPtCnt>1)
                    {
                        dy=vAngle/float(yPtCnt-1);
                        yDist=-vAngle*0.5f;
                    }

                    float tanXDistTxAlpha=tan(xDist)*xAlpha;
                    int xRow=int((0.5f-tanXDistTxAlpha)*(sizeX-0.5f));

                    if (xRow<0)
                        xRow=0;
                    if (xRow>=sizeX)
                        xRow=sizeX-1;

                    float yAlpha=0.5f/(tan(yAngle*0.5f));

                    for (int i=0;i<yPtCnt;i++)
                    {
                        float tanYDistTyAlpha=tan(yDist)*h*yAlpha;
                        int yRow=int((tanYDistTyAlpha+0.5f)*(sizeY-0.5f));
                        if (yRow<0)
                            yRow=0;
                        if (yRow>=sizeY)
                            yRow=sizeY-1;
                        int indexP=3*(xRow+yRow*sizeX);
                        float intensity=(imgData->workImg[indexP+0]+imgData->workImg[indexP+1]+imgData->workImg[indexP+2])/3.0f;
                        float zDist=depthThresh+intensity*depthRange;
                        C3Vector v(tanXDistTxAlpha*xBeta*zDist,tanYDistTyAlpha*yBeta*zDist,zDist);
                        float l=v.getLength();
                        if (l>farthestValue)
                        {
                            v=(v/l)*farthestValue;
                            if (absCoords)
                                v=sensorTr*v;
                            returnData.push_back(v(0));
                            returnData.push_back(v(1));
                            returnData.push_back(v(2));
                            returnData.push_back(farthestValue);
                        }
                        else
                        {
                            if (absCoords)
                                v=sensorTr*v;
                            returnData.push_back(v(0));
                            returnData.push_back(v(1));
                            returnData.push_back(v(2));
                            returnData.push_back(l);
                        }
                        yDist+=dy;
                    }
                    xDist+=dx;
                }
            }
        }
        else
            simSetLastError(LUA_VELODYNEDATAFROMWORKIMG_COMMAND,"Invalid handle or work image not initialized.");
    }
    D.pushOutData(CScriptFunctionDataItem(false));
    if (returnData.size()>0)
        D.pushOutData(CScriptFunctionDataItem((char*)(&returnData[0]),returnData.size()*sizeof(float))); // packed data is much faster in Lua
    D.writeDataToStack(p->stackID);
}
// --------------------------------------------------------------------------------------

// This is the plugin start routine:
SIM_DLLEXPORT unsigned char simStart(void* reservedPointer,int reservedInt)
{ // This is called just once, at the start of CoppeliaSim
    // Dynamically load and bind CoppeliaSim functions:
    char curDirAndFile[1024];
#ifdef _WIN32
    #ifdef QT_COMPIL
        _getcwd(curDirAndFile, sizeof(curDirAndFile));
    #else
        GetModuleFileName(NULL,curDirAndFile,1023);
        PathRemoveFileSpec(curDirAndFile);
    #endif
#elif defined (__linux) || defined (__APPLE__)
    getcwd(curDirAndFile, sizeof(curDirAndFile));
#endif

    std::string currentDirAndPath(curDirAndFile);
    std::string temp(currentDirAndPath);

#ifdef _WIN32
    temp+="\\coppeliaSim.dll";
#elif defined (__linux)
    temp+="/libcoppeliaSim.so";
#elif defined (__APPLE__)
    temp+="/libcoppeliaSim.dylib";
#endif /* __linux || __APPLE__ */

    simLib=loadSimLibrary(temp.c_str());
    if (simLib==NULL)
    {
        printf("simExtVision: error: could not find or correctly load the CoppeliaSim library. Cannot start the plugin.\n"); // cannot use simAddLog here.
        return(0); // Means error, CoppeliaSim will unload this plugin
    }
    if (getSimProcAddresses(simLib)==0)
    {
        printf("simExtVision: error: could not find all required functions in the CoppeliaSim library. Cannot start the plugin.\n"); // cannot use simAddLog here.
        unloadSimLibrary(simLib);
        return(0); // Means error, CoppeliaSim will unload this plugin
    }

    simRegisterScriptVariable("simVision","require('simExtVision')",0);

    // Register the new Lua commands:

    // Spherical vision sensor:
    simRegisterScriptCallbackFunction(LUA_HANDLESPHERICAL_COMMAND_PLUGIN,strConCat("number result=",LUA_HANDLESPHERICAL_COMMAND,"(number passiveVisionSensorHandleForRGB,table_6 activeVisionSensorHandles,\nnumber horizontalAngle,number verticalAngle,number passiveVisionSensorHandleForDepth=-1)"),LUA_HANDLESPHERICAL_CALLBACK);

    // Anaglyph sensor:
    simRegisterScriptCallbackFunction(LUA_HANDLEANAGLYPHSTEREO_COMMAND_PLUGIN,strConCat("number result=",LUA_HANDLEANAGLYPHSTEREO_COMMAND,"(number passiveVisionSensorHandle,table_2 activeVisionSensorHandles,table_6 leftAndRightColors=nil)"),LUA_HANDLEANAGLYPHSTEREO_CALLBACK);


    // HDL-64E:
    simRegisterScriptCallbackFunction(LUA_CREATEVELODYNEHDL64E_COMMAND_PLUGIN,strConCat("number velodyneHandle=",LUA_CREATEVELODYNEHDL64E_COMMAND,"(table_4 visionSensorHandles,number frequency,number options=0,number pointSize=2,table_2 coloring_closeFarDist={1,5},number displayScalingFactor=1)"),LUA_CREATEVELODYNEHDL64E_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_DESTROYVELODYNEHDL64E_COMMAND_PLUGIN,strConCat("number result=",LUA_DESTROYVELODYNEHDL64E_COMMAND,"(number velodyneHandle)"),LUA_DESTROYVELODYNEHDL64E_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_HANDLEVELODYNEHDL64E_COMMAND_PLUGIN,strConCat("table points=",LUA_HANDLEVELODYNEHDL64E_COMMAND,"(number velodyneHandle,number dt)"),LUA_HANDLEVELODYNEHDL64E_CALLBACK);

    // VPL-16:
    simRegisterScriptCallbackFunction(LUA_CREATEVELODYNEVPL16_COMMAND_PLUGIN,strConCat("number velodyneHandle=",LUA_CREATEVELODYNEVPL16_COMMAND,"(table_4 visionSensorHandles,number frequency,number options=0,number pointSize=2,table_2 coloring_closeFarDist={1,5},number displayScalingFactor=1)"),LUA_CREATEVELODYNEVPL16_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_DESTROYVELODYNEVPL16_COMMAND_PLUGIN,strConCat("number result=",LUA_DESTROYVELODYNEVPL16_COMMAND,"(number velodyneHandle)"),LUA_DESTROYVELODYNEVPL16_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_HANDLEVELODYNEVPL16_COMMAND_PLUGIN,strConCat("table points=",LUA_HANDLEVELODYNEVPL16_COMMAND,"(number velodyneHandle,number dt)"),LUA_HANDLEVELODYNEVPL16_CALLBACK);

    // basic vision sensor processing:
    simRegisterScriptCallbackFunction(LUA_SENSORIMGTOWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_SENSORIMGTOWORKIMG_COMMAND,"(number visionSensorHandle)"),LUA_SENSORIMGTOWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_SENSORDEPTHMAPTOWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_SENSORDEPTHMAPTOWORKIMG_COMMAND,"(number visionSensorHandle)"),LUA_SENSORDEPTHMAPTOWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_WORKIMGTOSENSORIMG_COMMAND_PLUGIN,strConCat("",LUA_WORKIMGTOSENSORIMG_COMMAND,"(number visionSensorHandle,bool removeBuffer=true)"),LUA_WORKIMGTOSENSORIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_WORKIMGTOSENSORDEPTHMAP_COMMAND_PLUGIN,strConCat("",LUA_WORKIMGTOSENSORDEPTHMAP_COMMAND,"(number visionSensorHandle,bool removeBuffer=true)"),LUA_WORKIMGTOSENSORDEPTHMAP_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_WORKIMGTOBUFFER1_COMMAND_PLUGIN,strConCat("",LUA_WORKIMGTOBUFFER1_COMMAND,"(number visionSensorHandle)"),LUA_WORKIMGTOBUFFER1_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_WORKIMGTOBUFFER2_COMMAND_PLUGIN,strConCat("",LUA_WORKIMGTOBUFFER2_COMMAND,"(number visionSensorHandle)"),LUA_WORKIMGTOBUFFER2_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_SWAPBUFFERS_COMMAND_PLUGIN,strConCat("",LUA_SWAPBUFFERS_COMMAND,"(number visionSensorHandle)"),LUA_SWAPBUFFERS_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_BUFFER1TOWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_BUFFER1TOWORKIMG_COMMAND,"(number visionSensorHandle)"),LUA_BUFFER1TOWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_BUFFER2TOWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_BUFFER2TOWORKIMG_COMMAND,"(number visionSensorHandle)"),LUA_BUFFER2TOWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_SWAPWORKIMGWITHBUFFER1_COMMAND_PLUGIN,strConCat("",LUA_SWAPWORKIMGWITHBUFFER1_COMMAND,"(number visionSensorHandle)"),LUA_SWAPWORKIMGWITHBUFFER1_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_ADDWORKIMGTOBUFFER1_COMMAND_PLUGIN,strConCat("",LUA_ADDWORKIMGTOBUFFER1_COMMAND,"(number visionSensorHandle)"),LUA_ADDWORKIMGTOBUFFER1_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_SUBTRACTWORKIMGFROMBUFFER1_COMMAND_PLUGIN,strConCat("",LUA_SUBTRACTWORKIMGFROMBUFFER1_COMMAND,"(number visionSensorHandle)"),LUA_SUBTRACTWORKIMGFROMBUFFER1_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_ADDBUFFER1TOWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_ADDBUFFER1TOWORKIMG_COMMAND,"(number visionSensorHandle)"),LUA_ADDBUFFER1TOWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_SUBTRACTBUFFER1FROMWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_SUBTRACTBUFFER1FROMWORKIMG_COMMAND,"(number visionSensorHandle)"),LUA_SUBTRACTBUFFER1FROMWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_MULTIPLYWORKIMGWITHBUFFER1_COMMAND_PLUGIN,strConCat("",LUA_MULTIPLYWORKIMGWITHBUFFER1_COMMAND,"(number visionSensorHandle)"),LUA_MULTIPLYWORKIMGWITHBUFFER1_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_HORIZONTALFLIPWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_HORIZONTALFLIPWORKIMG_COMMAND,"(number visionSensorHandle)"),LUA_HORIZONTALFLIPWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_VERTICALFLIPWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_VERTICALFLIPWORKIMG_COMMAND,"(number visionSensorHandle)"),LUA_VERTICALFLIPWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_UNIFORMIMGTOWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_UNIFORMIMGTOWORKIMG_COMMAND,"(number visionSensorHandle,table_3 color)"),LUA_UNIFORMIMGTOWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_NORMALIZEWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_NORMALIZEWORKIMG_COMMAND,"(number visionSensorHandle)"),LUA_NORMALIZEWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_COLORSEGMENTATIONONWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_COLORSEGMENTATIONONWORKIMG_COMMAND,"(number visionSensorHandle,number maxColorColorDistance)"),LUA_COLORSEGMENTATIONONWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_INTENSITYSCALEONWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_INTENSITYSCALEONWORKIMG_COMMAND,"(number visionSensorHandle,number start,number end,bool greyScale)"),LUA_INTENSITYSCALEONWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_SELECTIVECOLORONONWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_SELECTIVECOLORONONWORKIMG_COMMAND,"(number visionSensorHandle,table_3 color,table_3 colorTolerance,bool rgb,bool keep,bool removedPartToBuffer1)"),LUA_SELECTIVECOLORONONWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_SCALEANDOFFSETWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_SCALEANDOFFSETWORKIMG_COMMAND,"(number visionSensorHandle,table_3 preOffset,table_3 scaling,table_3 postOffset,bool rgb)"),LUA_SCALEANDOFFSETWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_BINARYWORKIMG_COMMAND_PLUGIN,strConCat("bool trigger,table packedDataPacket=",LUA_BINARYWORKIMG_COMMAND,"(number visionSensorHandle,number threshold,number oneProportion,number oneTol,\nnumber xCenter,number xCenterTol,number yCenter,number yCenterTol,number orient,number orientTol,number roundness,bool enableTrigger,table_3 overlayColor={1.0,0.0,1.0})"),LUA_BINARYWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_BLOBDETECTIONONWORKIMG_COMMAND_PLUGIN,strConCat("bool trigger,table packedDataPacket=",LUA_BLOBDETECTIONONWORKIMG_COMMAND,"(number visionSensorHandle,number threshold,number minBlobSize,bool modifyWorkImage,table_3 overlayColor={1.0,0.0,1.0})"),LUA_BLOBDETECTIONONWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_SHARPENWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_SHARPENWORKIMG_COMMAND,"(number visionSensorHandle)"),LUA_SHARPENWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_EDGEDETECTIONONWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_EDGEDETECTIONONWORKIMG_COMMAND,"(number visionSensorHandle,number threshold)"),LUA_EDGEDETECTIONONWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_SHIFTWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_SHIFTWORKIMG_COMMAND,"(number visionSensorHandle,table_2 shift,bool wrapAround)"),LUA_SHIFTWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_CIRCULARCUTWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_CIRCULARCUTWORKIMG_COMMAND,"(number visionSensorHandle,number radius,bool copyToBuffer1)"),LUA_CIRCULARCUTWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_RESIZEWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_RESIZEWORKIMG_COMMAND,"(number visionSensorHandle,table_2 scaling)"),LUA_RESIZEWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_ROTATEWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_ROTATEWORKIMG_COMMAND,"(number visionSensorHandle,number rotationAngle)"),LUA_ROTATEWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_MATRIX3X3ONWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_MATRIX3X3ONWORKIMG_COMMAND,"(number visionSensorHandle,number passes,number multiplied,table_9 matrix=nil)"),LUA_MATRIX3X3ONWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_MATRIX5X5ONWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_MATRIX5X5ONWORKIMG_COMMAND,"(number visionSensorHandle,number passes,number multiplied,table_25 matrix=nil)"),LUA_MATRIX5X5ONWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_RECTANGULARCUTWORKIMG_COMMAND_PLUGIN,strConCat("",LUA_RECTANGULARCUTWORKIMG_COMMAND,"(number visionSensorHandle,table_2 sizes,bool copyToBuffer1)"),LUA_RECTANGULARCUTWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_COORDINATESFROMWORKIMG_COMMAND_PLUGIN,strConCat("bool trigger,table packedDataPacket=",LUA_COORDINATESFROMWORKIMG_COMMAND,"(number visionSensorHandle,table_2 xyPointCount,bool evenlySpacedInAngularSpace)"),LUA_COORDINATESFROMWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_CHANGEDPIXELSONWORKIMG_COMMAND_PLUGIN,strConCat("bool trigger,table packedDataPacket=",LUA_CHANGEDPIXELSONWORKIMG_COMMAND,"(number visionSensorHandle,number threshold)"),LUA_CHANGEDPIXELSONWORKIMG_CALLBACK);
    simRegisterScriptCallbackFunction(LUA_VELODYNEDATAFROMWORKIMG_COMMAND_PLUGIN,strConCat("bool trigger,table packedDataPacket=",LUA_VELODYNEDATAFROMWORKIMG_COMMAND,"(number visionSensorHandle,table_2 xyPointCount,number vAngle)"),LUA_VELODYNEDATAFROMWORKIMG_CALLBACK);


    // Following for backward compatibility:
    simRegisterScriptVariable(LUA_HANDLESPHERICAL_COMMANDOLD,LUA_HANDLESPHERICAL_COMMAND,-1);
    simRegisterScriptVariable(LUA_HANDLEANAGLYPHSTEREO_COMMANDOLD,LUA_HANDLEANAGLYPHSTEREO_COMMAND,-1);
    simRegisterScriptVariable(LUA_CREATEVELODYNEHDL64E_COMMANDOLD,LUA_CREATEVELODYNEHDL64E_COMMAND,-1);
    simRegisterScriptVariable(LUA_DESTROYVELODYNEHDL64E_COMMANDOLD,LUA_DESTROYVELODYNEHDL64E_COMMAND,-1);
    simRegisterScriptVariable(LUA_HANDLEVELODYNEHDL64E_COMMANDOLD,LUA_HANDLEVELODYNEHDL64E_COMMAND,-1);
    simRegisterScriptVariable(LUA_CREATEVELODYNEVPL16_COMMANDOLD,LUA_CREATEVELODYNEVPL16_COMMAND,-1);
    simRegisterScriptVariable(LUA_DESTROYVELODYNEVPL16_COMMANDOLD,LUA_DESTROYVELODYNEVPL16_COMMAND,-1);
    simRegisterScriptVariable(LUA_HANDLEVELODYNEVPL16_COMMANDOLD,LUA_HANDLEVELODYNEVPL16_COMMAND,-1);
    simRegisterScriptCallbackFunction(LUA_HANDLESPHERICAL_COMMANDOLD_PLUGIN,strConCat("Please use the ",LUA_HANDLESPHERICAL_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(LUA_HANDLEANAGLYPHSTEREO_COMMANDOLD_PLUGIN,strConCat("Please use the ",LUA_HANDLEANAGLYPHSTEREO_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(LUA_CREATEVELODYNEHDL64E_COMMANDOLD_PLUGIN,strConCat("Please use the ",LUA_CREATEVELODYNEHDL64E_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(LUA_DESTROYVELODYNEHDL64E_COMMANDOLD_PLUGIN,strConCat("Please use the ",LUA_DESTROYVELODYNEHDL64E_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(LUA_HANDLEVELODYNEHDL64E_COMMANDOLD_PLUGIN,strConCat("Please use the ",LUA_HANDLEVELODYNEHDL64E_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(LUA_CREATEVELODYNEVPL16_COMMANDOLD_PLUGIN,strConCat("Please use the ",LUA_CREATEVELODYNEVPL16_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(LUA_DESTROYVELODYNEVPL16_COMMANDOLD_PLUGIN,strConCat("Please use the ",LUA_DESTROYVELODYNEVPL16_COMMAND," notation instead"),0);
    simRegisterScriptCallbackFunction(LUA_HANDLEVELODYNEVPL16_COMMANDOLD_PLUGIN,strConCat("Please use the ",LUA_HANDLEVELODYNEVPL16_COMMAND," notation instead"),0);
    simRegisterScriptVariable("simExtVision_createVelodyne",LUA_CREATEVELODYNEHDL64E_COMMAND,-1);
    simRegisterScriptVariable("simExtVision_destroyVelodyne",LUA_DESTROYVELODYNEHDL64E_COMMAND,-1);
    simRegisterScriptVariable("simExtVision_handleVelodyne",LUA_HANDLEVELODYNEHDL64E_COMMAND,-1);



    visionTransfContainer = new CVisionTransfCont();
    visionVelodyneHDL64EContainer = new CVisionVelodyneHDL64ECont();
    visionVelodyneVPL16Container = new CVisionVelodyneVPL16Cont();

    return(4);  // initialization went fine, we return the version number of this extension module (can be queried with simGetModuleName)
                // Version 2 since 3.2.1
                // Version 3 since 3.3.1
                // Version 4 since 3.4.1
}

// This is the plugin end routine:
SIM_DLLEXPORT void simEnd()
{ // This is called just once, at the end of CoppeliaSim

    delete visionTransfContainer;
    delete visionVelodyneHDL64EContainer;
    delete visionVelodyneVPL16Container;

    unloadSimLibrary(simLib); // release the library
}

// This is the plugin messaging routine (i.e. CoppeliaSim calls this function very often, with various messages):
SIM_DLLEXPORT void* simMessage(int message,int* auxiliaryData,void* customData,int* replyData)
{ // This is called quite often. Just watch out for messages/events you want to handle

    // This function should not generate any error messages:
    int errorModeSaved;
    simGetIntegerParameter(sim_intparam_error_report_mode,&errorModeSaved);
    simSetIntegerParameter(sim_intparam_error_report_mode,sim_api_errormessage_ignore);

    void* retVal=NULL;

    if ( (message==sim_message_eventcallback_lastinstancepass)||(message==sim_message_eventcallback_instanceswitch)||(message==sim_message_eventcallback_undoperformed)||(message==sim_message_eventcallback_redoperformed) )
    {
        for (std::map<int,CVisionSensorData*>::iterator it=_imgData.begin();it!=_imgData.end();it++)
            delete it->second;
        _imgData.clear();
    }

    if (message==sim_message_eventcallback_instancepass)
    {
        if (auxiliaryData[0]&1)
            visionTransfContainer->removeInvalidObjects();

        for (std::map<int,CVisionSensorData*>::iterator it=_imgData.begin();it!=_imgData.end();it++)
        {
            if (simIsHandleValid(it->first,-1)==0)
                removeImgData(it->first);
        }
    }

    if (message==sim_message_eventcallback_simulationended)
    { // Simulation just ended
        visionTransfContainer->removeAll();
        visionVelodyneHDL64EContainer->removeAll();
        visionVelodyneVPL16Container->removeAll();
    }

    simSetIntegerParameter(sim_intparam_error_report_mode,errorModeSaved); // restore previous settings
    return(retVal);
}

