
#include "TransForm.h"
/*!
	都合よく見える組み合わせ、はユーザが決めるのは危険。
	こっち側で用意して上げたほうがよいと思い、fovy,near,などのカメラパラメータをハードコードにした。
	ウィンドウサイズはPCかタブレット端末か、デバイスによって変わるけど。。。アスペクト比が合っていることが重要。
*/
CTransForm::CTransForm()
	:mNearClip(0.1f),
	mFarClip(100.0f),
	m_Flags(MY_DRAW_BG),
	mFovy(35.0f),
	m_InitialZ(2.0f)
{
	m_BgColor.set(0.0,0.0,0.0,0.0);
	
	m_Zoom=m_InitialZ;
	m_CurrentQuaternion.toMat4(m_RotationMatrix.m);
	
	mTranslationVec.set(0,0,0);
	
	m_Top.set(18,69,98,255);
	m_Middle.set(71,134,126,255);
	m_Bottom.set(170,220,167,255);
	}

void CTransForm::ChangeView(int _val){
	//switch(_val){
	//
	//case GLUT_KEY_PAGE_DOWN:puts("bottom view");
	//	fixedview::Bottom(&m_TranslationMatrix,&m_RotationMatrix,&m_CurrentQuaternion);
	//	break;
	//
	//case GLUT_KEY_DOWN:puts("front view");
	//	fixedview::Front(&m_TranslationMatrix,&m_RotationMatrix,&m_CurrentQuaternion,&m_TargetQuaternion);
	//	break;
	//case GLUT_KEY_UP:puts("back view");//普通に回転すると上下逆さになってしまうので。
	//	fixedview::Back(&m_TranslationMatrix,&m_RotationMatrix,&m_CurrentQuaternion,&m_TargetQuaternion);
	//	break;
	//case GLUT_KEY_RIGHT:puts("right view");
	//	fixedview::Right(&m_TranslationMatrix,&m_RotationMatrix,&m_CurrentQuaternion,&m_TargetQuaternion);
	//	break;
	//case GLUT_KEY_LEFT:puts("left view");
	//	fixedview::Left(&m_TranslationMatrix,&m_RotationMatrix,&m_CurrentQuaternion,&m_TargetQuaternion);
	//	break;
	//case GLUT_KEY_HOME:puts("home view(same as top view");//（GLUT_KEY_PAGE_UP）と同じ
	//case GLUT_KEY_PAGE_UP:puts("top view");
	//	fixedview::Top(&m_TranslationMatrix,&m_RotationMatrix,&m_CurrentQuaternion,&m_TargetQuaternion);
	//	break;
	//}
}
void CTransForm::ChangeScene(int _val){
	switch(_val){
	case 9://morning モード
		m_Top.set(64,71,103,255);
		m_Middle.set(99,96,92,255);
		m_Bottom.set(132,114,99,255);
		break;
	case 10://Instagram風昼
		m_Top.set(18,69,98,255);
		m_Middle.set(71,134,126,255);
		m_Bottom.set(170,220,167,255);
		break;
	case 11://夕方モード
		m_Top.set(92,89,100,255);
		m_Middle.set(251,198,133,255);
		m_Bottom.set(92,89,100,255);
		break;
	case 12://night モード
		m_Top.set(12,19,27,255);
		m_Middle.set(28,41,47,255);
		m_Bottom.set(32,46,49,255);
		break;

	}

}
bool CTransForm::CheckParameterChange(){
	//透視投影か、正投影か
	static unsigned int lastState=(m_Flags & MY_ORTHOGONAL);
	if((m_Flags & MY_ORTHOGONAL)!=lastState){
		Reshape(m_WindowSize.x,m_WindowSize.y);
		lastState=(m_Flags & MY_ORTHOGONAL);
		return 0;
	}
	static unsigned int lastState2=(m_Flags & MY_PRINT);
	unsigned int current_printmode_state=(m_Flags & MY_PRINT);
	if(current_printmode_state!=lastState2){
		(current_printmode_state) ? m_BgColor.set(1.0f,1.0f,1.0f,1.0f) : m_BgColor.set(0.0f,0.0f,0.0f,1.0f);

		lastState2=current_printmode_state;
		return 0;
	}
	
	return 0;
}
void CTransForm::Disable(){
	glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
	glDisable(GL_DEPTH_TEST);
	
}
bool CTransForm::DrawBackGround(){
	float middle_point=m_Plane.corner[0].y*0.8f+m_Plane.corner[1].y*0.2f;//下の面積の割合
	glBegin(GL_QUADS);
		glColor3ubv(&m_Bottom.r);
		m_Plane.corner[0].glVertex();
		m_Plane.corner[1].glVertex();
		glColor3ubv(&m_Middle.r);
		(m_Plane.corner[1]+vec3<float>(0.0f,middle_point,0.0f)).glVertex();
		(m_Plane.corner[0]+vec3<float>(0.0f,middle_point,0.0f)).glVertex();
		(m_Plane.corner[0]+vec3<float>(0.0f,middle_point,0.0f)).glVertex();
		(m_Plane.corner[1]+vec3<float>(0.0f,middle_point,0.0f)).glVertex();
		glColor3ubv(&m_Top.r);
		m_Plane.corner[2].glVertex();
		m_Plane.corner[3].glVertex();
	glEnd();
	return 1;

}
void CTransForm::Enable(){
	//stringstream cdbg;
	//cdbg<<"clip direction="<<m_ClipDirection<<endl;
	//OutputDebugString(cdbg.str().c_str());
	glClearColor(m_BgColor.r,m_BgColor.g,m_BgColor.b ,m_BgColor.a);//なんでアルファを0.0にするんだっけ
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	if(m_Flags & MY_DRAW_BG){DrawBackGround();}//背景プレーンの描画
	gluLookAt(0,0,m_Zoom,0,0,0,0.0f,1.0f,0.0f);

	glEnable(GL_DEPTH_TEST);
	glPushMatrix();
	
	glMultMatrixf(m_TranslationMatrix.m);
	glMultMatrixf(m_RotationMatrix.m);
	
	
}
float CTransForm::farPlaneSize()const{//縦横どっちか忘れた
	//fovy is vertical
	return mNearClip*(float)tan(mFovy*(float)M_PI/360.0)*mFarClip/mNearClip;
}
void CTransForm::Info(){//原点からどのぐらい移動したのかを表示
	m_CurrentQuaternion.print("クォータニオン");
	setprecision(3);
	cout.setf(ios::fixed, ios::floatfield);
	cout<<"視野角"<<mFovy<<endl;
	cout<<"ニアークリッピング面"<<mNearClip<<endl;
	cout<<"ファークリッピング面"<<mFarClip<<endl;
}
void CTransForm::KeyBoard(unsigned char _c){
	assert(!"何も実装されてません。");
}

/*!
	すべてのコアライブラリから独立させるため、引数に_w,_hをつけた。
	各APIでウィンドウサイズを取得してこの引数に渡してください。
*/
void CTransForm::Reshape(int _w,int _h){
	
	m_WindowSize.x=_w;
	m_WindowSize.y=_h;
	glViewport(0, 0,m_WindowSize.x, m_WindowSize.y);
	float aspect;// アスペクト比
	if(m_WindowSize.y>m_WindowSize.x){
		aspect= (float)m_WindowSize.y/(float)m_WindowSize.x;
	}else{
		aspect= (float)m_WindowSize.x/(float)m_WindowSize.y;
	}
	
	glMatrixMode(GL_PROJECTION);	
	glPushMatrix();
	glLoadIdentity();
	//もう少しきれいにならないかな
	if(m_Flags & MY_ORTHOGONAL){
		miffy::PerspectiveToOrtho(mFovy,aspect,mNearClip,mFarClip,m_Zoom+m_InitialZ);
	}else{
		gluPerspective(mFovy,aspect,mNearClip,mFarClip);
	}

	//背景プレートの中心座標
	vec3<float> plane_center(0.0,0.0,-mFarClip+numeric_limits<float>::epsilon());
	m_Plane.setZaligned(plane_center,farPlaneSize()*aspect,farPlaneSize());
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}
/*!
	この関数要らないかも
	タブレット端末にも対応させたいなと思って作った。
	この関数内で、ウィンドウサイズで割るようにした。
	使う人はウィンドウサイズ知らないorめんどくさいかもしれないから。
	ウィンドウサイズの変化検知はReshapeに任せればいい。
*/
void CTransForm::RotateFromScreen(int _dx,int _dy){
	float dx=(float)_dx/(float)m_WindowSize.x;
	float dy=(float)_dy/(float)m_WindowSize.y;

	vec3<float> axis=vec3<float>(dy,dx,0.0);
	float axislength=axis.length();
	if(axislength!=0.0){
		float radian=axislength*2.0f*(float)M_PI*0.5f;
		axis.normalize();//軸の長さを1にする。
		quat<float> difq(cos(radian),axis.x*sin(radian),axis.y*sin(radian),0.0);
		m_TargetQuaternion=difq*m_CurrentQuaternion;
		m_TargetQuaternion.toMat4(const_cast<float*>(m_RotationMatrix.m));
	}
}
void CTransForm::SaveRotationState(){//Quaternionの姿勢を保存
	m_CurrentQuaternion=m_TargetQuaternion;
}
/*!
	AntTweakBar対策にわざわざxyzwの順序を変えている。
*/
void  CTransForm::RotateFromQuat(float* _quat){
	m_CurrentQuaternion.set(_quat[3],_quat[0],_quat[1],_quat[2]);
	m_CurrentQuaternion.toMat4(const_cast<float*>(m_RotationMatrix.m));
}
void CTransForm::IncrementTranslate(const vec3<float>& _val){
	mTranslationVec+=_val;
}
void CTransForm::TranslateFromScreeen(float _dx,float _dy){
	mTranslationVec+=vec3<float>(_dx,_dy,0.0);
	m_TranslationMatrix.m[12]=mTranslationVec.x;
	m_TranslationMatrix.m[13]=mTranslationVec.y;
}
float CTransForm::CalcProjectedSize(float _localsize){
	return (m_WindowSize.x/(float)_localsize*0.5f/(float)tan((double)mFovy*M_PI/360.0));

}
vec4<float> CTransForm::LocalCam(){//オブジェクト中心から見たカメラの相対位置
	mat4<float> modelview(m_TranslationMatrix*m_RotationMatrix);
	mat4<float> inv;
	modelview.inv(&mInvModelView);

	return vec4<float>(mInvModelView*vec4<float>(0.0,0.0,0.0,1.0));
}
/*!
	glOrthoの時は、普通の方法ではズームできないのでresizeを呼び出す。
	zoomしすぎるとひっくりかえるのをやめさせたい。
	×0.4してホイールスピードを調整している。ハードコードだからそのうちちゃんとすべき
	@param _direction ホイール　pushが＋pullが-.ワンホイール１になるような値で計算すること。現在のホイール値ではなく、ホイールの蓄積値を渡すこと。
*/
void CTransForm::zoom(int _direction){
	float cam_dis_candidate=(float)_direction*0.4f+m_InitialZ;
	if(cam_dis_candidate>0.0){ m_Zoom=cam_dis_candidate;}
	//orthoだと普通の方法ではズームできないようだ。
	if(m_Flags & MY_ORTHOGONAL){Reshape(m_WindowSize.x,m_WindowSize.y);}
}


void CTransForm::SetFlag(unsigned int _flags){m_Flags=_flags;}
