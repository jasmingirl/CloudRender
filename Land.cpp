#include "Land.h"


CLand::CLand()
  :m_bWhichData(false)
{
}
/*!
	@param _radarVolumeData 地面に投影する予定の雲のボリュームテクスチャ を渡す。floatでRGBAで既に虹色に変換されたデータにすること
	@param _size ボリュームデータのサイズ
*/
void CLand::Init(color<float>* _radarVolumeData,int _size){
	m_List[0]=glGenLists(2);
	//glGenBuffers(2, m_IdBufferId);
	//glGenBuffers(2, m_BufferId);
	InitData("../data/map/iruma.jmesh",0);
	InitData("../data/map/komaki.jmesh",1);
	
	
	glActiveTexture(GL_TEXTURE1);

	glGenTextures ( 1, &m_ProjectedSamp[0] );
	glBindTexture (GL_TEXTURE_2D, m_ProjectedSamp[0]);
	glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, _size, _size, 0, GL_RGBA, GL_FLOAT, _radarVolumeData);
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	
   glActiveTexture(GL_TEXTURE1);
   glGenTextures ( 1, &m_ProjectedSamp[1] );
   glBindTexture (GL_TEXTURE_2D, m_ProjectedSamp[1]);
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, _size, _size, 0, GL_RGBA, GL_FLOAT, _radarVolumeData);
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
   glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glActiveTexture(GL_TEXTURE0);
 
   cout<<"地図バッファ情報"<<endl;
   mProgram=new CGLSLClipProjected("land.vert","land.frag");
   mProgram->UpdateTextureUniform();
}
void CLand::InitCamera(float *_m){
	glUseProgram(mProgram->m_Program);
	mProgram->UpdateProjectionMat(_m);miffy::GetGLError("InitCamera");
	glUseProgram(0);
}
void CLand::Info(){
	cout<<"地図データです"<<endl;
}
void CLand::InitData(const string& _filepath,int _id){
	//テクスチャのパスは同じフォルダの同じ名前.bmpと決まっている
	char drive[200];
    char dir[200];
    char fname[200];
    char ext[200]; 
	_splitpath(_filepath.c_str(),drive,dir,fname,ext);
	
	
	glActiveTexture(GL_TEXTURE0);
	stringstream texpath;
	texpath<<drive<<dir<<fname<<".tga";
	tga::Load_Texture_RGB(texpath.str().c_str(),&m_TexId[_id] );
	JMeshHeader jmeshhead;
	J3Header j3head;
	HANDLE handle;
	DWORD dwnumread;
	
	handle = CreateFile(_filepath.c_str(),GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
	if(handle==INVALID_HANDLE_VALUE){
		cout<<_filepath<<endl;
		assert(!"ファイルがみつかりません");
	}
	ReadFile(handle,&jmeshhead.FileType,sizeof(JMeshHeader),&dwnumread ,NULL);
	ReadFile(handle,&j3head.xNum,sizeof(J3Header),&dwnumread ,NULL);
	
	int MAPVERTS=j3head.xNum*j3head.yNum*j3head.zNum;
	int MESHNUM;
	float TEXMAG;
	MESHNUM=(int)(sqrt((double)MAPVERTS));
	//_idが1の時にだめになる。なぜだ？？
	vec3<float>* temp=new vec3<float>[MAPVERTS];
	sVerts *m_Verts=new sVerts[MAPVERTS];
	vector<unsigned int> m_Index;
	ReadFile(handle,&temp[0].x,MAPVERTS*sizeof(vec3<float>),&dwnumread ,NULL);
	CloseHandle(handle);
	TEXMAG=1.0f/(float)MESHNUM;
	
	for(int i=0;i<MAPVERTS;i++){m_Verts[i].pos=temp[i];}
	tri<float> tri;
	for(int y=0;y<MESHNUM-2;y++){
		//往復の往路
		for(int x=0;x<MESHNUM-1;x++){
				tri.set(m_Verts[y*MESHNUM+x].pos,m_Verts[(y)*MESHNUM+(x+1)].pos,m_Verts[(y+1)*MESHNUM+x].pos);
				
				m_Verts[y*MESHNUM+x].normal=tri.CalcNormalVector();
				m_Verts[(y+1)*MESHNUM+x].normal=tri.CalcNormalVector();
				m_Verts[y*MESHNUM+x].texcoord=vec2<float>((float)x*TEXMAG,(float)y*TEXMAG);
				m_Verts[(y+1)*MESHNUM+x].texcoord=vec2<float>((float)x*TEXMAG,(float)(y+1)*TEXMAG);
				m_Index.push_back((y+1)*MESHNUM+x);
				m_Index.push_back(y*MESHNUM+x);
		}//end of x
		y++;
		//復路
		for(int x=MESHNUM-1;x>0;x--){
				tri.set(m_Verts[y*MESHNUM+x].pos,m_Verts[(y)*MESHNUM+(x-1)].pos,m_Verts[(y+1)*MESHNUM+x].pos);
				m_Verts[y*MESHNUM+x].normal=tri.CalcNormalVector();
				m_Verts[(y+1)*MESHNUM+x].normal=tri.CalcNormalVector();
				
				m_Verts[y*MESHNUM+x].texcoord=vec2<float>((float)x*TEXMAG,(float)y*TEXMAG);
				m_Verts[(y+1)*MESHNUM+x].texcoord=vec2<float>((float)x*TEXMAG,(float)(y+1)*TEXMAG);
				
				m_Index.push_back(y*MESHNUM+x);
				m_Index.push_back((y+1)*MESHNUM+x);
			
		}//end of x
		
	}//end of y
	delete temp;
	/*//VBOに登録
	glBindBuffer(GL_ARRAY_BUFFER, m_BufferId[_id]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sVerts)*MAPVERTS,&m_Verts[0].texcoord.x,GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IdBufferId[_id]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*m_Index.size(),&m_Index[0],GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER,0);
	
	m_IndexNum[_id]=m_Index.size();//あとでDrawElementsで使う*/
	glNewList(m_List[_id],GL_COMPILE);
	glBegin(GL_TRIANGLE_STRIP);
	for(int i=0;i<m_Index.size();i++){
		m_Verts[m_Index[i]].texcoord.glTexCoord();
		m_Verts[m_Index[i]].normal.glNormal();
		m_Verts[m_Index[i]].pos.glVertex();
	}
	glEnd();
	glEndList();
	delete[] m_Verts;
//	delete m_Index[_id];
}
bool CLand::Run(){
	miffy::GetGLError("CLand::Run(");
	glCullFace(GL_BACK);//
	static float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX,m);
	glUseProgram(mProgram->m_Program);
	miffy::GetGLError("before UpdateModelViewMat");
	mProgram->UpdateModelViewMat(m);
	static double d_eqn[4];
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT,GL_AMBIENT);//diffuseをglColor3fで渡すことにする。
	glActiveTexture ( GL_TEXTURE0 );//どこかでActiveTextureを0以外にするいたずらをされたっぽい。
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,m_TexId[m_bWhichData]);
	glActiveTexture ( GL_TEXTURE1 );
	glBindTexture(GL_TEXTURE_2D,m_ProjectedSamp[m_bWhichData]);
	miffy::GetGLError("glBindTexture");

	glCallList(m_List[m_bWhichData]);

	//glBindBuffer(GL_ARRAY_BUFFER, m_BufferId[m_bWhichData]);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IdBufferId[m_bWhichData]);GetGLError("wrong?");
	
	//glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
	//
	//glEnableClientState(GL_VERTEX_ARRAY);
	//glEnableClientState(GL_NORMAL_ARRAY);
 //   glEnableClientState(GL_TEXTURE_COORD_ARRAY);GetGLError("wrong");
	//glLineWidth(2);
	//
	//glDrawElements(GL_TRIANGLE_STRIP,m_IndexNum[m_bWhichData],GL_UNSIGNED_INT,0);//6分おきの雲にする、にするとここがエラー
	//
	//glDisableClientState(GL_VERTEX_ARRAY); 
	//glDisableClientState(GL_NORMAL_ARRAY);
 //   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
 //   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
 
	glBindTexture(GL_TEXTURE_2D,0);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture ( GL_TEXTURE0 );
	glDisable(GL_TEXTURE_2D);
	glUseProgram(0);
	return 1;
}
/*!
	LandとCLipの癒着関係をどうにかしたい。が、地面に投影する以上は仕方がない。
	こいつのおかげで、CLandはCTransformを持たなくて済む、とも言える。
*/
void CLand::SetClipPlane(double _clip_eqn[4]){

	glUseProgram(mProgram->m_Program);
	glUniform4f(mProgram->m_Map["clip_eqn"],(float)_clip_eqn[0],(float)_clip_eqn[1],(float)_clip_eqn[2],(float)_clip_eqn[3]);
	glUseProgram(0);
}

void  CLand::ToggleMapKind(){m_bWhichData=!m_bWhichData;}
CLand::~CLand(void)
{
//	glDeleteBuffers(2, m_BufferId);
	glDeleteLists(m_List[0],2);
}
