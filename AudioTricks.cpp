// AudioTricks.cpp : Defines the entry point for the application.
//
#include "BasePortAudio.h"
#include "AudioPassthrough.h"

#define SAMPLE_RATE (44100)

PaError err;

int main()
{
	checkPaError(Pa_Initialize());

	paSoundData data{ 0.0f, 440.0f, SAMPLE_RATE, 
		{Attack, 0.0f, 
		{1.0f/(0.2f*SAMPLE_RATE), (0.5f-1.0f)/(1.0f*SAMPLE_RATE), (0.2f-0.5f)/(1.0f*SAMPLE_RATE), (0.0f-0.2f)/(0.5f*SAMPLE_RATE)},
		{1.0f, 0.5f, 0.2f, 0.0f}} };

	audioRingBuffer ringBuffer(1024); // 10 seconds of audio at 44.1kHz

	PaStream* stream;
	/* Open an audio I/O stream. */
	err = Pa_OpenDefaultStream(&stream,
		1,          /* 1 input channel */
		2,          /* stereo output */
		paFloat32,  /* 32 bit floating point output */
		SAMPLE_RATE,
		paFramesPerBufferUnspecified,
						   /* frames per buffer, i.e. the number
						   of sample frames that PortAudio will
						   request from the callback. Many apps
						   may want to use
						   paFramesPerBufferUnspecified, which
						   tells PortAudio to pick the best,
						   possibly changing, buffer size.*/
		paPassThroughCallback, /* this is your callback function */
		&ringBuffer); /*This is a pointer that will be passed to
						   your callback*/
	if (err != paNoError) {
		std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
		return -1;
	}

	checkPaError(Pa_StartStream(stream));

	// Wait for user input to stop the stream
	std::cout << "Press Enter to stop the stream..." << std::endl;
	std::cin.get();

	checkPaError(Pa_StopStream(stream));

	checkPaError(Pa_CloseStream(stream));

	checkPaError(Pa_Terminate());

	return 0;
}
