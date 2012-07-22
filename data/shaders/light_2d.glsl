uniform vec2    u_v_position_light;
uniform float   u_f_influence_radius_light;

varying vec2 v_v_position;

void main()
{
    float f_distance = distance(v_v_position, u_v_position_light);

    if (f_distance > u_f_influence_radius_light)
        discard;

    gl_FragColor = vec4(1, 0, 0, 1);
}
