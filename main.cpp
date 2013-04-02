#include <sstream>
#include <bitset>
#include <AntTweakBar.h>
#include "CloudRender.h"
using namespace std;
HANDLE g_Sem;
CCloudRender* g_Render=0;//グローバル変数にする必要性。ファイルロードを分離すれば、、その必要はないかも。
float g_Quaternion[4]={0.0f,0.0f,0.0f,1.0f};//オブジェクト回転に使うxyzw
POINT g_lastPushed;//マウスを押した位置
HDC g_hdc;// ウィンドウプロシージャでも再描画命令出したいので外に出した
int g_TitleHeight=22;//タイトルバーの高さ 22かな。
void TW_CALL Reset(void *_render){
	CCloudRender *render = static_cast<CCloudRender *>(_render);
	render->Reset();

}
POINT g_WinPos={0,100};
POINT g_WinSize={1024,768};
//スクリーン座標系
RECT g_ClientRect={g_WinPos.x,g_WinPos.y,g_WinPos.x+g_WinSize.x,g_WinPos.y+g_WinSize.y};
//サイズ変更枠検知領域 右下 クライアント座標
RECT g_ResizableRegion={g_WinSize.x-8,g_WinSize.y-8,g_WinSize.x,g_WinSize.y};
bool g_bNowRotating=false;
// ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
	static int wheel_sum=0;
	ostringstream cdbg;

	if(TwEventWin(hWnd, msg, wp, lp)) {// send event to AntTweakBar
		return 0;
	}
	
	switch( msg )
	{
	case WM_NCMOUSEMOVE:
		if(wp==HTCAPTION){
			OutputDebugString("キャプションだよーーー\n");
			SetCursor((HCURSOR)LoadImage(// マウスカーソルを十字にして動かせることを示唆する。
			NULL, MAKEINTRESOURCE(IDC_SIZEALL), IMAGE_CURSOR,
			0, 0, LR_DEFAULTSIZE | LR_SHARED
			));
		}
		return 0;
	case WM_NCLBUTTONDOWN:
		if(wp==HTCAPTION){
			cdbg<<"キャプション押しx="<<LOWORD(lp)<<",y="<<HIWORD(lp)<<endl;
			OutputDebugString(cdbg.str().c_str());
			SetCursor((HCURSOR)LoadImage(// マウスカーソルを十字にして今動かせることをアピール
			NULL, MAKEINTRESOURCE(IDC_SIZEALL), IMAGE_CURSOR,
			0, 0, LR_DEFAULTSIZE | LR_SHARED
			));
		}
		return DefWindowProc( hWnd, msg, wp, lp );
		
	//case WM_ENTERSIZEMOVE:
	//	return 0;
	//case WM_EXITSIZEMOVE:
	//	SetCursor((HCURSOR)LoadImage(      // マウスカーソル
	//	NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR,
	//	0, 0, LR_DEFAULTSIZE | LR_SHARED
	//	));
	//	return 0;
	case WM_NCHITTEST://マウス カーソルが移動したことを示します。スクリーン座標で来るんだよね。
		POINT m;
		m.x=LOWORD(lp);
		m.y=HIWORD(lp);
		ScreenToClient(hWnd,&m);//クライアント座標に直す
		if((m.y)<g_TitleHeight){
			OutputDebugString("HTCAPTION!!!\n");
			return HTCAPTION;}
		//サイズ変更枠領域である
		if(PtInRect(&g_ResizableRegion,m)){
			return HTBOTTOMRIGHT;
		}
		return DefWindowProc( hWnd, msg, wp, lp );
	case WM_MOVE:
		g_ClientRect.left=LOWORD(lp);
		g_ClientRect.top=HIWORD(lp);
		return 0;
	case WM_CHAR:       
		if( wp==VK_ESCAPE )
			PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN://左ボタンダウン
		g_bNowRotating=true;
		g_lastPushed.x=LOWORD(lp);//位置を記録するだけ
		g_lastPushed.y=HIWORD(lp);
		break;
	case WM_MOUSEMOVE:
		if(g_bNowRotating){
			int dx=LOWORD(lp)-g_lastPushed.x;
			int dy=HIWORD(lp)-g_lastPushed.y;
			g_Render->m_TransForm->RotateFromScreen(dx,dy);
			break;
		}else{
			return DefWindowProc(hWnd,msg,wp,lp);
		}
		break;
	case WM_LBUTTONUP://左ボタンアップ
		g_bNowRotating=false;
		g_lastPushed.x=LOWORD(lp);//位置を記録するだけ
		g_lastPushed.y=HIWORD(lp);
		g_Render->m_TransForm->SaveRotationState();
		g_Render->m_TransForm->m_CurrentQuaternion.toXYZW(g_Quaternion);
		break;
	case WM_MOUSEWHEEL://マウスホイール
		wheel_sum+=GET_WHEEL_DELTA_WPARAM(wp)/120;
		g_Render->m_TransForm->zoom(wheel_sum);
		return 0;
	case WM_CREATE:
		OutputDebugString("WM_CREATE\n");
		return 0;
	case WM_SIZING:
		memcpy(&g_ClientRect,(RECT*)lp,sizeof(RECT));
		g_WinSize.x=g_ClientRect.right-g_ClientRect.left;
		g_WinSize.y=g_ClientRect.bottom-g_ClientRect.top;
		g_ResizableRegion.left=g_WinSize.x-8;
		g_ResizableRegion.top=g_WinSize.y-8;
		g_ResizableRegion.right=g_WinSize.x;
		g_ResizableRegion.bottom=g_WinSize.y;
		g_Render->m_TransForm->Reshape(((RECT*)lp)->right-((RECT*)lp)->left,((RECT*)lp)->bottom-((RECT*)lp)->top);
		return 0;

	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;
	}

	return DefWindowProc( hWnd, msg, wp, lp );
}
void FileLoad(void *arg){	
	cout<<"Fileのはじまり"<<endl;
	WaitForSingleObject(g_Sem,INFINITE);

	while(g_Render!=0){
		//printf("file run\n");
		g_Render->FileLoad();
	}
}
/*!
@param _hglrc 後でDeleteしたいから引数にした
*/
int CreateOpenGLContext(HWND _hwnd,HGLRC* _hglrc,HDC* _hdc){
	// OpenGL初期化
	// ピクセルフォーマット初期化
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, //Flags
		PFD_TYPE_RGBA, //RGBAモードか、インデックスカラーモードか
		32, //フレームバッファの色深度
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24, //Number of bits for the depthbuffer
		8, //Number of bits for the stencilbuffer
		0, //Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};
	*_hdc= GetDC(_hwnd);//HDCの取得
	//デバイスコンテキストにピクセルフォーマットを設定する
	int format = ChoosePixelFormat(*_hdc, &pfd);

	// OpenGLが使うデバイスコンテキストに指定のピクセルフォーマットをセット
	SetPixelFormat(*_hdc, format, &pfd);

	// OpenGL contextを作成
	*_hglrc = wglCreateContext(*_hdc);

	// 作成されたレンダリングコンテキストがカレントにする
	wglMakeCurrent(*_hdc, *_hglrc);//これないとエラーになる
	return 1;
}



// ウィンドウを作成する
HWND WindowCreate(HINSTANCE hInst)
{
	WNDCLASSEX wc;

	// ウィンドウクラスの情報を設定
	wc.cbSize = sizeof(wc);               // 構造体サイズ
	wc.style = CS_HREDRAW | CS_VREDRAW;   // スタイル
	wc.lpfnWndProc = WndProc;             // ウィンドウプロシージャ
	wc.cbClsExtra = 0;                    // 拡張情報１
	wc.cbWndExtra = 0;                    // 拡張情報２
	wc.hInstance = hInst;                 // インスタンスハンドル
	wc.hIcon = (HICON)LoadImage(          // アイコン
		NULL, MAKEINTRESOURCE(IDI_APPLICATION), IMAGE_ICON,
		0, 0, LR_DEFAULTSIZE | LR_SHARED
		);
	wc.hIconSm = wc.hIcon;                // 子アイコン
	wc.hCursor = (HCURSOR)LoadImage(      // マウスカーソル
		NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR,
		0, 0, LR_DEFAULTSIZE | LR_SHARED
		);
	wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH); // ウィンドウ背景
	wc.lpszMenuName = NULL;                     // メニュー名
	wc.lpszClassName = "Default Class Name";// ウィンドウクラス名

	// ウィンドウクラスを登録する
	if( RegisterClassEx( &wc ) == 0 ){ return NULL; }

	// ウィンドウを作成する
	return CreateWindow(
		wc.lpszClassName,      // ウィンドウクラス名
		"ウィンドウのタイトル",  // タイトルバーに表示する文字列
		WS_POPUP ,   // ウィンドウの種類WS_POPUP |WS_VISIBLE,WS_OVERLAPPED,//
		g_WinPos.x,         // ウィンドウを表示する位置（X座標）
		g_WinPos.y,         // ウィンドウを表示する位置（Y座標）
		g_WinSize.x,         // ウィンドウの幅
		g_WinSize.y,         // ウィンドウの高さ
		NULL,                  // 親ウィンドウのウィンドウハンドル
		NULL,                  // メニューハンドル
		hInst,                 // インスタンスハンドル
		NULL                   // その他の作成データ
		);
}
///< スレッド関数の引数データ構造体
typedef struct _thread_arg {
	int thread_no;
	HINSTANCE hInst;
} thread_arg_t;
//メトロ風UIにするための小細工
void MyMetroUI(){
	//MetroUI風描画する
		
		glMatrixMode(GL_PROJECTION);	
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0,g_WinSize.x,g_WinSize.y,0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		//gluLookAt(0,0,m_Zoom,0,0,0,0.0f,1.0f,0.0f);
		//タイトルバーもどきを描画
		
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front
		glDisable(GL_CULL_FACE);
		glColor4ub(0,0,0,128);
		glRectf(0,0,g_WinSize.x,g_TitleHeight);	
		//サイズ変更可能ボタンもどきを描画
			glBegin(GL_TRIANGLES);
			glVertex2i(g_ResizableRegion.left,g_ResizableRegion.bottom);
			glVertex2i(g_ResizableRegion.right,g_ResizableRegion.bottom);
			glVertex2i(g_ResizableRegion.right,g_ResizableRegion.top);
			glEnd();
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		
		glMatrixMode(GL_PROJECTION);	
		glPopMatrix();
}
void MyTwInit(bool *_flags1,bool* _flags2){
	// Initialize AntTweakBar
	TwInit(TW_OPENGL, NULL);
	// Create a tweak bar
	TwBar *bar = TwNewBar("Main");
	TwDefine(" Main label='Control Pane'  color='0 0 0' alpha=50 size='240 600' position='22 22' "); 

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
	TwAddVarRW(bar, "zscale", TW_TYPE_FLOAT, &g_Render->m_ZScale,"group=Appearance label='Height Scale' min=0.0703125 max=1.4 step=0.1 keyIncr=z keyDecr=Z help='Height Scale' ");
	TwAddVarRW(bar, "clip", TW_TYPE_DOUBLE, &g_Render->m_TransForm->m_ClippingEquation[3],"group=Appearance  label='Clipping Position' min=-0.5 max=0.5 step=0.01 keyIncr=; keyDecr=L help='Clipping Position' ");

	TwAddVarRW(bar, "ObjRotation", TW_TYPE_QUAT4F, &g_Quaternion, "group=Appearance  label='Object rotation' opened=true help='Change the object orientation.' ");
	TwAddVarRW(bar, "animate", TW_TYPE_BOOLCPP, &_flags1[MY_ANIMATE], "group=Wind key=a help='Toggle wind blow on or off.' "); 
	TwAddVarRW(bar, "karma", TW_TYPE_FLOAT, &g_Render->m_Karmapara, "group=Wind min=0.0 max=0.3 step=0.001 help='bigger is easy to reborn.'"); 
	TwAddVarRW(bar, "redpt", TW_TYPE_BOOLCPP, &_flags1[MY_RED_POINT], "label='red point' group=Wind"); 
	TwAddVarRW(bar, "yellows", TW_TYPE_BOOLCPP, &_flags1[MY_YELLOW_POINTS], "label='yellow points' group=Wind"); 
	TwAddVarRW(bar, "yellowanime", TW_TYPE_BOOLCPP, &_flags1[MY_ANIMATE_YELLOW], "label='animate' group=Wind"); 
	TwAddButton(bar, "reset", Reset, g_Render," label='reset anime' group=Wind");

	TwAddVarRW(bar, "voldata", TW_TYPE_BOOLCPP, &_flags1[MY_VOLDATA], "group=VolData label='change voldata' key=^ help='change voldata blow on or off.' "); 
	TwAddVarRW(bar, "printmode", TW_TYPE_BOOLCPP, &_flags2[MY_PRINT], "group=Utility label='mode' true='print' false='display' "); 
	TwAddVarRW(bar, "viewmode", TW_TYPE_BOOLCPP,&_flags2[MY_ORTHOGONAL], "group=Utility label='view mode' true='Orthogonal' false='Perspective'"); 
	TwAddVarRW(bar, "map", TW_TYPE_BOOLCPP,&_flags1[MY_LAND], "group=Display"); 
	TwAddVarRW(bar, "sky", TW_TYPE_BOOLCPP,&_flags2[MY_DRAW_BG], "group=Display"); 
	TwAddVarRW(bar, "measure", TW_TYPE_BOOLCPP, &_flags1[MY_MEASURE], "group=Display "); 

	TwAddVarRW(bar, "isosurface", TW_TYPE_BOOLCPP, &_flags1[MY_ISOSURFACE], "group=Display key=m help='display wind vector or not' "); 
	TwAddVarRW(bar, "vector", TW_TYPE_BOOLCPP, &_flags1[MY_VECTOR], "group=Display label='wind vector(corrupted)' key=v help='display wind vector or not' "); 
	//6分おきデータの時だけ登場するコントロール達
	TwAddVarRW(bar, "daytime", TW_TYPE_BOOLCPP, &_flags1[MY_FLOW_DAYTIME], "group=DayTime label='daytime' key=v help='' "); 
	
}
void Render(void *arg){
	// ウィンドウを作成する
	HWND hWnd = WindowCreate( ((thread_arg_t*)arg)->hInst );
	HGLRC hglrc;

	CreateOpenGLContext(hWnd,&hglrc,&g_hdc);
	// ウィンドウを表示する
	ShowWindow( hWnd, SW_SHOW );
	glfwInit();
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
	MSG msg;
	g_TitleHeight=GetSystemMetrics(SM_CYCAPTION);
	//最初のリシェイプ
	wglMakeCurrent(g_hdc, hglrc);
	RECT rc;
	GetClientRect(hWnd,&rc);
	g_Render->m_TransForm->Reshape(rc.right,rc.bottom);
	while (GetMessage( &msg, NULL, 0, 0))//ウィンドウが開いている場合
	{
		//入力
		//更新
		g_Render->CheckParameterChange();
		g_Render->m_TransForm->CheckParameterChange();
		if(!g_bNowRotating){g_Render->m_TransForm->RotateFromQuat(g_Quaternion);}
		ostringstream cdbg;
		// メッセージを処理する
		TranslateMessage( &msg );
		DispatchMessage( &msg );
		 cdbg<<"メッセージ"<<msg.message<<endl;
		 OutputDebugString(cdbg.str().c_str());
		//フラグを渡す
		for(int i=0;i<CCloudRender::MY_FLAG_NUM;i++){//あまりスマートなやり方じゃない。
			myflag[i]=miffyflag[i];
		}
		g_Render->SetFlag(myflag.to_ulong());
		for(int i=0;i<CTransForm::MY_FLAG_NUM;i++){//あまりスマートなやり方じゃない。
			traflagbits[i]=traflag[i];
		}
		g_Render->m_TransForm->SetFlag(traflagbits.to_ulong());

		//描画
		g_Render->Draw();
		TwDraw();
		MyMetroUI();
		SwapBuffers(g_hdc);
		

		ReleaseSemaphore(g_Sem,1,NULL);
	}
	delete g_Render;
	TwTerminate();
	wglMakeCurrent(NULL, NULL);// カレントコンテキストを無効にする
	wglDeleteContext(hglrc);// カレントコンテキストを削除
	ReleaseDC(hWnd, g_hdc);// デバイスコンテキスト解放

}
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine, int showCmd)
{

	thread_arg_t ftarg;
	thread_arg_t rtarg;

	rtarg.hInst=hInst;

	g_Sem = CreateSemaphore(NULL,0,1,NULL);
	ftarg.thread_no = 0;
	HANDLE fileget = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)FileLoad,(void *)&ftarg,0,NULL);
	SetThreadIdealProcessor(FileLoad,0);

	rtarg.thread_no = 0;
	HANDLE render = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Render,(void *)&rtarg,0,NULL);
	SetThreadIdealProcessor(render,1);


	WaitForSingleObject(render,INFINITE);
	cout<<"end root"<<endl;

	return 0;
}
