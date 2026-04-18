#include "Spectrogram.h"

int paRingBufferCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	audioRingBuffer* ringBuffer = (audioRingBuffer*)userData;
	const float* in = (const float*)input;
	float* out = (float*)output;

	if (statusFlags) {
		std::cerr << "Audio callback error: " << statusFlags << "\n";
	}

	if (PaUtil_WriteRingBuffer(&ringBuffer->ringBuffer, in, frameCount) == 0) {
		std::cerr << "Ring buffer overflow: Unable to write audio data\n";
	}

	for (unsigned int i = 0; i < frameCount; i++) {
		float sample = in ? *in++ : 0.0f;
		*out++ = sample;
		*out++ = sample;

	}
	return paContinue;
}

void glfw_error_callback(int error, const char* description) { std::cerr << "GLFW Error " << error << ": " << description << std::endl; }

Spectrogram::Spectrogram(size_t fftSize, size_t hopDivisor, audioRingBuffer* ringBuffer) : fftSize(fftSize), numBins(fftSize / 2 + 1), hopSize(fftSize / hopDivisor), ringBuffer(ringBuffer)
{
	// Create Hann window
	window.resize(fftSize);
	for (size_t i = 0; i < fftSize; i++) {
		window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (fftSize - 1)));
	}

	// Allocate FFT input and output arrays
	fftInput = new float[fftSize];
	fftOutput = new fftwf_complex[numBins];
	plotData.resize(maxHistory, std::vector<float>(numBins, -100.0f));

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
	// Read audio data from the ring buffer
	size_t availableSamples = PaUtil_GetRingBufferReadAvailable(&ringBuffer->ringBuffer);
	if (availableSamples < fftSize) {
		//std::cerr << "Not enough audio data available for FFT\n";
		return -1;
	}

	PaUtil_ReadRingBuffer(&ringBuffer->ringBuffer, fftInput, fftSize);
	// Apply window function
	for (size_t i = 0; i < fftSize; i++) {
		fftInput[i] *= window[i];
	}

	// Execute FFT
	fftwf_execute(fftPlan);
	// Update plot data with magnitude of FFT output
	std::vector<float> magnitudes(numBins);
	for (size_t i = 0; i < numBins; i++) {
		magnitudes[i] = sqrtf(fftOutput[i][0] * fftOutput[i][0] + fftOutput[i][1] * fftOutput[i][1]);
		
		if (logScale) {
			magnitudes[i] = 20.0f * log10f(magnitudes[i] + 1e-6f);
		}

		magnitudes[i] = std::clamp(magnitudes[i], -100.0f, 0.0f);
	}

	plotData.push_back(std::move(magnitudes));

	if (plotData.size() > maxHistory) { // Limit history to maxHistory frames
		removedCols += plotData.size() - maxHistory;
		plotData.erase(plotData.begin(), plotData.begin() + (plotData.size() - maxHistory));
	}

	return 0;
}

int Spectrogram::render(const ImVec2& size)
{
	glfwPollEvents();

	if (glfwWindowShouldClose(gWindow))
		return -1;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Spectrogram");

	if (!plotData.empty()) {

		const int rows = static_cast<int>(numBins);
		const int cols = static_cast<int>(plotData.size());

		// Flatten plotData into a contiguous buffer
		static std::vector<float> heatmapData;
		heatmapData.resize(rows * cols, -100.0f);

		for (int x = 0; x < cols; x++) {
			for (int y = 0; y < rows; y++) {
				// Flip vertically so low frequencies appear at bottom
				heatmapData[y * cols + x] = plotData[x][rows - 1 - y];
			}
		}

		ImPlot::PushColormap(ImPlotColormap_Jet);

		if (ImPlot::BeginPlot("##SpectrogramPlot")) {

			ImPlot::SetupAxes("Time (frames)", "Frequency (Hz)",
				ImPlotAxisFlags_None,
				ImPlotAxisFlags_None);

			ImPlot::SetupAxisLimits(ImAxis_X1, 0, cols, ImGuiCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, ringBuffer->sampleRate / 2.0f, ImGuiCond_Always);

			ImPlot::PlotHeatmap(
				"Spectrogram",
				heatmapData.data(),
				rows,
				cols,
				-100.0f,   // min value
				0.0f,      // max value
				nullptr,
				ImPlotPoint(0, 0),
				ImPlotPoint((double)cols, ringBuffer->sampleRate / 2.0)
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
