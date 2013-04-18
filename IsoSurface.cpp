#include "IsoSurface.h"
CIsoSurface::CIsoSurface(float elerange,string& _dir)
  :m_directory(_dir)
	,m_bJVert2(false)
{
	m_title="等値面";
	mEnlightCore=false;
	m_elerange=elerange;
	stringstream filename;
	string dir="iso";
	string type="jvert";
	if(type=="jvert"){//こっちは綺麗。
		cout<<"JVert1です"<<endl;
		for(int i=0;i<5;i++){mSurfaceData[i]=new JVerts();}
	}else{//こっちは壊れてる
		m_bJVert2=true;
		cout<<"JVert2です"<<endl;
		for(int i=0;i<5;i++){mSurfaceData[i]=new JVerts2();}
	}
	filename.str("");
	filename<<m_directory<<"old/"<<dir.c_str()<<"/64."<<type.c_str();
	mSurfaceData[0]->Load(filename.str().c_str());
	filename.str("");
	filename<<m_directory<<"old/"<<dir.c_str()<<"/72."<<type.c_str();
	mSurfaceData[1]->Load(filename.str().c_str());
	filename.str("");
	filename<<m_directory<<"old/"<<dir.c_str()<<"/92."<<type.c_str();
	mSurfaceData[2]->Load(filename.str().c_str());
	filename.str("");
	filename<<m_directory<<"old/"<<dir.c_str()<<"/120."<<type.c_str();
	mSurfaceData[3]->Load(filename.str().c_str());
	filename.str("");
	filename<<m_directory<<"old/"<<dir.c_str()<<"/136."<<type.c_str();
	mSurfaceData[4]->Load(filename.str().c_str());
	filename.str("");
	if(m_bJVert2){
		for(int i=0;i<5;i++){
			mSurfaceData[i]->Resize(vec3<float>(1.0f/160.0f,1.0f/160.0f,1.0f/160.0f));
		}
	}else{
		for(int i=0;i<5;i++){
			mSurfaceData[i]->Resize(vec3<float>(1.0f/80.0f,1.0f/80.0f,1.0f/80.0f));
		}
	}
	if(elerange<=0.0 || elerange>1.0){
		cout<<"elerange="<<elerange<<"が変な値"<<endl;
		cout<<"elerangeは0.0より大きく、1.0以下の値にしてください"<<endl;
		assert(!"不正な値");
	}
}

void CIsoSurface::Info(){
	cout<<m_title<<endl;
}
CIsoSurface::~CIsoSurface(void)
{
}
void CIsoSurface::Init(){
	
	glGenBuffers(1,&m_BufferId);
	glBindBuffer(GL_ARRAY_BUFFER, m_BufferId);
	int bytes=sizeof(vec3<float>)*(mSurfaceData[0]->VertNum()+mSurfaceData[1]->VertNum()
					+mSurfaceData[2]->VertNum()+mSurfaceData[3]->VertNum()
					+mSurfaceData[4]->VertNum());

	//cout<<"等値面サイズ"<<bytes/1024/1024<<"MB"<<endl;
	glBufferData(GL_ARRAY_BUFFER,bytes,0,GL_STATIC_DRAW);//ここでエラー
	//頂点情報も最初以降がおかしい。
	//最初のちょこっとしか届いてない。
	glBufferSubData(GL_ARRAY_BUFFER, 0                                             , sizeof(vec3<float>)*mSurfaceData[0]->VertNum(),mSurfaceData[0]->Data());
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3<float>)*mSurfaceData[0]->VertNum(), sizeof(vec3<float>)*mSurfaceData[1]->VertNum(),mSurfaceData[1]->Data());
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3<float>)*mSurfaceData[1]->VertNum(), sizeof(vec3<float>)*mSurfaceData[2]->VertNum(),mSurfaceData[2]->Data());
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3<float>)*mSurfaceData[2]->VertNum(), sizeof(vec3<float>)*mSurfaceData[3]->VertNum(),mSurfaceData[3]->Data());
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3<float>)*mSurfaceData[3]->VertNum(), sizeof(vec3<float>)*mSurfaceData[4]->VertNum(),mSurfaceData[4]->Data());
	if(m_bJVert2){
		mSurfaceData[0]->Info();
		glGenBuffers(1,&m_IdBufferId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m_IdBufferId);
		bytes=sizeof(unsigned int)*(mSurfaceData[0]->FaceNum()+mSurfaceData[1]->FaceNum()
										+mSurfaceData[2]->FaceNum()+mSurfaceData[3]->FaceNum()
										+mSurfaceData[4]->FaceNum());
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,bytes,0,GL_STATIC_DRAW);
		//VBOがおかしい
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0                                              , sizeof(unsigned int)*mSurfaceData[0]->FaceNum(),mSurfaceData[0]->FaceData());
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*mSurfaceData[0]->FaceNum(), sizeof(unsigned int)*mSurfaceData[1]->FaceNum(),mSurfaceData[1]->FaceData());
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*mSurfaceData[1]->FaceNum(), sizeof(unsigned int)*mSurfaceData[2]->FaceNum(),mSurfaceData[2]->FaceData());
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*mSurfaceData[2]->FaceNum(), sizeof(unsigned int)*mSurfaceData[3]->FaceNum(),mSurfaceData[3]->FaceData());
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*mSurfaceData[3]->FaceNum(), sizeof(unsigned int)*mSurfaceData[4]->FaceNum(),mSurfaceData[4]->FaceData());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		
	}
	glBindBuffer(GL_ARRAY_BUFFER,0);GetGLError("CGlsl::CreateShaderProgram");
	
}
bool CIsoSurface::Run(){
	glPushMatrix();
	
	glDisable(GL_CULL_FACE);
	glTranslatef(-0.5,-0.5,-0.5*m_elerange);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	//なぜか謎のクリッピング面が出現して困っている。
	glPointSize(8);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, m_BufferId);
	
	
	if(m_bJVert2){
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IdBufferId);
		glColor4ub(85,42,255,51);//青色
		mSurfaceData[0]->Draw();
		glColor4ub(73,241,249,51);//水色
		mSurfaceData[1]->Draw();
		glColor4ub(224,250,254,51);//
		mSurfaceData[2]->Draw();
		glColor4ub(255,249,207,51);//黄色
		mSurfaceData[3]->Draw();
		glColor4ub(255,2,83,51);//赤
		mSurfaceData[4]->Draw();
		glPointSize(8);
		glColor4ub(255,2,83,255);
		//JVert2のVBOがなぜかできない。保留
		/*glColor4ub(85,42,255,255);//青色
		glVertexPointer(3,GL_FLOAT,0,0);
		glDrawArrays(GL_POINTS,0,mSurfaceData[0]->VertNum());
		//glDrawElements(GL_LINE_LOOP,mSurfaceData[0]->FaceNum(),GL_UNSIGNED_INT,0);
		
		glColor4ub(73,241,249,51);//水色
		glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*mSurfaceData[0]->VertNum()));
		glDrawArrays(GL_POINTS,0,mSurfaceData[1]->VertNum());
		//glDrawElements(GL_LINE_LOOP,mSurfaceData[1]->FaceNum(),GL_UNSIGNED_INT,(GLubyte*)(sizeof(unsigned int)*mSurfaceData[0]->FaceNum()));
		
		glColor4ub(224,250,254,51);//
		glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*mSurfaceData[1]->VertNum()));
		glDrawArrays(GL_POINTS,0,mSurfaceData[2]->VertNum());
		//glDrawElements(GL_LINE_LOOP,mSurfaceData[2]->FaceNum(),GL_UNSIGNED_INT,(GLubyte*)(sizeof(unsigned int)*mSurfaceData[1]->FaceNum()));
		
		glColor4ub(255,249,207,51);//黄色
		glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*mSurfaceData[2]->VertNum()));
		glDrawArrays(GL_POINTS,0,mSurfaceData[3]->VertNum());
		//glDrawElements(GL_LINE_LOOP,mSurfaceData[3]->FaceNum(),GL_UNSIGNED_INT,(GLubyte*)(sizeof(unsigned int)*mSurfaceData[2]->FaceNum()));
		
		glColor4ub(255,2,83,51);//赤
		glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*mSurfaceData[3]->VertNum()));
		glDrawArrays(GL_POINTS,0,mSurfaceData[4]->VertNum());
		//glDrawElements(GL_LINE_LOOP,mSurfaceData[4]->FaceNum(),GL_UNSIGNED_INT,(GLubyte*)(sizeof(unsigned int)*mSurfaceData[3]->FaceNum()));
		*/
	}else{
		glColor4ub(85,42,255,51);//青色
		glVertexPointer(3,GL_FLOAT,0,0);
		glDrawArrays(GL_TRIANGLES,0,mSurfaceData[0]->VertNum());
		glColor4ub(73,241,249,51);//水色
		glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*mSurfaceData[0]->VertNum()));
		glDrawArrays(GL_TRIANGLES,0,mSurfaceData[1]->VertNum());
		glColor4ub(224,250,254,51);//
		glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*mSurfaceData[1]->VertNum()));
		glDrawArrays(GL_TRIANGLES,0,mSurfaceData[2]->VertNum());
		glColor4ub(255,249,207,51);//黄色
		glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*mSurfaceData[2]->VertNum()));
		glDrawArrays(GL_TRIANGLES,0,mSurfaceData[3]->VertNum());
		glColor4ub(255,2,83,51);//赤
		glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*mSurfaceData[3]->VertNum()));
		glDrawArrays(GL_TRIANGLES,0,mSurfaceData[4]->VertNum());
	}
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if(m_bJVert2){
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glDepthMask(GL_TRUE);
	glPopMatrix();
	return true;
}
void CIsoSurface::ToggleMode(bool _showtext){
	mEnlightCore=!mEnlightCore;if(_showtext){cout<<"コアの表示・非表示"<<endl;}
}
