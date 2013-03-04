
#include "RootWindow.h"
int gMenuId[5];
CRootWindow* g_RootWindow;
void root_menu(int _val){
  //g_Render->RootMenu(_val);
}
void view_menu(int val){
	//g_Render->specialkey(val,0,0);
}
void scene_menu(int val){
//	m_TransForm.ChangeScene(val);
//	m_Light->Change(val);
//	m_Cloud->UpdatePara();
}
void InitMenues ()
{
	//gMenuId[0]=glutCreateMenu(ChangeVolData);
	//glutAddMenuEntry("full PPI", 0);
	//glutAddMenuEntry("skip top 2 PPI", 1);
	//glutAddMenuEntry("skip top 3 PPI", 2);
	//glutAddMenuEntry("skip top 4 PPI", 3);
	//gMenuId[1]=glutCreateMenu(ChangeVolData);
	//glutAddSubMenu ("elevation interpolate",gMenuId[0]);
	//glutAddMenuEntry("radial interpolate", 4);
	//glutAddMenuEntry("both interpolate", 5);
	//右クリック
	gMenuId[2]=glutCreateMenu(scene_menu);
	glutAddMenuEntry("morning ", 9);
	glutAddMenuEntry("noon ", 10);
	glutAddMenuEntry("dawn ", 11);
	glutAddMenuEntry("evening ", 12);


	gMenuId[0]=glutCreateMenu(view_menu);
	glutAddMenuEntry("Home",GLUT_KEY_HOME);
	glutAddMenuEntry("Top", GLUT_KEY_PAGE_UP);
	glutAddMenuEntry("Bottom ", GLUT_KEY_PAGE_DOWN);
	glutAddMenuEntry("Front",GLUT_KEY_DOWN);
	glutAddMenuEntry("Back", GLUT_KEY_UP);
	glutAddMenuEntry("Left", GLUT_KEY_LEFT);
	glutAddMenuEntry("Right", GLUT_KEY_RIGHT);

	glutCreateMenu(root_menu);
	glutAddMenuEntry ("Show/Hide Static Volume",4);
	glutAddMenuEntry ("Show/Hide Moving Volume",5);
	glutAddMenuEntry("Change Wind Data[w]", 6);
	glutAddMenuEntry("Color Change", 7);
	glutAddMenuEntry("Show/Hide Wind Vector", 8);
	glutAddSubMenu ("Change Scene",gMenuId[2]);
	glutAddSubMenu ("View",gMenuId[0]);//親メニュー
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/*void FileLoad(void *arg){	
	WaitForSingleObject(sem,INFINITE);

	while(g_Render!=0){
		g_Render->m_Cloud->FileLoad();
	}
}
void Render(void *arg){
	
}*/
int main(int argc, char *argv[]){
	
	glutInit(&argc, argv);
	
	g_RootWindow=new CRootWindow();
	
	/*ftarg.thread_no = 0;
	
	HANDLE fileget = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)FileLoad,(void *)&ftarg,0,NULL);
	SetThreadIdealProcessor(FileLoad,0);
	rtarg.thread_no = 0;
	HANDLE render = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Render,(void *)&rtarg,0,NULL);
	SetThreadIdealProcessor(render,1);
	WaitForSingleObject(render,INFINITE);*/
	return 0;
}
