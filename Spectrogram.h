#pragma once
#define _USE_MATH_DEFINES

#include "portaudio.h"
#include "pa_ringbuffer.h"
#include "fftw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <GLFW/glfw3.h>

#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>

struct audioRingBuffer
{
public:
	PaUtilRingBuffer ringBuffer;
	const size_t sampleRate;

	audioRingBuffer(size_t elementCount, size_t sampleRate) : sampleRate(sampleRate) {
		buffer = new float[elementCount];
		assert(PaUtil_InitializeRingBuffer(&ringBuffer, sizeof(float), elementCount, buffer) == 0);
	}

	~audioRingBuffer() {
		delete[] buffer;
	}
private:
	float* buffer;
};

enum class windowMethod
{
	Hanning,
	Blackman,
	BlackmanHarris
};

int paRingBufferCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

class Spectrogram
{
public:
	Spectrogram(size_t fftSize, size_t hopDivisor, audioRingBuffer* ringBuffer, windowMethod method = windowMethod::Hanning);
	~Spectrogram();

	// Process the next audio block and update the spectrogram data
	int processAudioBlock();
	int render();

	GLFWwindow* gWindow = nullptr;
	std::vector<std::vector<float>> plotData;
private:
	const size_t fftSize;
	const size_t numBins;
	const size_t hopSize;
	audioRingBuffer* ringBuffer;

	fftwf_plan fftPlan;
	std::vector<float> window;
	float* fftInput;
	fftwf_complex* fftOutput;
	std::vector<float> overlapBuffer;
	const float normalization;
	float minMag;
	float maxMag;
	bool logScale = true;
	const size_t maxHistory = 500; // Max number of columns to keep in the spectrogram

	std::vector<float> freq;

	size_t removedCols = 0;
	float getColTime(size_t index);
};