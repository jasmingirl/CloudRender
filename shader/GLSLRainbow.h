#pragma once
#ifndef __GLEW_H__
#include <GL/glew.h>
#endif
#include <map>
using namespace std;
/*!
  @brief クリッピング面を虹色に描画する。クラス名変えたいかも。Rainbowというより、SlicePlaneだよね。
*/
class CGLSLRainbow
{
public:
	CGLSLRainbow(const string& _vertfile,const string& _fragfile);
	void GetLocations();
	~CGLSLRainbow(void);
	/*!
	ただのクラス内のメンバ関数をテンプレートにしたいとき、
	cpp内に書く方法はないのか。・・・！！？？？
	@param _type GL_UNSIGNED_BYTEなどの型情報を渡す
	*/
	template <typename T>
	void Add3DTexture(T* _data,int _xy,int _z,int _type){
		glActiveTexture(GL_TEXTURE1);
		LoadVolumeDataFromMemory(_data,&m_TexId[VOLUME_TEX],_xy,_z, _type);//断面図を描くのに使うテクスチャ
		miffy::GetGLError("Add3DTexture");
	}
	 void InitTransFuncFromFile(const string& _filepath);
	/*!
	@brief なぜここでやる必要があるか？こいつがm_TexIdをメンバに持っているからだ！
	あー、これってテンプレートの特殊化ができるかも
	*/
	template <typename T>
	void Reload3DTexture(T* _data,int _xy,int _z,int _type){
		glBindTexture ( GL_TEXTURE_3D, m_TexId[VOLUME_TEX] );
		glTexSubImage3D ( GL_TEXTURE_3D, 0, 0,0,0, _xy, _xy, _z,GL_LUMINANCE, _type, _data );
		miffy::GetGLError("Reload3DTexture");
	}
	void LoadTransFuncFromFile(const string& _filepath);
	void UpdateProjectionMat(float* _m);
	void UpdateModelViewMat(float* _m);
	void UpdateTextureUniform();
private:
	map<string,int> m_Map;//名前と値loc
	enum{VOLUME_TEX,TRANSFER_TEX,NUM_TEX};
	unsigned int m_TexId[NUM_TEX];
public:
	unsigned int  m_Program;//本体
};

