#include "GLSLRainbow.h"
#include <miffy/gl/glutility.h>//glgeterrorのためだけに呼んでいる。なんか無駄な気がする
#include <miffy/gl/shaderutility.h>
#include <miffy/volren/glvolren.h>
CGLSLRainbow::CGLSLRainbow(const string& _vertfile,const string& _fragfile)
{
  m_Program=miffy::CreateShaderFromFile(_vertfile,_fragfile);
	GetLocations();
	
}
void CGLSLRainbow::GetLocations(){
	string names[]={"ModelView","Proj","voltex","tf"};
	for(int i=0;i<4;i++){
		m_Map.insert(pair<string,int>(names[i],glGetUniformLocation(m_Program,names[i].c_str())));
	}
	
}

CGLSLRainbow::~CGLSLRainbow(void)
{
}
void CGLSLRainbow::UpdateTextureUniform(){
	glUseProgram(m_Program);
	glUniform1i(m_Map["tf"],0);
	glUniform1i(m_Map["voltex"],1);
	glUseProgram(0);
}
void CGLSLRainbow::UpdateProjectionMat(float* _m){
	glUseProgram(m_Program);
	glUniformMatrix4fv(m_Map["Proj"], 1, 0,_m);
	glUseProgram(0);
}
 void CGLSLRainbow::InitTransFuncFromFile(const string& _filepath){
	glActiveTexture(GL_TEXTURE0);
	miffy::LoadTFRawDataFromFile(&m_TexId[TRANSFER_TEX],_filepath.c_str());//虹色モードで使用
	
 }
 void CGLSLRainbow::LoadTransFuncFromFile(const string& _filepath){
	HANDLE handle;
	DWORD dwnumread;
	DWORD high,filebytes;
	handle = CreateFile(_filepath.c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	filebytes=GetFileSize(handle,&high);
	float* tfdata=new float[filebytes>>2];//
	ReadFile(handle,tfdata,filebytes,&dwnumread,NULL);
	CloseHandle(handle);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D,m_TexId[TRANSFER_TEX]);
	glTexSubImage1D(GL_TEXTURE_1D,0,0,filebytes>>4,GL_RGBA,GL_FLOAT,tfdata);
	delete tfdata;
 }
 void  CGLSLRainbow::UpdateModelViewMat(float* _m){
	 glUniformMatrix4fv(m_Map["ModelView"], 1, 0,_m);
 }
