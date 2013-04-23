#pragma once
#include <string>
#include <iostream>
#include <sstream>
using namespace std;
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <miffy/math/cube.h>
#include <miffy/math/matrix.h>
using namespace miffy;
/// 外枠の箱。＋ボリュームの断面の線を描く。緯度経度のグリッドも描く。
class CMeasure
{
public:
	CMeasure(float _elerange);
	~CMeasure(void);
	bool CalcClipPlane(double _clipEqn[4]);
	bool Draw();///< 外側の箱の他に緯度経度のマス目も描く
	void DrawClipPlane();///< 切断面の線だけ描いてくれる。
	double adjust;
private:
	void DrawLatMag();///< 面倒くさい緯度経度のマス目を描く
	
	GLuint m_TexId;///< スライス面に断面図を乗せるため
	/*! @name 定数
    */
    //@{
	const float m_MaxHeight;///< 11km
	const float m_LENGTH;///< 目盛りの長さ
	const int m_KmInterval;///< 何メートルごとに表示するのか
	const float m_LATITUDE;///< ローカル座標での緯度の長さ
	const float m_LONGITUDE;///< ローカル座標での経度の長さ
	//@}
public:
	cube<float>  mBoundingBox;///< オブジェクトを囲む四角い枠  切り口を虹色にするのに使用する
	vec3<float> m_intersection[6];///< 断面の頂点　表示・非表示にかかわらずいつも計算する　外部からもつかいたいのでメンバ化した。
	bool m_IsCipVisible;///<  今断面が見えているかどうか 断面が見えていてはじめて重い描画計算をしたいから。風アニメにも影響する
};

