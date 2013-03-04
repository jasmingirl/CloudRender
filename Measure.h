#pragma once
#include <string>
#include <iostream>
#include <sstream>
using namespace std;
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <miffy/math/cube.h>
#include <miffy/object/inode.h>
#include <miffy/math/matrix.h>
#include "TransForm.h"
#include <miffy/gl/ui/button.h>
using namespace miffy;
class CMeasure :
  public INode,public IToggle
{
public:
	CMeasure(float _elerange,CTransForm* _tra);
	~CMeasure(void);
	bool Run();
	void AppendChild(INode* _ch){}
	void DrawClipPlane();
	bool GetCurrentState();
	string* GetToggleText();
	void Toggle();
	double adjust;
private:
	void Draw();
	void DrawLatMag();
	bool m_Hide;
	float m_ZScale;
	cube<float>  mBoundingBox;//オブジェクトを囲む四角い枠
	CTransForm* m_pTra;
	GLuint m_TexId;//スライス面に断面図を乗せるため
	const float m_MaxHeight;//11km
	const float m_LENGTH;//目盛りの長さ
	const int m_KmInterval;//何メートルごとに表示するのか
	string m_ToggleText[2];
	const float m_LATITUDE;//ローカル座標での緯度の長さ
	const float m_LONGITUDE;//ローカル座標での経度の長さ
};

