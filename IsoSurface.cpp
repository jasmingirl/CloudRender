#include "IsoSurface.h"
CIsoSurface::CIsoSurface()
{
	m_VolData=false;
	m_Back=false;
	m_NowRendering=false;
	
	m_Colors[0].set(85,42,255,51);//青色
	m_Colors[1].set(73,241,249,51);//水色
	m_Colors[2].set(224,250,254,51);//
	m_Colors[3].set(255,249,207,51);//黄色
	m_Colors[4].set(255,2,83,51);//赤
	
}

CIsoSurface::~CIsoSurface(void)
{
}
void CIsoSurface::Init(const char* _inifilepath){

	char str_from_inifile[200];
	
	string names[]={"iso0","iso1","iso2","iso3","iso4"};
	
	for(int i=4;i>=0;i--){
		GetPrivateProfileString("iruma",names[i].c_str(),"失敗",str_from_inifile,200,_inifilepath);
		
		if(!m_PlyData[i].LoadPlyData(str_from_inifile)){
			OutputDebugString("ロード失敗\n");
			assert(!"ロード失敗");
		}
		m_VertNum[i]=m_PlyData[i].mVertNum;
		m_IndexNum[i]=m_PlyData[i].mTriangleNum*3;
	}	
	for(int i=0;i<3;i++){
		m_BackPlyData[i]=new Ply(m_PlyData[i*2]);
	}
}
void CIsoSurface::Reload(int _fileId){

	for(int i=0;i<3;i++){
		delete m_BackPlyData[i];
		m_BackPlyData[i]=new Ply(m_PlyData[i*2]);
		//cout<<"頂点の数="<<m_VertNum[i]<<", インデックスの数="<<m_IndexNum[i]<<endl;
	}
	while(m_NowRendering){}//待つ
	m_Back=true;
	cout<<"裏データon"<<endl;
	//これやるとたぶん、６分おきから→特機のときバグるなぁ。
	for(int i=0;i<5;i+=2){m_PlyData[i].DeleteArrays();}
	string names[]={"iso0","iso1","iso2","iso3","iso4"};
	stringstream filepath;

	for(int i=0;i<5;i+=2){
		filepath.str("");
		filepath<<"../data/"<<names[i]<<"/ply/cappi"<<_fileId<<".ply";
		cout<<filepath.str()<<endl;
		if(!m_PlyData[i].LoadPlyData(filepath.str().c_str())){
			OutputDebugString("ロード失敗\n");
			assert(!"ロード失敗");
		}
		//VBOにしようと思って、ここに入れた
		m_VertNum[i]=m_PlyData[i].mVertNum;//これって今は使ってない。
		m_IndexNum[i]=m_PlyData[i].mTriangleNum*3;
		//cout<<"頂点の数="<<m_VertNum[i]<<", インデックスの数="<<m_IndexNum[i]<<endl;
	}
	m_Back=false;
	cout<<"裏データoff"<<endl;
}

bool CIsoSurface::Run(){//ここでエラーかな。
	
	glPushMatrix();
//	glTrans
	glDisable(GL_CULL_FACE);
	
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	//なぜか謎のクリッピング面が出現して困っている。
	glPointSize(8);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_INDEX_ARRAY);
	if(m_VolData){
		//cout<<"back data?"<<m_Back<<endl;
		if(m_Back){//代役登場
			for(int i=0;i<3;i++){//6分おきのやつは等値面３個しかないので偶数にだけ入れた。
				m_Colors[i*2].glColor();	
				//glVertexPointer(3,GL_FLOAT,0,&(m_PlyData[i].m_Verts[0].x));
				//glIndexPointer(GL_INT,0,&(m_PlyData[i].m_Indices[0]));
				//glDrawElements(GL_TRIANGLES,m_IndexNum[i],GL_UNSIGNED_INT,&(m_PlyData[i].m_Indices[0]));
				glBegin(GL_TRIANGLES);//なぜか頂点配列じゃ書けなくなってしまった。
				for(int j=0;j<m_BackPlyData[i]->mTriangleNum*3;j++){//読んでいる最中に書こうとしている
					m_BackPlyData[i]->m_Verts[m_BackPlyData[i]->m_Indices[j]].glVertex();
				}
				glEnd();
			}
		}else{
			m_NowRendering=true;
			for(int i=0;i<5;i+=2){//6分おきのやつは等値面３個しかないので偶数にだけ入れた。
				m_Colors[i].glColor();	
				//glVertexPointer(3,GL_FLOAT,0,&(m_PlyData[i].m_Verts[0].x));
				//glIndexPointer(GL_INT,0,&(m_PlyData[i].m_Indices[0]));
				//glDrawElements(GL_TRIANGLES,m_IndexNum[i],GL_UNSIGNED_INT,&(m_PlyData[i].m_Indices[0]));
				glBegin(GL_TRIANGLES);//なぜか頂点配列じゃ書けなくなってしまった。
				for(int j=0;j<m_IndexNum[i];j++){//読んでいる最中に書こうとしている
					m_PlyData[i].m_Verts[m_PlyData[i].m_Indices[j]].glVertex();
				}
				glEnd();
			}
			m_NowRendering=false;
		}
	}else{
		for(int i=0;i<5;i++){
			m_Colors[i].glColor();	
			glVertexPointer(3,GL_FLOAT,0,&(m_PlyData[i].m_Verts[0].x));
			glIndexPointer(GL_INT,0,&(m_PlyData[i].m_Indices[0]));
			glDrawElements(GL_TRIANGLES,m_IndexNum[i],GL_UNSIGNED_INT,&(m_PlyData[i].m_Indices[0]));
		}
	}

	glDisableClientState(GL_INDEX_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glDepthMask(GL_TRUE);
	glPopMatrix();
	return true;
}
