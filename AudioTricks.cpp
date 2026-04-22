// AudioTricks.cpp : Defines the entry point for the application.
//
#include "BasePortAudio.h"
#include "Spectrogram.h"
#include <numeric>

#define SAMPLE_RATE (44100)

PaError err;

int main()
{
	checkPaError(Pa_Initialize());

	audioRingBuffer ringBuffer(2048*4, SAMPLE_RATE);

	PaStream* stream;
	/* Open an audio I/O stream. */
	err = Pa_OpenDefaultStream(&stream,
		1,
		0,
		paFloat32,
		SAMPLE_RATE,
		paFramesPerBufferUnspecified,
		paRingBufferCallback,
		&ringBuffer);

	if (err != paNoError) {
		std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
		return -1;
	}

	Spectrogram spectrogram(1024, 4, &ringBuffer, windowMethod::BlackmanHarris);

	checkPaError(Pa_StartStream(stream));

	// Wait for user input to stop the stream
	/*std::cout << "Press Enter to stop the stream..." << std::endl;
	std::cin.get();*/
	size_t k = 0;
	while (spectrogram.atmRend.load()) {
		spectrogram.processAudioBlock();
	}

	checkPaError(Pa_StopStream(stream));

	checkPaError(Pa_CloseStream(stream));

	checkPaError(Pa_Terminate());

	//std::cin.get(); // Wait for user input before exiting

	return 0;
}
