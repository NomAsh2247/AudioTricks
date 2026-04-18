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

	audioRingBuffer ringBuffer(2048, SAMPLE_RATE);

	PaStream* stream;
	/* Open an audio I/O stream. */
	err = Pa_OpenDefaultStream(&stream,
		1,
		2,
		paFloat32,
		SAMPLE_RATE,
		paFramesPerBufferUnspecified,
		paRingBufferCallback,
		&ringBuffer);

	if (err != paNoError) {
		std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
		return -1;
	}

	Spectrogram spectrogram(1024, 4, &ringBuffer);

	checkPaError(Pa_StartStream(stream));

	// Wait for user input to stop the stream
	/*std::cout << "Press Enter to stop the stream..." << std::endl;
	std::cin.get();*/

	while (!glfwWindowShouldClose(spectrogram.gWindow)) {
		if (spectrogram.processAudioBlock() != 0) {
			// Sleep briefly to avoid busy-waiting if not enough audio data is available
			Pa_Sleep(10);
		}
		else {
			//std::cout << "Processed audio block, rendering spectrogram...\n";
			spectrogram.render(ImVec2(800, 600));
		}
	}

	checkPaError(Pa_StopStream(stream));

	checkPaError(Pa_CloseStream(stream));

	checkPaError(Pa_Terminate());

	//std::cin.get(); // Wait for user input before exiting

	return 0;
}
