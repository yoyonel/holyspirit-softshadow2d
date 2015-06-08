// --------------------------------------------------------------------------------
// file: pixel_ss_2d.glsl
// --------------------------------------------------------------------------------
// action:  calcul d'occlusion d'un mur par rappport aÃÂaÃÂaÃÂaÃÂ  une source de lumiaÃÂaÃÂaÃÂaÃÂ¨re disque
//          le segment mur est en dehors de(n'intersecte pas) la source de lumiaÃÂaÃÂaÃÂaÃÂ¨re.
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

void    tests_for_discard_texel( in vec2 P );
bool    inside_half_plane(in vec2 A, in vec2 B, in vec2 P);
//
float   compute_disc_portion_area( in vec2 P0, in vec2 P1, in float r );
//float   compute_visibility_light(in vec2 P, in vec2 E0, in vec2 E1, in float r);
float   compute_visibility_light(in vec2 P, in vec2 E0, in vec2 E1, in float r);
//
float   signed_distance_point_line( in vec2 P, in vec2 A, in vec2 B );
float   distance_point_line( in vec2 P, in vec2 A, in vec2 B );
//
vec4    encode_shadow_R(float coef_shadow);
vec4    encode_shadow_RGBA(float coef_shadow);
vec4    encode_shadow_DEBUG(float coef_shadow);

void main()
{
    vec2    v_pos_in_ls         = v_v_position - u_v_position_light; // position du vertex edge dans le repaÃÂaÃÂaÃÂaÃÂ¨re lumiaÃÂaÃÂaÃÂaÃÂ¨re (Light Space)
    vec2    v_pos_light_in_ls   = vec2(0.); // origine du repaÃÂaÃÂaÃÂaÃÂ¨re

    // Tests pour rejeter rapidement (le plus possible)
    // les points non inclus dans la zone d'influence de paÃÂaÃÂaÃÂaÃÂ©nombre de l'araÃÂaÃÂaÃÂaÃÂªte par rapport au cercle de lumiaÃÂaÃÂaÃÂaÃÂ¨re
    tests_for_discard_texel( v_pos_in_ls );

    float f_edge_covering = 1. - compute_visibility_light( v_pos_in_ls, u_v_e0, u_v_e1, u_f_radius_light );

    // shadow coefficient
    float f_coef_shadow = f_edge_covering;

    vec4 color = encode_shadow(f_coef_shadow);

    // write result
    gl_FragColor = color;
}

float compute_visibility_light(in vec2 P, in vec2 E, in float r)
{
    float visibilty = 1.;
    vec2 pos_light = vec2(0., 0.);

    float f_signed_distance = signed_distance_point_line(pos_light, P, E);

    float d = f_signed_distance/r;
    d = clamp(d, -1., +1.); // clip (use for full-light and full-shadow)

    // Aire de visibilitaÃÂaÃÂaÃÂaÃÂ© de la source (disque de lumiere)
    visibilty = (1.-d)*0.5;

    return visibilty;
}

/*
float compute_visibility_light(in vec2 P, in vec2 E0, in vec2 E1, in float r)
{
    float visibilty = 1.;

    // Compute visibility of the circle light
    float A0 = compute_visibility_light(P, E0, r);
    float A1 = compute_visibility_light(P, E1, r);

    // Do some tests to fix the visibilty (depends of orientations between receiver point, edge, and the circle light source)
    vec2 Edge = E1 - E0;
    vec2 P_E0 = E0 - P;
    vec2 P_E1 = E1 - P;
    vec2 n_P_E0 = NORMAL(P_E0);
    vec2 n_P_E1 = NORMAL(P_E1);
    // Orientation du volume d'ombre
    A0 = dot(n_P_E0, +Edge) > 0. ? (1.-A0) : A0;
    A1 = dot(n_P_E1, -Edge) > 0. ? (1.-A1) : A1;
    // Sens de projection de l'ombre
    A0 = float(dot(P_E0, E0) < 0.) * A0;
    A1 = float(dot(P_E1, E1) < 0.) * A1;

    visibilty = (A0 + A1);

    return visibilty;
}
*/

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
    aire_triangle_isocele = d*sqrtf(r*r - d*d); // se daÃÂaÃÂaÃÂaÃÂ©composaÃÂaÃÂaÃÂaÃÂ© en deux triangles rectangles dont le rayon du cercle sont les hypothaÃÂaÃÂaÃÂaÃÂ©nuses

    // aire de la portion de disque
    A = alpha*r*r - d*sqrtf(r*r - d*d);

    return A;
}

// [OK]
// calcul la visibilitaÃÂaÃÂaÃÂaÃÂ© de la lumiaÃÂaÃÂaÃÂaÃÂ¨re
// pour un point P
// occultaÃÂaÃÂaÃÂaÃÂ© par un segment daÃÂaÃÂaÃÂaÃÂ©fini
// par deux points E0 et E1 en dehors ou sur le disque de lumiaÃÂaÃÂaÃÂaÃÂ¨re
float compute_visibility_light(in vec2 P, in vec2 E0, in vec2 E1, in float r)
{
    float visibilty = 1.;
    float inv_area_light = 1. / (M_PI*r*r);

    // Compute visibility of the circle light
    float A0 = compute_disc_portion_area(P, E0, r );
    float A1 = compute_disc_portion_area(P, E1, r );

    A0 *= inv_area_light;
    A1 *= inv_area_light;

    // Do some tests to fix the visibilty (depends of orientations between receiver point, edge, and the circle light source)
    vec2 Edge = E1 - E0;
    //
    vec2 P_E0 = E0 - P;
    vec2 P_E1 = E1 - P;
    //
    vec2 n_P_E0 = NORMAL(normalize(P_E0));
    vec2 n_P_E1 = NORMAL(normalize(P_E1));
    // Orientation du volume d'ombre
    A0 = DOT(n_P_E0, +1.*Edge) > EPSILON ? (1. - A0) : A0;
    // A t'on calculaÃÂaÃÂaÃÂaÃÂ© l'aire de visibilitaÃÂaÃÂaÃÂaÃÂ© ou d'occultation ?
    A1 = DOT(n_P_E1, -1.*Edge) > EPSILON ? (1. - A1) : A1;
    // Sens de projection de l'ombre
    A0 = float(DOT(P_E0, E0) < -EPSILON) * A0;
    A1 = float(DOT(P_E1, E1) < -EPSILON) * A1;

    visibilty = (A0 + A1);

    if (EQUAL_EPS((dot(normalize(P_E0), normalize(P_E1))), 1., 0.001*EPSILON))
        visibilty = 1.;

    return visibilty;
}


void tests_for_discard_texel( in vec2 P )
{
    // inside influence circle ?
    if ( dot(P, P) > u_f_influence_radius_light*u_f_influence_radius_light )
        discard;
    //
//    if ( !inside_half_plane(u_v_e0, u_v_e1, P) )
//        discard;

    if ( dot(normalize(u_v_e0), normalize(u_v_e1)) >= (0.97) )
    {
        float l0 = dot(u_v_e0, u_v_e0);
        float l1 = dot(u_v_e1, u_v_e1);
        bool test;
        if (l0 < l1)
            test = inside_half_plane(u_v_e0, u_v_e0 + NORMAL(u_v_e0), P);
        else
            test = inside_half_plane(u_v_e1, u_v_e1 + NORMAL(u_v_e1), P);
        if ( !test )
            discard;
    }
}

bool inside_half_plane(in vec2 A, in vec2 B, in vec2 P)
{
    // Du bon cotaÃÂaÃÂaÃÂaÃÂ© du demi-plan dont l'edge est la frontiaÃÂaÃÂaÃÂaÃÂ¨re (ou sa droite) et orientaÃÂaÃÂaÃÂaÃÂ© pour ne pas contenir la source de lumiaÃÂaÃÂaÃÂaÃÂ¨re
    // equation paramaÃÂaÃÂaÃÂaÃÂ©trique d'une droite: (1) a.x + b.y + c = 0, avec (a,b) normale de la droite
    vec2 v_dir      = B - A;
    vec2 v2_normal   = NORMAL(v_dir);
    // on calcul c => c = -(a*x + b*y), on prend (x, y) = A (1 point de la droite)
    float c = - dot(v2_normal, A);
    // dans quel sens la normale est orientaÃÂaÃÂaÃÂaÃÂ©e pour relier le point P et la droite ?
    float side_of_P = dot(P, v2_normal) + c;
    // selon le sens (qui indique le sens de la normale), on daÃÂaÃÂaÃÂaÃÂ©termine si le point est dans le demi-plan
    return (side_of_P*sign(c)<0.);
}

vec4 encode_shadow_R(float f_coef_shadow)
{
    vec4 color = vec4( f_coef_shadow, 0., 0., 1. );
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

// RepaÃÂaÃÂaÃÂaÃÂ¨re main droite pour retrouver le sens de la normale de la droite (son 'Z')
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
