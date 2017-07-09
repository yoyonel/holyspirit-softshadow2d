#include "Light_Wall.h"
#include "Light_SV_Tools.h"

using namespace bv; // bounding volume namespace

int indice_text_line = -1;

Light_Wall::Light_Wall()
{
    Initialize();
}

Light_Wall::Light_Wall(const vec2 &_pos, const float _inner_radius, const float _influence_radius, const vec2 &vertex_wall_0, const vec2 &vertex_wall_1)
{
    Initialize();
    Set(_pos, _inner_radius, _influence_radius, vertex_wall_0, vertex_wall_1);
}

void Light_Wall::Initialize()
{
    m_vector_bv.clear();
}

void Light_Wall::Set(const vec2 &_pos, const float _inner_radius, const float _influence_radius, const vec2 &vertex_wall_0, const vec2 &vertex_wall_1)
{
    // Light
    pos                 = _pos;
    inner_radius        = _inner_radius;
    square_inner_radius = inner_radius*inner_radius;
    influence_radius    = _influence_radius;
    // Wall
    vertex_wall[0]      = vertex_wall_0;
    vertex_wall[1]      = vertex_wall_1;
}

void Light_Wall::Compute()
{
    Initialize();
    
    // - E0_LS et E1_LS sont les positions des deux extremites du mur, relatives au centre de la lumiere
    const vec2 E0_LS(vertex_wall[0] - pos);
    const vec2 E1_LS(vertex_wall[1] - pos);
    
    vec2 intersections_segment_influence_circle[2];
    
    // Est ce que le mur est dans le cercle d'influence de la lumiere (donc projete de l'ombre) ?
    bool b_segment_inside_influence_light = Compute_Intersection_Segment_Circle(pos, influence_radius, vertex_wall[0], vertex_wall[1], intersections_segment_influence_circle);
    
    if (b_segment_inside_influence_light)
    {
        // On a les deux vertex issus du clipping du segment par le cercle d'influence de la lumieres
        vec2 intersections_segment_inner_circle[2];
        // Est ce que le segment est a  l'interieur du cercle de lumiere ?
        bool b_segment_intersect_inner_light = Compute_Intersection_Segment_Circle(pos, inner_radius, vertex_wall[0], vertex_wall[1], intersections_segment_inner_circle);
        if (b_segment_intersect_inner_light)
        {
            // On recupere les vertex issus du clipping du segment par le cercle de lumiere
            const vec2 list_vertex[4] = {
                E0_LS,  // EO
                intersections_segment_inner_circle[0],  // I0
                intersections_segment_inner_circle[1],  // I1
                E1_LS   // E1
            };
            typ_vertex_compared_circle   type_intersections_circle[4];
            
            // On determine la position relative des vertex par rapport au cercle (out, on or in)
            Compute_Types_Vertex( list_vertex, type_intersections_circle );
            
            // On construit la shape englobante pour l'influence de ce segment par rapport a  la lumiere
            // en l'occurence si la segment est a  l'interieur de la lumiere, il influence toute la projection de la lumiere
            sf::Shape shape_bb_light;
            shape_bb_light.AddPoint(vec2(-influence_radius, -influence_radius), WHITE, WHITE);
            shape_bb_light.AddPoint(vec2(+influence_radius, -influence_radius), WHITE, WHITE);
            shape_bb_light.AddPoint(vec2(+influence_radius, +influence_radius), WHITE, WHITE);
            shape_bb_light.AddPoint(vec2(-influence_radius, +influence_radius), WHITE, WHITE);
            shape_bb_light.SetBlendMode(sf::Blend::None);
            shape_bb_light.SetPosition(pos);
            
            // On genere le bounding volume associe
            Bounding_Volume bv_wall_inside_light( list_vertex, type_intersections_circle, shape_bb_light, PENUMBRAS_WIL );
            
            //
            m_vector_bv.push_back(bv_wall_inside_light);
        }
        else // segment est en dehors du disque de lumiere
        {
            vec2    proj_intersections[2];
            
            // Add Bounding Volume for Hard Shadow (Hard Shadow and Inner Penumbra)
            Bounding_Volume bv_shadow_volume = Construct_Shadow_Volume_Bounding_Volume(intersections_segment_influence_circle, proj_intersections);
            m_vector_bv.push_back(bv_shadow_volume);
            
            // Add Bounding Volumes for Penumbra (Inner Outer)
            const vec2 EO_E1_LS = E1_LS - E0_LS;
            std::vector <Bounding_Volume> vector_bv_ssv = Construct_Penumbras_Bounding_Volumes(intersections_segment_influence_circle, proj_intersections, EO_E1_LS);
            for(std::vector<Bounding_Volume>::iterator IterBV=vector_bv_ssv.begin(); IterBV!=vector_bv_ssv.end(); ++IterBV)
                m_vector_bv.push_back(*IterBV);
        }
    }
}

std::vector<Bounding_Volume> Light_Wall::Construct_Penumbras_Bounding_Volumes(
        const vec2 intersections_segment_circle[2],
const vec2 proj_intersections[2], const vec2 &E0_LS_to_E1_LS) const
{
    std::vector<Bounding_Volume> vector_bv;
    
    vec2 intersections_dir[2];
    vec2 intersections_normals[2];
    vec2 penumbra_proj_centers[2][2];
    vec2 proj_intersections_penumbra[2][2];
    Typ_Solutions_Quadratic_Equation solutions[2][2];
    vec2 isc_ppc[2][2];
    vec2 isc_ppc_normal[2][2]; // pas unitaire, juste la direction de la normale
    vec2 bounding_vertex_penumbra[2][2];
    
    // - vecteur Z oriente par rapport au sens de [E0_LS,E1_LS]
    // - ce vecteur sert pour la suite a  orienter les volumes inner/outer de penombre
    sf::Vector3f z = PROD_VEC(TO_VEC3(intersections_segment_circle[0]), TO_VEC3(E0_LS_to_E1_LS));
    z.z = z.z<0?-1:1;
    
    // Centre et rayon du 1er cercle: cercle de lumiere
    vec2 O0 = vec2(0, 0);
    float R0 = inner_radius;
    
    // On boucle sur: les deux sommets formant le segment/wall clippe
    for(int i=0; i<2; ++i)
    {
        // Calcul des centres de projections
        // pour les volumes de penombres (INNER, OUTER)
        vec2 intersections_circles[2];
        {
            // Centre et rayon du 2nd cercle:
            //  O1: Milieu du segment reliant un des points clippe du segment/wall et le centre de la source de lumiere
            //  Distance de O1 (par rapport au centre du cercle d'influence)
            vec2 O1 = COMPUTE_MIDDLE(O0, intersections_segment_circle[i]);
            float R1 = NORM(O1 - O0);
            // On calcul les intersections des cercles qui sont les centres de projection (non orientes, pour l'instant) pour les volumes outer/inner penumbra
            Compute_Intersection_Circles(O0, R0, O1, R1, intersections_circles);
            // Vecteur directeur de ces centres de projection
            intersections_dir[i] = intersections_segment_circle[i] - O0;
        }
        
        // On calcul la normale (orientee) du vecteur directeur
        sf::Vector3f normal         = PROD_VEC(z, TO_VEC3(intersections_dir[i]));
        intersections_normals[i]    = TO_VEC2(normal);
        
        // On boucle sur:
        for(int j=0; j<2; ++j)
        {
            // (1-a) Calcul les centres de projections pour les zones de penombre
            {
                // on determine l'orientation du segment pour aªtre sure de caracteriser les zones: outer et inner distinctement
                float f_sign_orientation    = j ? -1.f : +1.f;
                vec2 oriented_normal        = f_sign_orientation*intersections_normals[i];
                float orientation           = DOT(oriented_normal, intersections_circles[j]);
                penumbra_proj_centers[i][j] = orientation >= 0 ? intersections_circles[j]:intersections_circles[1-j];
            }
            // (1-b) vecteur directeur reliant
            //          un des sommets clippes (par le cercle d'influence) de l'araªte
            //          un des centres de projection (caracterisant les volumes de penombres)
            isc_ppc[i][j]           = (intersections_segment_circle[i] - penumbra_proj_centers[i][j]);
            // (1-c) Normal du segment oriente isc_ppc
            isc_ppc_normal[i][j]    = NORMAL(isc_ppc[i][j]);
            
            // (2) Resolution d'un systeme d'equation quadratique pour retrouver l'intersection de
            //      la ligne (orientee) support du segment isc_ppc
            //      et le cercle d'influence de la lumiere
            float f_square_influence_radius_light   = influence_radius*influence_radius;
            solutions[i][j]                         = Solve_Quadratic_Equation(penumbra_proj_centers[i][j], isc_ppc[i][j], f_square_influence_radius_light);
            // On recupere la solution max (qui correspond a  projection orientee qui nous interesse)
            // et on l'utilise pour retrouver le point d'intersection
            proj_intersections_penumbra[i][j]   = penumbra_proj_centers[i][j] + isc_ppc[i][j]*(float)(MAX(solutions[i][j].f_u0, solutions[i][j].f_u1));
            
            // (3) Calcul d'un des sommet englobant les volumes de penombres (outer/inner)
            //      Ce sommet correspond a  l'intersection des lignes
            //          possedant le sommet (2) et de normale (1-c)
            //          possedant la projection d'un sommet de l'araªte et la normale qui est tangente au cercle en ce point
            bounding_vertex_penumbra[i][j]      = Compute_Intersection_Lines(
                        proj_intersections_penumbra[i][j],  proj_intersections_penumbra[i][j]+isc_ppc_normal[i][j],
                        proj_intersections[i],              proj_intersections[i]+intersections_normals[i]
                        );
            
            // (4) Construct Shape: outer ou inner penumbra (volume englobant)
            sf::Shape shape;
            sf::Color   color_outer_ssv = RED,
                    color_inner_ssv = GREEN;
            sf::Color shape_color = (i+j)%2 ? color_inner_ssv : color_outer_ssv;
            shape.AddPoint( intersections_segment_circle[i],    shape_color, shape_color );
            shape.AddPoint( proj_intersections_penumbra[i][j],  shape_color, shape_color );
            shape.AddPoint( bounding_vertex_penumbra[i][j],     shape_color, shape_color );
            shape.AddPoint( proj_intersections[i],              shape_color, shape_color );
            shape.SetPosition(pos);
            shape.SetBlendMode(sf::Blend::None);
            
            // Add Bounding Volume
            vector_bv.push_back(Bounding_Volume(intersections_segment_circle, shape, (j+i)%2 ? INNER_PENUMBRA : OUTER_PENUMBRA));
        }
    }
    
    return vector_bv;
}

Bounding_Volume Light_Wall::Construct_Shadow_Volume_Bounding_Volume(const vec2 intersections_segment_circle[2], sf::Vector2f proj_intersections[2]) const
{
    sf::Shape shape;
    
    // projection des sommets clippes sur le cercle d'influence de la lumiere
    proj_intersections[0] = NORMALIZE(intersections_segment_circle[0]) * influence_radius;
    proj_intersections[1] = NORMALIZE(intersections_segment_circle[1]) * influence_radius;
    
    // Milieu du segment clippe
    sf::Vector2f mid_i0_i1       = COMPUTE_MIDDLE(proj_intersections[0], proj_intersections[1]);
    // Projection du milieu (a) sur le cercle
    sf::Vector2f proj_mid_i0_i1  = NORMALIZE(mid_i0_i1) * influence_radius;
    // Calcul des vertex englobant
    // Ils correspondent aux intersections des lignes formees par:
    //  les sommets de projection des intersections
    //  et leurs normales associees, tangentes au cercle.
    vec2 bounding_vertex_sv[2];
    bounding_vertex_sv[0] = Compute_Intersection_Lines(proj_intersections[0], proj_mid_i0_i1);
    bounding_vertex_sv[1] = Compute_Intersection_Lines(proj_intersections[1], proj_mid_i0_i1);
    
    // Construction de la shape ShadowVolume
    sf::Color color_sv = BLUE;
    shape.AddPoint( intersections_segment_circle[0],    color_sv, color_sv);
    shape.AddPoint( proj_intersections[0],              color_sv, color_sv);
    shape.AddPoint( bounding_vertex_sv[0],              color_sv, color_sv);
    shape.AddPoint( bounding_vertex_sv[1],              color_sv, color_sv);
    shape.AddPoint( proj_intersections[1],              color_sv, color_sv);
    shape.AddPoint( intersections_segment_circle[1],    color_sv, color_sv);
    // On regle le shape sans blending et on fournit la position
    shape.SetBlendMode(sf::Blend::None);
    shape.SetPosition(pos);
    
    return Bounding_Volume(intersections_segment_circle, shape, SHADOW_VOLUME);
}

void Light_Wall::Add_Shapes( sf::Shape &_shape, const vec2 _edges[2], type_bv _type )
{
    m_vector_bv.push_back(Bounding_Volume(_edges, _shape, _type));
}

void Light_Wall::Add_Shapes( std::vector <sf::Shape> &_vector_shapes, const vec2 _edges[2], type_bv _type )
{
    for(std::vector<sf::Shape>::iterator IterShape=_vector_shapes.begin(); IterShape!=_vector_shapes.end(); ++IterShape)
        m_vector_bv.push_back(Bounding_Volume(_edges, *IterShape, _type));
}

void Light_Wall::Add_Bounding_Volumes( const Bounding_Volume &_bv )
{
    m_vector_bv.push_back( _bv );
}

void Light_Wall::Add_Bounding_Volumes( std::vector<Bounding_Volume> &_vector_bv )
{
    for(std::vector<Bounding_Volume>::iterator IterBV=_vector_bv.begin(); IterBV!=_vector_bv.end(); ++IterBV)
        m_vector_bv.push_back(*IterBV);
}

/**
 * @brief Light_Wall::Compute_Types_Vertex
 * @param list_vertex
 * @param type_intersections_circle
 */
void Light_Wall::Compute_Types_Vertex(
        const vec2 list_vertex[4],
        typ_vertex_compared_circle type_intersections_circle[4]
)
{
    float               f_square_distances[4];
    
    for(int i=0; i<4; ++i) {
        f_square_distances[i]        = NORM2(list_vertex[i]);
        type_intersections_circle[i] = EQUAL_EPS(f_square_distances[i], square_inner_radius, 1.f) ? ON_CIRCLE : f_square_distances[i] > square_inner_radius ? OUTSIDE_CIRCLE : INSIDE_CIRCLE;
    }
}

type_bv Light_Wall::Compute_Type_BV(
        const typ_vertex_compared_circle type_intersections_circle[4]
) const
{
    const typ_vertex_compared_circle    combinaisons[5][4]     = {
        { OUTSIDE_CIRCLE, ON_CIRCLE,      INSIDE_CIRCLE,  INSIDE_CIRCLE     },
        { INSIDE_CIRCLE,  INSIDE_CIRCLE,  ON_CIRCLE,      OUTSIDE_CIRCLE    },
        { ON_CIRCLE,      ON_CIRCLE,      ON_CIRCLE,      ON_CIRCLE         },
        { OUTSIDE_CIRCLE, ON_CIRCLE,      ON_CIRCLE,      OUTSIDE_CIRCLE    },
        { INSIDE_CIRCLE,  INSIDE_CIRCLE,  INSIDE_CIRCLE,  INSIDE_CIRCLE     }
    };
    // TODO: à refactorer ! C'est très moche !
    unsigned int i = 0;
    bool not_found = true;
    while(not_found && i<5)
    {
        bool            test_match = true;
        unsigned int    j = 0;
        while(test_match && j<4)
        {
            test_match = type_intersections_circle[j] == combinaisons[i][j];
            ++j;
        }
        not_found = !test_match;
        ++i;
    }
    i--;
    return (type_bv)( PENUMBRAS_WIL_TYPE_0 + i);
}

void Light_Wall::Draw_Analytic_Circle_Light( sf::RenderTarget* App, sf::Shader &shader)
{
    const float &r = inner_radius;
    
    sf::Shape shape = sf::Shape::Rectangle(pos.x-r, pos.y-r, r*2, r*2, sf::Color(255, 10, 0, 128));
    shader.SetParameter("u_v_position_light",         pos);
    shader.SetParameter("u_f_influence_radius_light", r);
    App->Draw(shape, shader);
}
