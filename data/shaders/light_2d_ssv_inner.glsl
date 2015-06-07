#define DET(A, B)               ((A).x*(B).y - (B).x*(A).y)

#define __USE_RETRO_PROJECTION__
//#define __USE_METHOD_WITH_DISTANCE__

#define __USE_REMAP__
// Remap penumbra coefficient for the inner-penumbra: [0, 1] -> [0.5, 0]
#define REMAP_PENUMBRA(coef_penumbra)   (0.5 - ((coef_penumbra)/2))

uniform vec2    u_v_position_light;
uniform float   u_f_radius_light;
uniform float   u_f_influence_radius_light ;
//
uniform vec2 u_v_e0;
uniform vec2 u_v_e1;
uniform vec2 u_v_intersection_normal;

//varying  vec2 v_v_position;
in vec2 v_v_position;

float   distance_point_line(in vec2 A, in vec2 B, in vec2 C);
vec2    compute_intersection_lines( vec2 P1, vec2 P2, vec2 P3, vec2 P4);

float   compute_penumbra_wd(in vec2 v_pos, in vec2 v_edge);
float   compute_penumbra_rp(in vec2 v_pos, in vec2 v_edge, in vec2 v_normal);
float   compute_penumbra_rp(in vec2 v_pos, in vec2 v_edges[2], in vec2 v_normal);

float   hermite(float s, float P1, float P2, float T1, float T2);

void main()
{
    vec2    v_pos_in_ls         = v_v_position - u_v_position_light; // position du vertex edge dans le repere lumiere (Light Space)
    vec2    v_pos_light_in_ls   = vec2(0.f); // origine du repere

    if (length(v_pos_in_ls) > u_f_influence_radius_light )
        discard;

    float coef_penumbra;

    vec2 v_intersections[2];
    v_intersections[0] = u_v_e0;
    v_intersections[1] = u_v_e1;
    float coef_penumbra_rp = compute_penumbra_rp(v_pos_in_ls, v_intersections, u_v_intersection_normal);

    float coef_penumbra_wd = compute_penumbra_wd(v_pos_in_ls, u_v_e0);

    #ifdef __USE_RETRO_PROJECTION__
        coef_penumbra = coef_penumbra_rp;
    #endif
    #ifdef  __USE_METHOD_WITH_DISTANCE__
        coef_penumbra = coef_penumbra_wd;
    #endif
//    coef_penumbra = abs(coef_penumbra_wd - coef_penumbra_rp)*10; // difference entre les methodes (normalement il ne devrait pas en avoir ...)

    // remap le coefficient de penombre
    #ifdef __USE_REMAP__
        coef_penumbra = REMAP_PENUMBRA(coef_penumbra);
    #endif

    gl_FragColor = vec4(coef_penumbra, 0, 0, 1);
}

////////////////////////////////////////////////////////////
/// \brief Calcul un coefficient de penombre (Methode1)
///
/// \param v_pos position du texel en light space
/// \param v_edge position du sommet de l'edge en light space
///
/// \return coefficient de penombre compris dans [0, 1]
///
////////////////////////////////////////////////////////////
float compute_penumbra_wd(in vec2 v_pos, in vec2 v_edge)
{
    // -    Methode par calcul du ratio de la distance entre le point courant et
    //      sa projection orthogonale sur l'axe reliant le vertex edge et le centre de la source de lumiere (droite support du Shadow-Volume)

    // distance du texel courant par rapport ÃÂ 
    // l'axe reliant le centre de la source de lumiere et le vertex de l'edge
    float   f_dist_P    = distance_point_line(v_edge, vec2(0), v_pos);
    // Utilisation d'un theoreme de Thales pour calculer des rapports de distance
    float   f_l         = u_f_radius_light*distance(v_pos, v_edge)/length(v_edge);
    // Calcul du coefficient de penombre (une approximation)
    return abs(f_dist_P/f_l);
}

////////////////////////////////////////////////////////////
/// \brief Calcul un coefficient de penombre (Methode2-a)
///
/// \param v_pos position du texel en light space
/// \param v_edge position du sommet de l'edge en light space
/// \param v_normal normale de la ligne liant v_edge au centre de la source de lumiere (elle peut ne pas ÃÂªtre unitaire ou orientee, on ne desire que sa direction)
///
/// \return coefficient de penombre compris dans [0, 1]
///
////////////////////////////////////////////////////////////
float compute_penumbra_rp(in vec2 v_pos, in vec2 v_edge, in vec2 v_normal)
{
    // -    Methode2-a par retro-projection d'un sommet de l'edge sur la source de lumiere ("reconstruction" du recouvrement de l'arÃÂªte sur la source de lumiere)
    //      hypothese: les volumes de penombres des sommets consecutifs ne se recouvrent pas.
    // - intersection des lignes: (v_pos, v_edge) (centre de la lumiere, un des 2 centres de projection pour la lumiere etendue)
    //      L'intersection peut etre considerer comme la projection du sommet edge sur la source de lumiere.
    //      La distance de cette intersection nous fournit un coefficient de recouvrement/d'occlusion de la projection de l'arÃÂªte sur la source de lumiere.
    vec2 intersection_lines = compute_intersection_lines( v_pos, v_edge, vec2(0), v_normal);
    // rapport de distance qui nous fournit le coefficient de penombre (ou recouvrement de la source de lumiere par l'arÃÂªte)
    return (length(intersection_lines)/u_f_radius_light);
}

////////////////////////////////////////////////////////////
/// \brief Calcul un coefficient de penombre (Methode2-b)
///
/// \param v_pos position du texel en light space
/// \param v_edges positions des sommets de l'edge en light space
/// \param v_normal normale de la ligne liant v_edge au centre de la source de lumiere (elle peut ne pas ÃÂªtre unitaire ou orientee, on ne desire que sa direction)
///
/// \return coefficient de penombre compris dans [0, 1]
///
////////////////////////////////////////////////////////////
float compute_penumbra_rp(in vec2 v_pos, in vec2 v_edges[2], in vec2 v_normal)
{
    // -    Methode2-b par retro-projection de l'edge sur la source de lumiere

    // - intersection des lignes: (v_pos, v_edge) (centre de la lumiere, un des 2 centres de projection pour la lumiere etendue)
    //      L'intersection peut etre considerer comme la projection du sommet edge sur la source de lumiere.
    //      La distance de cette intersection nous fournit un coefficient de recouvrement/d'occlusion de la projection de l'arÃÂªte sur la source de lumiere.
    vec2    intersections_lines[2];
    float   lengths[2];

    for(int i=0; i<2; ++i)
    {
        intersections_lines[i] = compute_intersection_lines( v_pos, v_edges[i], vec2(0), v_normal);
        lengths[i] = length(intersections_lines[i]);
    }

    float f_coef_covering = u_f_radius_light - (min(lengths[1], u_f_radius_light) - lengths[0]);

    // rapport de distance qui nous fournit le coefficient de penombre (ou recouvrement de la source de lumiere par l'arÃÂªte)
    return f_coef_covering/u_f_radius_light;
}

float distance_point_line(in vec2 A, in vec2 B, in vec2 C)
{
    vec2 AB = B - A;
    vec2 CA = A - C;
    float f_length_AB = length(AB);
    return DET(AB, CA)/f_length_AB;
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
