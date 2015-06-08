#define DET(A, B)               ((A).x*(B).y - (B).x*(A).y)
#define NORMAL(V2)              vec2(-(V2).y, (V2).x)
#define EQUAL0_EPS(_f, _eps)    ((_f)>=-(_eps) && (_f)<=+(_eps))
#define EQUAL_EPS(_f1, _f2, _eps)   (((_f1)-(_f2))>=-(_eps) && ((_f1)-(_f2))<=+(_eps))

uniform vec2    u_v_position_light;
uniform float   u_f_radius_light;
uniform float   u_f_influence_radius_light ;
//
uniform vec2 u_v_e0;
uniform vec2 u_v_e1;
uniform vec2 u_v_intersection_normal;

varying vec2 v_v_position;

vec2    compute_intersection_lines( vec2 P1, vec2 P2, vec2 P3, vec2 P4);
float   compute_penumbra_rp(in vec2 v_pos, in vec2 v_edges[2], in vec2 v_normal);
bool    inside_half_plane(in vec2 A, in vec2 B, in vec2 P);

void main()
{
    vec2    v_pos_in_ls         = v_v_position - u_v_position_light; // position du vertex edge dans le repere lumiere (Light Space)
    vec2    v_pos_light_in_ls   = vec2(0); // origine du repere

    // inside influence circle ?
    if (length(v_pos_in_ls) > u_f_influence_radius_light )
        discard;
    //
    if (!inside_half_plane(u_v_e0, u_v_e1, v_pos_in_ls))
        discard;
    // Discards pour eviter les problemes quand
    // le centre de la source de lumiere est sur la droite portant l'edge
    // Texel receiver du bon cote ?
    float dot_P_i0 = dot(v_pos_in_ls, u_v_e0);
    if (dot_P_i0 < 0)
        discard;
    // La lumiere est alignee avec la droite support du segment-edge
//    vec2 v_dir_i0i1 = normalize(u_v_e1 - u_v_e0);
//    float f_dot = abs(dot(normalize(u_v_e0), v_dir_i0i1));
//    if (f_dot > 0.99519)
//        discard;

    vec2 v_dir_i0P = v_pos_in_ls - u_v_e0;
    vec2 v_normal_i0P = normalize(NORMAL(v_dir_i0P));
    vec2 v_normal_i1P = normalize(NORMAL(v_pos_in_ls - u_v_e1));

    vec2 v_il_0 = compute_intersection_lines( v_pos_in_ls, u_v_e0, v_pos_light_in_ls, v_normal_i0P);
    vec2 v_il_1 = compute_intersection_lines( v_pos_in_ls, u_v_e1, v_pos_light_in_ls, v_normal_i1P);

    float l0 = dot(v_il_0, v_normal_i0P);
    float l1 = dot(v_il_1, v_normal_i1P);

    float l_min = clamp( min(l1, l0), -u_f_radius_light, +u_f_radius_light);
    float l_max = clamp( max(l1, l0), -u_f_radius_light, +u_f_radius_light);

    float f_coef_covering = (u_f_radius_light - (l_max-l_min));
    float f_coef = f_coef_covering/(2*u_f_radius_light) + 0.5;

    f_coef = (f_coef<0.5 && f_coef>0.0) ? f_coef : 0.0;

    gl_FragColor = vec4(f_coef, f_coef, f_coef, 1);
}

////////////////////////////////////////////////////////////
/// \brief Calcul un coefficient de penombre (Methode2-b)
///
/// \param v_pos position du texel en light space
/// \param v_edges positions des sommets de l'edge en light space
/// \param v_normal normale de la ligne liant v_edge au centre de la source de lumiere (elle peut ne pas aªtre unitaire ou orientee, on ne desire que sa direction)
///
/// \return coefficient de penombre compris dans [0, 1]
///
////////////////////////////////////////////////////////////
float compute_penumbra_rp(in vec2 v_pos, in vec2 v_edges[2], in vec2 v_normal)
{
    // -    Methode2-b par retro-projection de l'edge sur la source de lumiere

    // - intersection des lignes: (v_pos, v_edge) (centre de la lumiere, un des 2 centres de projection pour la lumiere etendue)
    //      L'intersection peut etre considerer comme la projection du sommet edge sur la source de lumiere.
    //      La distance de cette intersection nous fournit un coefficient de recouvrement/d'occlusion de la projection de l'araªte sur la source de lumiere.
    vec2    intersections_lines[2];
    float   lengths[2];

    for(int i=0; i<2; ++i)
    {
        intersections_lines[i] = compute_intersection_lines( v_pos, v_edges[i], vec2(0), v_normal);
        lengths[i] = length(intersections_lines[i]);
    }

    float f_coef_covering = u_f_radius_light - (min(lengths[1], u_f_radius_light) - lengths[0]);
    return f_coef_covering/u_f_radius_light;
}

// Line0: (P1, P2)
// Line1: (P3, P4)
// result: intersection point
// note: don't test if lines arec oolinear
vec2 compute_intersection_lines(vec2 P1, vec2 P2, vec2 P3, vec2 P4)
{
    vec2 P3P1 = P1 - P4;
    vec2 P1P2 = P2 - P1;
    vec2 P3P4 = P4 - P3;

    float denum = 1.f/DET(P1P2, P3P4);
    float ua    = DET(P3P4, P3P1)*denum;

    return P1 + P1P2*ua;
}

bool inside_half_plane(in vec2 A, in vec2 B, in vec2 P)
{
    // Du bon cote du demi-plan dont l'edge est la frontiere (ou sa droite) et oriente pour ne pas contenir la source de lumiere
    // equation parametrique d'une droite: (1) a.x + b.y + c = 0, avec (a,b) normale de la droite
    vec2 v_dir      = B - A;
    vec2 v_normal   = NORMAL(v_dir);
    // on calcul c => c = -(a*x + b*y), on prend (x, y) = A (1 point de la droite)
    float c = - dot(v_normal, A);
    // dans quel sens la normale est orientee pour relier le point P et la droite ?
    float side_of_P = dot(P, v_normal) + c;
    // selon le sens (qui indique le sens de la normale), on determine si le point est dans le demi-plan
    return (side_of_P*sign(c)<0);
}

//
//    float coef_penumbra;
//
//    vec2 v_intersections[2];
//    v_intersections[0] = u_v_e0;
//    v_intersections[1] = u_v_e1;
//    float coef_penumbra_rp = compute_penumbra_rp(v_pos_in_ls, v_intersections, u_v_intersection_normal);
//
//    coef_penumbra = coef_penumbra_rp;
//
//    gl_FragColor = vec4(coef_penumbra, coef_penumbra, coef_penumbra, 1);

