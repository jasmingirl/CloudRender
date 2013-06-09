#include "CloudRender.h"
extern CCloudRender *g_Render;
const float CENTER_ELE=92.7716980f;//レーダー観測地点での高度m
stringstream FILENAME;

/*!
	@brief コンストラクタ glewInitよりも前にやっていい初期化をここでする。PrivateProfileで初期設定読み込むのもここ
*/
CCloudRender::CCloudRender(const vec2<int>& _pos)
	:m_Flags(MY_CLOUD|MY_LAND)
	,m_fTimeRatio(0.0f)
	,m_fWindSpeed(0.02f)
	,m_IniFile("../data/setting.ini")
	,m_ClipDirection(0)//x
	,m_ZScale(1.0)
	,m_SceneId(1)//デフォルトは昼
{	
	//これも本当は設定ファイルからやんなきゃ
	
	
	char str_from_inifile[200];
	GetPrivateProfileString("common","elerange","0.072265625",str_from_inifile,200,m_IniFile.c_str());
	m_fixedZScale=atof(str_from_inifile);
	m_FirstPeriodSum=m_SecondPeriodSum=m_nFrame=0;
	m_RedPoint.set(0.25f,0.0f,0.0f);
	
	GetPrivateProfileSectionNames(str_from_inifile,200,m_IniFile.c_str());
	m_DataKey[0]="iruma";
	//m_DataKey[0]="anjou";//これはhard-codeじゃなくて上の関数でなんとかするべき
	m_DataKey[1]="komaki";
	m_nFileId=GetPrivateProfileInt(m_DataKey[1].c_str(),"frame",88,m_IniFile.c_str());
	srand((unsigned int)time(NULL));//あとで風アニメで使用する
	
	mFontColor.set(1.0,1.0,1.0,1.0);//文字色　背景が白の時は黒・背景が黒の時は白
	mBackGround.set(0.0,0.0,0.0,0.0);
	m_TransForm=new CTransForm();
	
	GetPrivateProfileString("common","dir","../data/",str_from_inifile,200,m_IniFile.c_str());
	m_IsoSurface=new CIsoSurface();

	m_Light=new CLight(GL_LIGHT0);
	m_Land=new CLand();
	m_Measure=new CMeasure();

	
	GetPrivateProfileString(m_DataKey[0].c_str(),"name","失敗",str_from_inifile,200,m_IniFile.c_str());
	m_ToggleText[1]=string(str_from_inifile)+"にする";
	GetPrivateProfileString(m_DataKey[1].c_str(),"name","失敗",str_from_inifile,200,m_IniFile.c_str());
	m_ToggleText[0]=string(str_from_inifile)+"にする";
	
	///黄色い点の初期化
	m_ClippingEquation[0]=-1.0;m_ClippingEquation[1]=0.0;m_ClippingEquation[2]=0.0;m_ClippingEquation[3]=0.7;

	
}
void CCloudRender::VolumeArrayInit(size_t _size){
	m_ucIntensity=new unsigned char[_size];
}
bool CCloudRender::BlendTime(){
	//考えなおすべき
	
	if((m_Flags& MY_VOLDATA)==0)return 0;
	if((m_Flags& MY_FLOW_DAYTIME)==0)return 0;
	if(m_fTimeRatio>1.0f){m_fTimeRatio=1.0f;}
	assert(!"未実装BlendTime");
	int index=0;
	//ファイルによって閾値以上の点の数が違うので
	//x,y,zすべての場所で整理整頓する必要がある。
	//もしぐちゃぐちゃになったら、信号強度0の点を捨てているせい。
	for(int z=0;z<m_VolSize.z;z++){
		for(int y=0;y<m_VolSize.xy;y++){
			for(int x=0;x<m_VolSize.xy;x++){
				index=(z*m_VolSize.xy+y)*m_VolSize.xy+x;
				/*m_Static[index].pos=(mBeforeVoxel[index].pos*(1.0f-m_fTimeRatio))+(mAfterVoxel[index].pos*m_fTimeRatio);
				m_Static[index].normal=(mBeforeVoxel[index].normal*(1.0f-m_fTimeRatio))+(mAfterVoxel[index].normal*m_fTimeRatio);
				m_Static[index].intensity=(mBeforeVoxel[index].intensity*(1.0f-m_fTimeRatio))+(mAfterVoxel[index].intensity*m_fTimeRatio);*/
			}
		}
	}
	m_fTimeRatio+=0.1f;
	return 0;
}
void CCloudRender::ChangeWindSpeed(float _windspeed,vec3<float>* _rawdata,vec3<float>** _renderdata){
	for(int z=0;z<m_Wind.z;z++){
		for(int y=0;y<m_Wind.xy;y++){
			for(int x=0;x<m_Wind.xy;x++){
				int i=(z*m_Wind.xy+y)*m_Wind.xy+x;
				(*_renderdata)[i]=_rawdata[i];

				(*_renderdata)[i]*=_windspeed;//1.0/8.0
			}
		}
	}
}
void CCloudRender::ChangeVolData(){//6分おきのデータにする、ボタンを押すとここに来る。
	m_Flags&=~MY_CLOUD;//処理が終わるまで雲を非表示にしておく
	
	Destroy();//前の雲データの片付け
	LoadWindData(m_IniFile,&m_rawdata);
	LoadVolData();
	glUseProgram(0);
	glUseProgram(mRainbowProgram->m_Program);
	//断面図の伝達関数を変えないといけない。
	char str[200];
	GetPrivateProfileString(m_DataKey[(m_Flags & MY_VOLDATA)>>10].c_str(),"tf","失敗",str,200,m_IniFile.c_str());
	
	mRainbowProgram->Reload3DTexture(&m_ucIntensity[0],m_VolSize.xy,m_VolSize.z,GL_UNSIGNED_BYTE);
	mRainbowProgram->LoadTransFuncFromFile(str);
	glUseProgram(0);
	m_Flags|=MY_CLOUD;//描画ロック解除
}
/*!
	パラメータの変化を検知する。GLFWでKeyBoardを登録している場合は使用しない。
*/
bool CCloudRender::CheckParameterChange(){
	
	static unsigned int lastVolData=m_Flags & MY_VOLDATA;
	if((m_Flags & MY_VOLDATA)!=lastVolData){
		m_Land->ToggleMapKind();
		if(m_Flags & MY_VOLDATA){
			m_Flags &=~MY_FLOW_DAYTIME;
			cout<<"6分おきに変わるデータをロードします"<<endl;
		}
		ChangeVolData();
		lastVolData=(m_Flags & MY_VOLDATA);
		m_IsoSurface->m_VolData=(lastVolData>>10);
		return 0;
	}
	
	
	static unsigned int sLastClipDirection=m_ClipDirection;
	if(m_ClipDirection !=sLastClipDirection){
		switch(m_ClipDirection){
		case 0://x
			m_ClippingEquation[0]=-1.0;m_ClippingEquation[1]=0.0;m_ClippingEquation[2]=0.0;
			break;
		case 1://y
			m_ClippingEquation[0]=0.0;m_ClippingEquation[1]=-1.0;m_ClippingEquation[2]=0.0;

			break;
		case 2://z
			m_ClippingEquation[0]=0.0;m_ClippingEquation[1]=0.0;m_ClippingEquation[2]=-1.0;

			break;
		}
		sLastClipDirection=m_ClipDirection;
		return 0;
	}
	static unsigned int sLastScene=m_SceneId;
	char	str_from_inifile[200];
	string section_str;
	if(m_SceneId !=sLastScene){
		switch(m_SceneId){
		case 0:
			section_str="morning";
			break;
		case 1:
			section_str="noon";
			break;
		case 2:
			section_str="twilight";
			break;
		case 3:
			section_str="evening";
			break;
	}
		GetPrivateProfileString(section_str.c_str(),"diffuse","0.0",str_from_inifile,200,m_IniFile.c_str());
		color<float> diffuse;
		stringstream ss(str_from_inifile);
		ss>>diffuse.r>>diffuse.g>>diffuse.b;
		GetPrivateProfileString(section_str.c_str(),"ambient","0.0",str_from_inifile,200,m_IniFile.c_str());
		color<float> ambient;
		ss.str(str_from_inifile);
		ss.seekg(0,ios::beg);
		ss>>ambient.r>>ambient.g>>ambient.b;
		GetPrivateProfileString(section_str.c_str(),"pos","0.0",str_from_inifile,200,m_IniFile.c_str());
		vec3<float> pos;
		ss.str(str_from_inifile);
		ss.seekg(0,ios::beg);
		ss>>pos.x>>pos.y>>pos.z;
		m_Light->set(ambient,diffuse,pos);
		GetPrivateProfileString(section_str.c_str(),"top","0.0",str_from_inifile,200,m_IniFile.c_str());
		ss.str(str_from_inifile);
		ss.seekg(0,ios::beg);
		int r,g,b;
		ss>>r>>g>>b;
		m_Top.set((unsigned char)r,(unsigned char)g,(unsigned char)b,255);
		GetPrivateProfileString(section_str.c_str(),"middle","0.0",str_from_inifile,200,m_IniFile.c_str());
		ss.str(str_from_inifile);
		ss.seekg(0,ios::beg);
		ss>>r>>g>>b;
		m_Middle.set((unsigned char)r,(unsigned char)g,(unsigned char)b,255);
		GetPrivateProfileString(section_str.c_str(),"bottom","0.0",str_from_inifile,200,m_IniFile.c_str());
		ss.str(str_from_inifile);
		ss.seekg(0,ios::beg);
		ss>>r>>g>>b;
		m_Bottom.set((unsigned char)r,(unsigned char)g,(unsigned char)b,255);
		sLastScene=m_SceneId;
	}
	return 0;
}
void CCloudRender::Destroy(){
	delete[] m_ucIntensity;
	//風関係
	delete[] m_renderdata;
	delete[] m_rawdata;
	m_YellowPoints->clear();
	cout<<"deleteしました"<<endl;
}
/*!
	@brief assimp使ってファイルを読み込み、矢印を描く。　glCallListにしよぉかなぁ、
*/
void CCloudRender::DrawWindVector(){
	//±0.5の座標系に合わせる
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front
	//glColor4f(0.0,0.0,1.0,0.5);
	//矢羽を書く位置
	float xpos=0.5f;
	float ypos=-0.5f;
	int show_mag=2;//2:1個飛ばしで描く　1:フル解像度で描く
	//矢羽同士の間隔
	float increment=1.0f/(float)m_Wind.xy*(float)show_mag;
	float radius=0.1f*increment;//横幅に対して0.1倍程度
	float theta;
	int i=0;
	int colorindex;
	float scale=(float)show_mag/(float)m_Wind.xy;
	//float width;
	//そのまま書けば、勝手に±0.5になるようにjmeshを調整する
	for(int y=0;y<m_Wind.xy;y+=show_mag){
		xpos=-0.5f;
		for(int x=0;x<m_Wind.xy;x+=show_mag){
			i=(y)*m_Wind.xy+x;
			if(m_rawdata[i].zero()){//無風のところは描かない
				xpos+=increment;
				continue;
			}
				colorindex=(int)(m_rawdata[i].length()*255.0);
					glPushMatrix();
					glTranslatef(xpos,ypos,0.0);
					(m_ucWindTF[(int)(colorindex)]).glColor();
					//矢印一個を描く場面
					glPushMatrix();
					glScalef(scale,scale,scale);
					glPushMatrix();
					theta=atan2(m_rawdata[i].y,m_rawdata[i].x)* 180.0f /(float) M_PI;
					glRotatef(theta,0.0f,0.0f,1.0f);
					
					glCallList(m_CallList);
					glPopMatrix();//end scale
				glPopMatrix();//end translate
			glPopMatrix();//end rotate
			xpos+=increment;
			
		}
		ypos+=increment;
	}//end y
	//glPopMatrix();
	glDisable(GL_BLEND);
}
bool CCloudRender::FileLoad(){//ファイルスレッド用関数
	if((m_Flags& MY_VOLDATA)==0){return 0;}
	if((m_Flags& MY_FLOW_DAYTIME)==0)return 0;
		if(m_fTimeRatio>=1.0){
			m_fTimeRatio=0.0;
			PrepareAnimeVol();
			m_fTimeRatio=0.0;
		}
	return 1;
}

/*! ウィンドウサイズが変化した時だけ処理したいのがあるけど、今は毎フレーム処理している状態。
	どうせ毎フレームリサイズを☑してるんだから、これはRunに入れてもいいよね？
	もう二度と、コールバックモードに戻るつもりはないし。
*/
void CCloudRender::Reshape(){
	float m[16];
	glGetFloatv(GL_PROJECTION_MATRIX,m);
	
}
void CCloudRender::LoadWindData(const string _inifilepath,vec3<float>** _winddata){
	char str_from_inifile[200];
	//風の読み込み
	GetPrivateProfileString(m_DataKey[(m_Flags & MY_VOLDATA)>>10].c_str(),"wind"," ",str_from_inifile,200,_inifilepath.c_str());
	m_Wind.Read(str_from_inifile);
	m_renderdata=new vec3<float>[m_Wind.total()];
	*_winddata=new vec3<float>[m_Wind.total()];
	ifstream ifs(str_from_inifile,ios::binary);
	if(!ifs.is_open()){assert(!"ファイル名が間違い");}
	ifs.read((char*)(*_winddata),m_Wind.total()*m_Wind.each_voxel);
	cout<<"read:"<<str_from_inifile<<endl;
	cout<<m_Wind.total()*m_Wind.each_voxel<<"byte読み込み"<<endl;
	//for(int i=0;i<100;i++){(*_winddata)[i].print("[wind]");}
	ifs.close();
	ChangeWindSpeed(m_fWindSpeed,*_winddata,&m_renderdata);//レンダリング用の風データ
	m_YellowPoints=new list<vec3<float>>[YELLOW_PT_NUM];
	for(int i=0;i<YELLOW_PT_NUM;i++){//重ねて隠しておく
		
		//randだけど、風のないところには置きたくない
		vec3<float> pos((float)(rand()%m_Wind.xy),(float)(rand()%m_Wind.xy),(float)(rand()%m_Wind.z));
		int windindex=((int)(pos.z*m_Wind.xy+pos.y)*m_Wind.xy+(int)(pos.x));
		if(m_renderdata[windindex].zero()){
			m_YellowPoints[i].assign(TRAIL_NUM,pos);//意味不明なコードになってしもた。一個も風がないとエラーになるから仕方なく。。。
			continue;}//行き着いた先が無風状態だった場合
		//不運だったが飛ばし。リセットしない。
		pos.x=pos.x/(float)m_Wind.xy-0.5f;
		pos.y=pos.y/(float)m_Wind.xy-0.5f;
		pos.z=pos.z/(float)m_Wind.z;
		m_YellowPoints[i].assign(TRAIL_NUM,pos);//i番目のlistを確保
	}
	//if(m_YellowPoints->empty()){assert(!"空の風データです");}
	// 風専用の伝達関数の読み込み　１回め　伝達関数は、、、何か汎用的なフォーマットにしたいなぁ。
	GetPrivateProfileString("common","windtf","../data/tf/rainbow-256.tf",str_from_inifile,200,_inifilepath.c_str());
	miffy::ReadFileToTheEnd(str_from_inifile,&m_ucWindTF);
	m_CallList=glGenLists(1);
	glNewList(m_CallList,GL_COMPILE);
	//矢印オブジェクトのロード　
	
	GetPrivateProfileString("common","arrow","失敗",str_from_inifile,200,_inifilepath.c_str());
	Ply ply_data;
	ply_data.LoadPlyData(str_from_inifile);

	glBegin(GL_TRIANGLES);
	for(int i=0;i<ply_data.mTriangleNum*3;i++){
		ply_data.m_Normals[ply_data.m_Indices[i]].glNormal();
		ply_data.m_Verts[ply_data.m_Indices[i]].glVertex();
	}
	glEnd();
	glEndList();

}
void CCloudRender::LoadVolData(){
	char str_from_inifile[200];
	//ボリュームデータ読み込み
	GetPrivateProfileString(m_DataKey[(m_Flags & MY_VOLDATA)>>10].c_str(),"file","失敗",str_from_inifile,200,m_IniFile.c_str());
	if(m_Max3DTexSize<256){//ロースペックマシン対策
		GetPrivateProfileString(m_DataKey[0].c_str(),"lowfile","失敗",str_from_inifile,200,m_IniFile.c_str());
	}
	m_VolSize.Read(str_from_inifile);//ファイル名からサイズを解析
	//これからバイナリ部を読むぞ
	ifstream ifs(str_from_inifile,ios::binary);
	m_ucIntensity=new unsigned char[m_VolSize.total()];
	VolumeArrayInit(m_VolSize.total());	
	ifs.read((char*)m_ucIntensity,m_VolSize.total()*m_VolSize.each_voxel);
	memcpy(m_ucIntensity,m_ucIntensity,m_VolSize.total());
	ifs.close();
	m_dataMax=(unsigned char)GetPrivateProfileInt(m_DataKey[0].c_str(),"datamax",256,m_IniFile.c_str());
	
	if(m_Flags& MY_VOLDATA){//6分おきに変わるデータの場合。
		m_IsoSurface->Reload(m_nFileId);
		
	}
	
	
}
/*!
	@brief glewInit()した後にやりたい初期化処理
*/
void CCloudRender::Init(){
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE,&m_Max3DTexSize);
	cout<<"３次元テクスチャの最大サイズ"<<m_Max3DTexSize<<endl;
	
	// 点に貼り付ける、丸いぼんやりテクスチャの設定
	glEnable(GL_POINT_SPRITE);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
	glTexEnvf(GL_POINT_SPRITE,GL_COORD_REPLACE,GL_TRUE);
	
	glActiveTexture(GL_TEXTURE0);
	//なんか、コイツのせいでボリュームデータが見えなくなっちゃった。
	if(!tga::Load_Texture_8bit_Alpha("../data/texture/gaussian64.tga",&m_GausTexSamp)){assert(!"ガウシアンテクスチャが見つかりません！");}
	LoadWindData(m_IniFile,&m_rawdata);//風関連データのロード
	LoadVolData();//ボリュームデータのロード
	mProgram = new CGLSLRayCasting("raycasting.vert","raycasting.frag");GetGLError("glTexImage3D");
	mProgram->Add3DTexture(&m_ucIntensity[0],m_VolSize.xy,m_VolSize.z,GL_UNSIGNED_BYTE);
	mProgram->UpdateTextureUniform();GetGLError("glTexImage3D");
	
	
	mProgram->UpdateThresholdUniform((float)GetPrivateProfileInt(m_DataKey[0].c_str(),"threshold",64,m_IniFile.c_str())/255.0);
	m_IsoSurface->Init(m_IniFile.c_str());
	glUseProgram(0);
	//虹色モード用のシェーダ　
	mRainbowProgram = new CGLSLRainbow("rainbow.vert","rainbow.frag");
	///@caution 3Dの後1Dでなければならない。
	mRainbowProgram->Add3DTexture(&m_ucIntensity[0],m_VolSize.xy,m_VolSize.z,GL_UNSIGNED_BYTE);//断面図を描くのに使うテクスチャ
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	
	char str[200];
	GetPrivateProfileString(m_DataKey[0].c_str(),"tf","失敗",str,200,m_IniFile.c_str());
	mRainbowProgram->InitTransFuncFromFile(str);	
	mRainbowProgram->UpdateTextureUniform();

	//ボリュームデータを地面に投影しつつ虹色に彩色する。投影図を画像ファイルにしちゃおっかな？　そうすると、閾値を動的に変えられないなぁ。。
	color<float> *projectedRainbowVol;
	GenerateProjectedRainbow<unsigned char>(m_ucIntensity,m_VolSize.xy*m_VolSize.xy,m_VolSize.z,m_dataMax,mProgram->m_threshold,projectedRainbowVol);
	
	m_Land->Init(projectedRainbowVol,m_VolSize.xy,m_IniFile,m_DataKey[0]);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
 
	glTexEnvf(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
 
	glTexEnvf(GL_TEXTURE_ENV,GL_SRC0_RGB,GL_PRIMARY_COLOR);
	glTexEnvf(GL_TEXTURE_ENV,GL_SRC1_RGB,GL_PRIMARY_COLOR);//glColor
 
	glTexEnvf(GL_TEXTURE_ENV,GL_SRC0_ALPHA,GL_TEXTURE);
	glTexEnvf(GL_TEXTURE_ENV,GL_SRC1_ALPHA,GL_PRIMARY_COLOR);
	string section_str="noon";
	char str_from_inifile[200];
	GetPrivateProfileString(section_str.c_str(),"diffuse","0.0",str_from_inifile,200,m_IniFile.c_str());
	color<float> diffuse;
	stringstream ss(str_from_inifile);
	ss>>diffuse.r>>diffuse.g>>diffuse.b;
	GetPrivateProfileString(section_str.c_str(),"ambient","0.0",str_from_inifile,200,m_IniFile.c_str());
	color<float> ambient;
	ss.str(str_from_inifile);
	ss.seekg(0,ios::beg);
	ss>>ambient.r>>ambient.g>>ambient.b;
	GetPrivateProfileString(section_str.c_str(),"pos","0.0",str_from_inifile,200,m_IniFile.c_str());
	vec3<float> pos;
	ss.str(str_from_inifile);
	ss.seekg(0,ios::beg);
	ss>>pos.x>>pos.y>>pos.z;
	m_Light->set(ambient,diffuse,pos);
	GetPrivateProfileString(section_str.c_str(),"top","0.0",str_from_inifile,200,m_IniFile.c_str());
	ss.str(str_from_inifile);
	ss.seekg(0,ios::beg);
	int r,g,b;
	ss>>r>>g>>b;
	m_Top.set((unsigned char)r,(unsigned char)g,(unsigned char)b,255);
	GetPrivateProfileString(section_str.c_str(),"middle","0.0",str_from_inifile,200,m_IniFile.c_str());
	ss.str(str_from_inifile);
	ss.seekg(0,ios::beg);
	ss>>r>>g>>b;
	m_Middle.set((unsigned char)r,(unsigned char)g,(unsigned char)b,255);
	GetPrivateProfileString(section_str.c_str(),"bottom","0.0",str_from_inifile,200,m_IniFile.c_str());
	ss.str(str_from_inifile);
	ss.seekg(0,ios::beg);
	ss>>r>>g>>b;
	m_Bottom.set((unsigned char)r,(unsigned char)g,(unsigned char)b,255);
	}

/*!
	@brief メインのdisplay関数　毎フレーム呼び出されるやつの親玉。
	等値面と普段のボリュームは相反する関係だから、そこのところうまくやりたいかも。<br>
	でも、等値面もボリュームも両方隠したい時もある。。。<br>
	何を表示して、何を表示しないのかフラグは引数がいいのか、メンバ変数がいいのか迷う。<br>
	内部的にフラグをいじることもあるからやっぱりフラグはメンバ変数かなぁ

*/
bool CCloudRender::Draw(){
								
	Reshape();
	m_TransForm->Enable(m_Top,m_Middle,m_Bottom);
	glPushMatrix();
	glScalef(1.0,1.0,m_ZScale);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);//シェーダの中で点のサイズをいじるのを可能にしてくれる。
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front
	glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glSampleCoverage(GL_SAMPLE_ALPHA_TO_COVERAGE,GL_TRUE);
	glSampleCoverage(GL_SAMPLE_ALPHA_TO_ONE,GL_TRUE);
	glEnable(GL_MULTISAMPLE);
	glClipPlane(GL_CLIP_PLANE0,m_ClippingEquation);
	
		m_Land->SetClipPlane(m_ClippingEquation);/// @note　本来地図はクリップ情報は要らないけど、断面図を投影する関係で要ることになってしまった。
		//脇役達 等値面、地形、ライト、目盛り
		if(m_Flags & MY_LAND){
			m_Light->Enable();//そのうちライトは複数のオブジェクトに当てる予定だからlandの引数にはしない
			m_Land->Run();
			m_Light->Disable();
		}
		
		if(m_Flags & MY_ISOSURFACE){m_Flags&=~MY_CLOUD;m_IsoSurface->Run();}
		
		float m[16];
		glGetFloatv(GL_MODELVIEW_MATRIX,m);
		//主役の雲を一番手前に
		//cout<<"m_bCurrentVolData"<<m_bCurrentVolData<<endl;
		
		if(m_ClippingEquation[2]==0.0){
			m_Measure->CalcClipPlane(m_ClippingEquation);
		}else{//z軸の場合の特別処理
			double clip_eqn[4];
			memcpy(clip_eqn,m_ClippingEquation,sizeof(double)*4);
			//clip_eqn[2]=-clip_eqn[2];//なんかしらないけど、zだけ見えてほしい面が逆だったので、こうした。
			//一時的な打開策。これをちゃんとしたいなら、全体の座標系を大改革する必要があるかも。
			//たぶん、そもそもglulookAtの時点で何かおかしいことをしてるのよね。
			//clip_eqn[3]+=0.5;
			m_Measure->CalcClipPlane(clip_eqn);
		}
		glPushMatrix();
		glScalef(1.0,1.0,m_fixedZScale);
		if(m_Flags & MY_MEASURE){m_Measure->Draw();}
	
		if(m_Measure->m_IsCipVisible){//断面の描画
			glPushAttrib(GL_ENABLE_BIT);
			glEnable(GL_CULL_FACE);glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GREATER,0.5);
				glUseProgram(mRainbowProgram->m_Program);
				
					if(m_Flags&MY_ANIMATE){
						mRainbowProgram->Reload3DTexture(&m_ucIntensity[0],m_VolSize.xy,m_VolSize.z,GL_UNSIGNED_BYTE);
					}
						glBegin(GL_POLYGON);
						for(int i = 5; i >= 0; i--) {
							m_Measure->m_intersection[i].glVertex();
						}
						glEnd();
				
			glUseProgram(0);
			glPopAttrib();//CULL_FACEとALPHA_TEST
		}//end of 断面の描画
		//ここよりあと
		glPushAttrib(GL_ENABLE_BIT);
		glEnable(GL_POINT_SPRITE);//これでgl_PointCoordが有効になる これで見た目がかなり変わる。
		if(m_Flags & MY_CLOUD){
			//等値面がいるときも断面を出したい、という都合上ここに配置
			glPushAttrib(GL_ENABLE_BIT);
			glEnable(GL_CLIP_PLANE0);
			glUseProgram(mProgram->m_Program);
				glUniform1f(mProgram->m_Map["threshold"],mProgram->m_threshold);
					m_Light->Enable();//
						
								glActiveTexture ( GL_TEXTURE0 );
								glEnable ( GL_TEXTURE_3D );
								glBindTexture ( GL_TEXTURE_3D,mProgram->m_TexId );
									glPushMatrix();
									glTranslatef(0.0,0.0,0.5);
									glutSolidCube(1.0);
									glPopMatrix();

				glPopAttrib();//light off
			glUseProgram(0);
			
		}
		glPopAttrib();//GL_POINT_SPRITE
		if(m_Flags & MY_RED_POINT){
			glPushAttrib(GL_POINT_BIT);
				static GLfloat atten[] = { 0.0, 0.0, 1.0 };
				glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION,atten);
				glDepthMask(GL_FALSE);
				//glEnable(GL_BLEND);
				//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front

				glActiveTexture ( GL_TEXTURE0 );
				glEnable ( GL_TEXTURE_2D );
				glBindTexture ( GL_TEXTURE_2D,m_GausTexSamp );

				//赤い点のレンダリング
				glColor3f(1.0,0.0,0.0);
				glPointSize(20);
				glBegin(GL_POINTS);
				m_RedPoint.glVertex();
				glEnd();
				glDisable(GL_TEXTURE_2D);
				//glDisable(GL_BLEND);
				glDepthMask(GL_TRUE);
				if(m_Flags & MY_ANIMATE){
					bool is_need_reset=BlowVoxel(&m_RedPoint);//赤い点も移動させる
					if(is_need_reset){m_RedPoint.set(0.25,0,0);}
				}
			glPopAttrib();
		}
		if(m_Flags & MY_YELLOW_POINTS){
			glPushAttrib(GL_POINT_BIT);
				static GLfloat atten[] = { 0.0, 0.0, 0.8 };
				glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION,atten);
				glDepthMask(GL_FALSE);
				glActiveTexture ( GL_TEXTURE0 );
				glEnable ( GL_TEXTURE_2D );
				glBindTexture ( GL_TEXTURE_2D,m_GausTexSamp );
				//風を表すサンプル点群のレンダリング
				glPointSize(20);
				glBegin(GL_POINTS);
				glColor3f(1.0,1.0,0.0);
				for(int i=0;i<YELLOW_PT_NUM;i++){
					m_YellowPoints[i].front().glVertex();
				}
				glEnd();
				glDisable(GL_TEXTURE_2D);
				//あとは線で
			
				for(int i=0;i<YELLOW_PT_NUM;i++){
					glBegin(GL_LINE_STRIP);
					list<vec3<float>>::iterator it=m_YellowPoints[i].begin();
					while(it!=m_YellowPoints[i].end()){
						it->glVertex();
						it++;
					}
					glEnd();
					
				}
				glDepthMask(GL_TRUE);
			glPopAttrib();//gl_EnablePointSpriteの無効化

			//黄色い点のアニメーション
			if((m_Flags & MY_ANIMATE) | (m_Flags & MY_ANIMATE_YELLOW)){
				for(int i=0;i<YELLOW_PT_NUM;i++){
					bool is_need_reset=BlowVoxelWithTrail(&m_YellowPoints[i]);
					if(is_need_reset){//resetしたい場合
						//randだけど、風のないところには置きたくない
						vec3<float> pos((float)(rand()%m_Wind.xy),(float)(rand()%m_Wind.xy),(float)(rand()%m_Wind.z));
						int windindex=((int)(pos.z*m_Wind.xy+pos.y)*m_Wind.xy+(int)(pos.x));
						if(m_renderdata[windindex].zero()){continue;}//行き着いた先が無風状態だった場合　不運だったが飛ばし。リセットしない。
						pos.x=pos.x/(float)m_Wind.xy-0.5f;
						pos.y=pos.y/(float)m_Wind.xy-0.5f;
						pos.z=pos.z/(float)m_Wind.z;
						fill(m_YellowPoints[i].begin(),m_YellowPoints[i].end(),pos);//trailだからfillする　すべて同じ値で初期化。
					}
				}
			}
		}//MY_YELLOW_POINTS
		glPopMatrix();//pop fixedZScale
		
		//光源の描画

		glPopMatrix();//zScaleを無効化する。
		glPushAttrib(GL_ENABLE_BIT);
		glActiveTexture ( GL_TEXTURE0 );
		glEnable ( GL_TEXTURE_2D );
		glBindTexture ( GL_TEXTURE_2D,m_GausTexSamp );
		glPointSize(400);
		glColor4ub(245,239,207,255);
		glBegin(GL_POINTS);
		m_Light->m_pos.glVertex();
		glEnd();
		glPopAttrib();
		if(m_Flags & MY_VECTOR ){DrawWindVector();}
		BlendTime();//0～6分後のブレンドする
		m_TransForm->Disable();
		if(m_Flags & MY_CAPTURE){
			Sleep(1000);
		}
		
		return 0;
}

void CCloudRender::JoyStick(){
	unsigned char buttons[8];
	float pos[2];
	//状態の取得
	glfwGetJoystickButtons ( GLFW_JOYSTICK_1,buttons,8 );
	glfwGetJoystickPos (GLFW_JOYSTICK_1,pos,2);
	if(buttons[glfw::START]){ //飛行モードスタート
		m_Flags|=MY_FLYING;
	}
	if(buttons[glfw::SELECT]){
		
		//move to flight start point
		//クリッピング面を見えなくする
		//mClipPlane=0.0;mClipEqn[3]=mClipPlane; glClipPlane(GL_CLIP_PLANE0,mClipEqn);
		//front viewにする
		//m_TransForm->ChangeView(GLUT_KEY_DOWN);
		//飛行スタート地点に平行移動
		//SetTranslateVec(0.0,0.06,-1.35);

		m_Flags|=MY_FLYING;
		printf("スタート地点へ\n");
		m_Flags|=MY_ANIMATE;//風アニメ有効化
		}
	if(buttons[glfw::L]){//L
		//mRad+=M_PI/1800.0;
	}
	if(buttons[glfw::R]){//R
		//mRad-=M_PI/1800.0;
	}
	float zincrement=0.0;
	if(m_Flags & MY_FLYING){
		zincrement=-0.003f;
	}
	//JoyStickの入力にしたがって平湖移動する
	//IncrementTranslateVec(pos[0],pos[1],zincrement);
	

}
/*!
	@brief ファイルスレッドで流れる
*/
void CCloudRender::PrepareAnimeVol(){
	static stringstream filename;
	filename.str("");
	filename<<"../data/cappi/cappi"<<m_nFileId<<"_200_z17_1byte.raw";
	
	static HANDLE handle;
	static DWORD dwnumread;
	handle = CreateFile(filename.str().c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	ReadFile(handle,m_ucIntensity,sizeof(unsigned char)*m_VolSize.total(),&dwnumread,NULL);
	CloseHandle(handle);
	
	m_nFileId++;
	cout<<m_nFileId<<"フレーム目"<<endl;	
	if(m_nFileId>163){m_nFileId=24;}
	
	//nFrame++した後のを読み込む(次フレームの準備)
	filename.str("");
	filename<<"../data/cappi/cappi"<<m_nFileId<<"_200_z17_1byte.raw";
	handle = CreateFile(filename.str().c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	ReadFile(handle,m_ucIntensity,sizeof(unsigned char)*m_VolSize.total(),&dwnumread,NULL);
	CloseHandle(handle);

	//6分おきのデータは各風データを持っている
	//LoadVelocityData();
	filename.str("");
	filename<<"../data/vvp/vvp"<<m_nFileId<<"_40_z1_12byte.raw";
	//cout<<"風データ"<<filename.str()<<endl;
	handle = CreateFile(filename.str().c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	ReadFile(handle,&m_rawdata[0].x,sizeof(vec3<float>)*m_Wind.total(),&dwnumread,NULL);
	CloseHandle(handle);
	ChangeWindSpeed(m_fWindSpeed,m_rawdata,&m_renderdata);
	//等値面も読まなきゃ
	if(m_Flags& MY_ISOSURFACE){
		m_IsoSurface->Reload(m_nFileId);
	}
}
void CCloudRender::SetFlag(unsigned int _flags){
	m_Flags=_flags;
}

CCloudRender::~CCloudRender(void){
	printf("総フレーム数%d\n",m_nFrame);
	printf("前半の時間平均%fmsec,後半の時間平均%fmsec\n",(double)m_FirstPeriodSum/(double)m_nFrame,(double)m_SecondPeriodSum/(double)m_nFrame);

	Destroy();
	delete[] m_ucWindTF;
	stringstream str;
	str<<(int)((mProgram->m_threshold)*255.0);
	WritePrivateProfileString("anjou","threshold",str.str().c_str(),m_IniFile.c_str());
	
}
