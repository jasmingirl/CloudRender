//------------------------------------------------------------------------------/
// File Name  / CloudRender.cpp
//------------------------------------------------------------------------------/
// Abstruct		/一番メインのプログラム
// Declared in	/																/
// Include		/																/
// History		/ Rev 1.0.0 2013.02.22 Minako Yamahiro
//------------------------------------------------------------------------------/
// Copyright ( C ) 2012 Japan Radio Co., Ltd, All right reserved				/
//------------------------------------------------------------------------------/
#include "CloudRender.h"
extern CCloudRender *g_Render;
GLColorDialog* gColorDialog;
const float CENTER_ELE=92.7716980f;//レーダー観測地点での高度m
stringstream FILENAME;
float ELERANGE=74.0f/1024.0f;
static void idle(void ){
	if(g_Render->m_bCapture){
		Sleep(1000);
	}
	glutPostRedisplay();
}
static void display(){
	g_Render->Run();
	glutSwapBuffers();
}
static void myReshape(int _w, int _h){
	g_Render->m_TransForm->resize(_w,_h);
	g_Render->InitCamera();
}
void specialkey(int c, int x, int y){
	g_Render->m_TransForm->ChangeView(c);
	glutPostRedisplay();
}
static void mykeyboard(unsigned char c, int x, int y){
	g_Render->KeyBoard(c);
}
static void joystick(unsigned int buttonmask, int x, int y, int z){
	g_Render->JoyStick( buttonmask,  x,  y,  z);
}
static void move(int _x,int _y){
	g_Render->m_InputHandle->move(_x,_y,g_Render->m_TransForm);
}
static void mouse(int button ,int touch_state, int _x, int _y){
	g_Render->m_InputHandle->mouse(button,touch_state,_x,_y,g_Render->m_TransForm);
}
static void wheel(int wheel_number,int direction,int x,int y){
	g_Render->m_TransForm->zoom((float)-direction*0.05f);
	glutPostRedisplay();
}
CCloudRender::CCloudRender(const vec2<int>& _pos)
	:m_bCurrentVolData(false)//デフォルトは「解像度の良い雲」
	,m_Hide(false)//ボリュームデータは隠さない
	,m_bStopDayTime(true)//大域時間は流さない
	,m_bSliceVisible(false)//断面は見えていない
	,m_bHideVector(true)//風ベクトルは非表示にしておく
	,m_fTimeRatio(0.0f)
	,m_fWindSpeed(0.02f)
	,m_Karmapara(0.45f)//生まれ変わりやすを支配するパラメータ あとでロシアンルーレット形式風アニメにも使おうかな	
	,m_IniFile("../data/setting.ini")
{	
	char test[200];
	GetPrivateProfileSectionNames(test,200,m_IniFile.c_str());
	m_DataKey[0]="iruma";
	//m_DataKey[0]="anjou";//これはhard-codeじゃなくて上の関数でなんとかするべき
	m_DataKey[1]="komaki";
	m_nFrame=GetPrivateProfileInt(m_DataKey[1].c_str(),"frame",88,m_IniFile.c_str());
	mBoundingBox.set(vec3<float>(0.0f,0.0f,0.5f),0.5f,0.5f);
	srand((unsigned int)time(NULL));//あとで風アニメで使用する
	current_me=this;//これはしなくてもいいかも。
	m_bCapture=false;
	m_bPrint=false;
	zSpeed=1.0;
	m_DoneGlewInit=mStartFlightFlag=false;
	m_BgColor.set(0.0,0.0,0.0,0.0);
	m_nNode=0;
	m_Nodes=new INode*[2];
	mFontColor.set(1.0,1.0,1.0,1.0);//文字色　背景が白の時は黒・背景が黒の時は白
	mBackGround.set(0.0,0.0,0.0,0.0);
	m_TransForm=new CTransForm(ELERANGE,35.0f,0.1f,100.0,-2.0f,_pos,vec2<int>(1024,768),"高さスケール");
	m_InputHandle=new CInputHandler();
	
	GetPrivateProfileString("common","dir","../data/",test,200,m_IniFile.c_str());
	m_IsoSurface=new CIsoSurface(ELERANGE,string(test));

	m_Light=new CLight(color<float>(185.0f/255.0f,194.0f/255.0f,137.0f/255.0f,1.0f),
		color<float>(1.0f,254.0f/255.0f,229.0f/255.0f,1.0f),
		vec3<float>(0.0f,1.0f,0.0f),GL_LIGHT0);
	m_Land=new CLand();
	m_Measure=new CMeasure(ELERANGE,m_TransForm);

	//ツリー構造生成
	m_Light->AppendChild(m_Land);
	m_Light->AppendChild(m_IsoSurface);

	AppendChild(m_Light);
	AppendChild(m_Measure);
	GetPrivateProfileString(m_DataKey[0].c_str(),"name","失敗",test,200,m_IniFile.c_str());
	m_ToggleText[1]=string(test)+"にする";
	GetPrivateProfileString(m_DataKey[1].c_str(),"name","失敗",test,200,m_IniFile.c_str());
	m_ToggleText[0]=string(test)+"にする";
	m_dataMax=(unsigned char)GetPrivateProfileInt(m_DataKey[0].c_str(),"datamax",256,m_IniFile.c_str());
	m_threshold=(unsigned char)GetPrivateProfileInt(m_DataKey[0].c_str(),"threshold",64,m_IniFile.c_str());
	cout<<"初期設定閾値:"<<(int)m_threshold<<" 最大値"<<(int)m_dataMax<<endl;
	m_ValidPtNum=0;
	GetPrivateProfileString(m_DataKey[0].c_str(),"wind","../data/Typhoon1_32.jmesh",test,200,m_IniFile.c_str());
	HANDLE handle;
	DWORD dwnumread;
	JMeshHeader header;
	J3Header j3header;
	handle = CreateFile(test, GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	ReadFile(handle,&header.FileType,sizeof(JMeshHeader),&dwnumread ,NULL);
	ReadFile(handle,&j3header.xNum,sizeof(J3Header),&dwnumread ,NULL);


	m_Wind.xy=j3header.xNum;
	m_Wind.z=j3header.zNum;

	m_rawdata=new vec3<float>[m_Wind.total()];
	m_renderdata=new vec3<float>[m_Wind.total()];

	ReadFile(handle,&m_rawdata[0],sizeof(vec3<float>)*m_Wind.total(),&dwnumread ,NULL);
	CloseHandle(handle);
	ChangeWindSpeed();
	GetPrivateProfileString("common","windtf","../data/tf/rainbow-256.tf",test,200,m_IniFile.c_str());
	handle = CreateFile(test, GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	if(handle==INVALID_HANDLE_VALUE){assert(!"ファイルが存在しません。");}
	DWORD high;
	DWORD low=GetFileSize(handle,&high);
	m_ucWindTF=new color<float>[low/sizeof(color<float>)];
	ReadFile(handle,m_ucWindTF,low,&dwnumread ,NULL);
	CloseHandle(handle);
}
bool CCloudRender::Animate(){//風ベクトルと、ボリュームデータと初期位置が必要。
	//ここは速さが大事。
	if(m_bStopMoment){return 1;}
	//位置の更新
	//zの範囲も0.0-1.0*ELERANGEと考えてよいのかな。
		static vec3<int> windpos;
		static vec3<int> indexpos;
		static vec3<float> n_indexpos;//0.0-1.0に正規化されているインデックス
		static int index;
		static int next_index;
		static int windindex;
//		int randomval;
		//static float randomratio=0.5f;
		//static vec3<float> red(0.0,0.0,0.0);
		//static変数をnewしたらどうなるんだろう。どこでdeleteすればいいのか？
		//演算子のオペレータの呼び出し厳禁　遅くなる
		//インライン化しても遅い
		static unsigned char* previous_intensity=new unsigned char[m_Size.total()];//断面図を動的に変化させるために使う。
		if(m_bSliceVisible){
			memcpy(previous_intensity,m_ucIntensity,sizeof(unsigned char)*m_Size.total());
		}
		for(int i=0;i<m_ValidPtNum;i++){
			n_indexpos.x=m_Dynamic[i].x+0.5f;//;//現在の粒子の位置を0.0-1.0に正規化
			n_indexpos.y=m_Dynamic[i].y+0.5f;
			n_indexpos.z=m_Dynamic[i].z;
			
			//もし配列の範囲を超えてしまったら位置をリセットする。
			if(n_indexpos.x>1.0f){	m_Dynamic[i]=m_Static[i].pos;continue;}
			if(n_indexpos.x<0.0f){	m_Dynamic[i]=m_Static[i].pos;continue;}
			if(n_indexpos.y>1.0f){	m_Dynamic[i]=m_Static[i].pos;continue;}
			if(n_indexpos.y<0.0f){	m_Dynamic[i]=m_Static[i].pos;continue;}
			if(n_indexpos.z>1.0f){	m_Dynamic[i]=m_Static[i].pos;continue;}
			if(n_indexpos.z<0.0f){	m_Dynamic[i]=m_Static[i].pos;continue;}
			//ここで関数呼び出しすると激重　FPS3になる。 直書きにするとFPS1up
			//インライン化するか、直書きするか？迷い中。
			//indexpos*=vec3<float>((float)m_Size.xy,(float)m_Size.xy,(float)m_Size.z);
			indexpos.x=(int)(n_indexpos.x*(float)m_Size.xy);
			indexpos.y=(int)(n_indexpos.y*(float)m_Size.xy);
			indexpos.z=(int)(n_indexpos.z*(float)m_Size.z);
			
			index=((int)indexpos.z*m_Size.xy+(int)indexpos.y)*m_Size.xy+(int)indexpos.x;//
			
			
			if(previous_intensity[index]<(m_threshold)){//閾値以下だった場合:生まれ変わる
				m_Dynamic[i]=m_Static[i].pos;continue;
			}
			/*
			//閾値以上だった場合
			randomval=rand()%(m_threshold);//0-255の乱数
			if(randomval<=(int)(mKarmapara*randomratio*(float)m_ucIntensity[index])){
				m_Dynamic[i]=m_Static[i];continue;
			}
			*/
			
			windpos.x=(int)((n_indexpos.x)*(float)(m_Wind.xy));
			windpos.y=(int)((n_indexpos.y)*(float)(m_Wind.xy));
			windpos.z=(int)((n_indexpos.z)*(float)(m_Wind.z));
			
			windindex=((int)(windpos.z*m_Wind.xy+windpos.y)*m_Wind.xy+(int)(windpos.x));
			
			if(m_renderdata[windindex].zero()){//行き着いた先が無風状態だった場合
				m_Dynamic[i]=m_Static[i].pos;continue;
			}
			//数々の条件をクリアしたら 位置を更新！！
			//なぜ最初だけぶっとぶのか？
			m_Dynamic[i]+=(m_renderdata[windindex]);
			//テクスチャもずらす。
			n_indexpos.x=m_Dynamic[i].x+0.5f;//+shift;//現在の粒子の位置を0.0-1.0に正規化
			n_indexpos.y=m_Dynamic[i].y+0.5f;
			n_indexpos.z=m_Dynamic[i].z;
			
			//もし配列の範囲を超えてしまったらテクスチャの位置はずらさない。
			if(n_indexpos.x>1.0f){continue;}
			if(n_indexpos.x<0.0f){continue;}
			if(n_indexpos.y>1.0f){continue;}
			if(n_indexpos.y<0.0f){continue;}
			if(n_indexpos.z>1.0f){continue;}
			if(n_indexpos.z<0.0f){continue;}
			
			indexpos.x=n_indexpos.x*(float)m_Size.xy;
			indexpos.y=n_indexpos.y*(float)m_Size.xy;
			indexpos.z=n_indexpos.z*(float)m_Size.z;
			next_index=m_Size.serialId(indexpos.x,indexpos.y,indexpos.z);
			//断面図にも風を反映させるため
			if(m_bSliceVisible){
				m_ucIntensity[next_index]=previous_intensity[index];
			}
	}//i loop
		return 0;
}
bool CCloudRender::AnimateRandomFadeOut(){
	//箱やしきい値に関係なくランダムにフェードアウト
	//ここは速さが大事。
	if(m_bStopMoment){return 1;}
	//位置の更新
	//zの範囲も0.0-1.0*ELERANGEと考えてよいのかな。
		
		static vec3<int> windpos;
		static vec3<int> indexpos;
		static vec3<float> n_indexpos;//0.0-1.0に正規化されているインデックス
		static int index;
		static int next_index;
		static int windindex;
		static int randomval;
		static float randomratio=0.5f;
		//演算子のオペレータの呼び出しは遅くなる
		//インライン化しても遅い
		static unsigned char* previous_intensity=new unsigned char[m_Size.total()];//断面図を動的に変化させるために使う。
		
		if(m_bSliceVisible){//最初のスライスを記憶
			memcpy(previous_intensity,m_ucIntensity,sizeof(unsigned char)*m_Size.total());
		}
		static int random_control=m_ValidPtNum/256;//この割り算の値はパラメータ化した方がいい
		
		for(int i=0;i<m_ValidPtNum;i++){
			//無作為に生まれ変わる
			randomval=rand()%(m_ValidPtNum);
			if(randomval<random_control){m_Dynamic[i]=m_Static[i].pos;m_ucIntensity[i]=m_ucStaticIntensity[i];continue;}
			//現在の粒子の位置を0.0-1.0に正規化 現在の位置から、参照すべき風ベクトルの位置を知るため
			n_indexpos.x=m_Dynamic[i].x+0.5f;
			n_indexpos.y=m_Dynamic[i].y+0.5f;
			n_indexpos.z=m_Dynamic[i].z;
			
			//もし配列の範囲を超えてしまったら位置をリセットする。 無い風は参照にできないから　希望としてはボリュームデータよりも大きめに風データを置きたい　じゃないと端っこが変になるから
			if(n_indexpos.x>1.0f){	m_Dynamic[i]=m_Static[i].pos;m_ucIntensity[i]=m_ucStaticIntensity[i];continue;}
			if(n_indexpos.x<0.0f){	m_Dynamic[i]=m_Static[i].pos;m_ucIntensity[i]=m_ucStaticIntensity[i];continue;}
			if(n_indexpos.y>1.0f){	m_Dynamic[i]=m_Static[i].pos;m_ucIntensity[i]=m_ucStaticIntensity[i];continue;}
			if(n_indexpos.y<0.0f){	m_Dynamic[i]=m_Static[i].pos;m_ucIntensity[i]=m_ucStaticIntensity[i];continue;}
			if(n_indexpos.z>1.0f){	m_Dynamic[i]=m_Static[i].pos;m_ucIntensity[i]=m_ucStaticIntensity[i];continue;}
			if(n_indexpos.z<0.0f){	m_Dynamic[i]=m_Static[i].pos;m_ucIntensity[i]=m_ucStaticIntensity[i];continue;}
			//ここで関数呼び出しすると激重　FPS3になる。 直書きにするとFPS1up
			//インライン化するか、直書きするか？迷い中。
			indexpos.x=(int)(n_indexpos.x*(float)(m_Size.xy-1));
			indexpos.y=(int)(n_indexpos.y*(float)(m_Size.xy-1));
			indexpos.z=(int)(n_indexpos.z*(float)(m_Size.z-1));
			
			index=((int)indexpos.z*m_Size.xy+(int)indexpos.y)*m_Size.xy+(int)indexpos.x;//閾値関係ないインデックス
			
			windpos.x=(int)((n_indexpos.x)*(float)(m_Wind.xy));
			windpos.y=(int)((n_indexpos.y)*(float)(m_Wind.xy));
			windpos.z=(int)((n_indexpos.z)*(float)(m_Wind.z));
			
			windindex=((int)(windpos.z*m_Wind.xy+windpos.y)*m_Wind.xy+(int)(windpos.x));
			
			if(m_renderdata[windindex].zero()){//行き着いた先が無風状態だった場合
				m_Dynamic[i]=m_Static[i].pos;m_ucIntensity[i]=m_ucStaticIntensity[i];continue;
			}
			//数々の条件をクリアしたら 位置を更新！！
			//なぜ最初だけぶっとぶのか？
			m_Dynamic[i]+=(m_renderdata[windindex]);

			//断面図にも風を反映させるため　ここがエラー！！
			if(m_bSliceVisible){
				//粒子の位置が移動した後、断面テクスチャにそれを反映させる
				n_indexpos.x=m_Dynamic[i].x+0.5f;//+shift;//現在の粒子の位置を0.0-1.0に正規化
				n_indexpos.y=m_Dynamic[i].y+0.5f;
				n_indexpos.z=m_Dynamic[i].z;
			
				//もし配列の範囲を超えてしまったらテクスチャの位置はずらさない。
				if(n_indexpos.x>1.0f){continue;}
				if(n_indexpos.x<0.0f){continue;}
				if(n_indexpos.y>1.0f){continue;}
				if(n_indexpos.y<0.0f){continue;}
				if(n_indexpos.z>1.0f){continue;}
				if(n_indexpos.z<0.0f){continue;}
			
				indexpos.x=n_indexpos.x*(float)(m_Size.xy-1);
				indexpos.y=n_indexpos.y*(float)(m_Size.xy-1);
				indexpos.z=n_indexpos.z*(float)(m_Size.z-1);
				next_index=m_Size.serialId(indexpos.x,indexpos.y,indexpos.z);
				m_ucIntensity[next_index]=previous_intensity[index];//
			}
	}//i loop
		return 0;
}
void CCloudRender::ArrayInit(size_t _size){
	cout<<"ArrayInit()"<<endl;
	m_ucIntensity=new unsigned char[_size];
	m_ucStaticIntensity=new unsigned char[_size];
	m_Dynamic=new  vec3<float>[_size];
	m_Static=new CParticle[_size];
	mBeforeVoxel=new CParticle[_size];
	mAfterVoxel=new CParticle[_size];
}
void CCloudRender::AppendChild(INode* _ch){
	m_Nodes[m_nNode]=_ch;
	m_nNode++;
}
bool CCloudRender::BlendTime(){
	if(!m_bCurrentVolData)return 0;
	if(m_bStopDayTime)return 0;
	if(m_fTimeRatio>1.0f){m_fTimeRatio=1.0f;}
	int index=0;
	//ファイルによって閾値以上の点の数が違うので
	//x,y,zすべての場所で整理整頓する必要がある。
	//もしぐちゃぐちゃになったら、信号強度0の点を捨てているせい。
	for(int z=0;z<m_Size.z;z++){
		for(int y=0;y<m_Size.xy;y++){
			for(int x=0;x<m_Size.xy;x++){
				index=(z*m_Size.xy+y)*m_Size.xy+x;
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
	m_Hide=true;//処理が終わるまでこうしておく
	stringstream filename;
	 char* dir;
	Destroy();
	char str[200];
	
	JMeshHeader jmeshhead;
	J3Header j3head;
	HANDLE handle;
	DWORD dwnumread;
 //風ヘッダの読み込み(ArrayInitのため)
	GetPrivateProfileString(m_DataKey[m_bCurrentVolData].c_str(),"wind","../data/Typhoon1_32.jmesh",str,200,m_IniFile.c_str());
	handle = CreateFile(str, GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	if(handle==INVALID_HANDLE_VALUE){cout<<str<<endl;assert(!"ファイルが存在しません。");}
	ReadFile(handle,&jmeshhead.FileType,sizeof(JMeshHeader),&dwnumread ,NULL);
	ReadFile(handle,&j3head.xNum,sizeof(J3Header),&dwnumread ,NULL);
	m_Wind.xy=j3head.xNum;
	m_Wind.z=j3head.zNum;
	CloseHandle(handle);
	//ボリュームデータ読み込み
	GetPrivateProfileString(m_DataKey[m_bCurrentVolData].c_str(),"file","失敗",str,200,m_IniFile.c_str());
		if(m_Max3DTexSize<256){
			GetPrivateProfileString(m_DataKey[0].c_str(),"lowfile","失敗",str,200,m_IniFile.c_str());
		}
 handle = CreateFile(str, GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
if(handle==INVALID_HANDLE_VALUE){cout<<str<<endl;assert(!"ファイルが存在しません。");}
	
	ReadFile(handle,&jmeshhead.FileType,sizeof(JMeshHeader),&dwnumread ,NULL);
	ReadFile(handle,&j3head.xNum,sizeof(J3Header),&dwnumread ,NULL);
	m_Size.xy=j3head.xNum;
	m_Size.z=j3head.zNum;

	if(m_bCurrentVolData){//6分おきのデータの場合
		m_pt_para->SetTransParency(m_Size.z);//こいつがなぜか効かない？
		m_pt_para->resizeVoxel(m_TransForm,(float)m_Size.xy);
		mProgram->Enable();
		mProgram->UpdateUni("pointsize",m_pt_para->size());
		mProgram->UpdateUni("uTransParency",m_pt_para->transparency());
		mProgram->Disable();
	}
	ArrayInit(m_Size.total());
	m_rawdata=new vec3<float>[m_Wind.total()];
	m_renderdata=new vec3<float>[m_Wind.total()];
	ReadFile(handle,m_ucIntensity,sizeof(unsigned char)*m_Size.total(),&dwnumread ,NULL);
	CloseHandle(handle);
	memcpy(m_ucStaticIntensity,m_ucIntensity,sizeof(unsigned char)*m_Size.total());
	
	m_ValidPtNum=0;
	m_dataMax=GetPrivateProfileInt(m_DataKey[m_bCurrentVolData].c_str(),"datamax",256,m_IniFile.c_str());
		
	if(m_bCurrentVolData){
		InitPoints(m_Static,0);
		PrepareAnimeVol();//ここで風データ読むのかな
	}else{
		InitPoints(m_Static,64);
	}
	
	for(int i=0;i<m_ValidPtNum;i++){//初期位置を記憶
			m_Dynamic[i]=m_Static[i].pos;
		}
	mRainbowProgram->Enable();
	//断面図の伝達関数を変えないといけない。
	GetPrivateProfileString(m_DataKey[m_bCurrentVolData].c_str(),"tf","失敗",str,200,m_IniFile.c_str());
    mRainbowProgram->Reload1DTexture(str);
	
	mRainbowProgram->Reload3DTexture(&m_ucIntensity[0],m_Size.xy,m_Size.z);
	mRainbowProgram->Disable();
	m_Hide=false;//描画ロック解除
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
	cout<<"deleteしました"<<endl;
}
void CCloudRender::DrawWindVector(){
	if(m_bHideVector){return;}
	//±0.5の座標系に合わせる
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front
	//glColor4f(0.0,0.0,1.0,0.5);
	//矢羽を書く位置
	float xpos=0.5f;
	float ypos=-0.5f;
	int show_mag=2;//2:1個飛ばしで描く　1:フル解像度で描く
	//矢羽同士の間隔
	float increment=1.0/(float)m_Wind.xy*(float)show_mag;
	float radius=0.1f*increment;//横幅に対して0.1倍程度
	float theta;
	int i=0;
	int colorindex;
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
				theta=atan2(m_rawdata[i].y,m_rawdata[i].x)* 180.0 / M_PI;
				glPushMatrix();
					glTranslatef(xpos,ypos,0.0);
					glRotatef(270.0,0.0,1.0,0.0);//XY平面にするための回転
					glRotatef(-theta,1.0,0.0,0.0);//なぜか逆になってしまった
					//glColor4f(0.0,0.0,1.0,1.0);
					m_ucWindTF[(int)(colorindex)].glColor();
					glutSolidCone(radius*1.2,increment*0.4,10,10);
					glRotatef(180.0,0.0,1.0,0.0);
					glutSolidCylinder(radius*0.8,increment*0.5,10,10);
				glPopMatrix();
			
			xpos+=increment;
			
		}
		ypos+=increment;
	}
	//glPopMatrix();
	/////glDisable(GL_BLEND);
}
bool CCloudRender::FileLoad(){//ファイルスレッド用関数
	if(!m_bCurrentVolData){return 0;}
	if(m_bStopDayTime)return 0;
		if(m_fTimeRatio>=1.0){
			m_fTimeRatio=0.0;
			PrepareAnimeVol();
			m_fTimeRatio=0.0;
		}
	return 1;
}
string* CCloudRender::GetToggleText(){return m_ToggleText;}
bool CCloudRender::GetCurrentState(){return m_bCurrentVolData;}
int CCloudRender::GenerateWindow(GLuint _parentWind){
	int winid=m_TransForm->GenerateWindow(_parentWind);
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutMotionFunc(move);
	glutMouseFunc(mouse);
	glutReshapeFunc(myReshape);
	glutKeyboardFunc(mykeyboard);
	glutSpecialFunc(specialkey);
	glutMouseWheelFunc(wheel);	
	glutJoystickFunc(joystick,10);
	Init();
	return winid;
}
void CCloudRender::InitCamera(){
	float m[16];
	glGetFloatv(GL_PROJECTION_MATRIX,m);
	mProgram->Enable();
	mProgram->UpdateUni("pointsize",m_pt_para->resizeVoxel(m_TransForm,(float)m_Size.xy));
	mProgram->UpdateProjUni(m);
	mProgram->Disable();
	mRainbowProgram->UpdateProjUni(m);
	m_Land->InitCamera(m);
}
CCloudRender::~CCloudRender(void){
	Destroy();
	delete[] m_ucWindTF;
	stringstream str;
	str<<m_threshold;
	WritePrivateProfileString("anjou","threshold",str.str().c_str(),m_IniFile.c_str());
	
}
void CCloudRender::Init(){
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE,&m_Max3DTexSize);
	cout<<"３次元テクスチャの最大サイズ"<<m_Max3DTexSize<<endl;
	string filepath;
	char str[200];
	if(m_Max3DTexSize<256){
			GetPrivateProfileString(m_DataKey[0].c_str(),"lowfile","失敗",str,200,m_IniFile.c_str());
			filepath=str;
		}else{
			GetPrivateProfileString(m_DataKey[0].c_str(),"file","失敗",str,200,m_IniFile.c_str());
			filepath=str;
		}
	 HANDLE handle;
	 DWORD dwnumread;
	handle = CreateFile(filepath.c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	if(handle==INVALID_HANDLE_VALUE){cout<<filepath<<endl;assert(!"ファイルが存在しません");}
	JMeshHeader jmeshhead;
	J3Header j3head;
	ReadFile(handle,&jmeshhead.FileType,sizeof(JMeshHeader),&dwnumread,NULL);
	ReadFile(handle,&j3head.xNum,sizeof(J3Header),&dwnumread,NULL);
	m_Size.xy=j3head.xNum;
	m_Size.z=j3head.zNum;

	cout<<"ボリュームデータのサイズ"<<m_Size.xy<<"×z="<<m_Size.z<<endl;
	
	m_pt_para=new CPointParameter(1.7f,8,m_Size.z);
	ArrayInit(m_Size.total());	
	ReadFile(handle,m_ucIntensity,sizeof(unsigned char)*m_Size.total(),&dwnumread,NULL);
	CloseHandle(handle);
	
	memcpy(m_ucStaticIntensity,m_ucIntensity,sizeof(unsigned char)*m_Size.total());
	InitPoints(m_Static,m_threshold);//ここでm_Staticに値が入る
	for(int i=0;i<m_ValidPtNum;i++){//初期位置を記憶
		m_Dynamic[i]=m_Static[i].pos;
	}


	m_Light->Init();
	m_GausTexSamp.ActiveTexture(GL_TEXTURE0);
	Load8BitmapTexture("gaussian64.bmp",&m_GausTexSamp.tex_id);
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
	glEnableClientState(GL_VERTEX_ARRAY);
	mProgram->AttribPointer(1,GL_FLOAT,sizeof(CParticle),const_cast<float*>(&m_Static[0].intensity));
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT,sizeof(CParticle),&(m_Static[0].pos.x));
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT,sizeof(CParticle),&(m_Static[0].normal.x));
	UpdatePara();
	//虹色モード用のシェーダ
	mRainbowProgram = new CGlsl("rainbow.vert","rainbow.frag");
	//テクスチャ→そのほか、の順でロケーションとること
	mRainbowProgram->GetUniformLocation("ModelView");
	mRainbowProgram->GetUniformLocation("Proj");
	mRainbowProgram->GetUniformLocation("voltex");
	mRainbowProgram->GetUniformLocation("tf");
	mRainbowProgram->Add3DTexture(&m_ucIntensity[0],m_Size.xy,m_Size.z);//断面図を描くのに使うテクスチャ
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	GetPrivateProfileString(m_DataKey[0].c_str(),"tf","失敗",str,200,m_IniFile.c_str());
	mRainbowProgram->Add1DTexture(str);	
	mRainbowProgram->UpdateTextureUniform();
	m_IsoSurface->Init();
	specialkey(GLUT_KEY_PAGE_UP,0,0);//正面を向くようにセット
	
	//下の断面図 2回読んでてマジ無駄だけどあとで～
	 DWORD upper,lower;
	 GetPrivateProfileString(m_DataKey[0].c_str(),"tf","失敗",str,200,m_IniFile.c_str());
	handle = CreateFile(str, GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	lower=GetFileSize(handle,&upper);
	color<float> *tf=new color<float>[lower/sizeof(color<float>)];
	ReadFile(handle,&tf[0].r,lower,&dwnumread,NULL);
	CloseHandle(handle);
	int xy_size=m_Size.xy*m_Size.xy;
	color<float> *projectedTex=new color<float>[xy_size];
	//一番下のだけだと物足りなかったのでmaxとる。
	unsigned char* xyIntensity=new unsigned char[xy_size];
	
	memset(xyIntensity,0,xy_size);
	for(int z=0;z<m_Size.z;z++){
		for(int i=0;i<xy_size;i++){
			if(xyIntensity[i]<m_ucIntensity[z*xy_size+i]){
				xyIntensity[i]=m_ucIntensity[z*xy_size+i];
				projectedTex[i]=tf[m_ucIntensity[z*xy_size+i]];
			}
		}
	}
	m_Land->Init(projectedTex,m_Size.xy);
	
}
void CCloudRender::InitColorDialog(int winid){
	gColorDialog=new GLColorDialog(winid,400,400);
}
void CCloudRender::InitPoints(CParticle* _voxel,unsigned char _thre){

	static vec3<float> sample1;//法線の計算のために必要
	static vec3<float> sample2;
	static vec3<int> magnitude;
	//どの頂点が手前下によって詰め方を変えなくてはいけない。

	int c=0;
	for(int z=0;z<m_Size.z;z++){
		for(int y=0;y<m_Size.xy;y++){
			for(int x=0;x<m_Size.xy;x++){
				int idx,idy,idz;//視線順に並び替えるために必要。
				idx= x;
				idy= y;
				idz= z;
				int index=m_Size.serialId(idx,idy,idz);
				if(m_ucStaticIntensity[index]>=(float)_thre){

					_voxel[c].pos=vec3<float>(
						(float)idx/(float)m_Size.xy-0.5f,
						(float)idy/(float)m_Size.xy-0.5f,
						(float)idz/(float)m_Size.z);
					//法線を計算する
					if(z>0&& y>0 && x>0 && z<m_Size.z-1 && y<m_Size.xy-1 && x<m_Size.xy-1){
						sample1.x=(float)m_ucStaticIntensity[m_Size.serialId(x+1,y,z)];
						sample2.x=(float)m_ucStaticIntensity[m_Size.serialId(x-1,y,z)];
						sample1.y=(float)m_ucStaticIntensity[m_Size.serialId(x,y+1,z)];
						sample2.y=(float)m_ucStaticIntensity[m_Size.serialId(x,y-1,z)];
						sample1.z=(float)m_ucStaticIntensity[m_Size.serialId(x,y,z+1)];
						sample2.z=(float)m_ucStaticIntensity[m_Size.serialId(x,y,z-1)];
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
void CCloudRender::KeyBoard(unsigned char c){
	m_TransForm->KeyBoard(c);
	switch(c){
	/*case '^':
		m_bStopDayTime=true;
		cout<<"6分おきに変わるデータをロードします"<<endl;
		m_bCurrentVolData=1;
		ChangeVolData();
		break;*/
	case '@':
		cout<<"6分おきの時間を流します"<<endl;
		m_bStopDayTime=!m_bStopDayTime;
		break;
	case 'a':
		m_bStopMoment=!m_bStopMoment;
		if(m_bStopMoment){
			cout<<"アニメオフ"<<endl;
		}else{
			cout<<"アニメオン"<<endl;
		}
		break;
	case 'i':
		m_Hide=!m_Hide;
		if(m_Hide){
			cout<<"ボリュームデータの非表示"<<endl;
		}else{
			cout<<"ボリュームデータの表示"<<endl;
		}
		m_IsoSurface->ToggleHide(true);
		break;
	case 'k':
		m_Karmapara+=0.01f;
		printf("生まれ変わりやすさ%f\n",m_Karmapara);
		break;
	case 'K':
		m_Karmapara-=0.01f;
		printf("生まれ変わりやすさ%f\n",m_Karmapara);
		break;
	case 's'://
		m_fWindSpeed-=0.0005f;
		printf("風速%f\n",m_fWindSpeed);
		ChangeWindSpeed();
		break;
	case 'S'://
		m_fWindSpeed+=0.0005f;
		printf("風速%f\n",m_fWindSpeed);	
		ChangeWindSpeed();
		break;
	case 'r':
		printf("粒子の位置をリセット");
		Reset();
		break;
	case '+':
		incrementThreshold(4);
		break;
	case '-':
		incrementThreshold(-4);
		break;
	case 't':
		incrementThreshold(2);
		//m_pt_para->IncrementTransparency(-0.05f,true);
		break;
	case 'T':
		incrementThreshold(-2);
		//m_pt_para->IncrementTransparency(0.05f,true);
		break;
	case 'p':
		m_pt_para->IncrementPointSize(-0.1f,*m_TransForm,true);
		break;
	case 'P':
		m_pt_para->IncrementPointSize(0.1f,*m_TransForm,true);
		break;
	case 'v':
		m_bHideVector=!m_bHideVector;
		cout<<"風ベクトルの表示・非表示切り替え"<<endl;
		break;
	case 'V':
		m_Hide=!m_Hide;
		if(m_Hide){
			cout<<"ボリュームデータの非表示"<<endl;
		}else{
			cout<<"ボリュームデータの表示"<<endl;
		}
		break;
	case 'h':
		PrintKeyHelp();
		break;
	case 'g':
		m_bCapture=!m_bCapture;
		if(m_bCapture){
			cout<<"gifアニメ撮影モードオン\n"<<endl;
		}else{
			cout<<"gifアニメ撮影モードオフ\n"<<endl;}
		break;
	case 'b':
		m_bPrint=!m_bPrint;
		if(m_bPrint){
			cout<<"印刷モード"<<endl;
			mFontColor.set(0.0,0.0,0.0,1.0);
			mBackGround.set(1.0,1.0,1.0,1.0);
		}else{
			printf("背景黒：通常モード\n");
			mBackGround.set(0.0,0.0,0.0,0.0);
			mFontColor.set(1.0,1.0,1.0,1.0);
		}
		break;

	case 'e':
		m_IsoSurface->ToggleMode(true);
		break;
	case 'm':
		cout<<"m:zスケール定規の表示・非表示切り替え"<<endl;
		m_Measure->Toggle();
		break;
	case 'M':
		puts("地図の表示・非表示切り替え");
		m_Land->Toggle();
		break;

	default://親の持ってるキーが反応する
		//cout<<"登録されていないキーです"<<c<<endl;
		break;
	}
	m_Light->Init();
	UpdatePara();
	glutPostRedisplay();
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
bool CCloudRender::Run(void ){
	glClearColor(m_BgColor.r,m_BgColor.g,m_BgColor.b ,m_BgColor.a);//なんでアルファを0.0にするんだっけ
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	m_TransForm->Enable();
	m_Land->SetClipPlane(-1.0f,0.0f,0.0f,m_TransForm->m_Clip.GetVal());
	//脇役達 等値面、地形、ライト、目盛り
	for(int i=0;i<m_nNode;i++){
				m_Nodes[i]->Run();
			}
	
	//主役の雲を一番手前に
	//cout<<"m_bCurrentVolData"<<m_bCurrentVolData<<endl;
	static vec3<float> intersection[6];
	static float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX,m);
	m_bSliceVisible=mBoundingBox.CalcClipPlaneVerts(m_TransForm->getEqn(),intersection);
	if(m_bSliceVisible){//断面の描画
		glEnable(GL_CULL_FACE);
		
		glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER,0.5);
				mRainbowProgram->Enable();
				mRainbowProgram->UpdateModelViewUni(m);
				
				if(!m_bStopMoment){
					mRainbowProgram->Reload3DTexture(&m_ucIntensity[0],m_Size.xy,m_Size.z);
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
	if(!m_Hide){
		//等値面がいるときも断面を出したい、という都合上ここに配置
		mProgram->Enable();
			glEnable(GL_POINT_SPRITE);//これでgl_PointCoordが有効になる これで見た目がかなり変わる。
				//m_pt_para->Uniform();
				//if(gColorDialog->mWaitColorChange){
				//gColorDialog->GetColor(&m_Light->diffuse(),&m_Light->ambient());
				//m_Light->UpdatePara(mProgram);
				//gColorDialog->mWaitColorChange=false;
				//}
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
	
									glDrawArrays(GL_POINTS,0,m_ValidPtNum);

									glVertexPointer(3,GL_FLOAT,sizeof(vec3<float>),&(m_Dynamic[0].x));
									glDrawArrays(GL_POINTS,0,m_ValidPtNum);
	
								glDisableClientState(GL_VERTEX_ARRAY);
								glDisableClientState(GL_NORMAL_ARRAY);
	
							glDisable(GL_TEXTURE_2D);
						glDisable(GL_BLEND);
					glDepthMask(GL_TRUE);
				glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
			glDisable(GL_POINT_SPRITE);
		
		glUseProgram(0);
	}
	
	DrawWindVector();
	AnimateRandomFadeOut();//風ベクトルに沿って動かす
	BlendTime();//0～6分後のブレンドする
	m_TransForm->Disable();
	return 0;
}
void CCloudRender::Reset(){
		for(int i=0;i<m_ValidPtNum;i++){//初期位置を記憶
			m_Dynamic[i]=m_Static[i].pos;
		}
		//断面テクスチャも元に戻す
	memcpy(m_ucIntensity,m_ucStaticIntensity,sizeof(unsigned char)*m_Size.total());

}
void CCloudRender::RootMenu(int val){
	switch(val){
	case 6:
		printf("empty menu\n");
		break;
	case 7:
		/*gColorDialog->mDiffuseLabel.mColor=m_Light->diffuse();
		gColorDialog->mAmbientLabel.mColor=m_Light->ambient();
		gColorDialog->show();//ここで現在の色を渡せばいい。*/
		break;
	case 8:
		//m_wind->ToggleHide(true);
		break;
	case 4:
		//			mShowStaticVolume=!mShowStaticVolume;
		break;
	case 5:
		//			mShowMovingVolume=!mShowMovingVolume;
		break;
	}
}
void CCloudRender::JoyStick(unsigned int buttonmask, int x, int y, int z){
	/*if(buttonmask==START){ 
		mStartFlightFlag=!mStartFlightFlag;
	}
	if(buttonmask==SELECT){
		PrintTransformInfo();
		//move to flight start point
		mClipPlane=0.0;mClipEqn[3]=mClipPlane; glClipPlane(GL_CLIP_PLANE0,mClipEqn);
		//front viewにする
		m_ModelView->Front();
		SetTranslateVec(0.0,0.06,-1.35);

		mStartFlightFlag=true;
		printf("スタート地点へ\n");
		m_stirizer->SetAnimate(true);
		zScale=0.3;
		}
	if(buttonmask==L){//L
		//mRad+=M_PI/1800.0;
	}
	if(buttonmask==R){//R
		//mRad-=M_PI/1800.0;
	}
	float zincrement=0.0;
	if(mStartFlightFlag){
		zincrement=-0.003;
	}
	IncrementTranslateVec((x/1000)*0.003,(y/1000)*0.003,zincrement);*/
	glutPostRedisplay();

}

void CCloudRender::PrepareAnimeVol(){
	//--------------------------------------------------------------------------/
	// Function		/ PrepareAnimeVol
	//--------------------------------------------------------------------------/
	// Abstruct		/ 
	// Return		/ 
	// Arguments	/ 
	//				/                                                           /
	// History		/ 2013.02.22 Minako Yamahiro First Edition					/
	//--------------------------------------------------------------------------/
	static stringstream filename;
	//このファイルの読み方って、もしかして遅い？？？WinAPIの方がいいのかな。
	filename.str("");
	filename<<"../data/cappi/cappi"<<m_nFrame<<".jmesh";
	static HANDLE handle;
	static DWORD dwnumread;
	handle = CreateFile(filename.str().c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	SetFilePointer(handle,sizeof(JMeshHeader)+sizeof(J3Header),NULL,FILE_BEGIN);
	ReadFile(handle,m_ucStaticIntensity,sizeof(unsigned char)*m_Size.total(),&dwnumread,NULL);
	CloseHandle(handle);
	
	InitPoints(mBeforeVoxel,0);
	m_nFrame++;
	cout<<m_nFrame<<"フレーム目"<<endl;
	if(m_nFrame>163){m_nFrame=24;}
	
	//nFrame++した後のを読み込む
	filename.str("");
	filename<<"../data/cappi/cappi"<<m_nFrame<<".jmesh";
	handle = CreateFile(filename.str().c_str(), GENERIC_READ,FILE_SHARE_READ, NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL );
	SetFilePointer(handle,sizeof(JMeshHeader)+sizeof(J3Header),NULL,FILE_BEGIN);
	ReadFile(handle,m_ucStaticIntensity,sizeof(unsigned char)*m_Size.total(),&dwnumread,NULL);
	CloseHandle(handle);

	InitPoints(mAfterVoxel,0);
	//6分おきのデータは各風データを持っている
	//LoadVelocityData();
	filename.str("");
	filename<<"../data/vvp/vvp"<<m_nFrame<<".jmesh";
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
void CCloudRender::Toggle(){
	m_Land->ToggleMap();
	
	//解像度の良いデータか、6分おきのデータか切り替える
	m_bCurrentVolData=!m_bCurrentVolData;
	ChangeVolData();
	//if(m_Cloud->m_bCurrentVolData){//6分おき用の地図データの場合（小牧）
	//	m_Land->ChangeData("../data/map/komakimap100.mesh",vec3<float>((-136.9293),(-35.2621),0.0),"../data/map/komakimap.bmp");
	//	m_Land->ChangeData("../data/map/map100.mesh",vec3<float>((-139.406984f),(-35.8431139f),0.0f),"../data/map/map.bmp");
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
