#define DET(A, B)               ((A).x*(B).y - (B).x*(A).y)
#define NORMAL(V2)              vec2(-(V2).y, (V2).x)
#define EQUAL0_EPS(_f, _eps)    ((_f)>=-(_eps) && (_f)<=+(_eps))
#define EQUAL_EPS(_f1, _f2, _eps)   (((_f1)-(_f2))>=-(_eps) && ((_f1)-(_f2))<=+(_eps))

uniform vec2    u_v_position_light;
uniform float   u_f_radius_light;
uniform float   u_f_influence_radius_light ;
//
uniform vec2 u_v_intersection0_segment_circle;
uniform vec2 u_v_intersection1_segment_circle;
uniform vec2 u_v_intersection_normal;

varying vec2 v_v_position;

vec2    compute_intersection_lines( vec2 P1, vec2 P2, vec2 P3, vec2 P4);
float   compute_penumbra_rp(in vec2 v_pos, in vec2 v_edges[2], in vec2 v_normal);
bool    inside_half_plane(in vec2 A, in vec2 B, in vec2 P);

void main()
{
    vec2    v_pos_in_ls         = v_v_position - u_v_position_light; // position du vertex edge dans le repère lumière (Light Space)
    vec2    v_pos_light_in_ls   = vec2(0); // origine du repère

    // inside influence circle ?
    if (length(v_pos_in_ls) > u_f_influence_radius_light )
        discard;
    //
    if (!inside_half_plane(u_v_intersection0_segment_circle, u_v_intersection1_segment_circle, v_pos_in_ls))
        discard;
    // Discards pour éviter les problèmes quand
    // le centre de la source de lumière est sur la droite portant l'edge
    // Texel receiver du bon coté ?
    float dot_P_i0 = dot(v_pos_in_ls, u_v_intersection0_segment_circle);
    if (dot_P_i0 < 0)
        discard;
    // La lumière est alignée avec la droite support du segment-edge
//    vec2 v_dir_i0i1 = normalize(u_v_intersection1_segment_circle - u_v_intersection0_segment_circle);
//    float f_dot = abs(dot(normalize(u_v_intersection0_segment_circle), v_dir_i0i1));
//    if (f_dot > 0.99519)
//        discard;

    //
    vec2 v_dir_i0P = v_pos_in_ls - u_v_intersection0_segment_circle;
    vec2 v_dir_i1P = v_pos_in_ls - u_v_intersection1_segment_circle;
    //
    vec2 v_normal_i0P = normalize(NORMAL(v_dir_i0P));
    vec2 v_normal_i1P = normalize(NORMAL(v_dir_i1P));
    //
    vec2 v_il_0 = compute_intersection_lines( v_pos_in_ls, u_v_intersection0_segment_circle, v_pos_light_in_ls, v_normal_i0P);
    vec2 v_il_1 = compute_intersection_lines( v_pos_in_ls, u_v_intersection1_segment_circle, v_pos_light_in_ls, v_normal_i1P);
    // distances signées (orientées)
    float l0 = dot(v_il_0, v_normal_i0P);
    float l1 = dot(v_il_1, v_normal_i1P);
    // min max des longueurs
    float l_min = min(l1, l0);
    float l_max = max(l1, l0);
    // Clipping des min/max
    l_min = max(l_min, -u_f_radius_light);
    l_max = min(l_max, +u_f_radius_light);
    // edge covering:
    float f_edge_covering = (l_max-l_min)/(2*u_f_radius_light);
    // shadow coefficient:
    float f_coef_shadow = 1.0 - f_edge_covering;

    // write result
    gl_FragColor = vec4(f_coef_shadow);
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
    // Du bon coté du demi-plan dont l'edge est la frontière (ou sa droite) et orienté pour ne pas contenir la source de lumière
    // equation paramétrique d'une droite: (1) a.x + b.y + c = 0, avec (a,b) normale de la droite
    vec2 v_dir      = B - A;
    vec2 v_normal   = NORMAL(v_dir);
    // on calcul c => c = -(a*x + b*y), on prend (x, y) = A (1 point de la droite)
    float c = - dot(v_normal, A);
    // dans quel sens la normale est orientée pour relier le point P et la droite ?
    float side_of_P = dot(P, v_normal) + c;
    // selon le sens (qui indique le sens de la normale), on détermine si le point est dans le demi-plan
    return (side_of_P*sign(c)<0);
}

//
//    float coef_penumbra;
//
//    vec2 v_intersections[2];
//    v_intersections[0] = u_v_intersection0_segment_circle;
//    v_intersections[1] = u_v_intersection1_segment_circle;
//    float coef_penumbra_rp = compute_penumbra_rp(v_pos_in_ls, v_intersections, u_v_intersection_normal);
//
//    coef_penumbra = coef_penumbra_rp;
//
//    gl_FragColor = vec4(coef_penumbra, coef_penumbra, coef_penumbra, 1);

