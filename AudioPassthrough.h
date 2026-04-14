#pragma once
#include "portaudio.h"
#include "pa_ringbuffer.h"
#include <cassert>

struct audioRingBuffer
{
public:
	PaUtilRingBuffer ringBuffer;

	audioRingBuffer(size_t elementCount) {
		buffer = new float[elementCount];
		assert(PaUtil_InitializeRingBuffer(&ringBuffer, sizeof(float), elementCount, buffer) == 0);
	}

	~audioRingBuffer() {
		delete[] buffer;
	}
private:
	float* buffer;
};

/*static int paPassInCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	audioRingBuffer* data = (audioRingBuffer*)userData;
	float* in = (float*)input;
	(void)output; // Prevent unused variable warning

	PaUtil_WriteRingBuffer(&data->ringBuffer, in, frameCount);
	return paContinue;
}

static int paPassOutCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	audioRingBuffer* data = (audioRingBuffer*)userData;
	float* out = (float*)output;
	(void)input; // Prevent unused variable warning

	PaUtil_ReadRingBuffer(&data->ringBuffer, out, frameCount * 2); // Read stereo samples (2 channels)
	return paContinue;

	//PaUtil_WriteRingBuffer(&data->ringBuffer, in, frameCount);
}*/

static int paPassThroughCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	float* in = (float*)input;
	float* out = (float*)output;
	float sample;

	if (statusFlags) {
		std::cout << "Audio glitch: " << statusFlags << "\n";
	}
	
	for (unsigned int i = 0; i < frameCount; i++) {
		sample = in ? *in++ : 0.0f;
		*out++ = sample * 0.5f;
		*out++ = sample * 0.5f;
	}

	return paContinue;
}