#pragma once
#ifndef MIFFYSHADER
#define MIFFYSHADER
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <string>
#include <cstdio>
#include "glutility.h"
#include "../math/vec3.h"
#include "../math/color.h"
#include "../math/matrix.h"
using namespace std;
namespace miffy{
  /*!
	@brief こういう分け方はよくないかも、と思えてきた。オブジェクト間の連絡がいっぱい発生するから。
	*/
	template <typename T>
	struct GLSLpara{
	public:
		T val;//値
		GLSLpara():loc(0){}
		GLSLpara(const string& _name):name(_name),loc(0){}
		void Info(){cout<<"loc="<<loc<<" name="<<name<<" val="<<val<<endl; } 
		void Uniform(){}//これってvalを引数にしたほうがいいかも。動的に変化する奴は特に。
		void Uniform(const T& _val){}
		void GetUniformLocation(GLuint _program,const string& _name){
			name=_name;
			loc=glGetUniformLocation(_program,name.c_str());
			if(loc==-1){assert(!"存在しないユニフォーム変数名");}
		}
		void GetUniformLocation(GLuint _program){
			loc=glGetUniformLocation(_program,name.c_str());
			if(loc==-1){assert(!"存在しないユニフォーム変数名");}
			//if(loc==0){assert(!"glProgram()した？");}//0でもよかったよね？
		}
		T operator+=(T _val){return val+=_val;}
	private:
		GLuint loc;//シェーダに渡す用の名前
		string name;//シェーダ上に書かれている名前	
	};
	template<> void GLSLpara<float>::Uniform(){glUniform1f(loc,val);}
	template<> void GLSLpara< color<float> >::Uniform(){glUniform3f(loc,val.r,val.g,val.b);}
	template<> void GLSLpara< vec3<float> >::Uniform(){glUniform3f(loc,val.x,val.y,val.z);}
	template<> void GLSLpara< mat4<float> >::Uniform(){glUniformMatrix4fv(loc, 1, 0,val.m);}
	template<> void GLSLpara< mat4<float> >::Uniform(const mat4<float>& _val){glUniformMatrix4fv(loc, 1, 0,_val.m);}
	struct GLSLAtt{//アトリビュート変数
	public:
		GLuint loc;//シェーダに渡す用の名前
		GLSLAtt(){}
		void GetLocation(GLuint _program,const string& _name){
			loc=glGetAttribLocation(_program,_name.c_str());
		}
		template <typename T>
		void glPointer(int _elemNum,int _datatype,int _stride,T* _p){
			glVertexAttribPointer(loc,_elemNum,_datatype,GL_FALSE,_stride,_p);
			glEnableVertexAttribArray(loc);
		}
		void Enable(){glEnableVertexAttribArray(loc);}

	};
	struct GLSLsamp{//テクスチャサンプラー
	public:
		GLuint loc;
		string name;
		GLuint active_tex;
		GLuint tex_id;
		GLSLsamp(){}
		void Uniform(){glUniform1i(loc,active_tex);}
		void Bind2D(){glBindTexture ( GL_TEXTURE_2D,tex_id );}
		void GetUniformLocation(GLuint _program){//名前、getの方がいいかなぁ
			loc=glGetUniformLocation(_program,name.c_str());
		}
		void GetUniformLocation(GLuint _program,const string& _name){
			name=_name;
			loc=glGetUniformLocation(_program,_name.c_str());
		}
		void ActiveTexture(GLuint _active_tex){
			glActiveTexture(_active_tex);
			active_tex=(_active_tex ^ 0x84C0);
			//注意！GL_TEXTURE0などの定数参照
			//GL_TEXTURE16 no!
			if(_active_tex>=GL_TEXTURE16){assert(!"GL_TEXTURE16より大きい数は未対応。新たにコーディングしてください。");}
		}
	};
	static void GetShaderInfoLog(GLuint shader)
	{//シェーダの情報を表示する
		GLsizei bufSize;

		/* シェーダのコンパイル時のログの長さを取得する */
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &bufSize);

		if (bufSize > 1) {
			GLchar *infoLog = (GLchar *)malloc(bufSize);

			if (infoLog != NULL) {
				GLsizei length;

				/* シェーダのコンパイル時のログの内容を取得する */
				glGetShaderInfoLog(shader, bufSize, &length, infoLog);
				printf("InfoLog:\n%s\n\n", infoLog);
				free(infoLog);
			}
			else
				printf("Could not allocate InfoLog buffer.\n");
		}
	}
	static GLuint loadShader(GLenum shaderType, const char* pSource) {
		GLuint shader = glCreateShader(shaderType);
		if (shader) {
			glShaderSource(shader, 1, &pSource, NULL);
			glCompileShader(shader);//シェーダをコンパイルする。
			GLint compiled = 0;
			//コンパイルチェック
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
			if (!compiled) {
				GetShaderInfoLog(shader);
				glDeleteShader(shader);
			}
		}else{
			GetGLError("shader is zero!");
			GetShaderInfoLog(shader);
			printf("shader is zero!!%d\n",shader);
		}
		return shader;
	}
	static GLuint CreateShaderProgram(const char* pVertexSource, const char* pFragmentSource) {
		// シェーダプログラムをロードする
		GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
		if (!vertexShader) {
			printf("vertexShader=%d\n",vertexShader);
			assert("Could not create program.");
			GetGLError("why could not create program?\n");
			return 0;
		}
		GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
		if (!pixelShader) {
			printf("pixelShader=%d\n",pixelShader);
			assert("Could not create program.");
			GetGLError("why could not create program?\n");
			return 0;
		}

		GLuint program = glCreateProgram();
		if (program) {
			glAttachShader(program, vertexShader);
			glAttachShader(program, pixelShader);
			glLinkProgram(program);
			GLint linkStatus = GL_FALSE;
			glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
			if (linkStatus != GL_TRUE) {
				GLint bufLength = 0;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
				if (bufLength) {
					char* buf = (char*) malloc(bufLength);
					if (buf) {
						glGetProgramInfoLog(program, bufLength, NULL, buf);
						printf("error=%s",buf);
						free(buf);
					}
				}
				glDeleteProgram(program);
				program = 0;
			}
		}else{
			printf("program is zeo!! at %d",__LINE__);
		}
		return program;
	}
	static 	GLuint CreateShaderProgram(const char* pVertexSource, const char* pFragmentSource,const char* pGeomerySource) {
		// シェーダプログラムをロードする
		GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
		if (!vertexShader) {
			printf("vertexShader=%d\n",vertexShader);
			assert("Could not create program.");
			GetGLError("why could not create program?\n");
			return 0;
		}
		GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
		if (!pixelShader) {
			printf("pixelShader=%d\n",pixelShader);
			assert("Could not create program.");
			GetGLError("why could not create program?\n");
			return 0;
		}
		GLuint geometryShader = loadShader(GL_GEOMETRY_SHADER_EXT, pGeomerySource);
		if (!geometryShader) {
			printf("pixelShader=%d\n",geometryShader);
			assert("Could not create program.");
			GetGLError("why could not create program?\n");
			return 0;
		}
		GLuint program = glCreateProgram();
		if (program) {
			glAttachShader(program, vertexShader);
			glAttachShader(program, pixelShader);
			glAttachShader(program, geometryShader);
			
			

		}else{
			printf("program is zeo!! at %d",__LINE__);
		}
		
		return program;
	}
	static 	GLuint LinkProgram(GLuint _program){
			glLinkProgram(_program);
			GLint linkStatus = GL_FALSE;
			glGetProgramiv(_program, GL_LINK_STATUS, &linkStatus);
			if (linkStatus != GL_TRUE) {
				GLint bufLength = 0;
				glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &bufLength);
				if (bufLength) {
					char* buf = (char*) malloc(bufLength);
					if (buf) {
						glGetProgramInfoLog(_program, bufLength, NULL, buf);
						printf("error=%s",buf);
						free(buf);
					}
				}
				glDeleteProgram(_program);
				_program = 0;
			}
			return _program;
		}
	static GLuint CreateShaderFromFile(const miffychar* _vertexpath,const miffychar* _fragmentpath,const miffychar* _geometrypath){
		char* gVertexShader;char* gFragmentShader;char* pGeometryShader;
		FILE* fp;
		fopen_s(&fp,_vertexpath,"rb");
		if(fp==NULL){
			char buf[200];
			GetCurrentDirectory(200,buf);
			printf("current directory:%s\n",buf);
			assert(!"file could not open!");}
		//ファイルの長さを調べる
		fseek(fp,0,SEEK_END);
		int filebytes= ftell(fp);
		fseek(fp,0,SEEK_SET);//先頭まで移動
		gVertexShader=new char[filebytes+1];
		fread(gVertexShader,sizeof(char),filebytes,fp);
		fclose(fp);
		gVertexShader[filebytes]='\0';//終わりだよ印をつける
		fopen_s(&fp,_fragmentpath,"rb");
		if(fp==NULL){
			char buf[200];
			GetCurrentDirectory(200,buf);
			printf("current directory:%s\n",buf);
			assert(!"file could not open!");}
		//ファイルの長さを調べる
		fseek(fp,0,SEEK_END);
		filebytes= ftell(fp);
		fseek(fp,0,SEEK_SET);//先頭まで移動
		gFragmentShader=new char[filebytes+1];
		fread(gFragmentShader,sizeof(char),filebytes,fp);
		fclose(fp);
		gFragmentShader[filebytes]='\0';//終わりだよ印をつける
		
		
		fopen_s(&fp,_geometrypath,"rb");
		if(fp==NULL){
			char buf[200];
			GetCurrentDirectory(200,buf);
			printf("current directory:%s\n",buf);
			assert(!"file could not open!");}
		//ファイルの長さを調べる
		fseek(fp,0,SEEK_END);
		 filebytes= ftell(fp);
		fseek(fp,0,SEEK_SET);//先頭まで移動
		pGeometryShader=new char[filebytes+1];
		fread(pGeometryShader,sizeof(char),filebytes,fp);
		fclose(fp);
		pGeometryShader[filebytes]='\0';//終わりだよ印をつける
		return CreateShaderProgram(gVertexShader,gFragmentShader,pGeometryShader);

	}
	static GLuint CreateShaderFromFile(const string& _vertexpath,const string& _fragmentpath){
		char* gVertexShader;char* gFragmentShader;
		FILE* fp;
		fopen_s(&fp,_vertexpath.c_str(),"rb");
		if(fp==NULL){
			char buf[200];
			GetCurrentDirectory(200,buf);
			printf("current directory:%s\n",buf);
			assert(!"file could not open!");}
		//ファイルの長さを調べる
		fseek(fp,0,SEEK_END);
		int filebytes= ftell(fp);
		fseek(fp,0,SEEK_SET);//先頭まで移動
		gVertexShader=new char[filebytes+1];
		fread(gVertexShader,sizeof(char),filebytes,fp);
		fclose(fp);
		gVertexShader[filebytes]='\0';//終わりだよ印をつける
		fopen_s(&fp,_fragmentpath.c_str(),"rb");
		if(fp==NULL){
			char buf[200];
			GetCurrentDirectory(200,buf);
			printf("current directory:%s\n",buf);
			assert(!"file could not open!");}
		//ファイルの長さを調べる
		fseek(fp,0,SEEK_END);
		filebytes= ftell(fp);
		fseek(fp,0,SEEK_SET);//先頭まで移動
		gFragmentShader=new char[filebytes+1];
		fread(gFragmentShader,sizeof(char),filebytes,fp);
		fclose(fp);
		gFragmentShader[filebytes]='\0';//終わりだよ印をつける
		return CreateShaderProgram(gVertexShader,gFragmentShader);
	}
	
	static void PrintActiveAttributesNum(GLuint _program){
		GLint activeattribnum;
		glGetProgramiv(_program, GL_ACTIVE_ATTRIBUTES, &activeattribnum);
		printf("active attributes num=%d\n",activeattribnum);
	}
	static void GetProgramInfo(GLuint _program,GLenum _pname,const char* _message){
		int name;
		glGetProgramiv(_program,_pname,&name);
		printf("%s:%d\n",_message,name);
	}

}
#endif
