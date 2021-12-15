package hiklib

// #include <stdio.h>
// #include <stdlib.h>
// #include "hiklib.h"
// #include <string.h>
//
// extern int onmessage(int command, char *sDVRIP, char *pBuf, unsigned int dwBufLen);
// extern void onmessagev30(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUserData);
//
// static NET_DVR_ALARMINFO_V30 getalarminfo(char *pAlarmInfo) {
//    NET_DVR_ALARMINFO_V30 struAlarmInfo;
//    memcpy(&struAlarmInfo, pAlarmInfo, sizeof(NET_DVR_ALARMINFO_V30));
//    return struAlarmInfo;
// }
//
// static void OnAlarmV30(int user,int alarmport) {
//   HListenAlarmV30(user,alarmport,onmessagev30);
// }
// static void OnAlarm(int user,int alarmport) {
//   HListenAlarm(user,alarmport,onmessage);
// }
// static void myprint(char* s) {
//   printf("%s\n", s);
// }
import "C"
import (
	"log"
	"unsafe"
)

type DevInfo struct {
	ByStartChan int
}

type MotionVideos struct {
	Videos []MotionVideo
	Count  int
}

type MotionVideo struct {
	Filename   string
	Size       int64
	From_year  int
	From_month int
	From_day   int
	From_hour  int
	From_min   int
	From_sec   int
	To_year    int
	To_month   int
	To_day     int
	To_hour    int
	To_min     int
	To_sec     int
}

type MotionAreas struct {
	Areas []MotionArea
}

type MotionArea struct {
	X float32
	Y float32
	W float32
	H float32
}

type AlarmItem struct {
	IP        string
	Command   int
	AlarmType int
}

type OnMsg func(item AlarmItem)

var Listeners []OnMsg

const COMM_ALARM int = 0x1100              //8000 Upload alarm message
const COMM_ALARM_V30 int = 0x4000          //9000 upload alarm message
const COMM_DEV_STATUS_CHANGED int = 0x7000 //Device status change alarm upload

func HikTest() {
	cs := C.CString("Hello from stdio11")
	C.myprint(cs)
	C.free(unsafe.Pointer(cs))
}

//export onmessagev30
func onmessagev30(command C.int, pAlarmer *C.NET_DVR_ALARMER, pAlarmInfo *C.char, dwBufLen C.uint, pUserData unsafe.Pointer) {
	i := AlarmItem{IP: C.GoString(&pAlarmer.sDeviceIP[0]), Command: int(command)}
	switch int(command) {
	case COMM_ALARM_V30:
		log.Println("ALARM")
		i.AlarmType = int(C.getalarminfo(pAlarmInfo).dwAlarmType)
		for _, f := range Listeners {
			f(i)
		}
		break
	case COMM_DEV_STATUS_CHANGED:
		log.Printf("COMM_DEV_STATUS_CHANGED")
		break
	default:
		log.Printf("Unknown Alarm [0x%x] !!!", command)
	}
}

//export onmessage
func onmessage(command C.int, ip *C.char, data *C.char, ln C.uint) C.int {
	i := AlarmItem{IP: C.GoString(ip), Command: int(command)}

	switch int(command) {
	case COMM_ALARM_V30:
		i.AlarmType = int(C.getalarminfo(data).dwAlarmType)
		for _, f := range Listeners {
			f(i)
		}
		break
	case COMM_DEV_STATUS_CHANGED:
		log.Printf("COMM_DEV_STATUS_CHANGED %s %s", C.GoString(ip), C.GoString(data))
		break
	default:
		log.Printf("Unknown Alarm [0x%x] %s %s !!!", command, C.GoString(ip), C.GoString(data))
	}
	return 1
}

// HikVersion - hikvision sdk version
func HikVersion() string {
	cs := C.CString("")
	C.HVersion(cs)
	defer C.free(unsafe.Pointer(cs))
	return C.GoString(cs)
}

// HikLogin - connect and login to camera
func HikLogin(ip, user, pass string) (int, DevInfo) {
	// var user = C.int(-1)
	var dev = C.DevInfo{byStartChan: 0}
	u := C.HLogin(C.CString(ip), C.CString(user), C.CString(pass), &dev)
	return int(u), DevInfo{ByStartChan: int(dev.byStartChan)}
}

// HikLogout - disconnect from camera
func HikLogout(user int) {
	C.HLogout(C.int(user))
}

// int HMotionArea(int lUserID, struct MotionAreas *areas);
func HikMotionArea(lUserID int) (int, MotionAreas) {
	var ma = C.MotionAreas{}
	ret := C.HMotionArea(C.int(lUserID), &ma)

	mma := MotionAreas{}
	for i := 0; i < 8; i++ {
		mma.Areas = append(mma.Areas, MotionArea{
			X: float32(ma.areas[i].x),
			Y: float32(ma.areas[i].y),
			W: float32(ma.areas[i].w),
			H: float32(ma.areas[i].h),
		})
	}

	return int(ret), mma
}

// int HCaptureImage(int lUserID, int byStartChan, char *imagePath);
func HikCaptureImage(user int, byStartChan int, imagePath string) int {
	return int(C.HCaptureImage(C.int(user), C.int(byStartChan), C.CString(imagePath)))
}

// 	C.OnAlarmV30(C.int(user), C.int(*alarmParam))
func HikOnAlarmV30(user int, alarmPort int, f OnMsg) {
	Listeners = append(Listeners, f)
	C.OnAlarmV30(C.int(user), C.int(alarmPort))
}

// 	C.OnAlarm(C.int(user), C.int(*alarmParam))
func HikOnAlarm(user int, alarmPort int, f OnMsg) {
	Listeners = append(Listeners, f)
	C.OnAlarm(C.int(user), C.int(alarmPort))
}

// int HReboot(int user);
func HikReboot(user int) int {
	return int(C.HReboot(C.int(user)))
}

// int HListVideo(int lUserID, struct MotionVideos *videos);
func HikListVideo(lUserID int) (int, MotionVideos) {
	v := C.MotionVideos{}
	ret := C.HListVideo(C.int(lUserID), &v)
	vv := MotionVideos{}
	vv.Count = int(v.count)
	for i := 0; i < int(v.count); i++ {
		vv.Videos = append(vv.Videos, MotionVideo{
			Filename:   C.GoString(v.videos[i].filename),
			Size:       int64(v.videos[i].size),
			From_year:  int(v.videos[i].from_year),
			From_month: int(v.videos[i].from_month),
			From_day:   int(v.videos[i].from_day),
			From_hour:  int(v.videos[i].from_hour),
			From_min:   int(v.videos[i].from_min),
			From_sec:   int(v.videos[i].from_sec),
			To_year:    int(v.videos[i].to_year),
			To_month:   int(v.videos[i].to_month),
			To_day:     int(v.videos[i].to_day),
			To_hour:    int(v.videos[i].to_hour),
			To_min:     int(v.videos[i].to_min),
			To_sec:     int(v.videos[i].to_sec),
		})
	}
	return int(ret), vv
}

// int HSaveFile(int userId, char *srcfile, char *destfile);
func HikSaveFile(userId int, srcfile string, destfile string) int {
	return int(C.HSaveFile(C.int(userId), C.CString(srcfile), C.CString(destfile)))
}

// HikFormatDisk - format disks
func HikFormatDisk(user int, disk int) int {
	return int(C.HFormatDisk(C.int(user), C.int(disk)))
}
