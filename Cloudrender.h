#pragma once
#include "globaldehs.h"
#include <stdio.h>
#include <assert.h>
#include <sstream>
#include <iostream>
using namespace std;
#include <miffy/gl/textureutility.h>
#include <miffy/math/color.h>
#include <miffy/math/quaternion.h>
#include <miffy/gamepad.h>
#include <miffy/object/Irenderobject.h>
#include <miffy/math/vec3.h>
#include <miffy/scene/light.h>
#include <miffy/object/INode.h>
#include <miffy/gl/glsl.h>
#include "pointparameter.h"
#include "IsoSurface.h"
#include "Land.h"
#include "Measure.h"
#include "GLColorDialog.h"
#include "TransForm.h"
#include "InputHandle.h"
#pragma comment(lib, "glew32.lib")

using namespace miffy;
struct sVolSize{
  int xy;
	int z;
	int total(){return xy*xy*z;}
	int serialId(int _x,int _y,int _z){return ((_z*xy+_y)*xy+_x);}
};
//このように、位置、法線と隣り合わせにした方が速くなるらしい
//アトリビュート変数もそうなんだろうか？
class CParticle{
public:
	vec3<float> pos;
	vec3<float> normal;
	float intensity;//アトリビュート変数
};
class CCloudRender:public INode,public IToggle
{
public:
	CCloudRender(const vec2<int>& _pos);
	~CCloudRender(void);
	void ArrayInit(size_t _size);
	bool Animate();
	bool AnimateRandomFadeOut();
	void AppendChild(INode* _ch);
	void ChangeWindSpeed();
	bool FileLoad();
	int GenerateWindow(GLuint _parentWind);
	string* GetToggleText();
	bool GetCurrentState();
	bool Run(void);
	void Init();
	void InitColorDialog(int winid);
	void InitCamera();
	int incrementThreshold(int _th);
	void InitPoints(CParticle* _voxel,unsigned char _thre);
	void KeyBoard(unsigned char c);
	void RootMenu(int val);
	void JoyStick(unsigned int buttonmask, int x, int y, int z);
	void PrepareAnimeVol();
	void PrintKeyHelp();
	void Toggle();
	void UpdatePara();
	bool BlendTime();
	void ChangeVolData();
	void Destroy();
	void DrawWindVector();
	void keyboard(unsigned char c);
	void Reset();
public:
	CInputHandler* m_InputHandle;
	CTransForm* m_TransForm;
	bool m_bCapture;//キャプチャーモードは1フレーム1秒になる
	CLand* m_Land;//CControlPaneで使う
	CMeasure* m_Measure;
	
private:
	CCloudRender* current_me;	
	bool m_bPrint;//背景の白黒を切り替える
	bool m_DoneGlewInit;//glewInitちゃんと終わりましたよフラグ 意味ないかも
	bool m_Hide;//雲の表示・非表示切り替え
	bool m_bStopMoment;//局所的な風を止めるか、止めないか
	bool m_bStopDayTime;//6分おきの時間の流れを止めるかどうか この変数はm_bCurrentVolData=1のときだけ働きたい
	bool m_bSliceVisible;//今断面が見えているかどうか
	bool m_bHideVector;//風ベクトルの表示・非表示
	bool m_bCurrentVolData;//解像度の良い雲か、6分おきに変わる雲か
	const string m_IniFile;//設定ファイルパス
	color<float> m_BgColor;//背景色：白か黒
	CIsoSurface* m_IsoSurface;
	CLight* m_Light;
	color<float> mFontColor;//文字色　背景が白の時は黒・背景が黒の時は白
	color<float> mBackGround;
	CGlsl *mProgram;
	CGlsl *mRainbowProgram;
	CPointParameter* m_pt_para;//点の見た目を左右するパラメータ
	cube<float>  mBoundingBox;//オブジェクトを囲む四角い枠 切り口を虹色にするのに使用する
	CParticle* m_Static;//粒子の初期位置として必要
	CParticle* mBeforeVoxel;//6分おきに流すのに必要なメンバ
	CParticle* mAfterVoxel;//6分おきに流すのに必要なメンバ
	color<float>* m_ucWindTF;//風の矢印を虹色にするための伝達関数
	GLSLsamp m_GausTexSamp;//ガウシアンテクスチャ　シェーダに渡す
	GLSLpara< vec4<float> > mLocalCam;//オブジェクトに中心を原点とした時のカメラ位置
	GLSLAtt mIntensity;//点の透明度　ボリュームデータは動的に変わるけど、シェーダ単位では変わらないからここに存在させておく
	float zScale;//ボリュームデータはz方向が足りないのが多いのでこうする
	float zSpeed;
	float mKarmapara;//輪廻転生調節パラメータ
	float m_fWindSpeed;//風の速さ調節パラメータ
	float m_Karmapara;
	float m_fTimeRatio;//今→６分後　までどのぐらい遷移したのか
	
	bool mStartFlightFlag;
	INode** m_Nodes;
	int m_nNode;
	struct sVolSize m_Wind;//風データのサイズ
	struct sVolSize m_Size;//ボリュームデータのサイズ
	string m_ToggleText[2];
	int m_nFrame;
	int m_ValidPtNum;
	int m_Max3DTexSize;	
	map<string,int> m_LuaMap;//セクションマップ
	string m_DataKey[2];
	vec3<float>* m_Dynamic;
	vec3<float>* m_renderdata;//レンダリング用風データ
	vec3<float>* m_rawdata;//オリジナル風データ
	unsigned char* m_ucIntensity;//生データ。断面図テクスチャに使う 風アニメする
	unsigned char* m_ucStaticIntensity;//リセットしたときにテクスチャを元に戻す、ファイルスレッドで使う
	unsigned char m_dataMax;//ボクセルの強度の最大値
	unsigned char m_threshold;
	
};

