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
		m_IndexNum[i]=plydata[i].mFacenum;
	}	
		
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

	for(int i=0;i<5;i++){
		m_Colors[i].glColor();	
		glVertexPointer(3,GL_FLOAT,0,&(plydata[i].m_Verts[0].x));
		glIndexPointer(GL_INT,0,&(plydata[i].m_Indices[0]));
		glDrawElements(GL_TRIANGLES,m_IndexNum[i],GL_UNSIGNED_INT,&(plydata[i].m_Indices[0]));
	}
	
	glDisableClientState(GL_INDEX_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glDepthMask(GL_TRUE);
	glPopMatrix();
	return true;
}
void CIsoSurface::ToggleMode(bool _showtext){
	mEnlightCore=!mEnlightCore;if(_showtext){cout<<"コアの表示・非表示"<<endl;}
}
