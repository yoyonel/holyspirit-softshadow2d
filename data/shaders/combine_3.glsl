uniform sampler2D   texture_normal_map;
uniform sampler2D   texture;
//
uniform vec3        u_v_color_light;
uniform vec3        u_v_ambiant_color;
uniform vec2        u_v_position_light;
uniform float       u_f_influence_radius_light ;

varying             vec2 v_v_position;

void main()
{
    vec2    v_pos_in_ls         = v_v_position - u_v_position_light; // position du vertex edge dans le repere lumiere (Light Space)
    vec2    v_pos_light_in_ls   = vec2(0.f); // origine du repere

    if (length(v_pos_in_ls) > u_f_influence_radius_light )
        discard;

    vec4 result_color = vec4(1);

	//
	vec4 texel_normal_map   = texture2D(texture_normal_map, vec2(gl_TexCoord[0].x, 1-gl_TexCoord[0].y)); // changement de repere: symetrie selon l'axe x (<=> inverse des y)
	vec3 normal             = texel_normal_map*2 - vec4(1); // decodage de la normale: [0, 1] -> [-1, +1]
    //
    float z_light = 50;
    float z_point = 0;
    vec3 L = normalize(vec3(v_pos_light_in_ls, z_light) - vec3(v_pos_in_ls, z_point));
    vec3 N = normal;
    float f_coef_normal_map = max(dot(L, N),  0.0);

    //
	vec4 texel_light_map    = texture2D(texture, gl_TexCoord[0].xy);
    float coef_shadow       = texel_light_map.x;
    //
    float dist_P_Light          = distance(v_v_position, u_v_position_light);
    float linear_attenuation    = 1 - (dist_P_Light/u_f_influence_radius_light);

    float f_coef_lighting       = linear_attenuation*coef_shadow*f_coef_normal_map;

    gl_FragColor = vec4(f_coef_lighting) * vec4(u_v_color_light, 1) + vec4(u_v_ambiant_color, 1);
}

