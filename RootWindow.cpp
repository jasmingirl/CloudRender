#include "RootWindow.h"
CCloudRender* g_Render;
CControlPane *g_ControlPane;
extern CRootWindow* g_RootWindow;
HANDLE sem;
static void FileLoad(void *arg){
  WaitForSingleObject(sem,INFINITE);
	cout<<"file"<<endl;
	while(1){
		g_Render->FileLoad();
	}
}
static void Render(void *arg){
	cout<<"render"<<endl;
	g_RootWindow->Renders();
}
CRootWindow::CRootWindow()
{
	g_Render=new CCloudRender(vec2<int>(200,0));
	

	rtarg.thread_no = 0;
	HANDLE render = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Render,(void *)&rtarg,0,NULL);
	SetThreadIdealProcessor(render,1);
	
	ftarg.thread_no = 0;
	HANDLE fileget = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)FileLoad,(void *)&ftarg,0,NULL);
	SetThreadIdealProcessor(FileLoad,0);
	

	WaitForSingleObject(render,INFINITE);
	cout<<"end root"<<endl;


}
void CRootWindow::Renders(){
	cout<<"root"<<endl;
	
	glutInitWindowPosition(100,100);
	glutInitWindowSize(1224,768);
	int m_ParentId= glutCreateWindow("volren");//ここでエラー
	glewInit();
	//PrintGLSpec();

	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH|GLUT_MULTISAMPLE|GL_ACCUM);//これって本来どのタイミングでするべき？
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);//glutMainLoopから抜け出してから何か表示するためのもの
	//InitMenues ();

	//g_Render=new CCloudRender(vec2<int>(200,0));
	vector<ISlider*> slider;
	slider.push_back(g_Render->m_TransForm);
	slider.push_back(&(g_Render->m_TransForm->m_Clip));
	
	vector<IToggle*> toggles;
	toggles.push_back(g_Render->m_Land);
	toggles.push_back(g_Render);
	toggles.push_back(g_Render->m_Measure);

	g_ControlPane=new CControlPane(vec2<int>(0,0),vec2<int>(200,768),toggles,slider);
	
	int friend_id=g_Render->GenerateWindow(m_ParentId);
	
	g_Render->InitColorDialog(m_ParentId);
	g_ControlPane->GenerateWindow(m_ParentId,friend_id);
	
	ReleaseSemaphore(sem,1,NULL);
	
	glutMainLoop();
}

CRootWindow::~CRootWindow(void)
{
}
