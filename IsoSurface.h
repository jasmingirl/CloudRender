#pragma once
#ifndef MIFFY_ISOSURFACE
#define MIFFY_ISOSURFACE
#include <vector>
#include <string>
#include <sstream>
using namespace std;
#include <miffy/gl/glutility.h>
#include <miffy/fileformat/jverts.h>
using namespace miffy;

/// 等値面クラス。
/*! データを読み込んで適切な色で表示するだけ。
    色もデータ化すべきよね。
  VBOにしようとして失敗している。
*/
class CIsoSurface {
public:
	CIsoSurface(float elerange,string& _dir);
	~CIsoSurface(void);
	JFormat* mSurfaceData[5];///< 等値面　とりあえず、jvertとjvert2両対応にした。
	void Init();
	bool Run();
	void Info();
	void ToggleMode(bool _showtext);
private:
	string m_title;
	bool mEnlightCore;
	string m_directory;///< いずれ消したいメンバ変数
	float m_elerange;///< こいつもいずれ消したい
	vector<GLuint> m_ListId;///< ディスプレイリストのID 使ってない。
	GLuint m_BufferId;///< 今はこれ、頂点バッファで描いている。
	GLuint m_IdBufferId;///< 頂点インデックスのバッファ
	bool m_bJVert2;///< jvert2なのかどうか
};

#endif
