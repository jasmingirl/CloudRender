#include "CloudRender.h"
extern CCloudRender *g_Render;
const float CENTER_ELE=92.7716980f;//レーダー観測地点での高度m
stringstream FILENAME;
float ELERANGE=74.0f/1024.0f;
//こいつらはポインタでつながっているので、離れ離れにしてはならない。
const aiScene* assimp_scene;
	aiVector3D* assimp_verts;
	aiMesh* assimp_mesh;

Assimp::Importer importer;
/*!
	@brief コンストラクタ glewInitよりも前にやっていい初期化をここでする。PrivateProfileで初期設定読み込むのもここ
*/
CCloudRender::CCloudRender(const vec2<int>& _pos)
	:m_bSliceVisible(false)//断面は見えていない
	,m_Flags(MY_CLOUD|MY_LAND)
	,m_fTimeRatio(0.0f)
	,m_fWindSpeed(0.02f)
	,m_Karmapara(0.01f)//生まれ変わりやすを支配するパラメータ あとでロシアンルーレット形式風アニメにも使おうかな	
	,m_IniFile("../data/setting.ini")
	,m_ZScale(ELERANGE)
{	
	m_FirstPeriodSum=m_SecondPeriodSum=m_nFrame=0;
	m_RedPoint.set(0.25f,0.0f,0.0f);
	char test[200];
	GetPrivateProfileSectionNames(test,200,m_IniFile.c_str());
	m_DataKey[0]="iruma";
	//m_DataKey[0]="anjou";//これはhard-codeじゃなくて上の関数でなんとかするべき
	m_DataKey[1]="komaki";
	m_nFileId=GetPrivateProfileInt(m_DataKey[1].c_str(),"frame",88,m_IniFile.c_str());
	mBoundingBox.set(vec3<float>(0.0f,0.0f,0.5f),0.5f,0.5f);
	srand((unsigned int)time(NULL));//あとで風アニメで使用する
	///黄色い点の初期化
	m_YellowPoints=new list<vec3<float>>[YELLOW_PT_NUM];
	for(int i=0;i<YELLOW_PT_NUM;i++){//重ねて隠しておく
		m_YellowPoints[i].assign(TRAIL_NUM,vec3<float>((float)(rand()%100)*0.01f-0.5f,(float)(rand()%100)*0.01f-0.5f,(float)(rand()%100)*0.01f));
	}
	mFontColor.set(1.0,1.0,1.0,1.0);//文字色　背景が白の時は黒・背景が黒の時は白
	mBackGround.set(0.0,0.0,0.0,0.0);
	m_TransForm=new CTransForm();
	
	GetPrivateProfileString("common","dir","../data/",test,200,m_IniFile.c_str());
	m_IsoSurface=new CIsoSurface(ELERANGE,string(test));

	m_Light=new CLight(color<float>(185.0f/255.0f,194.0f/255.0f,137.0f/255.0f,1.0f),
		color<float>(1.0f,254.0f/255.0f,229.0f/255.0f,1.0f),
		vec3<float>(0.0f,1.0f,0.0f),GL_LIGHT0);
	m_Land=new CLand();
	m_Measure=new CMeasure(ELERANGE,m_TransForm);

	
	GetPrivateProfileString(m_DataKey[0].c_str(),"name","失敗",test,200,m_IniFile.c_str());
	m_ToggleText[1]=string(test)+"にする";
	GetPrivateProfileString(m_DataKey[1].c_str(),"name","失敗",test,200,m_IniFile.c_str());
	m_ToggleText[0]=string(test)+"にする";
	m_threshold=(unsigned char)GetPrivateProfileInt(m_DataKey[0].c_str(),"threshold",64,m_IniFile.c_str());
	cout<<"初期設定閾値:"<<(int)m_threshold<<" 最大値"<<(int)m_dataMax<<endl;
	m_ValidPtNum=0;
	GetPrivateProfileString(m_DataKey[0].c_str(),"wind","../data/Typhoon1_32.jmesh",test,200,m_IniFile.c_str());
	
	jmesh::Load(test,&m_Wind.xy,&m_Wind.z,&m_rawdata);
	m_renderdata=new vec3<float>[m_Wind.total()];
	// m_renderdataにレンダリング用に都合の良いように加工したm_rawdataを詰め込む。
	ChangeWindSpeed();
	// 風専用の伝達関数の読み込み　１回め　伝達関数は、、、何か汎用的なフォーマットにしたいなぁ。
	GetPrivateProfileString("common","windtf","../data/tf/rainbow-256.tf",test,200,m_IniFile.c_str());
	miffy::ReadFileToTheEnd(test,&m_ucWindTF);
	//矢印オブジェクトのロード　assimp使わないでもうちょっとシンプルにしたいな。
	assimp_scene = importer.ReadFile("../data/arrow.ply",0);
    if( !assimp_scene){printf("ロード失敗");}
    assimp_mesh=assimp_scene->mMeshes[0];
    assimp_verts=assimp_mesh->mVertices;
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
	cout<<"ArrayInit="<<_size<<endl;
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
void CCloudRender::ChangeWindSpeed(){
		for(int z=0;z<m_Wind.z;z++){
		for(int y=0;y<m_Wind.xy;y++){
			for(int x=0;x<m_Wind.xy;x++){
				int i=(z*m_Wind.xy+y)*m_Wind.xy+x;
				m_renderdata[i]=m_rawdata[i];
				m_renderdata[i]*=m_fWindSpeed;//1.0/8.0
			}
		}
	}

}
void CCloudRender::ChangeVolData(){//6分おきのデータにする、ボタンを押すとここに来る。
	m_Flags&=~MY_CLOUD;//処理が終わるまで雲を非表示にしておく
	
	Destroy();//前の雲データの片付け
	LoadVolData();

	m_pt_para->SetTransParency(m_VolSize.z-4);//こいつがなぜか効かない？
	m_pt_para->resizeVoxel(m_TransForm,(float)m_VolSize.xy);
	mProgram->Enable();
	mProgram->UpdateUni("pointsize",m_pt_para->size());
	mProgram->UpdateUni("uTransParency",m_pt_para->transparency());
	mProgram->Disable();

	mRainbowProgram->Enable();
	//断面図の伝達関数を変えないといけない。
	char str[200];
	GetPrivateProfileString(m_DataKey[(m_Flags & MY_VOLDATA)>>10].c_str(),"tf","失敗",str,200,m_IniFile.c_str());
	
	mRainbowProgram->Reload3DTexture(&m_ucIntensity[0],m_VolSize.xy,m_VolSize.z,GL_UNSIGNED_BYTE);
	mRainbowProgram->Reload1DTexture(str);
	mRainbowProgram->Disable();
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
		m_Land->ToggleMap();
		m_Flags &=~MY_FLOW_DAYTIME;
		cout<<"6分おきに変わるデータをロードします"<<endl;
		m_Flags |=MY_VOLDATA;
		ChangeVolData();
		lastVolData=(m_Flags & MY_VOLDATA);
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
void CCloudRender::DrawWindVector(){
	//±0.5の座標系に合わせる
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front
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
			if(m_rawdata[i].zero()){
				xpos+=increment;
				continue;
			}
				colorindex=(int)(m_rawdata[i].length()*255.0);
				theta=atan2(m_rawdata[i].y,m_rawdata[i].x)* 180.0f /(float) M_PI;
				glPushMatrix();
					
					glTranslatef(xpos,ypos,0.0);
					m_ucWindTF[(int)(colorindex)].glColor();
					//矢印一個を描く場面
					glPushMatrix();
					glScalef(scale,scale,scale);
					glBegin(assimp_mesh->mPrimitiveTypes);
						for(int i=0;i<assimp_mesh->mNumFaces;i++){
							glNormal3fv(&assimp_mesh->mNormals[assimp_mesh->mFaces[i].mIndices[0]].x);
							glVertex3fv(&assimp_verts[assimp_mesh->mFaces[i].mIndices[0]].x);
							glNormal3fv(&assimp_mesh->mNormals[assimp_mesh->mFaces[i].mIndices[1]].x);
							glVertex3fv(&assimp_verts[assimp_mesh->mFaces[i].mIndices[1]].x);
							glNormal3fv(&assimp_mesh->mNormals[assimp_mesh->mFaces[i].mIndices[2]].x);
							glVertex3fv(&assimp_verts[assimp_mesh->mFaces[i].mIndices[2]].x);
						}
						glEnd();
						glPopMatrix();
				glPopMatrix();
			
			xpos+=increment;
			
		}
		ypos+=increment;
	}
	//glPopMatrix();
	/////glDisable(GL_BLEND);
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
	mProgram->Enable();
	mProgram->UpdateUni("pointsize",m_pt_para->resizeVoxel(m_TransForm,(float)m_VolSize.xy));
	mProgram->UpdateProjUni(m);
	mProgram->Disable();
	mRainbowProgram->UpdateProjUni(m);
	m_Land->InitCamera(m);
}

/*!
	
*/
void CCloudRender::LoadVolData(){
	char str[200];
	//風の読み込み
	GetPrivateProfileString(m_DataKey[(m_Flags & MY_VOLDATA)>>10].c_str(),"wind","../data/Typhoon1_32.jmesh",str,200,m_IniFile.c_str());
	jmesh::Load(str,&m_Wind.xy,&m_Wind.z,&m_rawdata);
	m_renderdata=new vec3<float>[m_Wind.total()];
	ChangeWindSpeed();
	//ボリュームデータ読み込み
	GetPrivateProfileString(m_DataKey[(m_Flags & MY_VOLDATA)>>10].c_str(),"file","失敗",str,200,m_IniFile.c_str());
	if(m_Max3DTexSize<256){
		GetPrivateProfileString(m_DataKey[0].c_str(),"lowfile","失敗",str,200,m_IniFile.c_str());
	}
	jmesh::Load(str,&m_VolSize.xy,&m_VolSize.z,&m_ucStaticIntensity);
	VolumeArrayInit(m_VolSize.total());	
	memcpy(m_ucIntensity,m_ucStaticIntensity,sizeof(unsigned char)*m_VolSize.total());
	m_dataMax=(unsigned char)GetPrivateProfileInt(m_DataKey[0].c_str(),"datamax",256,m_IniFile.c_str());
	m_ValidPtNum=0;
	if(m_Flags& MY_VOLDATA){
		InitPoints(m_Static,0);
		PrepareAnimeVol();//ここで風データ読むのかな
	}else{
		InitPoints(m_Static,64);
	}
	// 0-m_ValidPtNumの間で変化し、
	
	m_RandomTable=new bool[m_ValidPtNum];//乱数テーブル配列確保
	for(int i=0;i<m_ValidPtNum;i++){//初期位置を記憶
		m_Dynamic[i]=m_Static[i].pos;
		int randval=rand()%m_ValidPtNum;
		m_RandomTable[i]= randval<(int)((float)m_ValidPtNum*m_Karmapara) ? true : false;
		//printf("%d,%d,%d\n",randval,(int)((float)m_ValidPtNum*m_Karmapara),m_RandomTable[i]);
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
	m_GausTexSamp.ActiveTexture(GL_TEXTURE0);
	tga::Load_Texture_8bit_Alpha("../data/texture/gaussian64.tga",&m_GausTexSamp.tex_id);
	glEnable(GL_POINT_SPRITE);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
	glTexEnvf(GL_POINT_SPRITE,GL_COORD_REPLACE,GL_TRUE);
	mProgram = new CGlsl("flowpoints.vert","passthrough.frag");//GetGLError("CreateShaderProgram");
	mProgram->GetUniformLocation("ModelView");//0
	mProgram->GetUniformLocation("Proj");//1
	mProgram->GetUniformLocation("gaustex");//2
	mProgram->UpdateTextureUniform();
	mProgram->GetUniformLocation("pointsize");//3
	mProgram->GetUniformLocation("uTransParency");//4
	mProgram->GetUniformLocation("uCamera");//5
	mProgram->GetUniformLocation("Ka");//6
	mProgram->GetUniformLocation("Kd");//7
	mProgram->GetUniformLocation("lightpos");//8
	mProgram->GetAttribLocation("intensity");//9
	
	m_IsoSurface->Init();
	
	LoadVolData();
	//ボリュームをロードし終え無いと、点の大きさは決まらない。
	m_pt_para=new CPointParameter(1.7f,8,m_VolSize.z);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	mProgram->AttribPointer(1,GL_FLOAT,sizeof(CParticle),const_cast<float*>(&m_Static[0].intensity));
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,sizeof(CParticle),&(m_Static[0].pos.x));
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT,sizeof(CParticle),&(m_Static[0].normal.x));
	UpdatePara();
	mProgram->Disable();
	//虹色モード用のシェーダ　
	mRainbowProgram = new CGlsl("rainbow.vert","rainbow.frag");
	//テクスチャ→そのほか、の順でロケーションとること
	mRainbowProgram->GetUniformLocation("ModelView");
	mRainbowProgram->GetUniformLocation("Proj");
	mRainbowProgram->GetUniformLocation("voltex");
	mRainbowProgram->GetUniformLocation("tf");
	///@caution 3Dの後1Dでなければならない。
	mRainbowProgram->Add3DTexture(&m_ucIntensity[0],m_VolSize.xy,m_VolSize.z,GL_UNSIGNED_BYTE);//断面図を描くのに使うテクスチャ
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	
	char str[200];
	GetPrivateProfileString(m_DataKey[0].c_str(),"tf","失敗",str,200,m_IniFile.c_str());
	mRainbowProgram->Add1DTexture(str);	
	mRainbowProgram->UpdateTextureUniform();

	//ボリュームデータを地面に投影しつつ虹色に彩色する。投影図を画像ファイルにしちゃおっかな？　そうすると、閾値を動的に変えられないなぁ。。
	color<float> *projectedRainbowVol;
	GenerateProjectedRainbow<unsigned char>(m_ucIntensity,m_VolSize.xy*m_VolSize.xy,m_VolSize.z,m_dataMax,m_threshold,projectedRainbowVol);
	
	m_Land->Init(projectedRainbowVol,m_VolSize.xy);
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

void CCloudRender::InitPoints(CParticle* _voxel,unsigned char _thre){

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
				if(m_ucStaticIntensity[index]>=(float)_thre){

					_voxel[c].pos=vec3<float>(
						(float)idx/(float)m_VolSize.xy-0.5f,
						(float)idy/(float)m_VolSize.xy-0.5f,
						(float)idz/(float)m_VolSize.z);
					//法線を計算する
					if(z>0&& y>0 && x>0 && z<m_VolSize.z-1 && y<m_VolSize.xy-1 && x<m_VolSize.xy-1){
						sample1.x=(float)m_ucStaticIntensity[m_VolSize.serialId(x+1,y,z)];
						sample2.x=(float)m_ucStaticIntensity[m_VolSize.serialId(x-1,y,z)];
						sample1.y=(float)m_ucStaticIntensity[m_VolSize.serialId(x,y+1,z)];
						sample2.y=(float)m_ucStaticIntensity[m_VolSize.serialId(x,y-1,z)];
						sample1.z=(float)m_ucStaticIntensity[m_VolSize.serialId(x,y,z+1)];
						sample2.z=(float)m_ucStaticIntensity[m_VolSize.serialId(x,y,z-1)];
						_voxel[c].normal=sample2-sample1;
						_voxel[c].normal.normalize();
					}else{
						_voxel[c].normal=vec3<float>(0.0f,1.0f,0.0f);
					}
					_voxel[c].intensity=((float)m_ucStaticIntensity[index]/(float)m_dataMax);
					c++;
				}//end of threshold

			}
		}
	}
	m_ValidPtNum=c;
}
int CCloudRender::incrementThreshold(int _th){//閾値を増やしたら、つめなおさないとだめよね。
	cout<<"閾値"<<m_threshold+_th<<endl;
	if(m_threshold+_th<=0){return 0;}
	if(m_threshold+_th<m_dataMax){
		m_threshold+=_th;
		InitPoints(m_Static,m_threshold);
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
	
		m_Land->SetClipPlane(m_TransForm->m_ClippingEquation);/// @note　本来地図はクリップ情報は要らないけど、断面図を投影する関係で要ることになってしまった。
	
		//脇役達 等値面、地形、ライト、目盛り
		m_Light->Enable();
		if(m_Flags & MY_LAND){m_Land->Run();}
		if(m_Flags & MY_ISOSURFACE){m_Flags&=~MY_CLOUD;m_IsoSurface->Run();}
		if(m_Flags & MY_MEASURE){m_Measure->Draw();}
	
		float m[16];
		glGetFloatv(GL_MODELVIEW_MATRIX,m);
		//主役の雲を一番手前に
		//cout<<"m_bCurrentVolData"<<m_bCurrentVolData<<endl;
		vec3<float> intersection[6];
		m_bSliceVisible=mBoundingBox.CalcClipPlaneVerts(m_TransForm->m_ClippingEquation,intersection);
		if(m_bSliceVisible){//断面の描画
			glEnable(GL_CULL_FACE);
		
			glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GREATER,0.5);
					mRainbowProgram->Enable();
					mRainbowProgram->UpdateModelViewUni(m);
				
					if(m_Flags&MY_ANIMATE){
						mRainbowProgram->Reload3DTexture(&m_ucIntensity[0],m_VolSize.xy,m_VolSize.z,GL_UNSIGNED_BYTE);
					}
						glBegin(GL_POLYGON);
						for(int i = 5; i >= 0; i--) {
							intersection[i].glVertex();
						}
						glEnd();
				glDisable(GL_ALPHA_TEST);
			glDisable(GL_BLEND);
			glUseProgram(0);
		}//end of 断面の描画
		if(m_Flags & MY_CLOUD){
			//等値面がいるときも断面を出したい、という都合上ここに配置
			mProgram->Enable();
				glEnable(GL_POINT_SPRITE);//これでgl_PointCoordが有効になる これで見た目がかなり変わる。
					glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);//こいつはdisableしなくていい？？？！！！
	
						mProgram->UpdateModelViewUni(m);
						mProgram->UpdateVec("uCamera",m_TransForm->LocalCam().toVec3());

						glDepthMask(GL_FALSE);
							glEnable(GL_BLEND);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front
	
								glActiveTexture ( GL_TEXTURE0 );
								glEnable ( GL_TEXTURE_2D );
									m_GausTexSamp.Bind2D();
	
									glEnableClientState(GL_VERTEX_ARRAY);
									glEnableClientState(GL_NORMAL_ARRAY);
										mProgram->AttribPointer(1,GL_FLOAT,sizeof(CParticle),&(m_Static[0].intensity));
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
			m_GausTexSamp.Bind2D();

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
			glEnable(GL_POINT_SPRITE);
			glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION,atten);
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front


			glActiveTexture ( GL_TEXTURE0 );
			glEnable ( GL_TEXTURE_2D );
			m_GausTexSamp.Bind2D();
			//黄色い点のレンダリング
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
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
			glDisable(GL_POINT_SPRITE);
			//黄色い点のアニメーション
			if((m_Flags & MY_ANIMATE) | (m_Flags & MY_ANIMATE_YELLOW)){
				for(int i=0;i<YELLOW_PT_NUM;i++){
					bool is_need_reset=BlowVoxelWithTrail(&m_YellowPoints[i]);
					if(is_need_reset){
						m_YellowPoints[i].front().set((float)(rand()%100)*0.01f-0.5f,(float)(rand()%100)*0.01f-0.5f,(float)(rand()%100)*0.01f);
						list<vec3<float>>::iterator it=m_YellowPoints[i].begin();
						while(it!=m_YellowPoints[i].end()){
							it->set(m_YellowPoints[i].front());
						}//end while
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
		zScale=0.3f;
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
	
	InitPoints(mBeforeVoxel,0);
	m_nFileId++;
	cout<<m_nFileId<<"フレーム目"<<endl;
	if(m_nFileId>163){m_nFileId=24;}
	
	//nFrame++した後のを読み込む
	filename.str("");
	filename<<"../data/cappi/cappi"<<m_nFileId<<".jmesh";
	handle = CreateFile(filename.str().c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	SetFilePointer(handle,sizeof(JMeshHeader)+sizeof(J3Header),NULL,FILE_BEGIN);
	ReadFile(handle,m_ucStaticIntensity,sizeof(unsigned char)*m_VolSize.total(),&dwnumread,NULL);
	CloseHandle(handle);

	InitPoints(mAfterVoxel,0);
	//6分おきのデータは各風データを持っている
	//LoadVelocityData();
	filename.str("");
	filename<<"../data/vvp/vvp"<<m_nFileId<<".jmesh";
	handle = CreateFile(filename.str().c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	SetFilePointer(handle,sizeof(JMeshHeader)+sizeof(J3Header),NULL,FILE_BEGIN);
	ReadFile(handle,&m_rawdata[0].x,sizeof(vec3<float>)*m_Wind.total(),&dwnumread,NULL);
	CloseHandle(handle);
	ChangeWindSpeed();
}
void CCloudRender::PrintKeyHelp(){
		puts("");
	printf("ヘルプ\n");
	printf("ヘルプ\n");
	puts("【視点関係】");
	puts("F1:現在の変換行列などの情報を表示");
	puts("平行移動:Shift押しながらクリック");
	puts("ズームイン・アウト:ホイール");
	puts("回転:ドラッグ");
	puts("【見た目関係】");
	puts("o:正投影・透視投影切り替え");
	puts("b:背景の白黒切り替え");
	puts("B:背景を描く/描かない");
	//printf("L,l:クリップ面操作:現在の値%f\n",mClipPlane);
	puts("\n【視点関係】");
	puts("平行移動:Shift押しながらクリック");
	puts("ズームイン・アウト:ホイール");
	puts("回転:ドラッグ");
	puts("真横から見る:矢印キー←or→");
	puts("真上から見る:PageUpキー");
	puts("真下から見る:PageDownキー");
	puts("\n【風関係】");
	//printf("a:風アニメーションon,off:現在の値=%d\n",mAnimationFlag);
	puts("k,K:生まれ変わりやすさ");
	puts("s,S:風のスピードUP/DOWN");
	puts("v:(velocity)風ベクトル表示・非表示切り替え");
	//	printf("g:gifアニメ撮影モード(onの時は１秒＝1frameで遅くなります):現在の値=%d",captureflag);
	puts("r:リセット。粒子の位置をすべて初期位置に戻す");
	puts("\n【見た目関係】");
	puts("P,p:点の大きさUP/DOWN");
	puts("m:(measure)zスケール定規の表示・非表示切り替え");
	puts("M:(Map)地図の表示・非表示切り替え");
	puts("o:(orthogonal)正投影・透視投影切り替え");
	puts("t,T:(transparency)全体の透明度アップ・ダウン");
	puts("V:(Volume)雲表示・非表示");
	puts("x:虹色モードon:off");
	puts("");
}
void CCloudRender::SetFlag(unsigned int _flags){
	m_Flags=_flags;
}
void CCloudRender::UpdatePara(){
	mProgram->Enable();
	mProgram->UpdateUni("pointsize",m_pt_para->size());
	mProgram->UpdateUni("uTransParency",m_pt_para->transparency());
	
	float col[4];
	glGetLightfv(GL_LIGHT0,GL_AMBIENT,col);
	mProgram->UpdateUni3f("Ka",col);
	glGetLightfv(GL_LIGHT0,GL_DIFFUSE,col);
	mProgram->UpdateUni3f("Kd",col);
	glGetLightfv(GL_LIGHT0,GL_POSITION,col);
	mProgram->UpdateUni3f("lightpos",col);
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
