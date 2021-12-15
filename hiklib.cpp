//
#include <stdio.h>

#include <cstring>
#include <chrono>
#include "HCNetSDK.h"
#include "hiklib.h"
#include "stdlib.h"

using namespace std::chrono;

unsigned int HVersion(char *ret)
{
  NET_DVR_Init();
  unsigned int uiVersion = NET_DVR_GetSDKBuildVersion();
  sprintf(ret, "HCNetSDK V%d.%d.%d.%d", (0xff000000 & uiVersion) >> 24, (0x00ff0000 & uiVersion) >> 16, (0x0000ff00 & uiVersion) >> 8, (0x000000ff & uiVersion));
  return uiVersion;
}

int HLogin(char *ip, char *username, char *password, struct DevInfo *devinfo)
{
  char logPath[11] = "./sdkLog.v";
  NET_DVR_Init();
  NET_DVR_SetLogToFile(0, logPath);
  NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
  NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};
  struLoginInfo.bUseAsynLogin = false;

  struLoginInfo.wPort = 8000;
  memcpy(struLoginInfo.sDeviceAddress, ip, NET_DVR_DEV_ADDRESS_MAX_LEN);
  memcpy(struLoginInfo.sUserName, username, NAME_LEN);
  memcpy(struLoginInfo.sPassword, password, NAME_LEN);

  int lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);

  if (lUserID < 0)
  {
    int err = NET_DVR_GetLastError();
    printf("\n\nError %d\n\n", err);
    NET_DVR_Cleanup();
    return 0 - err;
  }

  devinfo->byStartChan = struDeviceInfoV40.struDeviceV30.byStartChan;

  return lUserID;
}

void HLogout(int lUserID)
{
  NET_DVR_Logout_V30(lUserID);
  NET_DVR_Cleanup();
}

int HMotionArea(int lUserID, struct MotionAreas *areas)
{
  int iChannelNO = 1;
  int iRet;

  //1.Get picture params.
  DWORD uiReturnLen;
  NET_DVR_PICCFG_V40 struParams = {0};
  struParams.dwSize = sizeof(NET_DVR_PICCFG_V40);
  iRet = NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_PICCFG_V40, iChannelNO, &struParams, sizeof(NET_DVR_PICCFG_V40), &uiReturnLen);
  if (!iRet)
  {
    printf("NET_DVR_GET_PICCFG_V40 %d error.\n", NET_DVR_GetLastError());
    return 2;
  }
  // printf("Channel %d Name is %s.\n", iChannelNO, struParams.sChanName);

  // printf("Motion enable %d.\n", struParams.struMotion.byEnableHandleMotion);
  // printf("Motion detection mode %d.\n", struParams.struMotion.byConfigurationMode);
  // printf("Motion enable highlight display %d.\n", struParams.struMotion.byEnableDisplay);
  if (struParams.struMotion.byConfigurationMode > 0)
  {
    auto st = struParams.struMotion.struMotionMode.struMotionMultiArea;
    for (int i = 0; i < 8; i++)
    {
      auto area = st.struMotionMultiAreaParam[i];
      if (area.byAreaNo && area.struRect.fHeight)
      {
        areas->areas[i].x = area.struRect.fX;
        areas->areas[i].y = area.struRect.fY;
        areas->areas[i].w = area.struRect.fWidth;
        areas->areas[i].h = area.struRect.fHeight;
        // printf("Motion area %d.\n", area.byAreaNo);
        // printf("Motion height [%f,%f] %fx%f.\n", area.struRect.fX, area.struRect.fY, area.struRect.fWidth, area.struRect.fHeight);
      }
    }
  }
  return iRet;
}

int HCaptureImage(int lUserID, int byStartChan, char *imagePath)
{
  printf("Capture image to [%s].\n", imagePath);
  NET_DVR_JPEGPARA strPicPara = {0};
  strPicPara.wPicQuality = 0;
  strPicPara.wPicSize = 0xff;
  int iRet;
  iRet = NET_DVR_CaptureJPEGPicture(lUserID, byStartChan, &strPicPara, imagePath);
  if (!iRet)
  {
    return 0 - NET_DVR_GetLastError();
  }
  return iRet;
}

BOOL CALLBACK OnMessage(int lCommand, char *sDVRIP, char *pBuf, DWORD dwBufLen)
{
  printf("OnMessage %d from %s [%s]%d\n", lCommand, sDVRIP, pBuf, dwBufLen);
  return true;
}

void AlarmMessageCallBack_V30(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void *pUserData)
{
  printf("OnMessage_V30 %d\n", lCommand);
}

int HListenAlarmV30(long lUserID, int alarmport, void (*AlarmCallback)(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void *pUserData))
{

  printf("Register callback\n");
  LONG lHandle;
  lHandle = NET_DVR_SetDVRMessageCallBack_V30(AlarmCallback, NULL);
  if (lHandle < 0)
  {
    printf("NET_DVR_SetDVRMessageCallBack_V30 error, %d\n", NET_DVR_GetLastError());
    return -1;
  }

  // lHandle = NET_DVR_StartListen_V30(NULL,alarmport, AlarmCallback, NULL);
  // if (lHandle < 0)
  // {
  // 	printf("NET_DVR_StartListen_V30 error, %d\n", NET_DVR_GetLastError());
  // 	return -1;
  // } else {
  NET_DVR_SETUPALARM_PARAM struSetupParam = {0};
  struSetupParam.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM);
  //Alarm information type to upload: 0-History Alarm (NET_DVR_PLATE_RESULT), 1-Real-Time Alarm (NET_ITS_PLATE_RESULT)
  struSetupParam.byAlarmInfoType = 1;
  //Arming Level: Level-2 arming (for traffic device) struSetupParam.byLevel=1;

  lHandle = NET_DVR_SetupAlarmChan_V41(lUserID, &struSetupParam);
  //lHandle = NET_DVR_SetupAlarmChan_V30(lUserID);
  if (lHandle < 0)
  {
    printf("NET_DVR_SetupAlarmChan_V30 error, %d\n", NET_DVR_GetLastError());
    return -1;
  }
  // }
  return 0;
}

int HListenAlarm(long lUserID,
                 int alarmport,
                 int (*fMessCallBack)(int lCommand,
                                      char *sDVRIP,
                                      char *pBuf,
                                      unsigned int dwBufLen)) // BOOL(CALLBACK *fMessCallBack)(LONG lCommand, char *sDVRIP, char *pBuf, DWORD dwBufLen))
{
  NET_DVR_SetDVRMessCallBack(fMessCallBack);
  if (NET_DVR_StartListen(NULL, alarmport))
  {
    printf("Start listen\n");
    LONG m_alarmHandle = NET_DVR_SetupAlarmChan(lUserID);
    if (m_alarmHandle < -1)
    {
      return 0 - NET_DVR_GetLastError();
    }
    else
    {
      return m_alarmHandle;
    }
  }
  else
  {
    printf("\n\nError start alarmlisten\n\n");
    return -1;
  }
}

int HReboot(int user)
{
  if (NET_DVR_RebootDVR(user) < 1)
  {
    return 0 - NET_DVR_GetLastError();
  }
  return 1;
}

int HListVideo(int lUserID, struct MotionVideos *videos)
{
  printf("List video.\n");

  auto now = system_clock::now();
  time_t tt = system_clock::to_time_t(now);
  tm local_tm = *localtime(&tt);

  // printf("%d\n", local_tm.tm_year);

  NET_DVR_FILECOND_V50 m_struFileCondV50;
  memset(&m_struFileCondV50, 0, sizeof(NET_DVR_FILECOND_V50));
  m_struFileCondV50.struStreamID.dwChannel = 1;
  // m_struFileCondV50.dwFileType = 0xff;
  m_struFileCondV50.dwFileType = 255;

  m_struFileCondV50.struStartTime.wYear = local_tm.tm_year + 1900;
  m_struFileCondV50.struStartTime.byMonth = local_tm.tm_mon + 1;
  m_struFileCondV50.struStartTime.byDay = 1;
  m_struFileCondV50.struStartTime.byHour = 0;
  m_struFileCondV50.struStartTime.byMinute = 0;
  m_struFileCondV50.struStartTime.bySecond = 0;

  m_struFileCondV50.struStopTime.wYear = local_tm.tm_year + 1900;
  m_struFileCondV50.struStopTime.byMonth = local_tm.tm_mon + 1;
  m_struFileCondV50.struStopTime.byDay = local_tm.tm_mday;
  m_struFileCondV50.struStopTime.byHour = 23;
  m_struFileCondV50.struStopTime.byMinute = 59;
  m_struFileCondV50.struStopTime.bySecond = 59;

  int lFindHandle = NET_DVR_FindFile_V50(lUserID, &m_struFileCondV50);

  if (lFindHandle < 0)
  {
    printf("find file fail,last error %d\n", NET_DVR_GetLastError());
    return 1;
  }
  NET_DVR_FINDDATA_V50 struFileData;

  int count = 0;
  int trycount = 10000;
  while (true)
  {
    int result = NET_DVR_FindNextFile_V50(lFindHandle, &struFileData);
    if (result == NET_DVR_ISFINDING)
    {
      trycount--;
      if (trycount > 0) {
        continue;
      } else {
        printf("Try count is 0.\n");
        break;
      }
    }
    else if (result == NET_DVR_FILE_SUCCESS)
    {
      videos->videos[count].filename = (char *)malloc(sizeof(char) * (100 + 1));
      strcpy(videos->videos[count].filename, struFileData.sFileName);
      videos->videos[count].size = struFileData.dwFileSize;
      videos->videos[count].from_year = struFileData.struStartTime.wYear;
      videos->videos[count].from_month = struFileData.struStartTime.byMonth;
      videos->videos[count].from_day = struFileData.struStartTime.byDay;
      videos->videos[count].from_hour = struFileData.struStartTime.byHour;
      videos->videos[count].from_min = struFileData.struStartTime.byMinute;
      videos->videos[count].from_sec = struFileData.struStartTime.bySecond;
      videos->videos[count].to_year = struFileData.struStopTime.wYear;
      videos->videos[count].to_month = struFileData.struStopTime.byMonth;
      videos->videos[count].to_day = struFileData.struStopTime.byDay;
      videos->videos[count].to_hour = struFileData.struStopTime.byHour;
      videos->videos[count].to_min = struFileData.struStopTime.byMinute;
      videos->videos[count].to_sec = struFileData.struStopTime.bySecond;
      count++;
      videos->count = count;

      // printf(
      //     "%d:%s, [%db] %d-%d-%d %d:%d:%d -- %d-%d-%d %d:%d:%d.\n", count,
      //     struFileData.sFileName, struFileData.dwFileSize,
      //     struFileData.struStartTime.wYear, struFileData.struStartTime.byMonth,
      //     struFileData.struStartTime.byDay, struFileData.struStartTime.byHour,
      //     struFileData.struStartTime.byMinute,
      //     struFileData.struStartTime.bySecond, struFileData.struStopTime.wYear,
      //     struFileData.struStopTime.byMonth, struFileData.struStopTime.byDay,
      //     struFileData.struStopTime.byHour, struFileData.struStopTime.byMinute,
      //     struFileData.struStopTime.bySecond);
      // break;
      continue;
    }
    else if (result == NET_DVR_FILE_NOFIND || result == NET_DVR_NOMOREFILE)
    {
      printf("No more files!\n");
      break;
    }
    else
    {
      printf("find file fail for illegal get file state\n");
      break;
    }

    NET_DVR_FindClose_V30(lFindHandle);
  }

  return 0;
}

int HSaveFile(int userId, char *srcfile, char *destfile)
{
  printf("Save %s\n", srcfile);
  int bRes = 1;

  int hPlayback = 0;
  if ((hPlayback = NET_DVR_GetFileByName(userId, srcfile, destfile)) < 0)
  {
    printf("GetFileByName failed. error[%d]\n", NET_DVR_GetLastError());
    bRes = -1;
    return bRes;
  }

  if (!NET_DVR_PlayBackControl(hPlayback, NET_DVR_PLAYSTART, 0, NULL))
  {
    printf("play back control failed [%d]\n", NET_DVR_GetLastError());
    bRes = -1;
    return bRes;
  }

  int pos = 0;
  int lastpos = 0;
  for (pos = 0; pos < 100 && pos >= 0;
       pos = NET_DVR_GetDownloadPos(hPlayback))
  {
    if (lastpos != pos)
    {
      printf("%d\r", pos);
      fflush(stdout);
      lastpos = pos;
    }
  }
  printf("\n");

  if (!NET_DVR_StopGetFile(hPlayback))
  {
    printf("failed to stop get file [%d]\n", NET_DVR_GetLastError());
    bRes = -1;
    return bRes;
  }

  printf("%s\n", destfile);

  if (pos < 0 || pos > 100)
  {
    printf("download err [%d]\n", NET_DVR_GetLastError());
    bRes = -1;
    return bRes;
  }
  else
  {
    return 0;
  }
}

int HFormatDisk(int userID,int disk) {
  return NET_DVR_FormatDisk(userID,disk);
}