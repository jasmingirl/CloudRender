uniform sampler2D map;
uniform sampler2D slice;
  void main() {  
	vec4 map_col=texture(map,gl_TexCoord[0].xy);
		vec4 slice_col=texture(slice,gl_TexCoord[1].xy);
		gl_FragColor=max(map_col,slice_col);	
}
