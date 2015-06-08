// --------------------------------------------------------------------------------
// file: pixel_ss_2d_wil_type4
// --------------------------------------------------------------------------------
// action:  calcul d'occlusion d'un mur par rappport a  une source de lumiere disque
//          _wil:   Wall In Light
//          =>      Le segment mur est a  l'interieur de la source de lumiere.
//          _type4:
//              E0: INSIDE_CIRCLE
//              I0: INSIDE_CIRCLE
//              I1: INSIDE_CIRCLE
//              E1: INSIDE_CIRCLE
// --------------------------------------------------------------------------------

#define NORMAL(V2)                  vec2(-(V2).y, (V2).x)
#define EQUAL0_EPS(_f, _eps)        ((_f)>=-(_eps) && (_f)<=+(_eps))
#define EQUAL_EPS(_f1, _f2, _eps)   (((_f1)-(_f2))>=-(_eps) && ((_f1)-(_f2))<=+(_eps))
#define EPSILON                     0.001
#define M_PI                        3.14159

#define CLAMP                       clamp
#define NORMALIZE                   normalize
#define sinf                        sin
#define DOT                         dot
#define M_PI                        3.14159
#define SIGN                        sign
#define acosf                       acos
#define sqrtf                       sqrt

// [MOG] : encodage des infos
#define encode_shadow   encode_shadow_R
//#define encode_shadow   encode_shadow_RGBA


uniform vec2    u_v_position_light;
uniform float   u_f_radius_light;
uniform float   u_f_influence_radius_light;
//
uniform vec2 u_v_e0;
uniform vec2 u_v_e1;
//
varying vec2 v_v_position;

float   compute_shadow_light(in vec2 P);
//
void    tests_for_discard_texel( in vec2 P );
//
float   compute_visibility_light_in_in(in vec2 P, in vec2 E0, in vec2 E1, in float r);
//
float   signed_distance_point_line( in vec2 P, in vec2 A, in vec2 B );
float   distance_point_line( in vec2 P, in vec2 A, in vec2 B );
float   area_triangle( in vec2 A, in vec2 B, in vec2 C );
bool    inside_half_plane(in vec2 A, in vec2 B, in vec2 P);
vec2    compute_projection_on_circle( in vec2 E, in vec2 P, in float r, in float inv_r );
float   compute_disc_portion_area( in vec2 P0, in vec2 P1, in float r );
//
vec4    encode_shadow_R(float coef_shadow);
vec4    encode_shadow_RGBA(float coef_shadow);
vec4    encode_shadow_DEBUG(float coef_shadow);

void main()
{
    vec2    v_pos_in_ls         = v_v_position - u_v_position_light; // position du vertex edge dans le repere lumiere (Light Space)
    vec2    v_pos_light_in_ls   = vec2(0.); // origine du repere

    // Tests pour rejeter rapidement (le plus possible)
    // les points non inclus dans la zone d'influence de penombre de l'araªte par rapport au cercle de lumiere
    tests_for_discard_texel( v_pos_in_ls );

    // shadow coefficient
    float f_coef_shadow = compute_shadow_light(v_pos_in_ls);

    vec4 color = encode_shadow(f_coef_shadow);

    // write result
    gl_FragColor = color;
}

// E0, I0, I1, E1 = INSIDE_CIRCLE, INSIDE_CIRCLE, INSIDE_CIRCLE, INSIDE_CIRCLE
float compute_shadow_light(in vec2 P)
{
    vec2 E0 = u_v_e0;
    vec2 E1 = u_v_e1;
    float r = u_f_radius_light;
    //
    float vis_light = 1.;

    vis_light -= 1 - compute_visibility_light_in_in(P, E0, E1, r);

    return(1 - vis_light);
}

void tests_for_discard_texel( in vec2 P )
{
    // inside influence circle ?
    if ( dot(P, P) > u_f_influence_radius_light*u_f_influence_radius_light )
        discard;
}

vec4 encode_shadow_R(float f_coef_shadow)
{
    vec4 color = vec4( f_coef_shadow, 0., 0., 0. );
    return color;
}

vec4 encode_shadow_RGBA(float f_coef_shadow)
{
    vec4 color = vec4(
                        float(f_coef_shadow<0.25)                        * f_coef_shadow,
                        float(f_coef_shadow>=0.25 && f_coef_shadow<0.50) * f_coef_shadow,
                        float(f_coef_shadow>=0.50 && f_coef_shadow<0.75) * f_coef_shadow,
                        float(f_coef_shadow>=0.75)                       * f_coef_shadow
                        );
    return color;
}

// Repere main droite pour retrouver le sens de la normale de la droite (son 'Z')
float signed_distance_point_line( in vec2 P, in vec2 A, in vec2 B )
{
    vec2 AB = B - A;

    vec2 v = normalize(NORMAL(AB));
    vec2 r = A - P;

    return dot(v, r);
}

float distance_point_line( in vec2 P, in vec2 A, in vec2 B )
{
    return abs(signed_distance_point_line( P, A, B ));
}

float area_triangle( in vec2 A, in vec2 B, in vec2 C )
{
    return 0.5*abs((B.x-A.x)*(C.y-A.y) - (C.x-A.x)*(B.y-A.y));
}

// [OK]
bool inside_half_plane(in vec2 A, in vec2 B, in vec2 P)
{
    // Du bon cote du demi-plan dont l'edge est la frontiere (ou sa droite) et oriente pour ne pas contenir la source de lumiere
    // equation parametrique d'une droite: (1) a.x + b.y + c = 0, avec (a,b) normale de la droite
    vec2 v_dir      = B - A;
    vec2 v2_normal   = NORMAL(v_dir);
    // on calcul c => c = -(a*x + b*y), on prend (x, y) = A (1 point de la droite)
    float c = - dot(v2_normal, A);
    // dans quel sens la normale est orientee pour relier le point P et la droite ?
    float side_of_P = dot(P, v2_normal) + c;
    // selon le sens (qui indique le sens de la normale), on determine si le point est dans le demi-plan
    return (side_of_P*sign(c) < -1000*EPSILON);
}

// [OK]
vec2 compute_projection_on_circle( in vec2 E, in vec2 P, in float r, in float inv_r )
{
    float   f_signed_distance, d, a;
    vec2    x, y, Proj_E_P;

    vec2 pos_light= vec2(0.);

    f_signed_distance = signed_distance_point_line( pos_light, P, E );
    // [TODO]: a  simplifier !
    d = f_signed_distance*inv_r;
    a = acos(d);
    y = normalize(E-P);
    x = NORMAL(y);
    Proj_E_P = pos_light + x*d*r + y*sin(a)*r;

    return Proj_E_P;
}

// [OK]
// P0 et P1: sont sur le cercle de position pos_light et (inverse) de rayon inv_r
float compute_disc_portion_area( in vec2 P0, in vec2 P1, in float r )
{
    float f_distance, d, A, aire_secteur_angulaire, aire_triangle_isocele, alpha;
    const vec2 pos_light = vec2(0., 0.);

    // On calcul de l'aire du bout de disque dans la zone d'occultation
    d = signed_distance_point_line( pos_light, P0, P1 );
    d = CLAMP(d, -r, +r);

    alpha = acosf(d/r); // angle
    aire_secteur_angulaire = alpha*r*r;
    aire_triangle_isocele = d*sqrtf(r*r - d*d); // se decompose en deux triangles rectangles dont le rayon du cercle sont les hypothenuses

    // aire de la portion de disque
    A = alpha*r*r - d*sqrtf(r*r - d*d);

    return A;
}

// [OK]
// E0: In       Circle
// E1: In       Circle
float   compute_visibility_light_in_in(in vec2 P, in vec2 E0, in vec2 E1, in float r)
{
    float visibility_light;
    float coef_shadow_light;
    float coef_area_triangle_0, coef_area_triangle_1;
    float coef_disc_portion_area;

    float inv_area_light = 1./(M_PI*r*r);
    float inv_r = 1./r;

    vec2 Proj_E0, Proj_E1;
    Proj_E0 = compute_projection_on_circle( E0, P, r, inv_r );
    Proj_E1 = compute_projection_on_circle( E1, P, r, inv_r );

    // occluder triangle (issu de E0)
    coef_area_triangle_0 = area_triangle(E0, Proj_E0, E1)*inv_area_light;
    // occluder triangle (issu de E1)
    coef_area_triangle_1 = area_triangle(E1, Proj_E0, Proj_E1)*inv_area_light;

    // 1 portion de disque
    coef_disc_portion_area = compute_disc_portion_area( Proj_E0, Proj_E1, r)*inv_area_light;
    vec2 N_Proj_E0_Proj_E1 = NORMAL(Proj_E1 - Proj_E0);
    coef_disc_portion_area = DOT( P - Proj_E0, N_Proj_E0_Proj_E1 ) > 0 ? 1 - coef_disc_portion_area : coef_disc_portion_area;
    coef_disc_portion_area = EQUAL_EPS(dot(normalize(P-Proj_E0), normalize(P-Proj_E1)), 1, EPSILON) ? 0.0 : coef_disc_portion_area;

    coef_shadow_light = coef_area_triangle_0 + coef_area_triangle_1 + coef_disc_portion_area;

    visibility_light = 1. - coef_shadow_light;

    return visibility_light;
}
