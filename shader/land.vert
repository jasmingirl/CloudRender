uniform mat4 ModelView;
uniform mat4 Proj;
uniform vec4 clip_eqn;///< クリッピング方程式

/*!
  @arg gl_Vertex 地形の頂点達
*/
void main(){
	gl_Position =Proj * ModelView*gl_Vertex;
	gl_TexCoord[0]=gl_MultiTexCoord0;
	vec3 vol_range=clamp(gl_Vertex.xyz,-0.5,0.5);//地図のうち、雲の範囲を切り取り
	vol_range+=vec3(0.5,0.5,0.0);//0.0-1.0に正規化
	float v_clipDist=dot(gl_Vertex.xyz,clip_eqn.xyz)+clip_eqn.w;

	if((v_clipDist)>0.0){//クリッピングされないところに投影面を表示
	//	//絶対に存在しなさそうなところに点を持ってくことにより、非表示を実現。
	//	//これもあとで、地図の範囲広げた時にバグになっちゃうなぁ
		gl_TexCoord[1]=vec4(100.0,100.0,100.0,1.0);
	}else{
		//こっちは表示される方
		gl_TexCoord[1]=vec4(vol_range,1.0);
	}
}
