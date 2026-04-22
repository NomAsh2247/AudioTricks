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
#include <mutex>
#include <thread>
#include <atomic>

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

	std::atomic<bool> atmRend;
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
	bool logMagnitude = true;
	bool logFrequency = true;
	const size_t maxHistory = 500; // Max number of columns to keep in the spectrogram
	GLFWwindow* gWindow = nullptr;

	std::vector<float> freq;

	size_t removedCols = 0, rendCols = 0;
	std::vector<std::vector<float>> procData, rendData;
	std::mutex mtxRend;
	std::thread thrRend;

	float getColTime(size_t index);
	float getRendColTime(size_t index);
	void initRender();
	void freeRender();
};