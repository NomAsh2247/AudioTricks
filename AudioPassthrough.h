#pragma once
#include "portaudio.h"
#include <iostream>

static int paPassThroughCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	float* in = (float*)input;
	float* out = (float*)output;
	float sample;

	if (statusFlags) {
		std::cerr << "Audio callback error: " << statusFlags << "\n";
	}
	
	for (unsigned int i = 0; i < frameCount; i++) {
		sample = in ? *in++ : 0.0f;
		*out++ = sample;
		*out++ = sample;
	}

	return paContinue;
}