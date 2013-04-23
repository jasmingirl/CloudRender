#include "Measure.h"
CMeasure::CMeasure(float _elerange)
	:m_MaxHeight(11.0f),
	m_LENGTH(1.0f),
	m_KmInterval(2),
	m_LATITUDE(110.88f/160.0f*0.125f),//箱は160kmだから。
	m_LONGITUDE(91.2888f/160.0f*0.125f)//度はおおざっぱすぎたのでちょっと掛け算した
	,m_IsCipVisible(false)
{
	  mBoundingBox.set(vec3<float>(0.0f,0.0f,0.5f),0.5f,0.5f);
	 adjust=0.0;
	 cout<<"緯度"<<m_LATITUDE<<endl;
	 cout<<"経度"<<m_LONGITUDE<<endl;
}
CMeasure::~CMeasure(void)
{
}

bool CMeasure::Draw(){
	glDisable(GL_CLIP_PLANE0);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//back-to-front
	glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
			
	static float x=mBoundingBox.corner[0].x;
	static float y=mBoundingBox.corner[0].y;
	static float z_interval=(float)m_KmInterval/m_MaxHeight;//目盛りを出す間隔 2kmおき
	float z=mBoundingBox.corner[0].z;
	
	int km=0;//現在のkm
	stringstream text;

	glLineWidth(1);
	//高さの線
	//mFontColor.glColor();
	for(int i=0;i<(int)m_MaxHeight/m_KmInterval;i++){
		glColor4f(1.0f,1.0f,1.0f,1.0f);
		text.str("");
		text<<km<<"km";
		glRasterPos3f(x,y,z);//0,0,0位置をスタート位置にする		
		//GLFWではフォントのレンダリングができない！
		//glutBitmapString(GLUT_BITMAP_HELVETICA_10,(const unsigned char*)(text.str().c_str()));
		z+=z_interval;
		km+=m_KmInterval;
		glColor4f(1.0f,1.0f,1.0f,0.5f);
		glBegin(GL_LINES);
		glVertex3f(x         ,y         ,z);
		glVertex3f(x+m_LENGTH,y         ,z);
		glVertex3f(x         ,y         ,z);
		glVertex3f(x         ,y+m_LENGTH,z);
		glEnd();
	}
	//壁のグリッドを表示
	DrawLatMag();
	//おしりは必ず表示
	glRasterPos3f(x,y,m_MaxHeight);//0,0,0位置をスタート位置にする		
	//glutBitmapString(GLUT_BITMAP_HELVETICA_10,(const unsigned char*)("11km"));
	glColor4f(1.0,1.0,1.0,1.0);
	//バウンディングボックスを描く
	static float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX,m);
	mBoundingBox.DrawOnlyBackWire(-m[2],-m[6],-m[10]);
	if(m_IsCipVisible){
		DrawClipPlane();
	}
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	glEnable(GL_CLIP_PLANE0);
	return 1;
}
void CMeasure::DrawLatMag(){
	glColor4f(1.0f,1.0f,1.0f,0.5f);
	float x=mBoundingBox.corner[0].x;
	float y=mBoundingBox.corner[0].y;
	static float z=0.0f;
	
	for(int i=0;i<(int)(1.0f/m_LATITUDE);i++){
		glBegin(GL_LINES);
		glVertex3f(x,y,0.0f);
		glVertex3f(x,y,m_LENGTH);
		glEnd();
		y+=m_LATITUDE;
	}
	y=mBoundingBox.corner[0].y;
	for(int i=0;i<(int)(1.0f/m_LONGITUDE);i++){
		glBegin(GL_LINES);
		glVertex3f(x,y,0.0f);
		glVertex3f(x,y,m_LENGTH);
		glEnd();
		x+=m_LONGITUDE;
	}
	x=-0.5f;
	//地面のグリッドを表示
	for(int i=0;i<16;i++){
		glBegin(GL_LINES);
		glVertex3f(-0.5f,y,0.0f);
		glVertex3f(0.5f,y,0.0f);
		glEnd();
		y+=1.0f/16.0f;
	}
	y=-0.5;
	for(int i=0;i<16;i++){
		glBegin(GL_LINES);
		glVertex3f(x,-0.5f,0.0f);
		glVertex3f(x,0.5f,0.0f);
		glEnd();
		x+=1.0f/16.0f;
	}
}
bool CMeasure::CalcClipPlane(double _clipEqn[4]){
	m_IsCipVisible= mBoundingBox.CalcClipPlaneVerts(_clipEqn,m_intersection);
	return m_IsCipVisible;
}
void CMeasure::DrawClipPlane(){
		glBegin(GL_LINE_LOOP);
		for(int i = 0; i < 6; ++i) {
			m_intersection[i].glVertex();
		}
		glEnd();

}
