#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CVCVideoCaptureApis {
	CVCVideoCaptureApis_Any = 0,
	CVCVideoCaptureApis_ObsoleteVFW = 200,
	CVCVideoCaptureApis_V4L = 200,
	CVCVideoCaptureApis_V4L2 = CVCVideoCaptureApis_V4L,
	CVCVideoCaptureApis_Firewire = 300,
	CVCVideoCaptureApis_Fireware = CVCVideoCaptureApis_Firewire,
	CVCVideoCaptureApis_IEEE1394 = CVCVideoCaptureApis_Firewire,
	CVCVideoCaptureApis_DC1394 = CVCVideoCaptureApis_Firewire,
	CVCVideoCaptureApis_CMU1394 = CVCVideoCaptureApis_Firewire,
	CVCVideoCaptureApis_ObsoleteQuicktime = 500,
	CVCVideoCaptureApis_ObsoleteUnicap = 600,
	CVCVideoCaptureApis_DirectShow = 700,
	CVCVideoCaptureApis_PvApi = 800,
	CVCVideoCaptureApis_OpenNaturalInteraction = 900,
	CVCVideoCaptureApis_OpenNaturalInteractionAsus = 910,
	CVCVideoCaptureApis_Android = 1000,
	CVCVideoCaptureApis_XimeaApi = 1100,
	CVCVideoCaptureApis_AvFoundation = 1200,
	CVCVideoCaptureApis_Giganetix = 1300,
	CVCVideoCaptureApis_MicrosoftMediaFoundation = 1400,
	CVCVideoCaptureApis_WinRt = 1410,
	CVCVideoCaptureApis_RealSense = 1500,
	CVCVideoCaptureApis_IntelPerceptualComputing = CVCVideoCaptureApis_RealSense,
	CVCVideoCaptureApis_OpenNaturalInteraction2 = 1600,
	CVCVideoCaptureApis_OpenNaturalInteraction2_Asus = 1610,
	CVCVideoCaptureApis_OpenNaturalInteraction2_Astra = 1620,
	CVCVideoCaptureApis_GPhoto2 = 1700,
	CVCVideoCaptureApis_GStreamer = 1800,
	CVCVideoCaptureApis_FFMPEG = 1900,
	CVCVideoCaptureApis_Images = 2000,
	CVCVideoCaptureApis_Aravis = 2100,
	CVCVideoCaptureApis_OpenCvMotionJpeg = 2200,
	CVCVideoCaptureApis_IntelMediaSdk = 2300,
	CVCVideoCaptureApis_Xine = 2400,
	CVCVideoCaptureApis_UEye = 2500,
	CVCVideoCaptureApis_ObSensor = 2600,
} CVCVideoCaptureApis;

typedef enum CVVideoCaptureProps {
	CVVideoCaptureProps_PositionMilliSeconds,
	CVVideoCaptureProps_PositionFrames,
	CVVideoCaptureProps_AviRatio,
	CVVideoCaptureProps_FrameWidth,
	CVVideoCaptureProps_FrameHeight,
	CVVideoCaptureProps_FramesPerSecond,
	CVVideoCaptureProps_FourCharacterCode,
	CVVideoCaptureProps_FrameCount,
	CVVideoCaptureProps_Format,
	CVVideoCaptureProps_Mode,
	CVVideoCaptureProps_Brightness,
	CVVideoCaptureProps_Contrast,
	CVVideoCaptureProps_Saturation,
	CVVideoCaptureProps_Hue,
	CVVideoCaptureProps_Gain,
	CVVideoCaptureProps_Exposure,
	CVVideoCaptureProps_ConvertRgb,
	CVVideoCaptureProps_WhiteBalanceBlueUnsupported,
	CVVideoCaptureProps_Rectification,
	CVVideoCaptureProps_Monochrome,
	CVVideoCaptureProps_Sharpness,
	CVVideoCaptureProps_AutoExposure,
	CVVideoCaptureProps_Gamma,
	CVVideoCaptureProps_Temperature,
	CVVideoCaptureProps_Trigger,
	CVVideoCaptureProps_TriggerDelay,
	CVVideoCaptureProps_WhiteBalanceRedV,
	CVVideoCaptureProps_Zoom,
	CVVideoCaptureProps_Focus,
	CVVideoCaptureProps_Guid,
	CVVideoCaptureProps_IsoSpeed,
	CVVideoCaptureProps_InvalidProperty0,
	CVVideoCaptureProps_Backlight,
	CVVideoCaptureProps_Pan,
	CVVideoCaptureProps_Tilt,
	CVVideoCaptureProps_Roll,
	CVVideoCaptureProps_Iris,
	CVVideoCaptureProps_Settings,
	CVVideoCaptureProps_BufferSize,
	CVVideoCaptureProps_AutoFocus,
	CVVideoCaptureProps_SARNumerator,
	CVVideoCaptureProps_SARDenominator,
	CVVideoCaptureProps_Backend,
	CVVideoCaptureProps_Channel,
	CVVideoCaptureProps_AutoWhiteBalance,
	CVVideoCaptureProps_WhiteBalanceTemperature,
	CVVideoCaptureProps_CodecPixelFormat,
	CVVideoCaptureProps_BitRate,
	CVVideoCaptureProps_OrientationMeta,
	CVVideoCaptureProps_OrientationAuto,
	CVVideoCaptureProps_HardwareAcceleration,
	CVVideoCaptureProps_HardwareDevice,
	CVVideoCaptureProps_HardwareAccelerationUseOpenCL,
	CVVideoCaptureProps_OpenTimeoutMilliSeconds,
	CVVideoCaptureProps_ReadTimeoutMilliSeconds,
	CVVideoCaptureProps_StreamOpenTimeMicroSeconds,
	CVVideoCaptureProps_VideoTotalChannels,
	CVVideoCaptureProps_VideoStream,
	CVVideoCaptureProps_AudioStream,
	CVVideoCaptureProps_AudioPosition,
	CVVideoCaptureProps_AudioShiftNanoSeconds,
	CVVideoCaptureProps_AudioDataDepth,
	CVVideoCaptureProps_AudioSamplesPerSecond,
	CVVideoCaptureProps_AudioBaseIndex,
	CVVideoCaptureProps_AudioTotalChannel,
	CVVideoCaptureProps_AudioTotalStreams,
	CVVideoCaptureProps_AudioSynchronize,
	CVVideoCaptureProps_LastRawFrameHasKeyFrame,
	CVVideoCaptureProps_CodecExtraDataIndex,
	CVVideoCaptureProps_FrameType,
	CVVideoCaptureProps_NumThreads,
	CVVideoCaptureProps_PresentationTimestamp,
	CVVideoCaptureProps_DecompressionTimestampsDelay,

	CVVideoCaptureProps_Count,
	CVVideoCaptureProps_Latest = CVVideoCaptureProps_Count,
} CVVideoCaptureProperties;

typedef struct CVCVideoCapture CVCVideoCapture;

typedef struct CVCVideoCaptureProperty {
	CVVideoCaptureProperties Key;
	int Value;
} CVCVideoCaptureProperty;

CVCVideoCapture* CVCVideoCaptureOpenAny(const char* FileName);
CVCVideoCapture* CVCVideoCaptureOpenWithApi(const char* FileName, CVCVideoCaptureApis Api);
CVCVideoCapture* CVCVideoCaptureOpenWithApiAndParams(
	const char* FileName,
	CVCVideoCaptureApis Api,
	const CVCVideoCaptureProperty* Props,
	size_t PropCount);

void CVCDestroyVideoCapture(CVCVideoCapture *VideoCapture);

#ifdef __cplusplus
}
#endif