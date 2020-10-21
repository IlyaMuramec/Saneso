#pragma once

#include <sstream>
#include <vector>

#define APP_PROGRAM "Program"


class CProgramSettings
{
public:
	CProgramSettings(void);
	~CProgramSettings(void);

	static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
		std::stringstream ss(s);
		std::string item;
		elems.clear();
		while (std::getline(ss, item, delim)) {
			elems.push_back(item);
		}
		return elems;
	}

	//Convert std::string to std::wstring
	//If you need a LPWSTR call .c_str() on the wstring.
	static std::wstring stdStr2WStr(std::string stdStr)
	{
		int len;
		int slength = (int)stdStr.length() + 1;
		len = MultiByteToWideChar(CP_ACP, 0, stdStr.c_str(), slength, 0, 0);
		wchar_t* buf = new wchar_t[len];
		MultiByteToWideChar(CP_ACP, 0, stdStr.c_str(), slength, buf, len);
		std::wstring r(buf);
		delete[] buf;
		return r;
	}

	//Convert LPWSTR to std string. This was taken from an example on the internet.
	static bool cvtLPW2stdstring(std::string& s, const LPWSTR pw)
	{
		bool res = false;
		char* p = 0;
		int bsz;
		UINT codepage = CP_ACP;

		bsz = WideCharToMultiByte(codepage, 0, pw, -1, 0, 0, 0, 0);
		if (bsz > 0)
		{
			p = new char[bsz];
			int rc = WideCharToMultiByte(codepage, 0, pw, -1, p, bsz, 0, 0);
			if (rc != 0)
			{
				p[bsz - 1] = 0;
				s = p;
				res = true;
			}
		}
		delete[] p;
		return res;
	}

	void Save(std::string folder);
	void Load(std::string folder);

	std::string imageFolder;
	std::string outputName;
	
private:
};

