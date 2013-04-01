#pragma once
#ifndef MIFFY_MODELVIEW
#define MIFFY_MODELVIEW
#include <iomanip>//setpresicionのため
#include <vector>
#include <limits>//backgroundのため
#define _USE_MATH_DEFINES
#include <math.h>
using namespace std;
#include <miffy/gl/glutility.h>
#include <GL/glfw.h>
#include <miffy/math/vec2.h>
#include <miffy/math/fixedview.h>
#include <miffy/math/color.h>
#include <miffy/math/quad.h>
using namespace miffy;
///// スライダーバーとクリップ面のいちを連動させるために作ったクラス　地図の断面図投影で使用されるようだ。
//class CClip
//{
//public:
//  CClip(){m_Eqn[0]=-1.0;m_Eqn[1]=0.0;m_Eqn[2]=0.0;m_Eqn[3]=0.5;}//本当は0.5がいい
//	string GetTitle(){return string("断面");}
//	void SetVal(float _val){
//		m_Eqn[3]=((double)_val-0.5);
//		//cout<<"クリップ面"<<m_Eqn[3]<<endl;
//	}
//	float GetVal(){return ((float)m_Eqn[3]+0.5f);}
//	
//};
/// オブジェクトの姿勢・位置を支配する。モデルビュー行列をメンバに持っている　その他、クリッピング方程式を持っていたり、zScaleを大きくしたりする役割も担う
/*! 独立させたい。。。！他にも朝・昼・晩のシーンを変えるなどの機能がある。
こいつのISliderはzスケールで、
この人のメンバのCClipのISliderはスライス面である。
*/
class CTransForm{//modelviewじゃなくてmodelだ。modelの位置と姿勢を変えるクラス
public:
	CTransForm();
	
	/*! @name 毎フレーム呼ばれる関数
	*/
	//@{
	bool CheckParameterChange();
	bool DrawBackGround();///< 背景を描く
	void Disable();///< glPopMatrixとかする
	void Enable();///< glPushMatrixの始まり
	vec4<float> LocalCam();///< シェーダにカメラ情報を渡すときに使う
	//@}
	/*! @name リサイズの時だけ呼ばれる関数
	*/
	//@{
	float farPlaneSize()const;///< 背景プレートの中心座標をセットするに使う。単純だからインライン化してもいいまも。
	void Reshape(int _w,int _h);///< 画面サイズが変わったときに対応する	
	//@}
	/*! @name ユーザ入力によって呼ばれる関数
	*/
	//@{
	void zoom(int _direction);///< ズームイン＆ズームアウト m_Zoomを操作し、gluLookAtに影響する。
	void SaveRotationState();///< InputHandlerで使用される。廃止したい！
	void RotateFromScreen(int _dx,int _dy);///< 画面をひっかいたら回転
	void RotateFromQuat(float* _quat);///< QUaternionを返してくれるAntTweakBarのようなGUI用
	void TranslateFromScreeen(float _dx,float _dy);///< InputHandlerで使用される。廃止したい！
	void IncrementTranslate(const vec3<float>& _val);
	void KeyBoard(unsigned char _c);
	void ChangeScene(int _val);///< 朝・昼・晩のシーンを変える　空の色は自動で変わるからそのうちこれは廃止になる
	void ChangeView(int _val);///< ある特定の視点にさっと変えたいときに呼び出される関数
	float CalcProjectedSize(float _localsize);///< レンダリングするのに適切な点の大きさを計算するための関数
	//@}
	/*! @name デバッグ用の関数
	*/
	//@{
	void Info();
	//@}
	void SetFlag(unsigned int _flags);///< フラグのセット。

public:
	static enum{
		MY_DRAW_BG=1,///< 背景のグラデーション板を書くかどうか　
		MY_PRINT=1<<1,///< 印刷モードかどうか
		MY_ORTHOGONAL=1<<2,///< 1:正投影モード　0:透視投影モード　
		MY_FLAG_NUM=3
	};
	/*! @name publicなメンバ変数
		色々なGUIでアクセスするためこうしてある。そのうちprivateにすべきかも。
	*/
	//@{
	GLdouble m_ClippingEquation[4];///< クリッピング方程式 こいつはあちこちから呼び出される人気の変数である。
	quat<float> m_CurrentQuaternion;///< 回転状態を保存するためにある　あとでdifに掛け算する。AntTweakBarと連携するためpublic化
	//@}
private:	
	/*! @name 変換行列に関わるパラメータ
	*/
	//@{
	const float mNearClip;
	const float mFarClip;
	const float mFovy;
	const float m_InitialZ;///< zの初期値として利用されている。	
	//@}
	mat4<float> mInvModelView;//< オブジェクト中心からみたカメラ座標を割り出すのに必要　シェーダに渡す	
	/*! @name ユーザ入力によって変化するパラメータ
	*/
	//@{
	unsigned int m_Flags;
	color<float> m_BgColor;///<  背景色：白か黒
	float m_Zoom;///< 現在のズーム値
	quat<float> m_TargetQuaternion;///< moveではtarget_quat.toMat4
	mat4<float> m_RotationMatrix;
	mat4<float> m_TranslationMatrix;
	vec3<float> mTranslationVec;
	//@}
	/*! @name リサイズで変化するパラメータ
	*/
	//@{
	vec2<int> m_WindowSize;
	//@}
	/*! @name 背景板関係
	*/
	//@{
	quad<float> m_Plane;///< ファークリッピング面に貼りついてる平面
	color<unsigned char> m_Top;
	color<unsigned char> m_Middle;
	color<unsigned char> m_Bottom;
	//@}
};
#endif
