#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include <iostream>
#include "portaudio.h"
#include <algorithm>

typedef struct {
	float left_phase;
	float right_phase;
} paStereoPhase;

typedef struct {
	float phase;
	float dir;
} paMonoPhase;

enum paADSRState
{
	Attack,
	Decay,
	Sustain,
	Release,
	Idle
};

typedef struct {
	paADSRState state;
	float value; // Current envelope value

	float rates[4]; // Attack, Decay, Sustain, Release rates
	float levels[4]; // Attack, Decay, Sustain, Release levels
} paADSR;

typedef struct {
	float phase;
	float frequency;
	float sampleRate;
	paADSR ADSR;
} paSoundData;

void checkPaError(const PaError err) {
	if (err != paNoError) {
		std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
		exit(-1);
	}
}

static int paTriangleWaveCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) 
{
	paMonoPhase* data = (paMonoPhase*)userData;
	float* out = (float*)output;
	(void)input; // Prevent unused variable warning

	for (unsigned int i = 0; i < frameCount; i++) {
		*out++ = data->phase; // Left channel
		*out++ = data->phase; // Right channel
		data->phase += data->dir;

		if (data->phase >= 1.0f || data->phase <= -1.0f) {
			data->dir = -data->dir; // Reverse direction
			data->phase = std::clamp(data->phase, -1.0f, 1.0f); // Clamp to prevent overshooting
		}
	}

	return paContinue;
}

static int paSinWaveCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
		paMonoPhase* data = (paMonoPhase*)userData;
	float* out = (float*)output;
	(void)input; // Prevent unused variable warning
	for (unsigned int i = 0; i < frameCount; i++) {
		float sample = sinf(data->phase);
		*out++ = sample; // Left channel
		*out++ = sample; // Right channel
		data->phase += data->dir;
		if (data->phase >= 2.0f * M_PI) {
			data->phase -= 2.0f * M_PI; // Wrap around
		}
	}
	return paContinue;
}

static int paToneCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	paSoundData* data = (paSoundData*)userData;
	float* out = (float*)output;
	(void)input; // Prevent unused variable warning

	float delta = 2.0f * M_PI * data->frequency / data->sampleRate;

	for (unsigned int i = 0; i < frameCount; i++) {
		float sample = sinf(data->phase);

		// Apply ADSR envelope
		sample = sample * data->ADSR.value;

		switch (data->ADSR.state) {
		case Attack:
			data->ADSR.value += data->ADSR.rates[Attack];
			if (data->ADSR.value >= data->ADSR.levels[Attack]) {
				data->ADSR.value = data->ADSR.levels[Attack];
				data->ADSR.state = Decay;
			}
			break;

		case Idle:
			data->ADSR.value = 0.0f;
			break;

		default:
			data->ADSR.value += data->ADSR.rates[data->ADSR.state];
			if (data->ADSR.value <= data->ADSR.levels[data->ADSR.state]) {
				data->ADSR.value = data->ADSR.levels[data->ADSR.state];
				data->ADSR.state = static_cast<paADSRState>(data->ADSR.state + 1);
			}
			break;
		}

		*out++ = sample; // Left channel
		*out++ = sample; // Right channel

		data->phase += delta;
		if (data->phase >= 2.0f * M_PI) {
			data->phase -= 2.0f * M_PI; // Wrap around
		}
	}

	return paContinue;
}