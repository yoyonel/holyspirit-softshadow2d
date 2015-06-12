#define DET(A, B)               ((A).x*(B).y - (B).x*(A).y)
#define NORMAL(V2)              vec2(-(V2).y, (V2).x)
#define EQUAL0_EPS(_f, _eps)    ((_f)>=-(_eps) && (_f)<=+(_eps))
#define EQUAL_EPS(_f1, _f2, _eps)   (((_f1)-(_f2))>=-(_eps) && ((_f1)-(_f2))<=+(_eps))

#define saturate(x) clamp(x, 0.0, 1.0)

#define EPSILON 0.001

#define USE_LIGHTING        // Utilise les calculs de lumieres (ombres, normal map, attenuation, etc ...)
#define USE_SHADOW          // Affiche les ombres
#define USE_NORMALMAP       // Utilise la map de normal map pour "bosseler" le sol
#define USE_ATTENUATION     // Utilise un coefficient d'attenuation lie a  la distance du centre de la lumiere
#define USE_COLOR           // Utilise la couleur de la lumiere
#define USE_SCATTEROING	    //
//#define USE_HARDSHADOW    // Affiche les volumes d'ombres dures (pas tout a fait equivalent a  une source ponctuelle de lumiere (en terme de visibilite))
//#define DEBUG_SHOW_HS_PV  // Affiche les volumes de penombres et ombres dures

#ifdef USE_NORMALMAP
    uniform sampler2D   u_texture_normal_map;
#endif

#ifdef USE_SHADOW
    uniform sampler2D   u_texture_light_map;
#endif
//
#ifdef USE_COLOR
    uniform vec3        u_v_color_light;
#endif

uniform vec2        u_v_position_light;
uniform float       u_f_influence_radius_light;

varying             vec2 v_v_position;

// MOG: probleme avec les variables uniformes
// et la transmission en argument de fonction
// Que sur ATI/AMD (fonctionne sur NVIDIA)

float compute_shadow( in vec2 tex_coords_lm );
float compute_normal_map( in vec3 P, in vec3 L, in vec2 tex_coords_nm );
//
float compute_attenuation( in vec3 P, in vec3 L, in float R );
float compute_attenuation( in float d, in float coefs_att[3] );
//
float InScatter(vec3 start, vec3 dir, vec3 lightPos, float d);
void Li(vec3 surfacePos, vec3 surfaceNormal, vec3 lightPos, vec3 eyePos, float specularPower, out float diffuse, out float specular);
vec3 LinearToSrgb(vec3 rgb);

// [MOG] : decodage des infos
#define decode_shadow decode_shadow_R
//#define decode_shadow decode_shadow_RGBA
//
float decode_shadow_R(vec4 color);
float decode_shadow_RGBA(vec4 color);

void main()
{
    vec4    result_color        = vec4(1);
    float   normal_map,
	    shadow,
	    att,
	    lighting;

    float   z_light = 50.0; // define a 3D position for light source
    float   z_point = 0.0;  //                          point receiver
    vec2    v2_pos_in_ls        = v_v_position - u_v_position_light;
    vec2    v2_pos_light_in_ls  = vec2(0);
    vec3    v3_pos_in_ls        = vec3(v2_pos_in_ls,       z_point);   // position du vertex edge dans le repere lumiere (Light Space)
    vec3    v3_pos_light_in_ls  = vec3(v2_pos_light_in_ls, z_light);   // origine du repere

    // Distance en espace ecran (coord2D)
    if ( dot(v2_pos_in_ls, v2_pos_in_ls) > u_f_influence_radius_light*u_f_influence_radius_light )
	discard;

    // Direct Lighting
    lighting = 1.0;
    #ifdef USE_SHADOW
	//MOG : bug sur l'appel de fonction
	shadow      = compute_shadow( gl_TexCoord[0].xy );
	#ifdef USE_HARDSHADOW
	    shadow = shadow <= 0.5 ? 0.0 : 1.0;
	#endif
	lighting    *= shadow;
    #endif

    #ifdef USE_NORMALMAP
	normal_map  = compute_normal_map(
				     v3_pos_in_ls,
				     v3_pos_light_in_ls,
				     vec2(gl_TexCoord[0].x, 1.0-gl_TexCoord[0].y) // changement de repere: symetrie selon l'axe x (<=> inverse des y)
				     );

	lighting    *= normal_map;
    #endif

    #ifdef USE_ATTENUATION
	att         = compute_attenuation( v3_pos_in_ls, v3_pos_light_in_ls, u_f_influence_radius_light );
	lighting    *= att;
    #endif

    // Final color
    #ifdef USE_LIGHTING
	result_color *= vec4(vec3(lighting), 1.0);
    #endif

    #ifdef USE_COLOR
	result_color *= vec4(u_v_color_light, 1.0);
    #endif

    #ifdef DEBUG_SHOW_HS_PV
	result_color = shadow<(1.0-EPSILON) && shadow>(0.0+EPSILON) ? vec4(1.0 - u_v_color_light, 1.0) : shadow <= (0.0 + EPSILON) ? vec4(u_v_color_light, 1.0) : vec4(1, 0, 0, 1);
    #endif

    #ifdef USE_SCATTEROING
	vec3 cameraPos = vec3(640, 360, z_light);
	vec3 lightPos = vec3(u_v_position_light, z_light*0.5);
	vec3 surfacePos = vec3(v_v_position, 0.0);

	vec3 dir = surfacePos - cameraPos;
	float l = length(dir);
	dir /= l;
	vec3 light_intensity = vec3(7.0, 3.0, 3.0) * 1.0;
	vec3 g_lightCol = u_v_color_light * light_intensity;
	//
	float g_scatteringCoefficient = 0.3;
	//
	vec3 scatter = g_lightCol * vec3(0.2, 0.5, 0.8) * InScatter(cameraPos, dir, lightPos, l) * g_scatteringCoefficient;
	//
	// calculate contribution from diffuse reflection
	float diffuse;
	float specular;

	float specularExponent = 15.0;
	float specularIntensity = 0.02;

	vec3 surfaceNormal = vec3(0, 0, 1.0);

	Li(surfacePos, surfaceNormal, lightPos, cameraPos, specularExponent, diffuse, specular);

	vec3 r = LinearToSrgb(g_lightCol * (diffuse + specular * specularIntensity) + scatter);

	result_color += vec4(r, 1.0) * (shadow*att);
    #endif

    gl_FragColor = result_color;
}

#ifdef USE_SHADOW
    float compute_shadow( in vec2 tex_coords_lm )
    {
	//
	vec4 texel_light_map    = texture2D(u_texture_light_map, tex_coords_lm);

	float shadow = decode_shadow(texel_light_map);

	return shadow;
    }
#endif

#ifdef USE_NORMALMAP
    float compute_normal_map( in vec3 P, in vec3 Light, in vec2 tex_coords_nm )
    {
	//
	vec4 texel_normal_map   = texture2D(u_texture_normal_map, tex_coords_nm);
	vec3 normal             = vec3(texel_normal_map)*2.0 - vec3(1.0); // decodage de la normale: [0, 1] -> [-1, +1]
	normal.y *= -1;
	//
	vec3 L = normalize(Light-P);
	vec3 N = normalize(normal);
	float normal_map = max(dot(N, L),  0.0);
	// Scale le lighting lie a  la normal map
	// revient a  modifier la courbe de la pente (et sa valeur min)
	float nm_scale = +0.45;
	normal_map = (normal_map + nm_scale) / (1.0 + nm_scale); // [MOG]: scale lighting normal-map

	return normal_map;
    }
#endif

float compute_attenuation( in float d, in float coefs_att[3] )
{
    float att = 1.0/(coefs_att[0] + d*coefs_att[1] + d*d*coefs_att[2]);
    return att;
}

float compute_attenuation( in vec3 P, in vec3 L, in float R )
{
    float   coef_attenuations[3],
	    d,
	    max_att,
	    att;

    // coef_attenuations
    //  [0]: constant   attenuation
    //  [1]: linear     attenuation
    //  [2]: quadratic  attenuation
    coef_attenuations[0] = 0.00;
    coef_attenuations[1] = 4.00;
    coef_attenuations[2] = 8.00;
    //
    d       = distance(P, L);
    d       = d/R;                  // normalisation de la distance
    max_att = compute_attenuation(1.0, coef_attenuations);
    if ( EQUAL_EPS(max_att, 1.0/coef_attenuations[0], EPSILON) )
	att = 1.0;
    else
	att = (compute_attenuation(d, coef_attenuations) - max_att)/(1.0-max_att);

    return att;
}

float decode_shadow_R(vec4 color)
{
    float shadow = 1.0 - color.r;
    return shadow;
}

float decode_shadow_RGBA(vec4 color)
{
    // color: texel_light_map.
    //  x: shadow coefficient between [0.00, 0.25]
    //  y: shadow coefficient between [0.25, 0.50]
    //  z: shadow coefficient between [0.50, 0.75]
    //  w: shadow coefficient between [0.75, 1.00]
    float shadow = 1.0 - min(dot(color, vec4(1.0, 1.0, 1.0, 1.0)), 1.0);

    return shadow;
}

float InScatter(vec3 start, vec3 dir, vec3 lightPos, float d)
{
    // calculate quadratic coefficients a,b,c
    vec3 q = start - lightPos;

    float b = dot(dir, q);
    float c = dot(q, q);

    // evaluate integral
    float s = 1.0f / sqrt(c - b*b);

    float l = s * (atan( (d + b) * s) - atan( b*s ));

    return l;
}

// simple diffuse + blinn phong lighting model
void Li(vec3 surfacePos, vec3 surfaceNormal, vec3 lightPos, vec3 eyePos, float specularPower, out float diffuse, out float specular)
{
	vec3 l = lightPos-surfacePos;
	vec3 e = normalize(eyePos-surfacePos);

	float d = length(l);
	l /= d;

	// half vector
	vec3 h = normalize(e+l);

	diffuse = saturate(dot(surfaceNormal, l)) / (d*d);
	specular = pow(saturate(dot(h,surfaceNormal)), specularPower);
}

vec3 LinearToSrgb(vec3 rgb)
{
	return pow(rgb, vec3(1.0/2.2));
}
