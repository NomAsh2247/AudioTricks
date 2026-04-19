#include "Spectrogram.h"

int paRingBufferCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	audioRingBuffer* ringBuffer = (audioRingBuffer*)userData;
	const float* in = (const float*)input;
	//float* out = (float*)output;

	if (statusFlags) {
		std::cerr << "Audio callback error: " << statusFlags << "\n";
	}

	ring_buffer_size_t written = PaUtil_WriteRingBuffer(&ringBuffer->ringBuffer, in, frameCount);
	if (written < frameCount) {
		std::cerr << "Ring buffer overflow: Unable to write " << (frameCount - written) * 100 / frameCount << "% samples (" << (frameCount - written) << " samples dropped)\n";
	}

	/*for (unsigned int i = 0; i < frameCount; i++) {
		float sample = in ? *in++ : 0.0f;
		*out++ = sample;
		*out++ = sample;

	}*/
	return paContinue;
}

void glfw_error_callback(int error, const char* description) { std::cerr << "GLFW Error " << error << ": " << description << std::endl; }

Spectrogram::Spectrogram(size_t fftSize, size_t hopDivisor, audioRingBuffer* ringBuffer, windowMethod method) : fftSize(fftSize), numBins(fftSize / 2 + 1), hopSize(fftSize / hopDivisor), ringBuffer(ringBuffer), normalization(static_cast<float>(fftSize) * 0.5f)
{
	// Create window
	window.resize(fftSize);
	
	switch (method)
	{
	default:
	case windowMethod::Hanning:
		for (size_t i = 0; i < fftSize; i++) {
			window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (fftSize - 1)));
		}
		break;
	case windowMethod::Blackman:
		for (size_t i = 0; i < fftSize; i++) {
			window[i] = 0.42f - 0.5f * cosf(2.0f * M_PI * i / (fftSize - 1)) + 0.08f * cosf(4.0f * M_PI * i / (fftSize - 1));
		}
		break;
	case windowMethod::BlackmanHarris:
		for (size_t i = 0; i < fftSize; i++) {
			window[i] = 0.35875f - 0.48829f * cosf(2.0f * M_PI * i / (fftSize - 1)) + 0.14128f * cosf(4.0f * M_PI * i / (fftSize - 1)) - 0.01168f * cosf(6.0f * M_PI * i / (fftSize - 1));
		}
		break;
	}

	// Allocate FFT input and output arrays
	fftInput = new float[fftSize];
	fftOutput = new fftwf_complex[numBins];
	plotData.resize(maxHistory, std::vector<float>(numBins, minDb));
	overlapBuffer.resize(fftSize, 0.0f);

	// Precalculate frequencies per bin
	freq.resize(numBins);
	for (int i = 0; i < numBins; i++) {
		freq[i] = ((float)(i) * ringBuffer->sampleRate) / (float)(fftSize);
	}

	// Create FFT plan
	fftPlan = fftwf_plan_dft_r2c_1d(static_cast<int>(fftSize), fftInput, fftOutput, FFTW_MEASURE);

	// IMGUI setup
	glfwSetErrorCallback(glfw_error_callback);

	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return;
	}

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	gWindow = glfwCreateWindow(1200, 800, "Spectrogram", nullptr, nullptr);
	if (!gWindow) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(gWindow);
	glfwSwapInterval(0); // Disable vsync

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

Spectrogram::~Spectrogram()
{
	fftwf_destroy_plan(fftPlan);
	delete[] fftInput;
	delete[] fftOutput;

	// Cleanup ImGui and GLFW
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
	glfwDestroyWindow(gWindow);
	glfwTerminate();
}

int Spectrogram::processAudioBlock()
{
	size_t availableSamples = PaUtil_GetRingBufferReadAvailable(&ringBuffer->ringBuffer);

	// Need at least hopSize new samples to advance one frame
	while (availableSamples >= hopSize) {

		// Shift old samples left by hopSize
		std::move(
			overlapBuffer.begin() + hopSize,
			overlapBuffer.end(),
			overlapBuffer.begin()
		);

		// Read new samples into the end of the overlap buffer
		PaUtil_ReadRingBuffer(
			&ringBuffer->ringBuffer,
			overlapBuffer.data() + (fftSize - hopSize),
			hopSize
		);

		availableSamples -= hopSize;

		// Copy overlap buffer into FFT input
		std::copy(overlapBuffer.begin(), overlapBuffer.end(), fftInput);

		// Remove DC offset
		float mean = 0.0f;
		for (size_t i = 0; i < fftSize; i++) {
			mean += fftInput[i];
		}
		mean /= static_cast<float>(fftSize);

		// Apply DC removal and window
		for (size_t i = 0; i < fftSize; i++) {
			fftInput[i] = (fftInput[i] - mean) * window[i];
		}

		// Execute FFT
		fftwf_execute(fftPlan);

		std::vector<float> magnitudes(numBins);

		for (size_t i = 0; i < numBins; i++) {
			const float re = fftOutput[i][0];
			const float im = fftOutput[i][1];

			float mag = sqrtf(re * re + im * im);

			// Normalize FFT magnitude
			mag /= normalization;

			if (logScale) {
				magnitudes[i] = 20.0f * log10f(mag + 1e-12f);
				magnitudes[i] = std::clamp(magnitudes[i], minDb, maxDb);
			}
			else {
				magnitudes[i] = mag;
			}
		}

		plotData.push_back(std::move(magnitudes));

		if (plotData.size() > maxHistory) {
			removedCols += plotData.size() - maxHistory;
			plotData.erase(
				plotData.begin(),
				plotData.begin() + (plotData.size() - maxHistory)
			);
		}
	}

	return 0;
}

int Spectrogram::render()
{
	glfwPollEvents();

	if (glfwWindowShouldClose(gWindow))
		return -1;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

	ImGui::Begin("Spectrogram", 
		nullptr,
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse
	);

	if (!plotData.empty()) {

		const int rows = static_cast<int>(numBins);
		const int cols = static_cast<int>(plotData.size());

		// Flatten plotData into a contiguous buffer
		static std::vector<float> heatmapData;
		heatmapData.resize(rows * cols, minDb);

		for (int x = 0; x < cols; x++) {
			for (int y = 0; y < rows; y++) {
				// Flip vertically so low frequencies appear at bottom
				heatmapData[y * cols + x] = plotData[x][rows - 1 - y];
			}
		}

		ImPlot::PushColormap(ImPlotColormap_Jet);
		ImVec2 plotSize = ImGui::GetContentRegionAvail();

		if (ImPlot::BeginPlot("##SpectrogramPlot", plotSize)) {

			ImPlotAxisFlags xFlags = ImPlotAxisFlags_None;
			ImPlotAxisFlags yFlags = ImPlotAxisFlags_None;

			if (logScale) {
				ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
			}

			ImPlot::SetupAxes(
				"Time (s)",
				"Frequency (Hz)",
				xFlags,
				yFlags
			);

			// Compute actual visible time range
			const float startTime = getColTime(0);
			const float endTime = getColTime(cols);

			ImPlot::SetupAxisLimits(
				ImAxis_X1,
				startTime,
				endTime,
				ImGuiCond_Always
			);

			// Avoid log-scale issues at 0 Hz
			const double minFreq = logScale ? 20.0 : 0.0;
			const double maxFreq = ringBuffer->sampleRate / 2.0;

			ImPlot::SetupAxisLimits(
				ImAxis_Y1,
				minFreq,
				maxFreq,
				ImGuiCond_Always
			);

			ImPlot::PlotHeatmap(
				"Spectrogram",
				heatmapData.data(),
				rows,
				cols,
				minDb,   // min value
				maxDb,      // max value
				nullptr,
				ImPlotPoint(startTime, 0.0),
				ImPlotPoint(endTime, maxFreq)
			);

			ImPlot::EndPlot();
		}

		ImPlot::PopColormap();
	}

	ImGui::End();

	ImGui::Render();

	int display_w, display_h;
	glfwGetFramebufferSize(gWindow, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(gWindow);

	return 0;
}

float Spectrogram::getColTime(size_t index)
{
	return ((float)(removedCols + index) * (float)hopSize + fftSize / 2.0f) / (float)ringBuffer->sampleRate;
}
