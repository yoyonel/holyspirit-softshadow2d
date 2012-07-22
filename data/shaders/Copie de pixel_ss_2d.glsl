#define DET(A, B)               ((A).x*(B).y - (B).x*(A).y)
#define NORMAL(V2)              vec2(-(V2).y, (V2).x)
#define EQUAL0_EPS(_f, _eps)    ((_f)>=-(_eps) && (_f)<=+(_eps))
#define EQUAL_EPS(_f1, _f2, _eps)   (((_f1)-(_f2))>=-(_eps) && ((_f1)-(_f2))<=+(_eps))
#define EPSILON 0.001
#define NORM2(v)                dot(v, v)

// [MOG] : encodage des infos
#define encode_shadow   encode_shadow_R
//#define encode_shadow   encode_shadow_RGBA

uniform vec2    u_v_position_light;
uniform float   u_f_radius_light;
uniform float   u_f_influence_radius_light;
//
uniform vec2 u_v_intersection0_segment_circle;
uniform vec2 u_v_intersection1_segment_circle;

//
varying vec2 v_v_position;

void    tests_for_discard_texel( in vec2 P );
vec2    compute_intersection_lines( in vec2 P1, in vec2 P2, in vec2 P3, in vec2 P4);
float   compute_penumbra_rp(in vec2 v_pos, in vec2 v_edges[2], in vec2 v2_normal);
bool    inside_half_plane(in vec2 A, in vec2 B, in vec2 P);
//
float   compute_covering    ( in vec2 P, in vec2 E0, in vec2 E1, in vec2 C, in float r );
float   compute_covering_2  ( in vec2 P, in vec2 E0, in vec2 E1, in vec2 C, in float r );
//
float   compute_visbility_light(in vec2 P, in vec2 E, in float r);
float   compute_visbility_light(in vec2 P, in vec2 E0, in vec2 E1, in float r);
//
bool    compute_intersections_circles(in vec2 O0, in float R0, in vec2 O1, in float R1, out vec2 P[2]);

vec4    encode_shadow_R(float coef_shadow);
vec4    encode_shadow_RGBA(float coef_shadow);
vec4    encode_shadow_DEBUG(float coef_shadow);

float   signed_distance_point_line( in vec2 P, in vec2 A, in vec2 B );
float   distance_point_line( in vec2 P, in vec2 A, in vec2 B );

void main()
{
    vec2    v_pos_in_ls         = v_v_position - u_v_position_light; // position du vertex edge dans le repère lumière (Light Space)
    vec2    v_pos_light_in_ls   = vec2(0); // origine du repère

    // Tests pour rejeter rapidement (le plus possible)
    // les points non inclus dans la zone d'influence de pénombre de l'arête par rapport au cercle de lumière
    tests_for_discard_texel( v_pos_in_ls );

    float f_edge_covering_0 = compute_covering( v_pos_in_ls, u_v_intersection0_segment_circle, u_v_intersection1_segment_circle, v_pos_light_in_ls, u_f_radius_light );
//    float f_edge_covering = compute_covering_2( v_pos_in_ls, u_v_intersection0_segment_circle, u_v_intersection1_segment_circle, v_pos_light_in_ls, u_f_radius_light );
//    float f_edge_covering = abs(
//                                compute_covering( v_pos_in_ls, u_v_intersection0_segment_circle, u_v_intersection1_segment_circle, v_pos_light_in_ls, u_f_radius_light ) -
//                                compute_covering_2( v_pos_in_ls, u_v_intersection0_segment_circle, u_v_intersection1_segment_circle, v_pos_light_in_ls, u_f_radius_light )
//                                )*5;
    float f_edge_covering_1 = 1 - compute_visbility_light( v_pos_in_ls, u_v_intersection0_segment_circle, u_v_intersection1_segment_circle, u_f_radius_light );

//    float f_edge_covering = abs(f_edge_covering_0 - f_edge_covering_1)*20;
    float f_edge_covering = f_edge_covering_1;

    // shadow coefficient
    float f_coef_shadow = f_edge_covering;

    vec4 color = encode_shadow(f_coef_shadow);

    // write result
    gl_FragColor = color;
}

// P:   point receiver
// E0:  sommet 0 de l'edge
// E1:  sommet 1 de l'edge
// C:   centre de la source de lumière
// r:   rayon de la source de lumière
float compute_covering( in vec2 P, in vec2 E0, in vec2 E1, in vec2 C, in float r )
{
    float result;

     // Vecteurs directeurs reliant le point receiver P et les sommets E0, E1 de l'edge
    vec2 v2_dir_i0_P = P - E0;
    vec2 v2_dir_i1_P = P - E1;

    vec2 P_norm = normalize(P);

    // On calcul l'intersection des demies-droites ]P, E0) et ]P, E1) avec (C, v2_normal_P_L)
    vec2 v2_normal_EO_E1    =  NORMAL(E1 - E0);
    vec2 v2_normal_E1_E0    =  -v2_normal_EO_E1;
    vec2 v2_normal_P_L      = NORMAL(P_norm);

    // On calcul l'intersection des droites
    vec2 v2_il_0 = compute_intersection_lines( P, E0, C, C + v2_normal_P_L );
    vec2 v2_il_1 = compute_intersection_lines( P, E1, C, C + v2_normal_P_L );

    // distances signées (orientées)
    float l0, l1;

    // On vérifie que les intersections sont bien sur les demies-droites
    // si ce n'est pas le cas, on détermine le recouvrement du cercle de lumière
    if (sign(dot(v2_dir_i0_P, P-v2_il_0)) < 0.0)
        l0 = sign(dot(v2_dir_i0_P, v2_normal_EO_E1))*r;
    else
        l0 = dot(v2_il_0, v2_normal_P_L);

    if (sign(dot(v2_dir_i1_P, P-v2_il_1)) < 0.0)
        l1 = sign(dot(v2_dir_i1_P, v2_normal_E1_E0))*r;
    else
        l1 = dot(v2_il_1, v2_normal_P_L);

    // min max des longueurs
    float l_min = min(l1, l0);
    float l_max = max(l1, l0);

    // Clipping des min/ max
    l_min = max(l_min, -r);
    l_max = min(l_max, +r);

    // edge covering
    result = (l_max-l_min)/(2.0*r);

    return result;
}

// P:   point receiver
// E0:  sommet 0 de l'edge
// E1:  sommet 1 de l'edge
// C:   centre de la source de lumière
// r:   rayon de la source de lumière
float compute_covering_2( in vec2 P, in vec2 E0, in vec2 E1, in vec2 C, in float r )
{
    float result;

    // On détermine dans 1er temps ce que percoit le point receiver P de la source de lumière
    // i.e. c'est comme si on calculait les centres de projections de pénombres (inner/outer) depuis P (comme point edge).
    vec2    intersections[2];
    //
    vec2    C2 = (P + C) / 2.0;
    float   r2 = distance(C2, C);
    //
    compute_intersections_circles( C, r, C2, r2, intersections);
    // projection inverse: On projette (le segment de visibilité de) la source de lumière sur l'edge
    vec2 v2_il_0 = compute_intersection_lines( E0, E1, P, intersections[0]);
    vec2 v2_il_1 = compute_intersection_lines( E0, E1, P, intersections[1]);

    vec2    v_dir       = v2_il_1 - v2_il_0;
    vec2    center      = (v2_il_0 + v2_il_1) / 2.0;
    float   l_circle    = distance(center, v2_il_0);

    // normalise le vecteur directeur
    v_dir = normalize(v_dir);

    // distances signées
    float l0 = dot(E0 - center, v_dir);
    float l1 = dot(E1 - center, v_dir);

    // min max des longueurs
    float l_min = min(l1, l0);
    float l_max = max(l1, l0);

    // Clipping des min/max
    l_min = max(l_min, -l_circle);
    l_max = min(l_max, +l_circle);

    // edge covering
    result = (l_max-l_min)/(2.0*l_circle);

    return result;
}

// Line0: (P1, P2)
// Line1: (P3, P4)
// result: intersection point
// note: don't test if lines arec oolinear
vec2    compute_intersection_lines( in vec2 P1, in vec2 P2, in vec2 P3, in vec2 P4)
{
    vec2 P3P1 = P1 - P4;
    vec2 P1P2 = P2 - P1;
    vec2 P3P4 = P4 - P3;

    float denum = 1.0/DET(P1P2, P3P4);
    float ua    = DET(P3P4, P3P1)*denum;

    return P1 + P1P2*ua;
}

bool inside_half_plane(in vec2 A, in vec2 B, in vec2 P)
{
    // Du bon coté du demi-plan dont l'edge est la frontière (ou sa droite) et orienté pour ne pas contenir la source de lumière
    // equation paramétrique d'une droite: (1) a.x + b.y + c = 0, avec (a,b) normale de la droite
    vec2 v_dir      = B - A;
    vec2 v2_normal   = NORMAL(v_dir);
    // on calcul c => c = -(a*x + b*y), on prend (x, y) = A (1 point de la droite)
    float c = - dot(v2_normal, A);
    // dans quel sens la normale est orientée pour relier le point P et la droite ?
    float side_of_P = dot(P, v2_normal) + c;
    // selon le sens (qui indique le sens de la normale), on détermine si le point est dans le demi-plan
    return (side_of_P*sign(c)<0.0);
}

void tests_for_discard_texel( in vec2 P )
{
    // inside influence circle ?
    if ( dot(P, P) > u_f_influence_radius_light*u_f_influence_radius_light )
        discard;
    //
//    if ( !inside_half_plane(u_v_intersection0_segment_circle, u_v_intersection1_segment_circle, P) )
//        discard;

    if ( dot(normalize(u_v_intersection0_segment_circle), normalize(u_v_intersection1_segment_circle)) >= (0.97) )
    {
        float l0 = dot(u_v_intersection0_segment_circle, u_v_intersection0_segment_circle);
        float l1 = dot(u_v_intersection1_segment_circle, u_v_intersection1_segment_circle);
        bool test;
        if (l0 < l1)
            test = inside_half_plane(u_v_intersection0_segment_circle, u_v_intersection0_segment_circle + NORMAL(u_v_intersection0_segment_circle), P);
        else
            test = inside_half_plane(u_v_intersection1_segment_circle, u_v_intersection1_segment_circle + NORMAL(u_v_intersection1_segment_circle), P);
        if ( !test )
            discard;
    }
}

// url: http://mathworld.wolfram.com/Circle-CircleIntersection.html
// Méthode utilisant un changement de repère (2D)
bool compute_intersections_circles(in vec2 O0, in float R0, in vec2 O1, in float R1, out vec2 P[2])
{
    bool b_has_intersections = false;
    //
    vec2 O0_to_01 = O1 - O0;
    vec2 normal_O0_to_01 = normalize(NORMAL(O0_to_01));
    //
    float d = length(O0_to_01);
    float r = R1;
    float R = R0;
    float d2 = d*d;
    float r2 = r*r;
    float R2 = R*R;
    // Calcul de la coordonnée x commune aux 2 intersections
    float denum       = 1.0/(2.0*d);
    float denum2      = denum*denum;
    float x           = (d2 - r2 + R2)*denum;
    // Calcul des y des intersections
    float square_y    = (4.0*d2*R2 - (d2-r2+R2)*(d2-r2+R2))*denum2;

    float y01         = sqrt(square_y);
    float y0          = +y01;
    float y1          = -y01;
    // Normalisation des vecteurs caractérisant les axes du repère de calcul d'intersections
    O0_to_01        = O0_to_01 / d;
    // Reconstruction des solutions (i.e. projection sur les axes du repère, local -> world)
    P[0] = O0_to_01*x + normal_O0_to_01*y0;
    P[1] = O0_to_01*x + normal_O0_to_01*y1;

    b_has_intersections = true;

    return b_has_intersections;
}

vec4 encode_shadow_R(float f_coef_shadow)
{
    vec4 color = vec4( f_coef_shadow, 0, 0, 0 );
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

// Repère main droite pour retrouver le sens de la normale de la droite (son 'Z')
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

float compute_visbility_light(in vec2 P, in vec2 E, in float r)
{
    float visibilty = 1.;
    vec2 pos_light = vec2(0., 0.);

    float f_signed_distance = signed_distance_point_line(pos_light, P, E);

    float d = f_signed_distance/r;
    d = clamp(d, -1, +1); // clip (use for full-light and full-shadow)

    // Aire de visibilité de la source (disque de lumiere)
    visibilty = (1-d)*0.5;

    return visibilty;
}

float compute_visbility_light(in vec2 P, in vec2 E0, in vec2 E1, in float r)
{
    float visibilty = 1.;

    // Compute visibility of the circle light
    float A0 = compute_visbility_light(P, E0, r);
    float A1 = compute_visbility_light(P, E1, r);

    // Do some tests to fix the visibilty (depends of orientations between receiver point, edge, and the circle light source)
    vec2 Edge = E1 - E0;
    vec2 P_E0 = E0 - P;
    vec2 P_E1 = E1 - P;
    vec2 n_P_E0 = NORMAL(P_E0);
    vec2 n_P_E1 = NORMAL(P_E1);
    // Orientation du volume d'ombre
    A0 = dot(n_P_E0, +Edge) > 0 ? (1-A0) : A0;
    A1 = dot(n_P_E1, -Edge) > 0 ? (1-A1) : A1;
    // Sens de projection de l'ombre
    A0 = float(dot(P_E0, E0) < 0.0) * A0;
    A1 = float(dot(P_E1, E1) < 0.0) * A1;

    visibilty = (A0 + A1);

    return visibilty;
}
