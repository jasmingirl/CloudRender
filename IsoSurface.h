#pragma once
#ifndef MIFFY_ISOSURFACE
#define MIFFY_ISOSURFACE
#include <vector>
#include <string>
#include <sstream>
using namespace std;
#include <miffy/fileformat/ply.h>
#include <miffy/gl/glutility.h>
using namespace miffy;

/// 等値面クラス。
/*! データを読み込んで適切な色で表示するだけ。
    色もデータ化すべきよね。
	VBOにしようとして失敗している。
*/
class CIsoSurface {
public:
	CIsoSurface(float elerange);
	~CIsoSurface(void);
	int m_VertNum[5];
	int m_IndexNum[5];
	void Init(const char* _inifilepath);
	bool Run();
	void Info();
	void ToggleMode(bool _showtext);
private:
	string m_title;
	bool mEnlightCore;
	float m_elerange;///< こいつもいずれ消したい
	Ply plydata[5];
	color<unsigned char> m_Colors[5];
};

#endif
