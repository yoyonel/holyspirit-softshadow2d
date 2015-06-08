#include "Light_SV_Tools.h"

bool Intersect_BoundingBox(const vec2& _v00, const vec2& _v01, const vec2& _v10, const vec2& _v11)
{
    vec2 min_0(fmin(_v00.x, _v01.x), fmin(_v00.y, _v01.y));
    vec2 max_0(fmax(_v00.x, _v01.x), fmax(_v00.y, _v01.y));
    vec2 min_1(fmin(_v10.x, _v11.x), fmin(_v10.y, _v11.y));
    vec2 max_1(fmax(_v10.x, _v11.x), fmax(_v10.y, _v11.y));
    //
    return !((min_0.x > max_1.x) || (min_1.x > max_0.x) || (min_0.y > max_1.y) || (min_1.y > max_0.y));
}

Typ_Solutions_Quadratic_Equation Solve_Quadratic_Equation(float A, float B, float C)
{
    Typ_Solutions_Quadratic_Equation solutions = {false, 0, 0};

    // (1-b)  systeme quadratique: AxÃÂ² + Bx + C = 0
    float f_constant    = C;
    float f_linear      = B;
    float f_quadratic   = A;
    // - f_delta: discriminant de l'equation
    float f_delta       = f_linear*f_linear - 4*f_constant*f_quadratic;
    // si
    // - f_delta < 0: 0 solution    <=> pas d'intersections
    // - f_delta = 0: 1 solution    <=> la droite est tangente au cercle
    // - f_delta > 0: 2 solutions   <=> la droite intersecte en deux points (distinctes) le cercle
    solutions.b_has_solutions = f_delta > 0+EPSILON;

    // - si intersection (la ligne support avec le cercle de lumiere)
    if (solutions.b_has_solutions)
    {
        // - calcul des points d'intersections
        //      f_quadratic*xÃÂ² + f_linear*x + f_constant = 0
        //      delta = f_linearÃÂ² - 4*f_quadratic*f_constant (delta>0)
        //          1ere solution= (-f_linear+racine(delta))/(2*f_quadratic)
        //          2nde solution= (-f_linear-racine(delta))/(2*f_quadratic)
        float f_square_root_delta = sqrt(f_delta);
        float f_denum = 1.f/(2*f_quadratic);
        solutions.f_u0 = (-f_linear-f_square_root_delta)*f_denum;
        solutions.f_u1 = (-f_linear+f_square_root_delta)*f_denum;
    }

    return solutions;
}

Typ_Solutions_Quadratic_Equation Solve_Quadratic_Equation(vec2 _A, vec2 _AB, float _radius)
{
    // (1-b)  systeme quadratique pour trouver les intersections de la droite support du mur et le cercle a  l'origine
    float f_constant    = NORM2(_A) - _radius;
    float f_linear      = 2*DOT(_A, _AB);
    float f_quadratic   = NORM2(_AB);
    //
    return Solve_Quadratic_Equation(f_quadratic, f_linear, f_constant);
}

bool Quadratic_Equation_Has_Solutions(vec2 _A, vec2 _AB, float _radius)
{
    // (1-b)  systeme quadratique pour trouver les intersections de la droite support du mur et le cercle a  l'origine
    float f_constant    = NORM2(_A) - _radius;
    float f_linear      = 2.f*DOT(_A, _AB);
    float f_quadratic   = NORM2(_AB);
    // - f_delta: discriminant de l'equation
    float f_delta       = f_linear*f_linear - 4.f*f_constant*f_quadratic;
    // si
    // - f_delta < 0: 0 solution    <=> pas d'intersections
    // - f_delta = 0: 1 solution    <=> la droite est tangente au cercle
    // - f_delta > 0: 2 solutions   <=> la droite intersecte en deux points (distinctes) le cercle
    return (f_delta > 0.f);
}

// P1, P2 points sur un maªme cercle
// Line0: (P1, P2=P1 + normal(OP1))
// Line1: (P3, P4=P3 + normal(OP2))
// result: intersection point
// note: don't test if lines arec oolinear
vec2 Compute_Intersection_Lines( const vec2& P1, const vec2& P3)
{
    const vec2 P2 = P1 + NORMAL(P1);
    const vec2 P4 = P3 + NORMAL(P3);

    vec2 P3P1(P1 - P4);
    vec2 P1P2(P2 - P1);
    vec2 P3P4(P4 - P3);

    float denum = 1.f/DET(P1P2, P3P4);
    float ua    = DET(P3P4, P3P1)*denum;

   return P1 + P1P2*ua;
}

// Line0: (P1, P2)
// Line1: (P3, P4)
// result: intersection point
// note: don't test if lines arec oolinear
vec2 Compute_Intersection_Lines( const vec2& P1, const vec2& P2, const vec2& P3, const vec2& P4)
{
    vec2 P3P1(P1 - P4);
    vec2 P1P2(P2 - P1);
    vec2 P3P4(P4 - P3);

    float denum = 1.f/DET(P1P2, P3P4);
    float ua    = DET(P3P4, P3P1)*denum;

   return P1 + P1P2*ua;
}

// Segment0: (P1, P2)
// Segment1: (P3, P4)
// result: intersection point
// note: don't test if lines arec oolinear
bool Compute_Intersection_Segments( const vec2& P1, const vec2& P2, const vec2& P3, const vec2& P4, vec2& P)
{
    bool b_segments_intersect = false;

    // intersection des droites
    P = Compute_Intersection_Lines(P1, P2, P3, P4);

    // est ce que le point est sur les segments ?
    const vec2 P1_P2 = P2-P1;
    const vec2 P3_P4 = P4-P3;
    const vec2 P_P4  = P4-P;
    const vec2 P_P2  = P2-P;
    // dans le meme sens, et de longueur (carree) inferieure a  celui du segment
    b_segments_intersect =
        DOT(P1_P2, P_P2) >= 0.0 && NORM2(P_P2) <= NORM2(P1_P2) &&
        DOT(P3_P4, P_P4) >= 0.0 && NORM2(P_P4) <= NORM2(P3_P4);

    return b_segments_intersect;
}

// url: http://mathworld.wolfram.com/Circle-CircleIntersection.html
// Methode utilisant un changement de repere (2D)
bool Compute_Intersection_Circles_1(const vec2& O0, const float R0, const vec2& O1, const float R1, vec2 P[2])
{
    bool b_has_intersections = false;
    //
    vec2 O0_to_01 = O1 - O0;
    vec2 normal_O0_to_01 = NORMAL(O0_to_01);
    //
    const float &r = R1;
    const float &R = R0;
    //
    const float d = NORM(O0_to_01);
    const float d2 = d*d;
    const float r2 = r*r;
    const float R2 = R*R;
    // Calcul de la coordonnee x commune aux 2 intersections
    const float denum       = 1/(2*d);
    const float denum2      = denum*denum;
    const float x           = (d2 - r2 + R2)*denum;
    // Calcul des y des intersections
    const float square_y    = (4*d2*R2 - (d2-r2+R2)*(d2-r2+R2))*denum2;
    if (square_y >= -EPSILON)
    {
        const float y01         = sqrtf(square_y);
        const float y0          = +y01;
        const float y1          = -y01;
        // Normalisation des vecteurs caracterisant les axes du repere de calcul d'intersections
        O0_to_01        = O0_to_01 / d;
        normal_O0_to_01 = NORMALIZE(normal_O0_to_01);
        // Reconstruction des solutions (i.e. projection sur les axes du repere, local -> world)
        P[0] = O0_to_01*x + normal_O0_to_01*y0;
        P[1] = O0_to_01*x + normal_O0_to_01*y1;

        b_has_intersections = true;
    }
    else
    {
        DEBUG_PRINT("square_y", square_y);
    }

    return b_has_intersections;
}

// Methode brute force, sans changement de repere
// url: http://math.15873.pagesperso-orange.fr/IntCercl.html#ancre0
bool Compute_Intersection_Circles_0(const vec2& O0, const float R0, const vec2& O1, const float R1, vec2 P[2])
{
    const float& x0 = O0.x;
    const float& y0 = O0.y;
    const float& x1 = O1.x;
    const float& y1 = O1.y;
    //
    float A, B, C;
    Typ_Solutions_Quadratic_Equation intersections;
    //
    float y0_y1 = y0-y1;
    float epsilon = 1e3*std::numeric_limits<float>::epsilon();
    if (!EQUAL0_EPS(y0_y1, epsilon))
    {
        float N = (R1*R1 - R0*R0 - x1*x1 + x0*x0 - y1*y1 + y0*y0)/(2*y0_y1);
        //
        float x0x1_on_y0y1          = (x0-x1)/y0_y1;
        float square_x0x1_on_y0y1   = x0x1_on_y0y1*x0x1_on_y0y1;
        //
        A = square_x0x1_on_y0y1 + 1;
        B = 2 * (x0x1_on_y0y1*(y0 - N) - x0);
        C = x0*x0 + y0*y0 - R0*R0 + N*(N - 2*y0);
        //
        intersections = Solve_Quadratic_Equation(A, B, C);
        //
        P[0].x = intersections.f_u0;
        P[1].x = intersections.f_u1;
        //
        P[0].y = N - P[0].x*x0x1_on_y0y1;
        P[1].y = N - P[1].x*x0x1_on_y0y1;
    }
    else
    {
        float x = (R1*R1 - R0*R0 - x1*x1 + x0*x0) / (2*(x0 - x1));
        A = 1;
        B = -2*y1;
        C = x1*x1 + x*x - 2*x1*x + y1*y1 - R1*R1;
        //
        intersections = Solve_Quadratic_Equation(A, B, C);
        //
        P[0].x = x;
        P[1].x = x;
        //
        P[0].y = intersections.f_u0;
        P[1].y = intersections.f_u1;
    }
    //
    return intersections.b_has_solutions;
}

// Repere main droite pour retrouver le sens de la normale de la droite (son 'Z')
float signed_distance_point_line( const vec2& P, const vec2& A, const vec2& B )
{
    vec2 AB = B - A;

    vec2 v = NORMALIZE(NORMAL(AB));
    vec2 r = A - P;

    return DOT(v, r);
}

float distance_point_line( const vec2& P, const vec2& A, const vec2& B )
{
    return abs(signed_distance_point_line( P, A, B ));
}

float area_triangle( const vec2& A, const vec2& B, const vec2& C )
{
    return 0.5f*fabs((B.x-A.x)*(C.y-A.y) - (C.x-A.x)*(B.y-A.y));
}

#define PRINT_STENCIL_INFO \
    { \
        GLint stencil_bits; \
        glGetIntegerv(GL_STENCIL_BITS, &stencil_bits); \
        std::cout<<"stencil_bits:"<<stencil_bits<<std::endl; \
    } \

// Affiche le nombre de primitive (en utilisant un texte graphique)
//        {
//            sf::Text s_nb_m_shape_sv;
//            std::ostringstream stm;
//            stm << "m_shape_sv.size():" << m_shape_sv.size();
//            s_nb_m_shape_sv.SetString(stm.str());
//            s_nb_m_shape_sv.SetFont(sf::Font::GetDefaultFont());
//            s_nb_m_shape_sv.SetPosition(0, 100);
//            s_nb_m_shape_sv.SetBlendMode(sf::Blend::None);
//            App->Draw(s_nb_m_shape_sv);
//        }

bool Compute_Intersection_Segment_Circle(
                                         // IN
                                         const vec2& pos_light,
                                         const float r,
                                         const vec2 &E0,
                                         const vec2 &E1,
                                         // OUT
                                         vec2 intersections_segment_circle[2]
                                         )
{
    bool result = false;

    vec2 l1 = E0 - pos_light;
    vec2 l2 = E1 - pos_light;
    vec2 l1_to_l2 = l2 - l1;

    // Test d'intersection entres les BBox du cercle et du segment/wall
    if (Intersect_BoundingBox(l1, l2, sf::Vector2f(-r,-r), sf::Vector2f(+r,+r)))
    {
        float f_square_radius_light = r*r;

        // (1-b)  systeme quadratique pour trouver les intersections de la droite support du mur et le cercle d'influence a  l'origine
        Typ_Solutions_Quadratic_Equation solutions = Solve_Quadratic_Equation(l1, l1_to_l2, f_square_radius_light);

        //
        bool b_wall_intersect_circle = true;

        // - si intersection (la ligne support avec le cercle de lumiere)
        if (solutions.b_has_solutions)
        {
            const float &f_u1 = solutions.f_u0;
            const float &f_u2 = solutions.f_u1;
            // On reconstruit les intersections
            sf::Vector2f intersections_line_circle[2] = {
                sf::Vector2f(l1 + l1_to_l2*f_u1),
                sf::Vector2f(l1 + l1_to_l2*f_u2)
            };
            // On calcul les distances au carre
            //  du segment [l1, l2]
            //  du segment [O, l1] (O: centre de la source de lumiere => O=(0, 0))
            //  du segment [O, l2] (O: centre de la source de lumiere => O=(0, 0))
            //  du segment [l1, i1]
            //  du segment [l1, i2]
            float f_square_l1l2 = DOT(l1_to_l2, l1_to_l2);
            float f_square_l1   = DOT(l1, l1);
            float f_square_l2   = DOT(l2, l2);
            float f_square_l1i1 = (f_u1*f_u1)*f_square_l1l2;
            float f_square_l1i2 = (f_u2*f_u2)*f_square_l1l2;

            // 4 cas d'intersections/inclusion possibles pour le segment et le cercle
            // Est ce que ...
            bool test10 = (f_square_l1<=f_square_radius_light);                                                         // ... le 1er point du segment/wall est dans le cercle ?
            bool test11 = (f_square_l2<=f_square_radius_light);                                                         // ... le 2nd point du segment/wall est dans le cercle ?
            bool test01 = !((f_u1<=0 && f_u2<=0) || (f_square_l1i1>=f_square_l1l2 && f_square_l1i2>=f_square_l1l2));    // ... le segment d'intersection intersecte le segment/wall ?
            bool test00 = !(test10 || test11);                                                                          // ... les deux points du segment sont en dehors du cercle ?
            bool test20 = !test10;                                                                                      // ... le 1er point du segment/wall n'est pas dans le cercle ?
            bool &test21 = test11;                                                                                      // ... le 2nd point du segment/wall est dans le cercle ?
            bool &test30 = test10;                                                                                      // ... le 1er point du segment/wall est dans le cercle ?
            bool test31 = !test11;                                                                                      // ... le 2nd point du segment/wall n'est pas dans le cercle ?
            //
            if (test00&&test01) // cas 1: les vertex du segment/wall ne sont pas inclus dans le cercle et il y a intersection
            {
                intersections_segment_circle[0] = intersections_line_circle[0];
                intersections_segment_circle[1] = intersections_line_circle[1];
            }
            else if (test10&&test11) // cas 2: les vertex du segment/wall sont inclus (tous les 2) dans le cercle
            {
                intersections_segment_circle[0] = l1;
                intersections_segment_circle[1] = l2;
            }
            else if (test20&&test21) // cas 3: Un des deux sommets du segment/wall est inclu dans le cercle (le 1er sommet dans ce cas)
            {
                intersections_segment_circle[0] = intersections_line_circle[0];
                intersections_segment_circle[1] = l2;
            }
            else if (test30&&test31) // cas 4: Un des deux sommets du segment/wall est inclu dans le cercle (le 2nd sommet dans ce cas)
            {
                intersections_segment_circle[0] = l1;
                intersections_segment_circle[1] = intersections_line_circle[1];

            }
            else
            {
                // sinon le segment/wall n'intersecte pas le cercle de lumiere (donc ne projette pas d'ombre)
                // Ce cas correspond a  la presence du segment dans un coin de la boundingbox du cercle (mais non inclut dans le cercle)
                b_wall_intersect_circle = false;
            }
        }
        result = b_wall_intersect_circle && solutions.b_has_solutions;
    }

    return result;
}

sf::Shape construct_shadow_volume_shape( const vec2 &pos_light, const float r, const vec2 intersections_segment_circle[2], const sf::Color &color, sf::Vector2f proj_intersections[2], sf::Vector2f bounding_vertex_sv[2] )
{
    sf::Shape shape;

    // projection des sommets clippes sur le cercle d'influence de la lumiere
    proj_intersections[0] = NORMALIZE(intersections_segment_circle[0]) * r;
    proj_intersections[1] = NORMALIZE(intersections_segment_circle[1]) * r;

    // Milieu du segment clippe
    sf::Vector2f mid_i0_i1       = COMPUTE_MIDDLE(proj_intersections[0], proj_intersections[1]);
    // Projection du milieu (a) sur le cercle
    sf::Vector2f proj_mid_i0_i1  = NORMALIZE(mid_i0_i1) * r;
    // Calcul des vertex englobant
    // Ils correspondent aux intersections des lignes formees par:
    //  les sommets de projection des intersections
    //  et leurs normales associees, tangentes au cercle.
    bounding_vertex_sv[0] = Compute_Intersection_Lines(proj_intersections[0], proj_mid_i0_i1);
    bounding_vertex_sv[1] = Compute_Intersection_Lines(proj_intersections[1], proj_mid_i0_i1);

    // Construction de la shape ShadowVolume
    shape.AddPoint( intersections_segment_circle[0],    color, WHITE);
    shape.AddPoint( proj_intersections[0],              color, WHITE);
    shape.AddPoint( bounding_vertex_sv[0],              color, WHITE);
    shape.AddPoint( bounding_vertex_sv[1],              color, WHITE);
    shape.AddPoint( proj_intersections[1],              color, WHITE);
    shape.AddPoint( intersections_segment_circle[1],    color, WHITE);
    // On regle le shape sans blending et on fournit la position
    shape.SetBlendMode(sf::Blend::None);
    shape.SetPosition(pos_light);
    // - ajout dans la shape SV des quads (sv) ainsi generes
    //m_shape_sv.push_back(shape);

    return shape;
}

