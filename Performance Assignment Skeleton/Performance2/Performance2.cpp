// Visual Studio 2017 version.


#include "stdafx.h"
#include "Performance2.h"


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

struct ImgInfo {
  int w, h;
  CString filename;
  CImage* img;
};

void BrightSum(uint8_t* pixel) {
	 *pixel += 100;
}

void BrightMax(uint8_t* pixel) {
	*pixel = 255;
}


uint8_t pixelLerp(uint8_t s, uint8_t e, float t) { return s + (e - s) * t; }

uint8_t bPixelLerp(uint8_t c00, uint8_t c10, uint8_t c01, uint8_t c11, float tx, float ty) {
  return pixelLerp(pixelLerp(c00, c10, tx), pixelLerp(c01, c11, tx), ty);
}

CImage* Scale(CImage* src, int scalex, int scaley) {
	int srcw = src->GetWidth();
	int srch = src->GetHeight();
  int newWidth = srcw * scalex;
  int newHeight = srch * scaley;
	int srcpitch = src->GetPitch();
	uint8_t* srcb = (uint8_t*)src->GetBits();
	int channels = src->GetBPP() >> 3;

	CImage* dst = new CImage();
	dst->Create(newWidth, newHeight, 24);
	int dstpitch = dst->GetPitch();
	uint8_t* dstb = (uint8_t*)dst->GetBits();

  int x, y;
	for (y = 0; y < newHeight; y++) {
		for (x = 0; x < newWidth; x++) {
			//Mapping dst pixels into src pixel
			float wx = (x / (float)(newWidth)) * (srcw - 1);
			float hy = (y / (float)(newHeight)) * (srch - 1);

			int wxi = (int)wx;
			int hyi = (int)hy;

			//current pixel
			uint8_t c00 = srcb[hyi * srcpitch + wxi * channels];
			//east pixel
			uint8_t c10 = srcb[hyi * srcpitch + ((wxi + 1) * channels)];
			//south pixel
			uint8_t c01 = srcb[(hyi + 1) * srcpitch + wxi * channels];
			//southeast pixel
			uint8_t c11 = srcb[(hyi + 1) * srcpitch + ((wxi + 1) * channels)];

			uint8_t result = bPixelLerp(c00, c10, c01, c11, wx - wxi, hy - hyi);
			dstb[y * dstpitch + x * channels] = result;
			dstb[y * dstpitch + x * channels + 1] = result;
			dstb[y * dstpitch + x * channels + 2] = result;
		}
	}

  return dst;
}



ImgInfo ProcessImage(CString path, CString filename) {
	CImage img;
	img.Load(path);
	int32_t width = img.GetWidth();
	int32_t height = img.GetHeight();
	int32_t channels = img.GetBPP() >> 3;
	CImage auximg;
	auximg.Create(width, height, 24);

	/*Functions to brighten images
	Pointer array save us from if statements to check the pixel overflow*/
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
		//We go through the dst pixels in the opposite direction to rotate the image
		uint8_t* dstline = dst + (height - y - 1) * dstpitch;
		uint8_t* srcline = src + y * srcpitch;
		for (x = 0; x < width; ++x) {
			uint8_t* current = srcline + x * channels;
      uint8_t r = *(current);
      uint8_t g = *(current + 1);
      uint8_t b = *(current + 2);
			uint8_t grayscale = (uint8_t)((r + g + b) / 3);

			current = dstline + (width - x - 1) * channels;

      *(current) = grayscale;
			int32_t j = (*current < max);
			bright_funcptr[j](current);
			++current;

      *(current) = grayscale;
      j = (*current < max);
      bright_funcptr[j](current);
      ++current;

      *(current) = grayscale;
      j = (*current < max);
      bright_funcptr[j](current);
      ++current;
		}
	}

	////Fill image information
	ImgInfo new_img{};
	new_img.filename = filename;
	//Scale image x2 using bilinear interpolation
	new_img.img = Scale(&auximg, 2, 2);
	new_img.w = width << 1;
	new_img.h = height << 1;
  new_img.filename.Delete(filename.GetLength() - 4, 4);

	auximg.Destroy();
	img.Destroy();

	return new_img;
}


CImage* ProcessImage(CImage* src, CImage* dst) {
  int srcw = src->GetWidth();
  int srch = src->GetHeight();
  int newWidth = dst->GetWidth();
  int newHeight = dst->GetHeight();
  int srcpitch = src->GetPitch();
	uint8_t* srcb = (uint8_t*)src->GetBits();
  int channels = src->GetBPP() >> 3;

  int dstpitch = dst->GetPitch();
	uint8_t* dstb = (uint8_t*)dst->GetBits();

  uint8_t max = 155; //MAX value a pixel can have to apply brighten func(rgb + 100)
  void(*bright_funcptr[2])(uint8_t * pixel);
  bright_funcptr[0] = BrightMax;
  bright_funcptr[1] = BrightSum;

  int x, y;
  for (y = 0; y < newHeight; y++) {
    for (x = 0; x < newWidth; x++) {
      //Mapping dst pixels into src pixel
      float wx = (x / (float)(newWidth)) * (srcw - 1);
      float hy = (y / (float)(newHeight)) * (srch - 1);

      int wxi = (int)wx;
      int hyi = (int)hy;

      //current pixel
      uint32_t* c00 = (uint32_t*)&srcb[hyi * srcpitch + wxi * channels];
      //east pixel
      uint32_t* c10 = (uint32_t*)&srcb[hyi * srcpitch + ((wxi + 1) * channels)];
      //south pixel
      uint32_t* c01 = (uint32_t*)&srcb[(hyi + 1) * srcpitch + wxi * channels];
      //southeast pixel
      uint32_t* c11 = (uint32_t*)&srcb[(hyi + 1) * srcpitch + ((wxi + 1) * channels)];

      //Bilinear interpolation
      uint8_t r = bPixelLerp(*c00 & 0xFF, *c10 & 0xFF, *c01 & 0xFF, *c11 & 0xFF, wx - wxi, hy - hyi);
      uint8_t g = bPixelLerp((*c00 >> 8) & 0xFF, (*c10 >> 8) & 0xFF, (*c01 >> 8) & 0xFF, (*c11 >> 8) & 0xFF, wx - wxi, hy - hyi);
      uint8_t b = bPixelLerp(*c00 >> 16 & 0xFF, *c10 >> 16 & 0xFF, *c01 >> 16 & 0xFF, *c11 >> 16 & 0xFF, wx - wxi, hy - hyi);

      //Gray scale
      uint8_t grayscale = ((r + g + b) / 3);

      int32_t j = (grayscale < max);
      bright_funcptr[j](&grayscale);

      //Write the new image pixels rotated
      dstb[(newHeight - y - 1) * dstpitch + (newWidth - x - 1) * channels] = grayscale;
      dstb[(newHeight - y - 1) * dstpitch + (newWidth - x - 1) * channels + 1] = grayscale;
      dstb[(newHeight - y - 1) * dstpitch + (newWidth - x - 1) * channels + 2] = grayscale;
    }
  }

  return dst;
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

		string path("./../input/");
		string extension(".JPG");

#if 1
		//TODO: Send 4 threads with 3 images in each one
		//std::vector<ImgInfo> images;
		for (auto& f : filesystem::directory_iterator(path)) {
			if (f.path().extension() == extension) {
				ImgInfo imgInfo = ProcessImage(f.path().c_str(), f.path().filename().c_str());
        CString image_path("../output/");
        image_path += imgInfo.filename;
        image_path += ".png";
        imgInfo.img->Save(image_path);
        imgInfo.img->Destroy();
        delete(imgInfo.img);
				//images.push_back(imgInfo);
			}
		}

		//for (auto& img : images) {
  //    CString image_path("../output/");
		//	image_path += img.filename;
  //    image_path += ".png";
		//	img.img->Save(image_path);
		//	img.img->Destroy();
		//	delete(img.img);
		//}

#else
		for (auto& f : filesystem::directory_iterator(path)) {
			if (f.path().extension() == extension) {
				CImage inputimg;
				CImage outputimg;
				inputimg.Load(f.path().c_str());
				outputimg.Create(inputimg.GetWidth() << 1, inputimg.GetHeight() << 1, 24);
        ProcessImage(&inputimg, &outputimg);
				CString filename = f.path().filename().c_str();
				filename.Delete(filename.GetLength() - 4, 4);
        CString image_path("../output/");
        image_path += filename;
        image_path += ".png";
        outputimg.Save(image_path);
				inputimg.Destroy();
				outputimg.Destroy();
			}
		}
#endif

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
