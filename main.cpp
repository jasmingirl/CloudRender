#include <sstream>
#include <bitset>
#include <AntTweakBar.h>
#include "CloudRender.h"
//OpenCV関係 画面キャプチャに必要
#include <opencv/cv.h>
#include <opencv/highgui.h>//cvLoadImageに必要
#include <opencv2/core/core.hpp>
#pragma comment(lib,"opencv_highgui240d.lib")
#pragma comment(lib,"opencv_core240d.lib")
using namespace std;
HANDLE g_Sem;
CCloudRender* g_Render=0;//グローバル変数にする必要性。ファイルロードを分離すれば、、その必要はないかも。
float g_Quaternion[4]={0.0f,0.0f,0.0f,1.0f};//オブジェクト回転に使うxyzw
POINT g_lastPushed;//マウスを押した位置
bool g_bNowRotating=false;
bool g_NowGUITouching=false;//GUIを触っている時、雲オブジェクトが回転してほしくない。
void GLFWCALL Resize(int width, int height)
{
	TwWindowSize(width, height);
}
 void GLFWCALL OnMousePos(int mouseX, int mouseY)  // your callback function called by GLFW when mouse has moved
  {
	  if(g_NowGUITouching= TwEventMousePosGLFW(mouseX, mouseY)){
		  g_Render->m_TransForm->RotateFromQuat(g_Quaternion);
	  }    
  }
 void GLFWCALL Wheel( int pos ){
	g_NowGUITouching= TwEventMouseWheelGLFW(pos);
 }
 void GLFWCALL MouseButton( int button, int action ){
	g_NowGUITouching=  TwEventMouseButtonGLFW(button,action);
 }
 void GLFWCALL Key( int key, int action ){
	 g_NowGUITouching= TwEventKeyGLFW(key,action);
 }
 void GLFWCALL Char( int character, int action ){
	 g_NowGUITouching= TwEventCharGLFW(character,action);
 }
void TW_CALL Reset(void *_render){
	CCloudRender *render = static_cast<CCloudRender *>(_render);
	render->Reset();

}

void FileLoad(void *arg){	
	OutputDebugString("Fileのはじまり");
	WaitForSingleObject(g_Sem,INFINITE);
	while(g_Render!=0 ){
		
		if(g_Render->CheckFlag(CCloudRender::MY_VOLDATA)){
		g_Render->FileLoad();
		}
	}

}

///< スレッド関数の引数データ構造体
typedef struct _thread_arg {
	int thread_no;
	int *data;
} thread_arg_t;
void MyTwInit(bool *_flags1,bool* _flags2){
	// Initialize AntTweakBar
	TwInit(TW_OPENGL, NULL);
	// Create a tweak bar
	TwBar *bar = TwNewBar("Main");
	TwDefine(" Main fontSize=3  label='Control Pane'  color='0 0 0' alpha=50 size='340 800' position='22 22' "); 

	//デバッグ用
	enum{
		MY_DRAW_BG,///< 背景のグラデーション板を書くかどうか　
		MY_PRINT,///< 印刷モードかどうか
		MY_ORTHOGONAL///< 1:正投影モード　0:透視投影モード　
	};
	enum{
		MY_CLOUD,///<   雲の表示・非表示切り替え
		MY_LAND,///< 地図を表示するかどうか
		MY_ANIMATE,///<  局所的な風を止めるか、止めないか
		MY_MEASURE,///< 外側の箱を表示するかどうか
		MY_RED_POINT,///< 赤い点を表示するかどうか
		MY_ISOSURFACE,///< 等値面を表示するかどうか
		MY_CAPTURE,///< 動画キャプチャモードなのかどうか キャプチャーモードは1フレーム1秒になる
		MY_FLOW_DAYTIME,///< 大域時間を流すかどうか 6分おきの時間の流れを止めるかどうか この変数はm_bCurrentVolData=1のときだけ働きたい
		MY_VECTOR,///<  風ベクトルの表示・非表示
		MY_FLYING,///< 飛行機運転モードのスタート
		MY_VOLDATA,///<  解像度の良い雲0か、6分おきに変わる雲1か GUIからアクセスする関係上ここに配置
		MY_YELLOW_POINTS,///< 黄色い点を表示するかどうか
		MY_TIME_MEASURE,///< 時間測定をスタートするかどうか
		MY_DYNAMIC_CLOUD,///< 風で動く雲を表示するかどうか
		MY_STATIC_CLOUD,///< 常に止まっている雲を表示するかどうか
		MY_ANIMATE_YELLOW///< 黄色い点を動かすかどうか
	};
	TwAddVarRW(bar, "cloud", TW_TYPE_BOOLCPP, &_flags1[MY_CLOUD], "group=Debug  key=V help='display cloud or not' "); 
	TwAddVarRW(bar, "moving cloud", TW_TYPE_BOOLCPP, &_flags1[MY_DYNAMIC_CLOUD], "group=Debug"); 
	TwAddVarRW(bar, "static cloud", TW_TYPE_BOOLCPP, &_flags1[MY_STATIC_CLOUD], "group=Debug"); 
	TwAddVarRW(bar, "zscale", TW_TYPE_FLOAT, &g_Render->m_ZScale,"group=Appearance label='Height Scale' min=1.0 max=7.0 step=0.1 keyIncr=z keyDecr=Z help='Height Scale' ");
	TwAddVarRW(bar, "clip", TW_TYPE_DOUBLE, &g_Render->m_ClippingEquation[3],"group=Appearance  label='Clipping Position' min=-0.5 max=1.0 step=0.01 help='Clipping Position'  keyIncr=L keyDecr=l  ");
	//zの時だけ範囲を変えたいなーー。
	// Create a new TwType called rotationType associated with the Scene::RotMode enum, and use it
	
	TwEnumVal clip_direction[] = {{ 0, "X"}, 
								  { 1, "Y"}, 
								  { 2, "Z"}};
	 TwType direction_type=TwDefineEnum( "Clip Direction", clip_direction, 3 );
	 TwAddVarRW(bar, "Clipping Direction", direction_type, &g_Render->m_ClipDirection, 
               " group='Appearance' key=d ");
	 TwEnumVal scene[] = {  { 0, "Morning"}, //朝　昼　夕　晩　の選択肢
							{ 1, "Noon"}, 
							{ 2, "Twilight"},
							{ 3, "Evening"}};
	TwType scene_type=TwDefineEnum( "Scene", scene, 4 );
	TwAddVarRW(bar, "Scene", scene_type, &g_Render->m_SceneId,  " group='Appearance'  ");
	TwAddVarRW(bar, "Clipping Direction", direction_type, &g_Render->m_ClipDirection,  " group='Analyze'  ");
	TwAddVarRW(bar, "ObjRotation", TW_TYPE_QUAT4F, &g_Quaternion, "group=Appearance  label='Object rotation' opened=true help='Change the object orientation.' ");
	TwAddVarRW(bar, "transparency", TW_TYPE_FLOAT, &g_Render->m_PtPara->mTransParency, " min=0.01 max=10.0 step=0.01 label='transparency' group=Appearance"); 
	TwAddVarRW(bar, "pointsize", TW_TYPE_FLOAT, &g_Render->m_PtPara->mPointSize, " min=1 max=20.0 step=1 label='Point Size' group=Appearance"); 
	
	TwAddVarRW(bar, "animate", TW_TYPE_BOOLCPP, &_flags1[MY_ANIMATE], "group=Wind key=a help='Toggle wind blow on or off.' "); 
	TwAddVarRW(bar, "karma", TW_TYPE_FLOAT, &g_Render->m_Karmapara, "group=Wind min=0.0 max=0.3 step=0.001 help='bigger is easy to reborn.'"); 
	TwAddVarRW(bar, "redpt", TW_TYPE_BOOLCPP, &_flags1[MY_RED_POINT], "label='red point' group=Wind"); 
	TwAddVarRW(bar, "yellows", TW_TYPE_BOOLCPP, &_flags1[MY_YELLOW_POINTS], "label='yellow points' group=Wind"); 
	TwAddVarRW(bar, "yellowanime", TW_TYPE_BOOLCPP, &_flags1[MY_ANIMATE_YELLOW], "label='animate' group=Wind"); 
	TwAddButton(bar, "reset", Reset, g_Render," label='reset anime' group=Wind");

	TwAddVarRW(bar, "printmode", TW_TYPE_BOOLCPP, &_flags2[MY_PRINT], "group=Utility label='mode' true='print' false='display' "); 
	TwAddVarRW(bar,"capture",TW_TYPE_BOOLCPP,&_flags1[MY_CAPTURE],"group=Utility label='capture'");
	TwAddVarRW(bar, "viewmode", TW_TYPE_BOOLCPP,&_flags2[MY_ORTHOGONAL], "group=Utility label='view mode' true='Orthogonal' false='Perspective'"); 
	TwAddVarRW(bar, "map", TW_TYPE_BOOLCPP,&_flags1[MY_LAND], "group=Display"); 
	TwAddVarRW(bar, "sky", TW_TYPE_BOOLCPP,&_flags2[MY_DRAW_BG], "group=Display"); 
	TwAddVarRW(bar, "measure", TW_TYPE_BOOLCPP, &_flags1[MY_MEASURE], "group=Display "); 

	TwAddVarRW(bar, "isosurface", TW_TYPE_BOOLCPP, &_flags1[MY_ISOSURFACE], "group=Display key=m help='display wind vector or not' "); 
	TwAddVarRW(bar, "vector", TW_TYPE_BOOLCPP, &_flags1[MY_VECTOR], "group=Display label='wind vector' key=v help='display wind vector or not' "); 
	//6分おきデータの時だけ登場するコントロール達
	TwAddVarRW(bar, "voldata", TW_TYPE_BOOLCPP, &_flags1[MY_VOLDATA], "group=DayTime label='change voldata' key=^ help='change voldata blow on or off.' "); 
	TwAddVarRW(bar, "daytime", TW_TYPE_BOOLCPP, &_flags1[MY_FLOW_DAYTIME], "group=DayTime label='daytime' key=v help='flow daytime' "); 
	
}
void capture(int _width,int _height)
	{
		
		cv::Mat cvmtx(cv::Size(_width,_height), CV_8UC3,cv::Scalar(0,0,0));//黒で初期化
		// 画像のキャプチャ
		glReadBuffer(GL_FRONT);// フロントを読み込む様に設定する
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // 初期値は4
		glReadPixels(0, 0, _width, _height,GL_BGR,GL_UNSIGNED_BYTE,(void*)cvmtx.data);
		//上下逆にする
		cv::flip(cvmtx,cvmtx,0);
		/* 画像の書き出し */
		cv::imwrite("capture.png", cvmtx);
	}
void Render(void *arg){
	// ウィンドウを作成する
	
	glfwInit();
	glfwOpenWindowHint(GLFW_FSAA_SAMPLES,4);//マルチサンプリング有効
	glfwOpenWindow( 1024,768, 0,0,0,0,0,0, GLFW_WINDOW );
    glfwSetWindowTitle("Cloud");
	glewInit();
	

	g_Render=new CCloudRender(vec2<int>(200,0));
	
	//レンダリングウィンドウ
	g_Render->Init();
	
											//このフラグセットはやりにくいなぁ。AntTweakBarのせいだよね？
										    // 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14
	bool miffyflag[CCloudRender::MY_FLAG_NUM]={1,1,0,0,0,0,0,0,0,0,0,0,0,1,1};
	bool traflag[CTransForm::MY_FLAG_NUM]={1,0,0};
	
	MyTwInit(miffyflag,traflag);
	bitset<CCloudRender::MY_FLAG_NUM> myflag;//8ビットのビット列を作り、値としてcを入れる。basic_string<bool>という手もあるけど、to_ulongが使えるからbitsetにした
	bitset<CTransForm::MY_FLAG_NUM> traflagbits;
	glfwSetWindowSizeCallback(Resize);
	glfwSetMouseButtonCallback(MouseButton);
    glfwSetMousePosCallback(OnMousePos);
	glfwSetMouseWheelCallback(Wheel);
	glfwSetKeyCallback(Key);
	glfwSetCharCallback(Char);
	//入力がらみが変になっちゃったー
	vec2<int> tra_lastpush;
	while (glfwGetWindowParam(GLFW_OPENED))//ウィンドウが開いている場合
	{
		int w,h;
		glfwGetWindowSize(&w,&h);
		
		g_Render->m_TransForm->Reshape(w,h);
		//画面からの回転操作
		int mx,my;
        glfwGetMousePos(&mx,&my);
		if(!g_NowGUITouching){//AntTweakBarを触っていない間だけ回転させる
			if(glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT)){
				g_bNowRotating=true;
				g_Render->m_TransForm->RotateFromScreen(mx,my);
			}else{//回転やめ
				g_bNowRotating=false;
				g_Render->m_TransForm->m_last_pushed.set(mx,my);
				g_Render->m_TransForm->m_CurrentQuaternion=g_Render->m_TransForm->m_TargetQuaternion;
				g_Quaternion[0]= g_Render->m_TransForm->m_CurrentQuaternion.x;
				g_Quaternion[1]= g_Render->m_TransForm->m_CurrentQuaternion.y;
				g_Quaternion[2]= g_Render->m_TransForm->m_CurrentQuaternion.z;
				g_Quaternion[3]= g_Render->m_TransForm->m_CurrentQuaternion.w;
			}
			if(glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)){//平行移動
				g_Render->m_TransForm->TranslateFromScreeen(
					(float)(mx-tra_lastpush.x)/(float)w,
			(float)(my-tra_lastpush.y)*-1.0f/(float)h);
				tra_lastpush.set(mx,my);
			}else{
				tra_lastpush.set(mx,my);
			}
		}//end if もしAntTweakBarを触っていないならば
		g_Render->m_TransForm->zoom(glfwGetMouseWheel());
		//フラグを渡す
		for(int i=0;i<CCloudRender::MY_FLAG_NUM;i++){//あまりスマートなやり方じゃない。
			myflag[i]=miffyflag[i];
		}
		g_Render->SetFlag(myflag.to_ulong());
		for(int i=0;i<CTransForm::MY_FLAG_NUM;i++){//あまりスマートなやり方じゃない。
			traflagbits[i]=traflag[i];
		}
		g_Render->m_TransForm->SetFlag(traflagbits.to_ulong());
		//更新
		g_Render->CheckParameterChange();
		g_Render->m_TransForm->CheckParameterChange();
		if(!g_bNowRotating){
			g_Render->m_TransForm->RotateFromQuat(g_Quaternion);
		}
		//ostringstream cdbg;
		//描画
		g_Render->Draw();//6分おきの等値面の時、ここでエラー
		TwDraw();
		if(g_Render->CheckFlag(CCloudRender::MY_CAPTURE)){
			Sleep(300);
		}
		if(glfwGetKey('C')){capture(w,h);}
		glfwSwapBuffers();
		ReleaseSemaphore(g_Sem,1,NULL);
	}
	delete g_Render;
	TwTerminate();
	
}
int  main(int argc, char *argv[])
{
	glutInit(&argc,argv);
	thread_arg_t ftarg;
	thread_arg_t rtarg;

	g_Sem = CreateSemaphore(NULL,0,1,NULL);
	ftarg.thread_no = 0;
	HANDLE filegetHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)FileLoad,(void *)&ftarg,0,NULL);
	SetThreadIdealProcessor(filegetHandle,0);

	rtarg.thread_no = 0;
	HANDLE renderHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Render,(void *)&rtarg,0,NULL);
	SetThreadIdealProcessor(renderHandle,1);


	WaitForSingleObject(renderHandle,INFINITE);
	cout<<"end root"<<endl;

	return 0;
}
