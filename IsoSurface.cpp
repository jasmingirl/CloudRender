#include "IsoSurface.h"
CIsoSurface::CIsoSurface(float elerange)

{
	m_title="等値面";
	mEnlightCore=false;
	m_elerange=elerange;
	
	if(elerange<=0.0 || elerange>1.0){
		cout<<"elerange="<<elerange<<"が変な値"<<endl;
		cout<<"elerangeは0.0より大きく、1.0以下の値にしてください"<<endl;
		assert(!"不正な値");
	}
	m_Colors[0].set(85,42,255,51);//青色
	m_Colors[1].set(73,241,249,51);//水色
	m_Colors[2].set(224,250,254,51);//
	m_Colors[3].set(255,249,207,51);//黄色
	m_Colors[4].set(255,2,83,51);//赤
}

void CIsoSurface::Info(){
	cout<<m_title<<endl;
}
CIsoSurface::~CIsoSurface(void)
{
}
void CIsoSurface::Init(const char* _inifilepath){
	char str_from_inifile[200];
	
	string names[]={"iso0","iso1","iso2","iso3","iso4"};
	
	for(int i=4;i>=0;i--){
		GetPrivateProfileString("iruma",names[i].c_str(),"失敗",str_from_inifile,200,_inifilepath);
		
		if(!plydata[i].LoadPlyData(str_from_inifile)){
			OutputDebugString("ロード失敗\n");
			assert(!"ロード失敗");
		}
		m_VertNum[i]=plydata[i].mVertNum;
		m_IndexNum[i]=plydata[i].mFacenum*3;
		//stringstream cdbg;//たぶん、位置が変なのよ
		//cdbg<<m_VertNum[i]<<endl;
		//OutputDebugString(cdbg.str().c_str());
	}	
	//glGenBuffers(1,&m_BufferId);
	//glBindBuffer(GL_ARRAY_BUFFER, m_BufferId);
	//int bytes=sizeof(vec3<float>)*(m_VertNum[0]+m_VertNum[1]+m_VertNum[2]+m_VertNum[3]+m_VertNum[4]);
	////for(int i=0;i<100;i++){
	////	stringstream cdbg;//たぶん、位置が変なのよ
	////	cdbg<<plydata[4].m_Verts[i].x<<endl;
	////	OutputDebugString(cdbg.str().c_str());
	////}
	////cout<<"等値面サイズ"<<bytes/1024/1024<<"MB"<<endl;
	//glBufferData(GL_ARRAY_BUFFER,bytes,0,GL_STATIC_DRAW);//ここでエラー
	////頂点情報も最初以降がおかしい。
	////最初のちょこっとしか届いてない。
	//glBufferSubData(GL_ARRAY_BUFFER, 0                               , sizeof(vec3<float>)*m_VertNum[0],&(plydata[0].m_Verts[0].x));
	//glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3<float>)*m_VertNum[0], sizeof(vec3<float>)*m_VertNum[1],&(plydata[1].m_Verts[0].x));
	//glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3<float>)*m_VertNum[1], sizeof(vec3<float>)*m_VertNum[2],&(plydata[2].m_Verts[0].x));
	//glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3<float>)*m_VertNum[2], sizeof(vec3<float>)*m_VertNum[3],&(plydata[3].m_Verts[0].x));
	//glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3<float>)*m_VertNum[3], sizeof(vec3<float>)*m_VertNum[4],&(plydata[4].m_Verts[0].x));
	//
	//glBindBuffer(GL_ARRAY_BUFFER,0);
	//glGenBuffers(1,&m_IdBufferId);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IdBufferId);
	//bytes=sizeof(unsigned int)*(m_IndexNum[0]+m_IndexNum[1]+m_IndexNum[2]+m_IndexNum[3]+m_IndexNum[4]);

	//glBufferData(GL_ELEMENT_ARRAY_BUFFER,bytes,0,GL_STATIC_DRAW);//ここでエラー
	////頂点情報も最初以降がおかしい。
	////最初のちょこっとしか届いてない。
	//glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0                               ,  sizeof(unsigned int)*m_IndexNum[0],&(plydata[0].m_Indices[0]));
	//glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*m_IndexNum[0], sizeof(unsigned int)*m_IndexNum[1],&(plydata[1].m_Indices[0]));
	//glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*m_IndexNum[1], sizeof(unsigned int)*m_IndexNum[2],&(plydata[2].m_Indices[0]));
	//glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*m_IndexNum[2], sizeof(unsigned int)*m_IndexNum[3],&(plydata[3].m_Indices[0]));
	//glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*m_IndexNum[3], sizeof(unsigned int)*m_IndexNum[4],&(plydata[4].m_Indices[0]));
	//
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	
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
	glEnableClientState(GL_INDEX_ARRAY);
	//glBindBuffer(GL_ARRAY_BUFFER, m_BufferId);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IdBufferId);
	//	glColor4ub(85,42,255,51);//青色
	//	glVertexPointer(3,GL_FLOAT,0,0);
	//	glIndexPointer(GL_INT,0,0);
	//	glDrawElements(GL_TRIANGLES,m_VertNum[0],GL_UNSIGNED_INT,0);
	//	//glDrawArrays(GL_TRIANGLES,0,m_VertNum[0]);
	//	glColor4ub(73,241,249,51);//水色
	//	glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*m_VertNum[0]));
	//	glIndexPointer(GL_INT,0,(GLubyte*)(sizeof(unsigned int)*m_IndexNum[0]));
	//	glDrawElements(GL_TRIANGLES,m_VertNum[1],GL_UNSIGNED_INT,0);
	//	//glDrawArrays(GL_TRIANGLES,0,m_VertNum[1]);
	//	glColor4ub(224,250,254,51);//
	//	glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*m_VertNum[1]));
	//	glIndexPointer(GL_INT,0,(GLubyte*)(sizeof(unsigned int)*m_IndexNum[1]));
	//	glDrawElements(GL_TRIANGLES,m_VertNum[2],GL_UNSIGNED_INT,0);
	//	//glDrawArrays(GL_TRIANGLES,0,m_VertNum[2]);
	//	glColor4ub(255,249,207,51);//黄色
	//	glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*m_VertNum[2]));
	//	glIndexPointer(GL_INT,0,(GLubyte*)(sizeof(unsigned int)*m_IndexNum[2]));
	//	glDrawElements(GL_TRIANGLES,m_VertNum[3],GL_UNSIGNED_INT,0);
	//	//glDrawArrays(GL_TRIANGLES,0,m_VertNum[3]);
	for(int i=0;i<5;i++){
		m_Colors[i].glColor();	
		glVertexPointer(3,GL_FLOAT,0,&(plydata[i].m_Verts[0].x));
		glIndexPointer(GL_INT,0,&(plydata[i].m_Indices[0]));
		glDrawElements(GL_TRIANGLES,m_IndexNum[i],GL_UNSIGNED_INT,&(plydata[i].m_Indices[0]));
	}
		//glVertexPointer(3,GL_FLOAT,0,(GLubyte*)(sizeof(vec3<float>)*m_VertNum[4]));
		//glIndexPointer(GL_INT,0,(GLubyte*)(sizeof(unsigned int)*m_IndexNum[4]));
		//glDrawElements(GL_TRIANGLES,m_IndexNum[4],GL_UNSIGNED_INT,0);
		//glDrawArrays(GL_TRIANGLES,0,m_VertNum[4]);
	
	glDisableClientState(GL_INDEX_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glDepthMask(GL_TRUE);
	glPopMatrix();
	return true;
}
void CIsoSurface::ToggleMode(bool _showtext){
	mEnlightCore=!mEnlightCore;if(_showtext){cout<<"コアの表示・非表示"<<endl;}
}
