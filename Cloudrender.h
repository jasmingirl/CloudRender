#pragma once
#include <list>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#define _USE_MATH_DEFINES
#include <cmath>
#include <ctime>//randのため
#include <cstdio>
#include <cassert>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include <sstream>
#include <iostream>
#include <functional>//mem_fnに必要
using namespace std;
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#pragma comment(lib,"assimp.lib")
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/glfw.h>
#include <miffy/math/vec3.h>
#include <miffy/math/vec4.h>
#include <miffy/gl/tga.h>
#include <miffy/gamepad.h>
#include <miffy/fileformat/jmesh.h>
#include <miffy/fileformat/jverts.h>
#include <miffy/math/triangle.h>
#include <miffy/gl/glsl.h>
#include <miffy/scene/light.h>
#include <miffy/volren/tf.h>
#include <miffy/fileread.h>
#include <mmsystem.h>
#include "GLSLReal.h"//雲シェーダ
#include "GLSLRainbow.h"//虹シェーダ
#include "IsoSurface.h"
#include "Land.h"
#include "Measure.h"
#include "TransForm.h"
#pragma comment(lib, "opengl32.lib")
static const int TRAIL_NUM=30;
static const int YELLOW_PT_NUM=50;
using namespace miffy;
/*
@brief インナークラスにしちゃった。
*/
class CPointParameter{
public:
	CPointParameter(float _gauspara,float _ptsize,size_t _znum){
		mGauspara=_gauspara;//2.0//ガウシアンテクスチャが占める面積の割合
		mPointSize=_ptsize;//初期状態で小さすぎて見えないよりかは大きい方がまし
		CalcTransParency(_znum);
	}
	float resizeVoxel(CTransForm* _scene,const float _range){//画面サイズによって点の大きさを調整する
		m_projectedVoxelSize=_scene->CalcProjectedSize(_range);
		mPointSize=mGauspara*m_projectedVoxelSize;
		if(mPointSize<0.0){assert(!"異常な点の大きさ");}
		return mPointSize;
	}
	void CalcTransParency(size_t _znum){mTransParency=20.2f/(float)_znum;}
	
public:
	float mTransParency;//全体の透明度
	float mPointSize;//点の大きさ　点の大きさはmGauspara,ウィンドウ幅、視野角に依存する
private:
	//ボリュームの見た目を左右するパラメータ
	float mGauspara;//ガウシアンテクスチャが全体の何割を占めているのか
	float m_projectedVoxelSize;//適切な点の大きさ（画面の大きさと解像度によって決まる）

};
/*!
@briefボリュームデータのサイズを表す構造体。サイズを参照する機会が沢山出現するので作った。
やっぱりこの中にボリュームデータへのポインタを入れたい。。。遅くならないかな？
*/
struct sVolSize{
	int xy;
	int z;
	int total(){return xy*xy*z;}
	int serialId(int _x,int _y,int _z){return ((_z*xy+_y)*xy+_x);}
};
/// 位置、法線と隣り合わせにするために作ったクラス
/*! このように、位置、法線と隣り合わせにした方が速くなるらしい.
アトリビュート変数もそうなんだろうか？
*/
class CParticle{
public:
	vec3<float> pos;
	vec3<float> normal;
	float intensity;//アトリビュート変数
};
/*!
@brief 一番メインの雲をレンダリングするクラス。雲だけじゃない。親玉
雲データの表示切替を左右する
@class CCloudRender CloudRender.h CloudRender
*/
class CCloudRender
{
public:
	CCloudRender(const vec2<int>& _pos);
	~CCloudRender(void);
	/*! @name 毎フレーム呼ばれる関数 */
	//@{
	bool CheckParameterChange();///< パラメータの変化を検知する。
	bool Draw();///< レンダリングの核　毎フレーム呼び出される
	void DrawWindVector();///< 風ベクトルの表示
	//@}
	/*! @name 毎フレーム呼ばれる中で、特に風アニメの時に使う関数 */
	//@{
	bool AnimateRandomFadeOut();///< ランダムにフェードアウトする風粒子
	/// ボクセルの位置を初期位置に戻す関数
	void ResetVoxelPosition(int _index){m_Dynamic[_index]=m_Static[_index].pos;m_ucIntensity[_index]=m_ucStaticIntensity[_index];}///< 各ボクセルの位置のリセット。何回も出てくるからインライン関数化した。
	///  風が吹いてボクセルの位置が動く
	/// @return 1:位置をリセットする必要あり 0:そのままでok
	/// @param _pos 粒子の位置情報
	bool BlowVoxel(vec3<float>* _pos){//風が吹いてボクセルの位置が動く

		//現在の粒子の位置を0.0-1.0に正規化 現在の位置から、参照すべき風ベクトルの位置を知るため
		vec3<float> normalized_index_pos(
			_pos->x+0.5f,
			_pos->y+0.5f,
			_pos->z);//0.0-1.0に正規化されているインデックス

		//もし配列の範囲を超えてしまったら位置をリセットする。 無い風は参照にできないから　希望としてはボリュームデータよりも大きめに風データを置きたい　じゃないと端っこが変になるから
		if(normalized_index_pos.x>=1.0f){return true;}
		if(normalized_index_pos.x<0.0f){return true;}
		if(normalized_index_pos.y>=1.0f){return true;}
		if(normalized_index_pos.y<0.0f){return true;}
		if(normalized_index_pos.z>=1.0f){return true;}
		if(normalized_index_pos.z<0.0f){return true;}

		vec3<int> windpos(
			(int)((normalized_index_pos.x)*(float)(m_Wind.xy)),
			(int)((normalized_index_pos.y)*(float)(m_Wind.xy)),
			(int)((normalized_index_pos.z)*(float)(m_Wind.z))
			);

		int windindex=((int)(windpos.z*m_Wind.xy+windpos.y)*m_Wind.xy+(int)(windpos.x));
		//行き着いた先が無風状態だった場合
		if(m_renderdata[windindex].zero()){return true;}
		//数々の条件をクリアしたら 位置を更新！！
		(*_pos)+=(m_renderdata[windindex]);

		return false;
	}
	//尾っぽ付きにしたい場合のBlowBoxel
	bool BlowVoxelWithTrail(list<vec3<float>>* _pos){//風が吹いてボクセルの位置が動く
		static int count=0;
		//何回か一回。今の粒子は過去に追いやる 1個ずらす　毎回ずらすと狭すぎてTrailの様子がわからない・
		//count++;
		//if(count%5==0){
		vec3<float> previous_pos=_pos->front();



		//現在の粒子の位置を0.0-1.0に正規化 現在の位置から、参照すべき風ベクトルの位置を知るため
		vec3<float> normalized_index_pos(
			_pos->front().x+0.5f,
			_pos->front().y+0.5f,
			_pos->front().z);//0.0-1.0に正規化されているインデックス

		//もし配列の範囲を超えてしまったら位置をリセットする。 無い風は参照にできないから　希望としてはボリュームデータよりも大きめに風データを置きたい　じゃないと端っこが変になるから
		if(normalized_index_pos.x>=1.0f){return true;}
		if(normalized_index_pos.x<0.0f){return true;}
		if(normalized_index_pos.y>=1.0f){return true;}
		if(normalized_index_pos.y<0.0f){return true;}
		if(normalized_index_pos.z>=1.0f){return true;}
		if(normalized_index_pos.z<0.0f){return true;}

		vec3<int> windpos(
			(int)((normalized_index_pos.x)*(float)(m_Wind.xy)),
			(int)((normalized_index_pos.y)*(float)(m_Wind.xy)),
			(int)((normalized_index_pos.z)*(float)(m_Wind.z))
			);

		int windindex=((int)(windpos.z*m_Wind.xy+windpos.y)*m_Wind.xy+(int)(windpos.x));
		//行き着いた先が無風状態だった場合
		if(m_renderdata[windindex].zero()){return true;}
		//数々の条件をクリアしたら 位置を更新！！
		_pos->push_front(_pos->front()+m_renderdata[windindex]);
		_pos->pop_back();
		return false;
	}
	//@}
	/*! @name ファイルスレッドの関数　６分おきのデータを扱うときに使用する関数*/
	//@{
	bool FileLoad();
	void PrepareAnimeVol();///< 6分おきのデータをレンダリングするとき、Fileスレッドで次のデータをロードする準備する関数
	bool BlendTime();///< 6分おきのデータをレンダリングするとき、前の時間と今の時間をブレンドするもの
	//@}
	//@}
	/*! @name 初期化に関する関数*/
	//@{
	void VolumeArrayInit(size_t _size);///< 配列の初期化
	void Init();///< 初期化処理
	void Reshape();///< カメラの初期化
	void InitPoints(CParticle* _voxel,unsigned char _thre);///< ボクセルの初期化。閾値以上のものをm_Staticなどに詰め込む
	void ChangeVolData();///< ボリュームデータを変える　引数にchar*があったほうがいいかも？
	void LoadVolData();///< 初期化の時も、changeの時も両方で共通のもの
	//@}
	/*! @name OpenGLコールバック関数*/
	//@{
	void JoyStick();///< ゲームパッドを扱うmInputHandlerクラスにすべき？
	//@}
	/*! @name OpenGLコールバック関数に関連して使う関数*/
	//@{
	int incrementThreshold(int _th);///< 閾値を増やす。未知のデータに遭遇した時に使うことがある
	void PrintKeyHelp();///< キーボードヘルプ
	void UpdatePara();///< シェーダに渡すパラメータのアップデート
	void ChangeWindSpeed();///< 風の速さを、好みに合わせて調整する。
	void Reset();///< 風粒子アニメのリセット
	//@}

	bool CheckFlag(unsigned int _flag){
		return (m_Flags & _flag);
	}

	void Destroy();///< 後片付け
	void SetFlag(unsigned int _flags);///< フラグのセット。orで繋げやすいように関数化した。あと、いくつか両立してはならないフラグを取り締まるため。
public:
	static enum{
		MY_CLOUD=1,///<   雲の表示・非表示切り替え
		MY_LAND=1<<1,///< 地図を表示するかどうか
		MY_ANIMATE=1<<2,///<  局所的な風を止めるか、止めないか
		MY_MEASURE=1<<3,///< 外側の箱を表示するかどうか
		MY_RED_POINT=1<<4,///< 赤い点を表示するかどうか
		MY_ISOSURFACE=1<<5,///< 等値面を表示するかどうか
		MY_CAPTURE=1<<6,///< 動画キャプチャモードなのかどうか キャプチャーモードは1フレーム1秒になる
		MY_FLOW_DAYTIME=1<<7,///< 大域時間を流すかどうか 6分おきの時間の流れを止めるかどうか この変数はm_bCurrentVolData=1のときだけ働きたい
		MY_VECTOR=1<<8,///<  風ベクトルの表示・非表示
		MY_FLYING=1<<9,///< 飛行機運転モードのスタート
		MY_VOLDATA=1<<10,///<  解像度の良い雲0か、6分おきに変わる雲1か GUIからアクセスする関係上ここに配置
		MY_YELLOW_POINTS=1<<11,///< 黄色い点を表示するかどうか
		MY_TIME_MEASURE=1<<12,///< 時間測定をスタートするかどうか
		MY_DYNAMIC_CLOUD=1<<13,///< 風で動く雲を表示するかどうか
		MY_STATIC_CLOUD=1<<14,///< 常に止まっている雲を表示するかどうか
		MY_ANIMATE_YELLOW=1<<15,///< 黄色い点を動かすかどうか
		MY_FLAG_NUM=16///< フラグの数
	};
	/*! @name 外部からアクセスする必要のある変数達 */
	//@{
	GLdouble m_ClippingEquation[4];///< クリッピング方程式 こいつはあちこちから呼び出される人気の変数である。
	
	unsigned int m_ClipDirection;///< 0=x,1=y,2=z
	CTransForm* m_TransForm;///< モデルビュー行列
	float m_ZScale;///< 高さ方向のスケール　これがGoogleEarthにはない大事な機能 矢印だけzをひしゃげたくないのでCTransFormから追い出してここに設置した。
	float m_Karmapara;///< 風の生まれ変わりを左右するパラメータ　0～1.0まで変化する 大きいほど生まれ変わりやすい
	CPointParameter* m_PtPara;///<  点の見た目を左右するパラメータ
	//@}
	/*! @name publicな脇役なレンダリングオブジェクト達 */
	//@{
	CLand* m_Land;///< CControlPaneで使う
	CMeasure* m_Measure;///< 外枠の箱
	//@}

private: 
	/*! @name 毎フレーム変化するメンバ変数変数 */
	//@{
	CParticle* m_Static;///<  粒子の初期位置として必要
	CParticle* mBeforeVoxel;///<  6分おきに流すのに必要なメンバ
	CParticle* mAfterVoxel;///<  6分おきに流すのに必要なメンバ
	vec3<float>* m_Dynamic;///< 風に吹かれる粒子
	float m_fTimeRatio;///<  今→６分後　までどのぐらい遷移したのか
	int m_nFileId;///< ６分おきのファイルをロードするのに使う。ファイル名が数字のインクリメントだから
	vec3<float> m_RedPoint;///< 風の動きを見るための赤い一粒の点
	list<vec3<float>>* m_YellowPoints;///< 風の動きを見せるための黄色い点[100]個　5個過去の履歴を残しておく 0が一番最近 どうしよ、黄色にしたくなくなっちゃった。
	//@}
	/*! @name 脇役なレンダリングオブジェクト達 */
	//@{
	CIsoSurface* m_IsoSurface;///< 等値面
	CLight* m_Light;///< ライト
	//@}
	/*! @name ユーザ操作によってたまに変化する変数 */
	//@{
	unsigned int m_Flags;///< 色々と表示・非表示を制御するフラグ
	color<float> mFontColor;///<  文字色　背景が白の時は黒・背景が黒の時は白
	color<float> mBackGround;///< 背景の四角い板グラデーション
	unsigned char m_threshold;///< 閾値　これ以下の信号強度は透明になる。
	int m_ValidPtNum;///< 閾値以下だった点の数　閾値と連動する
	string m_ToggleText[2];///< 0:特機の雲 1:6分おきの雲
	float m_fWindSpeed;///<  風の速さ調節パラメータ
	float zScale;///<  ボリュームデータはz方向が足りないのが多いのでこうする
	//@}
	/*! @name ボリュームデータごとに固定の変数*/
	//@{
	vec3<float>* m_renderdata;///<  レンダリング用風データ
	vec3<float>* m_rawdata;///<  オリジナル風データ
	unsigned char* m_ucIntensity;///<  生データ。断面図テクスチャに使う 風アニメする
	unsigned char* m_ucStaticIntensity;///<  リセットしたときにテクスチャを元に戻す、ファイルスレッドで使う
	struct sVolSize m_Wind;///<  風データのサイズ
	struct sVolSize m_VolSize;///<  ボリュームデータのサイズ
	//GLSLAtt mIntensity;///<  点の透明度　ボリュームデータは動的に変わるけど、シェーダ単位では変わらないからここに存在させておく
	bool *m_RandomTable;///< 乱数テーブル　風アニメで、生死判定に使う。
	//@}
	/*! @name 一度初期化したら変わらない変数 */
	//@{
	const string m_IniFile;///<  初期設定ファイル.iniのパス
	unsigned char m_dataMax;///<  ボクセルの強度の最大値　constにしたいけど、設定ファイルから読み込むから初期化子を使えない
	int m_Max3DTexSize;///< その端末で表示できる３Dテクスチャの最大サイズ
	color<float>* m_ucWindTF;///<  風の矢印を虹色にするための伝達関数
	GLuint m_GausTexSamp;///<  ガウシアンテクスチャ　シェーダに渡す 
	GLuint m_CallList;///< 矢印描画データ
	CGLSLReal *mProgram;///< GLSL
	CGLSLRainbow *mRainbowProgram;///< 雲を虹色にレンダリングするシェーダ
	string m_DataKey[2];///< ボリュームデータの識別
	//@}
	//@}
	/*! @name デバッグ用　時間測定、プロファイリングしたりする*/
	//@{
	unsigned long m_FirstPeriodSum;
	unsigned long m_SecondPeriodSum;
	unsigned long m_nFrame;///< フレームカウント
	//@}
};

