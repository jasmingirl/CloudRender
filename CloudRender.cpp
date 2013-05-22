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
	,m_Karmapara(0.01f)//生まれ変わりやすを支配するパラメータ あとでロシアンルーレット形式風アニメにも使おうかな	
	,m_IniFile("../data/setting.ini")
	,m_ClipDirection(0)//x
	,m_ZScale(1.0)
{	
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

	m_Light=new CLight(color<float>(185.0f/255.0f,194.0f/255.0f,137.0f/255.0f,1.0f),
		color<float>(1.0f,254.0f/255.0f,229.0f/255.0f,1.0f),
		vec3<float>(0.0f,1.0f,0.0f),GL_LIGHT0);
	m_Land=new CLand();
	m_Measure=new CMeasure();

	
	GetPrivateProfileString(m_DataKey[0].c_str(),"name","失敗",str_from_inifile,200,m_IniFile.c_str());
	m_ToggleText[1]=string(str_from_inifile)+"にする";
	GetPrivateProfileString(m_DataKey[1].c_str(),"name","失敗",str_from_inifile,200,m_IniFile.c_str());
	m_ToggleText[0]=string(str_from_inifile)+"にする";
	m_threshold=(unsigned char)GetPrivateProfileInt(m_DataKey[0].c_str(),"threshold",64,m_IniFile.c_str());
	cout<<"初期設定閾値:"<<(int)m_threshold<<" 最大値"<<(int)m_dataMax<<endl;
	///黄色い点の初期化
	m_ClippingEquation[0]=-1.0;m_ClippingEquation[1]=0.0;m_ClippingEquation[2]=0.0;m_ClippingEquation[3]=0.5;

	
}
bool CCloudRender::AnimateRandomFadeOut(){
		m_nFrame++;
		//箱やしきい値に関係なくランダムにフェードアウト
		//ここは速さが大事。
		//位置の更新
		//zの範囲も0.0-1.0*ELERANGEと考えてよいのかな。
		
		//演算子のオペレータの呼び出しは遅くなる
		//インライン化しても遅い
		//static unsigned char* previous_intensity=new unsigned char[m_VolSize.total()];//断面図を動的に変化させるために使う。
		
		//if(m_bSliceVisible){//最初のスライスを記憶
		//	memcpy(previous_intensity,m_ucIntensity,sizeof(unsigned char)*m_VolSize.total());
		//}
		for(int i=0;i<m_ValidPtNum;i++){
			//無作為に生まれ変わるm_Karmapara
			if(m_RandomTable[(m_nFrame+i)%m_ValidPtNum]){ResetVoxelPosition(i);continue;}//10msec
			unsigned long system_time_first= timeGetTime();
			bool is_need_reset=BlowVoxel(&m_Dynamic[i]);//136msec
			unsigned long system_time_second= timeGetTime();
			m_FirstPeriodSum+=system_time_second-system_time_first;
			//12msec
			if(is_need_reset){ResetVoxelPosition(i);continue;}
			unsigned long system_time_third= timeGetTime();
			m_SecondPeriodSum+=system_time_third-system_time_second;
			
			
			////断面図にも風を反映させるため　
			//if(m_bSliceVisible){
			//	//粒子の位置が移動した後、断面テクスチャにそれを反映させる
			//	vec3<float> n_indexpos;//0.0-1.0に正規化されているインデックス
			//	n_indexpos.x=m_Dynamic[i].x+0.5f;//+shift;//現在の粒子の位置を0.0-1.0に正規化
			//	n_indexpos.y=m_Dynamic[i].y+0.5f;
			//	n_indexpos.z=m_Dynamic[i].z;
			//
			//	//もし配列の範囲を超えてしまったらテクスチャの位置はずらさない。
			//	if(n_indexpos.x>1.0f){continue;}
			//	if(n_indexpos.x<0.0f){continue;}
			//	if(n_indexpos.y>1.0f){continue;}
			//	if(n_indexpos.y<0.0f){continue;}
			//	if(n_indexpos.z>1.0f){continue;}
			//	if(n_indexpos.z<0.0f){continue;}
			//	vec3<int> indexpos;
			//	indexpos.x=n_indexpos.x*(float)(m_VolSize.xy-1);
			//	indexpos.y=n_indexpos.y*(float)(m_VolSize.xy-1);
			//	indexpos.z=n_indexpos.z*(float)(m_VolSize.z-1);
			//	int next_index=m_VolSize.serialId(indexpos.x,indexpos.y,indexpos.z);
			//	m_ucIntensity[next_index]=previous_intensity[i];//
			//}
	}//i loop
		
		return 0;
}
void CCloudRender::VolumeArrayInit(size_t _size){
	m_ucIntensity=new unsigned char[_size];
	m_Dynamic=new  vec3<float>[_size];
	m_Static=new CParticle[_size];
	mBeforeVoxel=new CParticle[_size];
	mAfterVoxel=new CParticle[_size];
}
bool CCloudRender::BlendTime(){
	if((m_Flags& MY_VOLDATA)==0)return 0;
	if((m_Flags& MY_FLOW_DAYTIME)==0)return 0;
	if(m_fTimeRatio>1.0f){m_fTimeRatio=1.0f;}
	int index=0;
	//ファイルによって閾値以上の点の数が違うので
	//x,y,zすべての場所で整理整頓する必要がある。
	//もしぐちゃぐちゃになったら、信号強度0の点を捨てているせい。
	for(int z=0;z<m_VolSize.z;z++){
		for(int y=0;y<m_VolSize.xy;y++){
			for(int x=0;x<m_VolSize.xy;x++){
				index=(z*m_VolSize.xy+y)*m_VolSize.xy+x;
				m_Static[index].pos=(mBeforeVoxel[index].pos*(1.0f-m_fTimeRatio))+(mAfterVoxel[index].pos*m_fTimeRatio);
				m_Static[index].normal=(mBeforeVoxel[index].normal*(1.0f-m_fTimeRatio))+(mAfterVoxel[index].normal*m_fTimeRatio);
				m_Static[index].intensity=(mBeforeVoxel[index].intensity*(1.0f-m_fTimeRatio))+(mAfterVoxel[index].intensity*m_fTimeRatio);
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
	LoadVolData();
	if(m_Flags & MY_VOLDATA){//6分おきのデータはどうしても薄くなっちゃうから濃くする。
		m_PtPara->mTransParency=5.0f;
	}else{
		m_PtPara->CalcTransParency(m_VolSize.z);//こいつがなぜか効かない？
	}
	m_PtPara->resizeVoxel(m_TransForm,(float)m_VolSize.xy);
	mProgram->UpdatePointPara();
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
	static float previousKarma=m_Karmapara;
	if(previousKarma!=m_Karmapara){//
		printf("カルマ変数が変化した。乱数テーブルの作り直し\n");

		//if(m_Flags&MY_ANIMATE){
		//	m_Flags &=~MY_ANIMATE;//一時的にアニメを停止させる。
		//}
		for(int i=0;i<m_ValidPtNum;i++){//初期位置を記憶
			int randval=rand()%m_ValidPtNum;
			m_RandomTable[i]= randval<(int)((float)m_ValidPtNum*m_Karmapara) ? true : false;
			previousKarma=m_Karmapara;
		}
		//m_Flags|=MY_ANIMATE;//アニメ復帰
	}
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
	//点の大きさと透明度が変わったことを検知する
	static float sLastTransparency=m_PtPara->mTransParency;
	static float sLastPointSize=m_PtPara->mPointSize;
	if(sLastTransparency!=m_PtPara->mTransParency){
		mProgram->UpdatePointPara();
		sLastTransparency=m_PtPara->mTransParency;
	}
	//あれ、pointsizeいつも同じだなーー
	if(sLastPointSize!=m_PtPara->mPointSize){
		mProgram->UpdatePointPara();
		sLastPointSize=m_PtPara->mPointSize;
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
	return 0;
}
void CCloudRender::Destroy(){
	delete[] m_ucIntensity;
	delete[] m_ucStaticIntensity;
	delete[] m_Dynamic;
	delete[] m_Static;
	delete[] mBeforeVoxel;
	delete[] mAfterVoxel;
	delete[] m_renderdata;
	delete[] m_rawdata;
	delete[] m_RandomTable;
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
	m_PtPara->resizeVoxel(m_TransForm,(float)m_VolSize.xy);
	mProgram->UpdateEveryReshape(m);
	mRainbowProgram->UpdateProjectionMat(m);
	m_Land->InitCamera(m);
	
}
void CCloudRender::LoadWindData(const string _inifilepath,vec3<float>** _winddata){
	char str_from_inifile[200];
	//風の読み込み
	GetPrivateProfileString(m_DataKey[(m_Flags & MY_VOLDATA)>>10].c_str(),"wind"," ",str_from_inifile,200,_inifilepath.c_str());
	m_Wind.Read(str_from_inifile);
	m_renderdata=new vec3<float>[m_Wind.total()];
	*_winddata=new vec3<float>[m_Wind.total()];
	ifstream ifs(str_from_inifile,ios::binary);
	ifs.read((char*)(*_winddata),m_Wind.total()*m_Wind.each_voxel);
	ifs.close();
	ChangeWindSpeed(m_fWindSpeed,m_rawdata,&m_renderdata);//レンダリング用の風データ
	m_YellowPoints=new list<vec3<float>>[YELLOW_PT_NUM];
	for(int i=0;i<YELLOW_PT_NUM;i++){//重ねて隠しておく
		//randだけど、風のないところには置きたくない
		vec3<float> pos((float)(rand()%m_Wind.xy),(float)(rand()%m_Wind.xy),(float)(rand()%m_Wind.z));
		int windindex=((int)(pos.z*m_Wind.xy+pos.y)*m_Wind.xy+(int)(pos.x));
		if(m_renderdata[windindex].zero()){continue;}//行き着いた先が無風状態だった場合
		//不運だったが飛ばし。リセットしない。
		pos.x=pos.x/(float)m_Wind.xy-0.5f;
		pos.y=pos.y/(float)m_Wind.xy-0.5f;
		pos.z=pos.z/(float)m_Wind.z;
		m_YellowPoints[i].assign(TRAIL_NUM,pos);
	}
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
	m_ucStaticIntensity=new unsigned char[m_VolSize.total()];
	VolumeArrayInit(m_VolSize.total());	
	ifs.read((char*)m_ucStaticIntensity,m_VolSize.total()*m_VolSize.each_voxel);
	memcpy(m_ucIntensity,m_ucStaticIntensity,m_VolSize.total());
	ifs.close();
	m_dataMax=(unsigned char)GetPrivateProfileInt(m_DataKey[0].c_str(),"datamax",256,m_IniFile.c_str());
	
	if(m_Flags& MY_VOLDATA){//6分おきに変わるデータの場合。
		m_ValidPtNum=InitPoints(&m_Static,0,m_ucStaticIntensity);
		m_IsoSurface->Reload(m_nFileId);
		//PrepareAnimeVol();//ここで風データ読むのかな//ファイルスレッドにあるべきものを、ここで読んでるのが間違い。
	}else{
		m_ValidPtNum=InitPoints(&m_Static,64,m_ucStaticIntensity);
	}
	
	// 0-m_ValidPtNumの間で変化し、
	m_RandomTable=new bool[m_ValidPtNum];//乱数テーブル配列確保
	for(int i=0;i<m_ValidPtNum;i++){//初期位置を記憶
		m_Dynamic[i]=m_Static[i].pos;
		int randval=rand()%m_ValidPtNum;
		m_RandomTable[i]= randval<(int)((float)m_ValidPtNum*m_Karmapara) ? true : false;
	}
	
}
/*!
	@brief glewInit()した後にやりたい初期化処理
*/
void CCloudRender::Init(){
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE,&m_Max3DTexSize);
	cout<<"３次元テクスチャの最大サイズ"<<m_Max3DTexSize<<endl;
	m_Light->Init();
	// 点に貼り付ける、丸いぼんやりテクスチャの設定
	glEnable(GL_POINT_SPRITE);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
	glTexEnvf(GL_POINT_SPRITE,GL_COORD_REPLACE,GL_TRUE);
	glActiveTexture(GL_TEXTURE0);
	//なんか、コイツのせいでボリュームデータが見えなくなっちゃった。
	if(!tga::Load_Texture_8bit_Alpha("../data/texture/gaussian64.tga",&m_GausTexSamp)){assert(!"ガウシアンテクスチャが見つかりません！");}
	mProgram = new CGLSLReal("flowpoints.vert","passthrough.frag");
	mProgram->SetGaussianParameter(GL_TEXTURE0);
	LoadWindData(m_IniFile,&m_rawdata);//風関連データのロード
	LoadVolData();//ボリュームデータのロード
	m_IsoSurface->Init(m_IniFile.c_str());

	//ボリュームをロードし終え無いと、点の大きさは決まらない。
	m_PtPara=new CPointParameter(1.7f,8,m_VolSize.z);
	mProgram->SetDataPointer("uTransParency",&m_PtPara->mTransParency);
	mProgram->SetDataPointer("pointsize",&m_PtPara->mPointSize);
	glEnableClientState(GL_VERTEX_ARRAY);
	mProgram->AttribPointer(sizeof(CParticle),(void*)(&m_Static[0].intensity));
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,sizeof(CParticle),&(m_Static[0].pos.x));
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT,sizeof(CParticle),&(m_Static[0].normal.x));
	UpdatePara();
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
	GenerateProjectedRainbow<unsigned char>(m_ucIntensity,m_VolSize.xy*m_VolSize.xy,m_VolSize.z,m_dataMax,m_threshold,projectedRainbowVol);
	
	m_Land->Init(projectedRainbowVol,m_VolSize.xy,m_IniFile,m_DataKey[0]);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
 
	glTexEnvf(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
 
	glTexEnvf(GL_TEXTURE_ENV,GL_SRC0_RGB,GL_PRIMARY_COLOR);
	glTexEnvf(GL_TEXTURE_ENV,GL_SRC1_RGB,GL_PRIMARY_COLOR);//glColor
 
	glTexEnvf(GL_TEXTURE_ENV,GL_SRC0_ALPHA,GL_TEXTURE);
	glTexEnvf(GL_TEXTURE_ENV,GL_SRC1_ALPHA,GL_PRIMARY_COLOR);
			
	}
/*!
もともとは、このCloudRenderクラスが親ウィンドウでGLColorDialogが子ウィンドウだった。
*/

int CCloudRender::InitPoints(CParticle** _voxel,unsigned char _thre,unsigned char* _rawdata){

	static vec3<float> sample1;//法線の計算のために必要
	static vec3<float> sample2;
	static vec3<int> magnitude;
	//どの頂点が手前下によって詰め方を変えなくてはいけない。

	int c=0;
	for(int z=0;z<m_VolSize.z;z++){
		for(int y=0;y<m_VolSize.xy;y++){
			for(int x=0;x<m_VolSize.xy;x++){
				int idx,idy,idz;//視線順に並び替えるために必要。
				idx= x;
				idy= y;
				idz= z;
				int index=m_VolSize.serialId(idx,idy,idz);
				if(_rawdata[index]>=(float)_thre){

					(*_voxel)[c].pos=vec3<float>(
						(float)idx/(float)m_VolSize.xy-0.5f,
						(float)idy/(float)m_VolSize.xy-0.5f,
						(float)idz/(float)m_VolSize.z);
					//法線を計算する
					if(z>0&& y>0 && x>0 && z<m_VolSize.z-1 && y<m_VolSize.xy-1 && x<m_VolSize.xy-1){
						sample1.x=(float)_rawdata[m_VolSize.serialId(x+1,y,z)];
						sample2.x=(float)_rawdata[m_VolSize.serialId(x-1,y,z)];
						sample1.y=(float)_rawdata[m_VolSize.serialId(x,y+1,z)];
						sample2.y=(float)_rawdata[m_VolSize.serialId(x,y-1,z)];
						sample1.z=(float)_rawdata[m_VolSize.serialId(x,y,z+1)];
						sample2.z=(float)_rawdata[m_VolSize.serialId(x,y,z-1)];
						(*_voxel)[c].normal=sample2-sample1;
						(*_voxel)[c].normal.normalize();
					}else{
						(*_voxel)[c].normal=vec3<float>(0.0f,1.0f,0.0f);
					}
					(*_voxel)[c].intensity=((float)_rawdata[index]/(float)m_dataMax);
					c++;
				}//end of threshold

			}
		}
	}
	return c;
}
int CCloudRender::incrementThreshold(int _th){//閾値を増やしたら、つめなおさないとだめよね。
	cout<<"閾値"<<m_threshold+_th<<endl;
	if(m_threshold+_th<=0){return 0;}
	if(m_threshold+_th<m_dataMax){
		m_threshold+=_th;
		m_ValidPtNum=InitPoints(&m_Static,m_threshold,m_ucStaticIntensity);
		return 0;
	}else {
		cout<<"最大値より大きい閾値はダメ　最大値"<<int(m_dataMax)<<"閾値"<<int(m_threshold)<<endl;
		m_threshold=m_dataMax;
		return 1;//失敗
	}
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
	m_TransForm->Enable();
	glPushMatrix();
	glScalef(1.0,1.0,m_ZScale);
			
	glClipPlane(GL_CLIP_PLANE0,m_ClippingEquation);
	glEnable(GL_CLIP_PLANE0);
		m_Land->SetClipPlane(m_ClippingEquation);/// @note　本来地図はクリップ情報は要らないけど、断面図を投影する関係で要ることになってしまった。
		//脇役達 等値面、地形、ライト、目盛り
		if(m_Flags & MY_LAND){
			m_Land->Run();
		}
		
		if(m_Flags & MY_ISOSURFACE){m_Flags&=~MY_CLOUD;m_IsoSurface->Run();}
		if(m_Flags & MY_MEASURE){m_Measure->Draw();}
	
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
		if(m_Measure->m_IsCipVisible){//断面の描画
			glEnable(GL_CULL_FACE);
		
			glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GREATER,0.5);
				glUseProgram(mRainbowProgram->m_Program);
					mRainbowProgram->UpdateModelViewMat(m);
				
					if(m_Flags&MY_ANIMATE){
						mRainbowProgram->Reload3DTexture(&m_ucIntensity[0],m_VolSize.xy,m_VolSize.z,GL_UNSIGNED_BYTE);
					}
						glBegin(GL_POLYGON);
						for(int i = 5; i >= 0; i--) {
							m_Measure->m_intersection[i].glVertex();
						}
						glEnd();
				glDisable(GL_ALPHA_TEST);
			glDisable(GL_BLEND);
			glUseProgram(0);
		}//end of 断面の描画
		
		if(m_Flags & MY_CLOUD){
			glPushMatrix();
			glScalef(1.0,1.0,m_fixedZScale);//なぜこれが効かない？
			//等値面がいるときも断面を出したい、という都合上ここに配置
			glUseProgram(mProgram->m_Program);
				glEnable(GL_POINT_SPRITE);//これでgl_PointCoordが有効になる これで見た目がかなり変わる。
					glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);//こいつはdisableしなくていい？？？！！！
					glGetFloatv(GL_MODELVIEW_MATRIX,m);
					mProgram->UpdateEveryFrame(m,m_TransForm->LocalCam().toVec3());

						glDepthMask(GL_FALSE);
							glEnable(GL_BLEND);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front
	
								glActiveTexture ( GL_TEXTURE0 );
								glEnable ( GL_TEXTURE_2D );
									glBindTexture ( GL_TEXTURE_2D,m_GausTexSamp );
	
									glEnableClientState(GL_VERTEX_ARRAY);
									glEnableClientState(GL_NORMAL_ARRAY);
										mProgram->AttribPointer(sizeof(CParticle),&(m_Static[0].intensity));
										glVertexPointer(3,GL_FLOAT,sizeof(CParticle),&(m_Static[0].pos.x));
										glNormalPointer(GL_FLOAT,sizeof(CParticle),&(m_Static[0].normal));
										if(m_Flags & MY_STATIC_CLOUD){
											glDrawArrays(GL_POINTS,0,m_ValidPtNum);
										}
										if(m_Flags & MY_DYNAMIC_CLOUD){
											glVertexPointer(3,GL_FLOAT,sizeof(vec3<float>),&(m_Dynamic[0].x));
											glDrawArrays(GL_POINTS,0,m_ValidPtNum);
										}
	
									glDisableClientState(GL_VERTEX_ARRAY);
									glDisableClientState(GL_NORMAL_ARRAY);
	
								glDisable(GL_TEXTURE_2D);
							glDisable(GL_BLEND);
						glDepthMask(GL_TRUE);
					glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
				glDisable(GL_POINT_SPRITE);
		
			glUseProgram(0);
			glPopMatrix();//pop fixedZScale
		}
		
		
		if(m_Flags & MY_RED_POINT){
			static GLfloat atten[] = { 0.0, 0.0, 1.0 };
			glEnable(GL_POINT_SPRITE);
			glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION,atten);
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front

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
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
			glDisable(GL_POINT_SPRITE);
			if(m_Flags & MY_ANIMATE){
				bool is_need_reset=BlowVoxel(&m_RedPoint);//赤い点も移動させる
				if(is_need_reset){m_RedPoint.set(0.25,0,0);}
			}

		}
		if(m_Flags & MY_YELLOW_POINTS){
			static GLfloat atten[] = { 0.0, 0.0, 0.8 };
			glDisable(GL_LIGHTING);
			glEnable(GL_POINT_SPRITE);
			glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION,atten);
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front
			glActiveTexture ( GL_TEXTURE0 );
			glEnable ( GL_TEXTURE_2D );
			glBindTexture ( GL_TEXTURE_2D,m_GausTexSamp );
			//風を表すサンプル点群のレンダリング
			glPointSize(20);
			glBegin(GL_POINTS);
			glColor3f(1.0,1.0,0.0);//なぜこれが無効になるのか
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
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
			glDisable(GL_POINT_SPRITE);
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
		}
		
		glPopMatrix();//zScaleを無効化する。
		if(m_Flags & MY_VECTOR ){DrawWindVector();}
		m_Light->Disable();	
		
		if(m_Flags & MY_ANIMATE ){AnimateRandomFadeOut();}//風ベクトルに沿って動かす
		BlendTime();//0～6分後のブレンドする
		m_TransForm->Disable();
		if(m_Flags & MY_CAPTURE){
			Sleep(1000);
		}
		
		return 0;
}
void CCloudRender::Reset(){
		for(int i=0;i<m_ValidPtNum;i++){//初期位置を記憶
			m_Dynamic[i]=m_Static[i].pos;
		}
		//断面テクスチャも元に戻す
	memcpy(m_ucIntensity,m_ucStaticIntensity,sizeof(unsigned char)*m_VolSize.total());

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
	filename<<"../data/cappi/cappi"<<m_nFileId<<".jmesh";
	static HANDLE handle;
	static DWORD dwnumread;
	handle = CreateFile(filename.str().c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	SetFilePointer(handle,sizeof(JMeshHeader)+sizeof(J3Header),NULL,FILE_BEGIN);
	ReadFile(handle,m_ucStaticIntensity,sizeof(unsigned char)*m_VolSize.total(),&dwnumread,NULL);
	CloseHandle(handle);
	
	m_ValidPtNum=InitPoints(&mBeforeVoxel,0,m_ucStaticIntensity);
	m_nFileId++;
	cout<<m_nFileId<<"フレーム目"<<endl;	
	if(m_nFileId>163){m_nFileId=24;}
	
	//nFrame++した後のを読み込む(次フレームの準備)
	filename.str("");
	filename<<"../data/cappi/cappi"<<m_nFileId<<".jmesh";
	handle = CreateFile(filename.str().c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	SetFilePointer(handle,sizeof(JMeshHeader)+sizeof(J3Header),NULL,FILE_BEGIN);
	ReadFile(handle,m_ucStaticIntensity,sizeof(unsigned char)*m_VolSize.total(),&dwnumread,NULL);
	CloseHandle(handle);

	m_ValidPtNum=InitPoints(&mAfterVoxel,0,m_ucStaticIntensity);
	//6分おきのデータは各風データを持っている
	//LoadVelocityData();
	filename.str("");
	filename<<"../data/vvp/vvp"<<m_nFileId<<".jmesh";
	//cout<<"風データ"<<filename.str()<<endl;
	handle = CreateFile(filename.str().c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	SetFilePointer(handle,sizeof(JMeshHeader)+sizeof(J3Header),NULL,FILE_BEGIN);
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
void CCloudRender::UpdatePara(){
	mProgram->UpdatePointPara();
	mProgram->UpdateLightPara();
	glUseProgram(0);
}
CCloudRender::~CCloudRender(void){
	printf("総フレーム数%d\n",m_nFrame);
	printf("前半の時間平均%fmsec,後半の時間平均%fmsec\n",(double)m_FirstPeriodSum/(double)m_nFrame,(double)m_SecondPeriodSum/(double)m_nFrame);

	Destroy();
	delete[] m_ucWindTF;
	stringstream str;
	str<<m_threshold;
	WritePrivateProfileString("anjou","threshold",str.str().c_str(),m_IniFile.c_str());
	
}
