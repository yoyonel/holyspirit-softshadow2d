uniform sampler2D   texture;
//
uniform vec3        u_v_color_light;
uniform vec2        u_v_position_light;
uniform float       u_f_influence_radius_light;

varying             vec2 v_v_position;

float hermite(float s, float P1, float P2, float T1, float T2);

void main()
{
    vec2    v_pos_in_ls         = v_v_position - u_v_position_light; // position du vertex edge dans le repere lumiere (Light Space)
    vec2    v_pos_light_in_ls   = vec2(0.f); // origine du repere

    if (length(v_pos_in_ls) > u_f_influence_radius_light)
        discard;

    vec4 result_color;

	vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);

    float hard_shadow       = pixel.y*255;
    float inner_penumbra    = pixel.x;
    float outer_penumbra    = pixel.z;

    float coef_shadow;


    if (hard_shadow == 0.0)
    {
        if (outer_penumbra == 0.0)
            coef_shadow = 1 - inner_penumbra;
        else
            coef_shadow = outer_penumbra;
    }
    else if (hard_shadow == 1.0)
    {
        if (inner_penumbra == 0.0)
            coef_shadow = 0;
        else
            coef_shadow = inner_penumbra - outer_penumbra;
    }
    else if (hard_shadow > 1.0)
        coef_shadow = 1-(hard_shadow - inner_penumbra - outer_penumbra);

//    coef_shadow = outer_penumbra;
    coef_shadow = clamp(coef_shadow, 0, 1);

    float f_coef_lighting;

    f_coef_lighting = 1 - clamp(distance(v_v_position, u_v_position_light)/u_f_influence_radius_light, 0, 1);
    f_coef_lighting *= coef_shadow;

    //f_coef_lighting = hermite(f_coef_lighting, 0, 1, 0, 0);

    result_color = (vec4(f_coef_lighting, f_coef_lighting, f_coef_lighting, 1)) * vec4(u_v_color_light, 1);

    //gl_FragColor = result_color;
    gl_FragColor = vec4(0, 0, 0, 1);
}

float hermite(float s, float P1, float P2, float T1, float T2)
{
    float s2 = s*s;
    float s3 = s2*s;
    float h1 =  2*s3 - 3*s2 + 1;          // calculate basis function 1
    float h2 = -2*s3 + 3*s2;              // calculate basis function 2
    float h3 =   s3 - 2*s2 + s;         // calculate basis function 3
    float h4 =   s3 -  s2;              // calculate basis function 4
    float p =   h1*P1 +                    // multiply and sum all funtions
                h2*P2 +                    // together to build the interpolated
                h3*T1 +                    // point along the curve.
                h4*T2;
    return p;
}
