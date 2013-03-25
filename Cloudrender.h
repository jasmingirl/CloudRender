#pragma once

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

#include "pointparameter.h"
#include "IsoSurface.h"
#include "Land.h"
#include "Measure.h"
#include <miffy/gl/textureutility.h>//主にビットマップを読み込む
#include <miffy/gamepad.h>
#include <miffy/fileformat/jmesh.h>
#include <miffy/fileformat/jverts.h>
#include <miffy/math/triangle.h>
#include <miffy/gl/glsl.h>
#include <miffy/scene/light.h>
#include "TransForm.h"
#pragma comment(lib, "opengl32.lib")
using namespace miffy;
/// ボリュームデータのサイズを表す構造体。サイズを参照する機会が沢山出現するので作った。
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
	bool Run(void);///< レンダリングの核　毎フレーム呼び出される
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
	bool BlowVoxel(vec3<float>& _pos){//風が吹いてボクセルの位置が動く
		//現在の粒子の位置を0.0-1.0に正規化 現在の位置から、参照すべき風ベクトルの位置を知るため
			vec3<float> normalized_index_pos(
				_pos.x+0.5f,
				_pos.y+0.5f,
				_pos.z);//0.0-1.0に正規化されているインデックス
			
			//もし配列の範囲を超えてしまったら位置をリセットする。 無い風は参照にできないから　希望としてはボリュームデータよりも大きめに風データを置きたい　じゃないと端っこが変になるから
			if(normalized_index_pos.x>1.0f){return true;}
			if(normalized_index_pos.x<0.0f){return true;}
			if(normalized_index_pos.y>1.0f){return true;}
			if(normalized_index_pos.y<0.0f){return true;}
			if(normalized_index_pos.z>1.0f){return true;}
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
			_pos+=(m_renderdata[windindex]);
			return false;
	}
	/*! @name ファイルスレッドの関数　６分おきのデータを扱うときに使用する関数*/
	//@{
		bool FileLoad();
		void PrepareAnimeVol();///< 6分おきのデータをレンダリングするとき、Fileスレッドで次のデータをロードする準備する関数
		bool BlendTime();///< 6分おきのデータをレンダリングするとき、前の時間と今の時間をブレンドするもの
	//@}
	//@}
	/*! @name 初期化に関する関数*/
	//@{
		void ArrayInit(size_t _size);///< 配列の初期化
		void Init();///< 初期化処理
		void Reshape();///< カメラの初期化
		void InitPoints(CParticle* _voxel,unsigned char _thre);///< ボクセルの初期化。閾値以上のものをm_Staticなどに詰め込む
		int GenerateWindow();
		void ChangeVolData();///< ボリュームデータを変える　引数にchar*があったほうがいいかも？
	//@}
	/*! @name OpenGLコールバック関数*/
	//@{
		void KeyBoard();///< キーボード入力を扱う　GLFWのみで書くときは必要だけど、AntTweakBarを使うなら必要ない。
		void JoyStick();///< ゲームパッドを扱うmInputHandlerクラスにすべき？
		void RootMenu(int val);///< 右クリックメニュー
	//@}
	/*! @name OpenGLコールバック関数に関連して使う関数*/
	//@{
		int incrementThreshold(int _th);///< 閾値を増やす。未知のデータに遭遇した時に使うことがある
		void PrintKeyHelp();///< キーボードヘルプ
		void UpdatePara();///< シェーダに渡すパラメータのアップデート
		void ChangeWindSpeed();
		void Reset();///< 風粒子アニメのリセット

	
	string* GetToggleText();///< トグルボタン表示用のテキスト
	bool GetCurrentState();///< 現在のトグルの状態を取得する　関数名わかりにくいから変えるべき
	void Destroy();///< 後片付け
	
public:
	/*! @name 外部からアクセスする必要のある変数達 */
	//@{
	CTransForm* m_TransForm;///< モデルビュー行列
	bool m_bCapture;///< キャプチャーモードは1フレーム1秒になる
	bool m_bCurrentVolData;///<  解像度の良い雲か、6分おきに変わる雲か GUIからアクセスする関係上ここに配置
	bool m_bAnimateLocalWind;///<  局所的な風を止めるか、止めないか
	bool m_bShowCloud;///<   雲の表示・非表示切り替え
	bool m_bHideVector;///<  風ベクトルの表示・非表示
	bool m_bShowLand;///< 地図を表示するかどうか
	bool m_bFlowDayTime;///<  6分おきの時間の流れを止めるかどうか この変数はm_bCurrentVolData=1のときだけ働きたい
	//@}
/*! @name publicな脇役なレンダリングオブジェクト達 */
	//@{
	CLand* m_Land;///< CControlPaneで使う
	CMeasure* m_Measure;///< 外枠の箱
	//@}
	
private: 
	/*! @name 毎フレーム変化するメンバ変数変数 この沢山のフラグをまとめられないかなぁ。。？？*/
	//@{
		bool m_bSliceVisible;///<  今断面が見えているかどうか 断面が見えていてはじめて重い描画計算をしたいから。風アニメにも影響する
		CParticle* m_Static;///<  粒子の初期位置として必要
		CParticle* mBeforeVoxel;///<  6分おきに流すのに必要なメンバ
		CParticle* mAfterVoxel;///<  6分おきに流すのに必要なメンバ
		vec3<float>* m_Dynamic;///< 風に吹かれる粒子
		GLSLpara< vec4<float> > mLocalCam;///<  オブジェクトに中心を原点とした時のカメラ位置
		float m_fTimeRatio;///<  今→６分後　までどのぐらい遷移したのか
		int m_nFrame;///< フレームカウント６分おきのファイルをロードするのに使う。ファイル名が数字のインクリメントだから
		vec3<float> m_RedPoint;///< 風の動きを見るための赤い一粒の点
	//@}
	/*! @name 脇役なレンダリングオブジェクト達 */
		//@{
		CIsoSurface* m_IsoSurface;///< 等値面
		CLight* m_Light;///< ライト
		cube<float>  mBoundingBox;///<  オブジェクトを囲む四角い枠 切り口を虹色にするのに使用する
		
		//@}
	/*! @name ユーザ操作によってたまに変化する変数 */
	//@{
		color<float> m_BgColor;///<  背景色：白か黒
		color<float> mFontColor;///<  文字色　背景が白の時は黒・背景が黒の時は白
		color<float> mBackGround;///< 背景の四角い板グラデーション
		unsigned char m_threshold;
		int m_ValidPtNum;///< 閾値以下だった点の数　閾値と連動する
		string m_ToggleText[2];///< 0:特機の雲 1:6分おきの雲
		float m_fWindSpeed;///<  風の速さ調節パラメータ
		int m_Karmapara;///< 風の生まれ変わりを左右するパラメータ　0～m_ValidPtNumまで変化する
		bool mStartFlightFlag;///< 飛行機運転モードのスタート
		float zScale;///<  ボリュームデータはz方向が足りないのが多いのでこうする
		CPointParameter* m_pt_para;///<  点の見た目を左右するパラメータ
	//@}
	/*! @name ボリュームデータごとに固定の変数*/
	//@{
		vec3<float>* m_renderdata;///<  レンダリング用風データ
		vec3<float>* m_rawdata;///<  オリジナル風データ
		unsigned char* m_ucIntensity;///<  生データ。断面図テクスチャに使う 風アニメする
		unsigned char* m_ucStaticIntensity;///<  リセットしたときにテクスチャを元に戻す、ファイルスレッドで使う
		struct sVolSize m_Wind;///<  風データのサイズ
		struct sVolSize m_VolSize;///<  ボリュームデータのサイズ
		GLSLAtt mIntensity;///<  点の透明度　ボリュームデータは動的に変わるけど、シェーダ単位では変わらないからここに存在させておく
	//@}
	/*! @name 一度初期化したら変わらない変数 */
	//@{
		const string m_IniFile;///<  設定ファイルパス
		unsigned char m_dataMax;///<  ボクセルの強度の最大値　constにしたいけど、設定ファイルから読み込むから初期化子を使えない
		int m_Max3DTexSize;///< その端末で表示できる３Dテクスチャの最大サイズ
		color<float>* m_ucWindTF;///<  風の矢印を虹色にするための伝達関数
		GLSLsamp m_GausTexSamp;///<  ガウシアンテクスチャ　シェーダに渡す
		CGlsl *mProgram;///< GLSL
		CGlsl *mRainbowProgram;///< 雲を虹色にレンダリングするシェーダ
		string m_DataKey[2];///< ボリュームデータの識別
	//@}

	
	
	
	
	
	
};

