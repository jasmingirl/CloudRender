#pragma once
#ifndef MIFFY_LAND
#define MIFFY_LAND
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
using namespace std;
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/glfw.h>
#include <miffy/gl/glutility.h>
#include <miffy/gl/tga.h>
#include <miffy/math/vec2.h>
#include <miffy/math/vec3.h>
#include <miffy/math/triangle.h>
#include <miffy/object/Irenderobject.h>
#include <miffy/fileformat/jmesh.h>
#include <miffy/math/color.h>
#include "GLSLClipProjected.h"
using namespace miffy;
/// 地図の頂点に適した、テクスチャ座標・法線・位置の順番に並んでいる構造体。GL_T2F_N3F_V3F
struct sVerts{
  vec2<float> texcoord;
	vec3<float> normal;
	vec3<float> pos;
};
/// 地図表示クラス。
/*!
	クリッピング断面図を書く関係でCTransFormと癒着しているのがよろしくないかも。
*/
class CLand
{
public:
	/*! @name 毎フレーム呼ばれる関数
	*/
	//@{
	void SetClipPlane(double _clip_eqn[4]);///< CCloudRenderのRunにより毎フレーム呼び出される。GLSLに渡すため。 CCloudRenderはCTransFormとCLandの仲介役をしているだけ。
	//@}
	CLand();
	~CLand(void);
	bool Run();///< drawにしたいけど。。。
	void ToggleMapKind();
	void Init(color<float>* _radarVolumeData,int _size);
	void InitCamera(float *_m);
	void Info();
	void InitData(const string& _filepath,int _id);
	
private:
	CGLSLClipProjected *mProgram;
	GLuint m_TexId[2];
	GLuint m_List[2];
	
	//GLuint m_BufferId[2];///< 頂点バッファのID
	//GLuint m_IdBufferId[2];///< 頂点インデックスバッファのID
	GLuint m_ProjectedSamp[2];///< 下の断面図
	
	unsigned int m_IndexNum[2];
	bool m_bWhichData;///< 0:解像度の良い雲　1:6分おきの雲
	
	
};
#endif
