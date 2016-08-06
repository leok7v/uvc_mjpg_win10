#include "misc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Microsoft Media Foundation */

static const DWORD VIDEO_STREAM = (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM;
#define MF_SL MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK
#define MF_FN MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME

static const char* guid2format(const GUID &guidFormat);

static int mf_get_media_types(IMFSourceReader* reader) {
    for (int i = 0; ; i++) {
        com_t<IMFMediaType> type;
        if (reader->GetNativeMediaType(VIDEO_STREAM, i, &type) != 0) {
            break;
        }
        GUID guid_major_type = {0};
        if_failed_return_result(type->GetMajorType(&guid_major_type));
        if (guid_major_type == MFMediaType_Video) {
            GUID guid_subtype = {0};
            if_failed_return_result(type->GetGUID(MF_MT_SUBTYPE, &guid_subtype));
            AM_MEDIA_TYPE* amMediaType = null;
            if_failed_return_result(type->GetRepresentation(FORMAT_MFVideoFormat, (void**)&amMediaType));
            assert(amMediaType->cbFormat == sizeof(MFVIDEOFORMAT));
            const MFVIDEOFORMAT* mi = (const MFVIDEOFORMAT*)amMediaType->pbFormat;
            const MFVideoInfo* vi = &mi->videoInfo;
            int frames_per_second = vi->FramesPerSecond.Numerator / vi->FramesPerSecond.Denominator;
            BOOL compressed = false;
            if_failed_return_result(type->IsCompressedFormat(&compressed));
            print("[%2d] %-5s %4dx%-4d @%3d\n", i, guid2format(mi->guidFormat), vi->dwWidth, vi->dwHeight, frames_per_second); 
            if_failed_return_result(type->FreeRepresentation(FORMAT_MFVideoFormat, amMediaType));
        }
    }
    return 0;
}

static int mf_enumerate_cameras(int (*callback)(IMFSourceReader* reader)) {
    com_t<IMFAttributes> config;
    if_failed_return_result(MFCreateAttributes(&config, 1));
    if_failed_return_result(config->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
    UINT32 num = 0;
    IMFActivate** factory = null;  // [count]
    if_failed_return_result(MFEnumDeviceSources(config, &factory, &num));
    if (num == 0) {
        return 0;
    }
    for (int i = 0; i < (int)num; i++) {
        const char* sl = "";
        const char* fn= "";
        WCHAR* symbolic_link = null;
        WCHAR* friendly_name = null;
        UINT32 size = 0;
        if (OK(factory[i]->GetAllocatedString(MF_SL, &symbolic_link, &size))) { sl = wcs2str(symbolic_link); }
        if (OK(factory[i]->GetAllocatedString(MF_FN, &friendly_name, &size))) { fn = wcs2str(friendly_name); }
        com_t<IMFMediaSource> source;
        if_failed_return_result(factory[i]->ActivateObject(IID_PPV_ARGS(&source)));
        com_t<IMFSourceReader> reader;
        if_failed_return_result(MFCreateSourceReaderFromMediaSource(source, null, &reader));
        print("\nMMF camera %d %s %s\n", i, fn, sl);
        callback(reader);
        com_release(factory[i]);
    }
    CoTaskMemFree(factory);
    return S_OK;
} 

/* Direct Show */

static void free_media_type(AM_MEDIA_TYPE* &mt) {
    if (mt != null) {
        if (mt->cbFormat != 0 && mt->pbFormat != null) {
            CoTaskMemFree(mt->pbFormat);
            mt->cbFormat = 0;
            mt->pbFormat = null;
        }
        if (mt->pUnk != null) {
            com_release(mt->pUnk);
        }
        CoTaskMemFree(mt);
        mt = null;
    }
}

static IPin* ds_get_pin(IBaseFilter* filter, const char* pin_name) {
    com_t<IEnumPins> enum_pins;
    IPin* pin = null;		
    if_failed_return(filter->EnumPins(&enum_pins), null);
    while (enum_pins->Next(1, &pin, 0) == S_OK) {
        PIN_INFO pin_info = {0};
        pin->QueryPinInfo(&pin_info);
        com_release(pin_info.pFilter);
        if (stricmp(pin_name, wcs2str(pin_info.achName)) == 0) {
            return pin;
        }
        com_release(pin);
    }
    return null;
}

static int ds_get_media_types(IBaseFilter* camera) {
    com_t<IPin> camera_output_pin = ds_get_pin(camera, "capture");
    com_t<IAMStreamConfig> sc;
    if_failed_return_result(camera_output_pin->QueryInterface(&sc));
    int number_of_capabilities = 0;
    int capability_size = 0;
    if_failed_return(sc->GetNumberOfCapabilities(&number_of_capabilities, &capability_size), -1);
    for (int i = 0; i < number_of_capabilities; i++) {
        VIDEO_STREAM_CONFIG_CAPS scc;
        assert(sizeof(scc) == capability_size);
        AM_MEDIA_TYPE* mt = null;
        if_failed_return(sc->GetStreamCaps(i, &mt, (BYTE*)&scc), -1);
        BITMAPINFOHEADER* bih = null;
        if (mt->majortype == MEDIATYPE_Video && mt->formattype == FORMAT_VideoInfo2) {
            bih = &((VIDEOINFOHEADER2*)(mt->pbFormat))->bmiHeader;
        } else if (mt->majortype == MEDIATYPE_Video && mt->formattype == FORMAT_VideoInfo) { // FORMAT_VideoInfo == CLSID_KsDataTypeHandlerVideo
            bih = &((VIDEOINFOHEADER*)(mt->pbFormat))->bmiHeader; // VIDEOINFOHEADER2 died in Win10 build 14316
        }
        if (bih != null) {
            print("[%2d] %-5s %4d x %-4d\n", i, guid2format(mt->subtype), bih->biWidth, bih->biHeight);
        }
        free_media_type(mt);
    }
    return 0;
}

static int ds_enumerate_cameras(int (*callback)(IBaseFilter* camera)) {
    com_t<ICreateDevEnum> enumerator;
    if_failed_return(CoCreateInstance(CLSID_SystemDeviceEnum, null, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&enumerator)), -1);
    com_t<IEnumMoniker> class_enumerator;
    if_failed_return(enumerator->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &class_enumerator, 0), -1);
    int n = 0;
    while (class_enumerator != null) {
        com_t<IMoniker> moniker;
        ULONG fetched = 0;
        if (class_enumerator->Next(1, &moniker, &fetched) != S_OK || fetched != 1 || moniker == null) {
            break;
        }
        com_t<IPropertyBag> prop_bag;
        HRESULT hr = moniker->BindToStorage(0, 0, IID_PPV_ARGS(&prop_bag));
        if (OK(hr)) {
            com_t<IBaseFilter> camera;
            if (OK(moniker->BindToObject(0, 0, IID_PPV_ARGS(&camera)))) {
                const char* dp = "";
                const char* fn = "";
                VARIANT friendly_name;
                VariantInit(&friendly_name);
                if (OK(prop_bag->Read(L"FriendlyName", &friendly_name, 0))) { fn = wcs2str(friendly_name.bstrVal); }
                VARIANT device_path;
                VariantInit(&device_path);
                if (OK(prop_bag->Read(L"DevicePath", &device_path, 0))) { dp = wcs2str(device_path.bstrVal); }
                VariantClear(&friendly_name);
                VariantClear(&device_path);
                print("\nDirectShow camera %d %s %s\n", n++, fn, dp);
                callback(camera);
            } else {
                print("%s hr=0x%08X\n", strwinerr(hr), hr);
            }
        } else {
            print("%s hr=0x%08X\n", strwinerr(hr), hr);
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    if_failed_return(CoInitializeEx(0, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY), GetLastError());
    if_failed_return_result(MFStartup(MF_VERSION));
    if_failed_return_result(mf_enumerate_cameras(mf_get_media_types));
    if_failed_return_result(MFShutdown());
    if_failed_return_result(ds_enumerate_cameras(ds_get_media_types));
    getchar();
    CoUninitialize();
    return 0;
}

static const char* guid2format(const GUID &guidFormat) {
    #define if_return_str(guid, str) if (guidFormat == guid) { return str; }
    if_return_str(MFVideoFormat_RGB8  , "RGB8");
    if_return_str(MFVideoFormat_RGB555, "RGB555");
    if_return_str(MFVideoFormat_RGB565, "RGB565");
    if_return_str(MFVideoFormat_RGB24 , "RGB24");
    if_return_str(MFVideoFormat_RGB32 , "RGB32");
    if_return_str(MFVideoFormat_ARGB32, "ARGB32");
    if_return_str(MFVideoFormat_AI44  , "AI44");
    if_return_str(MFVideoFormat_AYUV  , "AYUV");
    if_return_str(MFVideoFormat_I420  , "I420");
    if_return_str(MFVideoFormat_IYUV  , "IYUV");
    if_return_str(MFVideoFormat_NV11  , "NV11");
    if_return_str(MFVideoFormat_NV12  , "NV12");
    if_return_str(MFVideoFormat_UYVY  , "UYVY");
    if_return_str(MFVideoFormat_Y41P  , "Y41P");
    if_return_str(MFVideoFormat_Y41T  , "Y41T");
    if_return_str(MFVideoFormat_Y42T  , "Y42T");
    if_return_str(MFVideoFormat_YUY2  , "YUY2");
    if_return_str(MFVideoFormat_YVU9  , "YVU9");
    if_return_str(MFVideoFormat_YV12  , "YV12");
    if_return_str(MFVideoFormat_YVYU  , "YVYU");

    if_return_str(MFVideoFormat_Y210, "Y210");
    if_return_str(MFVideoFormat_Y216, "Y216");
    if_return_str(MFVideoFormat_Y410, "Y410");
    if_return_str(MFVideoFormat_Y416, "Y416");
    if_return_str(MFVideoFormat_P210, "P210");
    if_return_str(MFVideoFormat_P216, "P216");
    if_return_str(MFVideoFormat_P010, "P010");
    if_return_str(MFVideoFormat_P016, "P016");
    if_return_str(MFVideoFormat_v210, "v210");
    if_return_str(MFVideoFormat_v216, "v216");
    if_return_str(MFVideoFormat_v410, "v410");
    if_return_str(MFVideoFormat_MP43, "MP43");
    if_return_str(MFVideoFormat_MP4S, "MP4S");
    if_return_str(MFVideoFormat_M4S2, "M4S2");
    if_return_str(MFVideoFormat_MP4V, "MP4V");
    if_return_str(MFVideoFormat_WMV1, "WMV1");
    if_return_str(MFVideoFormat_WMV2, "WMV2");
    if_return_str(MFVideoFormat_WMV3, "WMV3");
    if_return_str(MFVideoFormat_WVC1, "WVC1");
    if_return_str(MFVideoFormat_MSS1, "MSS1");
    if_return_str(MFVideoFormat_MSS2, "MSS2");
    if_return_str(MFVideoFormat_MPG1, "MPG1");
    if_return_str(MFVideoFormat_DVSL, "DVSL");
    if_return_str(MFVideoFormat_DVSD, "DVSD");
    if_return_str(MFVideoFormat_DVHD, "DVHD");
    if_return_str(MFVideoFormat_DV25, "DV25");
    if_return_str(MFVideoFormat_DV50, "DV50");
    if_return_str(MFVideoFormat_DVH1, "DVH1");
    if_return_str(MFVideoFormat_DVC,  "DVC");
    if_return_str(MFVideoFormat_H264, "H264");
    if_return_str(MFVideoFormat_MJPG, "MJPG");
    if_return_str(MFVideoFormat_420O, "420O");
    if_return_str(MEDIASUBTYPE_RGB24, "RGB24");
    static thread_local_storage char cc4[32];
    const char* bytes = (const char*)&guidFormat.Data1;
    if ('0' <= bytes[0] && bytes[0] <= 'Z') {
        memcpy(cc4, &guidFormat.Data1, 4); cc4[4] = 0;
    } else {
        _snprintf(cc4, countof(cc4), "0x%08X", guidFormat.Data1);
    }
    return cc4;
}


#ifdef __cplusplus
} // extern "C"
#endif

/*

Windows 10 version 1511 (OS Build 10586.494)

MMF camera 0 Logitech HD Pro Webcam C920 \\?\usb#vid_046d&pid_082d&mi_00#6&17c8b6a4&0&0000#{e5323777-f976-4f5b-9b55-b94699c46e44}\{bbefb6c7-2fc4-4139-bb8b-a58bba724083}
[ 0] MJPG   640x480  @ 30
[ 1] MJPG   160x90   @ 30
[ 2] MJPG   160x120  @ 30
[ 3] MJPG   176x144  @ 30
[ 4] MJPG   320x180  @ 30
[ 5] MJPG   320x240  @ 30
[ 6] MJPG   352x288  @ 30
[ 7] MJPG   432x240  @ 30
[ 8] MJPG   640x360  @ 30
[ 9] MJPG   800x448  @ 30
[10] MJPG   800x600  @ 30
[11] MJPG   864x480  @ 30
[12] MJPG   960x720  @ 30
[13] MJPG  1024x576  @ 30
[14] MJPG  1280x720  @ 30
[15] MJPG  1600x896  @ 30
[16] MJPG  1920x1080 @ 30
[17] RGB24  640x480  @ 30
[18] RGB24  160x90   @ 30
[19] RGB24  160x120  @ 30
[20] RGB24  176x144  @ 30
[21] RGB24  320x180  @ 30
[22] RGB24  320x240  @ 30
[23] RGB24  352x288  @ 30
[24] RGB24  432x240  @ 30
[25] RGB24  640x360  @ 30
[26] RGB24  800x448  @ 30
[27] RGB24  800x600  @ 30
[28] RGB24  864x480  @ 30
[29] RGB24  960x720  @ 30
[30] RGB24 1024x576  @ 30
[31] RGB24 1280x720  @ 30
[32] RGB24 1600x896  @ 30
[33] RGB24 1920x1080 @  5
[34] RGB24 2304x1296 @  2
[35] RGB24 2304x1536 @  2
[36] I420   640x480  @ 30
[37] I420   160x90   @ 30
[38] I420   160x120  @ 30
[39] I420   176x144  @ 30
[40] I420   320x180  @ 30
[41] I420   320x240  @ 30
[42] I420   352x288  @ 30
[43] I420   432x240  @ 30
[44] I420   640x360  @ 30
[45] I420   800x448  @ 30
[46] I420   800x600  @ 30
[47] I420   864x480  @ 30
[48] I420   960x720  @ 30
[49] I420  1024x576  @ 30
[50] I420  1280x720  @ 30
[51] I420  1600x896  @ 30
[52] I420  1920x1080 @  5
[53] I420  2304x1296 @  2
[54] I420  2304x1536 @  2

DirectShow camera 0 Logitech HD Pro Webcam C920 \\?\usb#vid_046d&pid_082d&mi_00#6&17c8b6a4&0&0000#{65e8773d-8f56-11d0-a3b9-00a0c9223196}\{bbefb6c7-2fc4-4139-bb8b-a58bba724083}
[ 0] MJPG   640 x 480
[ 1] MJPG   160 x 90
[ 2] MJPG   160 x 120
[ 3] MJPG   176 x 144
[ 4] MJPG   320 x 180
[ 5] MJPG   320 x 240
[ 6] MJPG   352 x 288
[ 7] MJPG   432 x 240
[ 8] MJPG   640 x 360
[ 9] MJPG   800 x 448
[10] MJPG   800 x 600
[11] MJPG   864 x 480
[12] MJPG   960 x 720
[13] MJPG  1024 x 576
[14] MJPG  1280 x 720
[15] MJPG  1600 x 896
[16] MJPG  1920 x 1080
[17] RGB24  640 x 480
[18] RGB24  160 x 90
[19] RGB24  160 x 120
[20] RGB24  176 x 144
[21] RGB24  320 x 180
[22] RGB24  320 x 240
[23] RGB24  352 x 288
[24] RGB24  432 x 240
[25] RGB24  640 x 360
[26] RGB24  800 x 448
[27] RGB24  800 x 600
[28] RGB24  864 x 480
[29] RGB24  960 x 720
[30] RGB24 1024 x 576
[31] RGB24 1280 x 720
[32] RGB24 1600 x 896
[33] RGB24 1920 x 1080
[34] RGB24 2304 x 1296
[35] RGB24 2304 x 1536
[36] I420   640 x 480
[37] I420   160 x 90
[38] I420   160 x 120
[39] I420   176 x 144
[40] I420   320 x 180
[41] I420   320 x 240
[42] I420   352 x 288
[43] I420   432 x 240
[44] I420   640 x 360
[45] I420   800 x 448
[46] I420   800 x 600
[47] I420   864 x 480
[48] I420   960 x 720
[49] I420  1024 x 576
[50] I420  1280 x 720
[51] I420  1600 x 896
[52] I420  1920 x 1080
[53] I420  2304 x 1296
[54] I420  2304 x 1536
*/