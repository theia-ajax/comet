#include "cvc.h"

#include <opencv4/opencv2/videoio.hpp>
#include <vector>

CVCVideoCapture* CVCVideoCaptureOpenAny(const char* FileName)
{
	CVCVideoCaptureOpenWithApiAndParams(FileName, CVCVideoCaptureApis_Any, nullptr, 0);
}

CVCVideoCapture* CVCVideoCaptureOpenWithApi(const char* FileName, CVCVideoCaptureApis Api)
{
	return CVCVideoCaptureOpenWithApiAndParams(FileName, Api, nullptr, 0);
}

CVCVideoCapture* CVCVideoCaptureOpenWithApiAndParams(
	const char* FileName,
	CVCVideoCaptureApis Api,
	const CVCVideoCaptureProperty* Params,
	size_t ParamCount)
{
	std::vector<int> ParamVector;
	ParamVector.reserve(ParamCount * 2);

	for (size_t PropIndex = 0; PropIndex < ParamCount; PropIndex++) {
		ParamVector.push_back(Params[PropIndex].Key);
		ParamVector.push_back(Params[PropIndex].Value);
	}

	cv::VideoCapture* NewVideoCapture = nullptr;

	try {
		NewVideoCapture = new cv::VideoCapture(std::string(FileName), Api, ParamVector);
	} catch (...) {
		NewVideoCapture = nullptr;
	}

	CVCVideoCapture* Result = reinterpret_cast<CVCVideoCapture*>(NewVideoCapture);

	return Result;
}

void CVCDestroyVideoCapture(CVCVideoCapture* VideoCapture)
{
	if (VideoCapture != nullptr) {
		cv::VideoCapture* RawVideoCapture = reinterpret_cast<cv::VideoCapture*>(VideoCapture);
		delete RawVideoCapture;
	}
}