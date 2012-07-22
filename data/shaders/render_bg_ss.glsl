uniform sampler2D   texture;
uniform vec3        u_v_ambiant_color;

void main()
{
	gl_FragColor =  texture2D(texture, gl_TexCoord[0].xy) + vec4(u_v_ambiant_color, 0.0);
}

