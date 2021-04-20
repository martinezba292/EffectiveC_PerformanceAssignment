// Visual Studio 2017 version.


#include "stdafx.h"
#include "Performance2.h"
#include <thread>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Timer - used to established precise timings for code.
class TIMER
{
	LARGE_INTEGER t_;

	__int64 current_time_;

	public:
		TIMER()	// Default constructor. Initialises this timer with the current value of the hi-res CPU timer.
		{
			QueryPerformanceCounter(&t_);
			current_time_ = t_.QuadPart;
		}

		TIMER(const TIMER &ct)	// Copy constructor.
		{
			current_time_ = ct.current_time_;
		}

		TIMER& operator=(const TIMER &ct)	// Copy assignment.
		{
			current_time_ = ct.current_time_;
			return *this;
		}

		TIMER& operator=(const __int64 &n)	// Overloaded copy assignment.
		{
			current_time_ = n;
			return *this;
		}

		~TIMER() {}		// Destructor.

		static __int64 get_frequency()
		{
			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency); 
			return frequency.QuadPart;
		}

		__int64 get_time() const
		{
			return current_time_;
		}

		void get_current_time()
		{
			QueryPerformanceCounter(&t_);
			current_time_ = t_.QuadPart;
		}

		inline bool operator==(const TIMER &ct) const
		{
			return current_time_ == ct.current_time_;
		}

		inline bool operator!=(const TIMER &ct) const
		{
			return current_time_ != ct.current_time_;
		}

		__int64 operator-(const TIMER &ct) const		// Subtract a TIMER from this one - return the result.
		{
			return current_time_ - ct.current_time_;
		}

		inline bool operator>(const TIMER &ct) const
		{
			return current_time_ > ct.current_time_;
		}

		inline bool operator<(const TIMER &ct) const
		{
			return current_time_ < ct.current_time_;
		}

		inline bool operator<=(const TIMER &ct) const
		{
			return current_time_ <= ct.current_time_;
		}

		inline bool operator>=(const TIMER &ct) const
		{
			return current_time_ >= ct.current_time_;
		}
};

CWinApp theApp;  // The one and only application object

using namespace std;

//Function that brights a pixel
void BrightSum(uint8_t* pixel) {
	 *pixel += 100;
}
/*
It assigns the maximum possible value to a pixel.
As each pixel component will be made up of unsigned chars, the maximum value will be 255.
Only executed if pixel value is up 155
*/
void BrightMax(uint8_t* pixel) {
	*pixel = 255;
}

//Applies linear interpolation between 2 values(s and e) regarding 't' factor
uint8_t pixelLerp(uint8_t s, uint8_t e, float t) { return s + (e - s) * t; }

//Applies bilinear interpolation between 4 values
//Will be used to scale images without losing quality
uint8_t bPixelLerp(uint8_t c00, uint8_t c10, uint8_t c01, uint8_t c11, float tx, float ty) {
  return pixelLerp(pixelLerp(c00, c10, tx), pixelLerp(c01, c11, tx), ty);
}

//Scales an image by scalex and scaley values using bilinear interpolation
CImage* Scale(CImage* src, int scalex, int scaley) {
	int srcw = src->GetWidth();
	int srch = src->GetHeight();

	//Getting new width and height
  int newWidth = srcw * scalex;
  int newHeight = srch * scaley;

	/*
	It contains information about how the image is structured in memory.
	A negative pitch value means the first pixel is in the bottom left corner.
	On the contrary, a positive value means that the origin is in the top left corner of the image
	It's used to access the pixels correctly
	*/
	int srcpitch = src->GetPitch();

	//Returns a pointer to the start of the image
	uint8_t* srcb = (uint8_t*)src->GetBits();

	/*
	GetBPP returns the number of bits per pixel
	Dividing this by 8 will return the bytes per pixel
	As the images are JPG files, they don't have alpha channel, only RGB,
	so the expected value is 3 channels per pixel
	*/
	int channels = src->GetBPP() >> 3;


	CImage* dst = new CImage();

	/*
	Creating the new scaled image with 24 bits per pixel(RGB)
	and getting its pitch and memory address
	*/
	dst->Create(newWidth, newHeight, 24);
	int dstpitch = dst->GetPitch();
	uint8_t* dstb = (uint8_t*)dst->GetBits();
	float wdiv = 1.0f / (float)(newWidth);
	float hdiv = 1.0f / (float)(newHeight);

  int x, y;
	for (y = 0; y < newHeight; y++) {
		for (x = 0; x < newWidth; x++) {
			//Mapping dst pixels into src pixel
			float wx = (float)(x * wdiv) * (srcw - 1);
			float hy = (float)(y * hdiv) * (srch - 1);

			int wxi = (int)(wx);
			int hyi = (int)(hy);

			//current pixel
			uint8_t c00 = srcb[hyi * srcpitch + wxi * channels];
			//east pixel
			uint8_t c10 = srcb[hyi * srcpitch + ((wxi + 1) * channels)];
			//south pixel
			uint8_t c01 = srcb[(hyi + 1) * srcpitch + wxi * channels];
			//southeast pixel
			uint8_t c11 = srcb[(hyi + 1) * srcpitch + ((wxi + 1) * channels)];

			/*
			Obtaining a new pixel from the bilinear interpolation of the pixels above
			The floating part of the difference between pixel indices will be the curve value
			*/
			uint8_t result = bPixelLerp(c00, c10, c01, c11, wx - wxi, hy - hyi);

			/*
			Writting bytes of the destination image.
			As the image have to be in grayscale, all the channels contains the same value
			*/
			dstb[y * dstpitch + x * channels] = result;
			dstb[y * dstpitch + x * channels + 1] = result;
			dstb[y * dstpitch + x * channels + 2] = result;
			//NOTE: I tried to create new images as 8bit images in order to write only 1 byte per pixel, but CImage saved them completely black
		}
	}

  return dst;
}

void ProcessImage(filesystem::directory_entry entry) {

	CImage img;
	img.Load(entry.path().c_str());
	int32_t width = img.GetWidth();
	int32_t height = img.GetHeight();
	int32_t channels = img.GetBPP() >> 3;
	CImage auximg;

	//Creating the new image with the width and height inverted since images are loaded rotated.
	auximg.Create(height, width, 24);

	/*Functions to brighten images
	Pointer array save us from "if" statements to check the pixel overflow*/
	void(*bright_funcptr[2])(uint8_t * pixel);
	bright_funcptr[0] = BrightMax;
	bright_funcptr[1] = BrightSum;
	uint8_t max = 155; //MAX value a pixel can have to apply brighten func(rgb + 100)

	uint8_t* src = (uint8_t*)img.GetBits();
	uint8_t* dst = (uint8_t*)auximg.GetBits();
	int srcpitch = img.GetPitch();
	int dstpitch = auximg.GetPitch();

	int x, y;
	for (y = 0; y < height; ++y) {
		//Getting pointer to source image row
		uint8_t* srcline = src + y * srcpitch;

		/*
		Getting the destiny image index to access the right column
		Since the image is rotated, we use the height as width
		*/
		uint32_t dstline_index = (height - y - 1) * channels;
		for (x = 0; x < width; ++x) {
			//Getting the current pixel of the source image
			uint8_t* current = srcline + x * channels;

			//Applying grayscale
			//Average value between the 3 channels of each pixel in the source image
      uint8_t r = *(current);
      uint8_t g = *(current + 1);
      uint8_t b = *(current + 2);
			uint8_t grayscale = (uint8_t)((r + g + b) / 3);

			//Getting the dst pixel 
			current = (dst + x * dstpitch) + dstline_index;

			/*
			Increasing pixel's color saturation with function pointers
			Avoiding "if(pixel > 255) pixel = 255"
			*/
      *(current) = grayscale;
			int32_t j = (*current < max);
			bright_funcptr[j](current);

			//As the image will be grayscale, the three channels will have the same value
			*(current + 1) = *current;
			*(current + 2) = *current;
      //NOTE: I tried to create new images as 8bit images in order to write only 1 byte per pixel, but CImage saved them completely black
		}
	}

	//Fill image information
	CString filename = entry.path().filename().c_str();

	//Scale image x2 using bilinear interpolation
	CImage* final_img = Scale(&auximg, 2, 2);

	//Removing the extension from the filename(".JPG" will be removed in this case)
  filename.Delete(filename.GetLength() - 4, 4);

	//Directory where the output images will be stored
  CString image_path("../output/");
  image_path += filename;
  image_path += ".png";

	//Saving new image and freeing up memory
	auximg.Destroy();
  final_img->Save(image_path);
	img.Destroy();
  final_img->Destroy();
  delete(final_img);
}

//Executes the ProcessImage function for each entry in the vector
void executeThread(vector<filesystem::directory_entry> v) {
	for_each(v.begin(), v.end(), ProcessImage);
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize Microsoft Foundation Classes, and print an error if failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		// Application starts here...

		// Time the application's execution time.
		TIMER start;	// DO NOT CHANGE THIS LINE. Timing will start here.

		//--------------------------------------------------------------------------------------
		// Insert your code from here...

		//Path from which the images are taken
		string path("./../input/");
		//Only images with this extension will be collected
    string extension(".JPG");

		vector<filesystem::directory_entry> entries;
		for (auto& f : filesystem::directory_iterator(path)) {
			//Getting directory entries of JPG images
			if (f.path().extension() == extension)
				entries.push_back(f);
		}

		if (entries.size() > 8) {
			//Sending 3 threads if more than 8 images are found
      int half_size = (int)(entries.size() >> 2);

			//Split vector that contains paths and filenames and sending thread with the data
      vector<filesystem::directory_entry> image_paths1(entries.begin(), entries.begin() + half_size);
      thread first(executeThread, image_paths1);
      vector<filesystem::directory_entry> image_paths2(entries.begin() + half_size, entries.begin() + (half_size << 1));
      thread second(executeThread, image_paths2);
      vector<filesystem::directory_entry> image_paths3(entries.begin() + (half_size << 1), (entries.begin() + (half_size * 3)));
      thread third(executeThread, image_paths3);
      vector<filesystem::directory_entry> image_paths4((entries.begin() + (half_size * 3)), entries.end());
			executeThread(image_paths4);

			//Waiting for threads to end
      first.join();
      second.join();
			third.join();
		} 
		else if (entries.size() > 4) {
			//Sending 2 threads if more than 4 images are found
		  int half_size = (int)(entries.size() >> 1);

		  vector<filesystem::directory_entry> image_paths1(entries.begin(), entries.begin() + half_size);
		  vector<filesystem::directory_entry> image_paths2(entries.begin() + half_size, entries.end());
		  
			thread first(executeThread, image_paths1);
		  thread second(executeThread, image_paths2);
			 
			first.join();
			second.join();
		}
		else {
			//If 4 images or less, the application runs in the main thread
			executeThread(entries);
		}

		//-------------------------------------------------------------------------------------------------------
		// How long did it take?...   DO NOT CHANGE FROM HERE...
		
		TIMER end;

		TIMER elapsed;
		
		elapsed = end - start;

		__int64 ticks_per_second = start.get_frequency();

		// Display the resulting time...

		double elapsed_seconds = (double)elapsed.get_time() / (double)ticks_per_second;

		cout << "Elapsed time (seconds): " << elapsed_seconds;
		cout << endl;
		cout << "Press a key to continue" << endl;

		char c;
		cin >> c;
	}

	return nRetCode;
}
