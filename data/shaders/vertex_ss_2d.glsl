varying vec2 v_v_position;

void main()
{
    gl_Position     = ftransform();
    gl_TexCoord[0]  = gl_MultiTexCoord0;
    v_v_position    = gl_Vertex.xy;  // position relative a  la lumiere
}

