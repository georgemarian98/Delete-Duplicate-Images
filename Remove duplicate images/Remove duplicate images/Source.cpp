#include <iostream>
#include <thread>
#include <array>
#include <regex>

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

#include <filesystem>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <CommDlg.h>
#include <ShlObj.h>

std::array<bool, 4> workDone;
const std::string outputDirectory = std::string("Duplicate Images\\");
const std::regex regex("[^\\s]+(\\.(png|jpg|jpeg|bmp|dib|jpe|jp2|pbm|pgm|ppm|pxm|pnm|pfm|sr|ras|tiff|tif|exr|hdr|pic|webp))$");

struct ImageProp{
	std::string name = "";
	std::string path = "";

	ImageProp(std::string& name, std::string& path) : name(name), path(path) { };
};

struct ThreadProp{
	cv::Mat img1;
	cv::Mat img2;
	int startI;
	int startJ;
	int stopI;
	int stopJ;
	int index;
};

//This function is called by each worker thread
void part_equal(ThreadProp prop)
{
	for(int i = prop.startI; i < prop.stopI; i++){
		for(int j = prop.startJ; j < prop.stopJ; j++){

			uchar color1 = prop.img1.at<uchar>(i, j);
			uchar color2 = prop.img2.at<uchar>(i, j);

			if(abs(color1 - color2) > 20){
				return;
			}
		}
	}

	workDone[prop.index] = true;
}

void InitArray(bool value = false)
{
	for(bool& work : workDone )
	{
		work = value;
	}
}

bool EqualImg(const cv::Mat& a, const cv::Mat& b)
{
	if((a.rows != b.rows) || (a.cols != b.cols))
		return false;

	static constexpr int noThreads = 4;
	int hWidth = a.cols / noThreads;
	int hHeight = a.rows / noThreads;

	static std::array < std::thread, 4> threads;
	InitArray( );

	for(int i = 0; i < noThreads; i++){
		//slice the image in separate parts 
		ThreadProp prop;
		prop.img1 = a;
		prop.img2 = b;
		prop.startI =  (i / 2) * hHeight;
		prop.startJ =  (i % 2) * hWidth;
		prop.stopI = ( (i / 2) + 1) * hHeight;
		prop.stopJ = ( (i % 2) + 1) * hWidth;
		prop.index = i;

		//TODO: thread pool
		threads[i] = std::thread(part_equal, prop);
	}

	for(auto& thread : threads ){
		thread.join( );
	}

	for(auto& work : workDone){
		if(work == false){
			return false;
		}
	}
	
	return true;
}


void DisplayErrorBox(const char* lpszFunction)
{
	// Retrieve the system error message for the last-error code
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError( );
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and clean up
	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
					TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}


bool IsImage(std::string name)
{
	return std::regex_match(name, regex);
}

std::vector<std::string> SearchDirectory(const char* filepath1)
{
	WIN32_FIND_DATA ffd;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;

	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.
	StringCchLength(filepath1, MAX_PATH, &length_of_arg);
	if(length_of_arg > (MAX_PATH - 3)){
		_tprintf(TEXT("\nDirectory path is too long.\n"));
		return std::vector<std::string>();
	}

	// Prepare string for use with FindFile functions. First, copy the
	// string to a buffer, then append '\*' to the directory name.
	StringCchCopy(szDir, MAX_PATH, filepath1);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.
	hFind = FindFirstFile(szDir, &ffd);
	if(INVALID_HANDLE_VALUE == hFind){
		DisplayErrorBox(TEXT("FindFirstFile"));
		return std::vector<std::string>( );
	}

	std::vector<std::string> images;

	// List all the files in the directory with some info about them.
	do{
		if(ffd.dwFileAttributes == 0x20){

			if(IsImage(ffd.cFileName)){
				images.emplace_back(ffd.cFileName );
			}
		}
	} while(FindNextFile(hFind, &ffd) != 0);
	dwError = GetLastError( );

	if(dwError != ERROR_NO_MORE_FILES){
		DisplayErrorBox(TEXT("FindFirstFile"));
	}
	FindClose(hFind);

	return images;
}

int openFolderDlg(char* folderName)
{
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	SHGetPathFromIDList(SHBrowseForFolder(&bi), folderName);
	return strcmp(folderName, "");
}

std::vector<ImageProp> PrepareInput(std::string folderPath )
{
	auto imgsNames = SearchDirectory(folderPath.c_str() );

	if(imgsNames.size() == 0 ){
		MessageBox(NULL, (LPCTSTR)"There are no images in the folder or the images are not supported", TEXT("Error"), MB_RETRYCANCEL | MB_ICONHAND);
		exit(0);
	}

	folderPath += "\\";
	std::vector<ImageProp> imgs;

	for(auto& filepath : imgsNames){
		std::string name = filepath;
		std::string path = folderPath + filepath;

		imgs.emplace_back(name, path);
	}

	return imgs;
}

void MoveImage(const ImageProp& ImgProp, bool IsEqual)
{
	if(IsEqual == true){

		std::cout << "Deleted photo: " + ImgProp.path << std::endl;

		std::string output = outputDirectory + ImgProp.name;
		bool a = MoveFile(ImgProp.path.c_str( ), output.c_str( ));

		DWORD dwError = GetLastError( );

		if(dwError != 0 && dwError != 203L){
			DisplayErrorBox(TEXT("Error"));
			exit( -1 );
		}
	}
}

void CompareImages(std::vector<ImageProp>& ImagesPath1, std::vector<ImageProp>& ImagesPath2)
{
	std::vector <cv::Mat> images;
	images.reserve( ImagesPath2.size() );

	{// First iteration through ImagesPath2 for cache pourpose and check with the first element of ImagesPath1 as well
		cv::Mat img = cv::imread(ImagesPath1[0].path, cv::IMREAD_GRAYSCALE);

		for(auto& imgPath2 : ImagesPath2){

			cv::Mat img2 = cv::imread(imgPath2.path, cv::IMREAD_GRAYSCALE);
			images.push_back(img2);

			bool eq = EqualImg(img, img2);

			MoveImage(ImagesPath1[0], eq);
		}
	}


	for(auto imgPath = ImagesPath1.begin( ) + 1; imgPath < ImagesPath1.end( ); ++imgPath){

		cv::Mat img = cv::imread( imgPath->path, cv::IMREAD_GRAYSCALE );

		for(auto& image : images){

			bool eq = EqualImg(img, image);

			MoveImage(*imgPath, eq);
		}
	}

	
}

std::string GetInput( )
{
	while(true){
		char path[MAX_PATH];
		openFolderDlg(path);

		if(strlen(path) != 0){
			return std::string{path};
		}

		int errorCode = MessageBox(NULL, (LPCTSTR)"Filepath not ok!", TEXT("Error"), MB_RETRYCANCEL | MB_ICONHAND);

		if( errorCode != IDRETRY){
			exit( -1 );
		}
	}
	
}

int main( )
{
	std::string fisier = GetInput( );
	auto imagePaths = PrepareInput(fisier);

	std::string fisier2 = GetInput( );
	auto imagePaths2 = PrepareInput(fisier2);

	std::filesystem::remove_all("Duplicate Images");
	std::filesystem::create_directories("Duplicate Images");

	CompareImages (imagePaths, imagePaths2);

	std::cout << "Press enter to exit!\n";
	std::cin.get( );
	return 0;
}