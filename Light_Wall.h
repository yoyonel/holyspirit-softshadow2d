#ifndef __LIGHT_WALL_H__
#define __LIGHT_WALL_H__

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Bounding_Volume.h"

#include "Light_Wall_Debug.h"

#ifndef vec2
#define vec2 sf::Vector2f
#endif

using namespace bv; // bounding volume namespace

class Light_Wall
{
    public:
        Light_Wall();
        Light_Wall(const vec2 &_pos = vec2(0, 0), const float _inner_radius = 0, const float _influence_radius = 1, const vec2 &vertex_wall_0 = vec2(0, 0), const vec2 &vertex_wall_1 = vec2(0, 0));

        void Set(const vec2 &_pos = vec2(0, 0), const float _inner_radius = 0, const float _influence_radius = 1, const vec2 &vertex_wall_0 = vec2(0, 0), const vec2 &vertex_wall_1 = vec2(0, 0));

        inline vec2     Position()          const { return pos; };
        inline float    InnerRadius()       const { return inner_radius; };
        inline float    InfluenceRadius()   const { return influence_radius; };

        inline std::vector <Bounding_Volume> List_Bounding_Volumes() const { return m_vector_bv; };

        void Compute();

        void Debug(sf::RenderTarget* App, const vec2& P=vec2(400, 200)) const;
        void Debug(sf::RenderTarget* App, sf::Shader &shader_debug, const vec2& P=vec2(400, 200));

    protected:
        void Initialize();

        void Add_Shapes( std::vector <sf::Shape> &_vector_shapes, const vec2 _edges[2], type_bv _type );
        void Add_Shapes( sf::Shape &_shapes, const vec2 _edges[2], type_bv _type );
        void Add_Bounding_Volumes( const Bounding_Volume &_bv );
        void Add_Bounding_Volumes( std::vector<Bounding_Volume> &_vector_bv );

        std::vector <Bounding_Volume>   Construct_Penumbras_Bounding_Volumes(const vec2 intersections_segment_circle[2], const vec2 proj_intersections[2], const vec2 &l1_to_l2) const;
        Bounding_Volume                 Construct_Shadow_Volume_Bounding_Volume(const vec2 intersections_segment_circle[2], sf::Vector2f proj_intersections[2]) const;

        void Draw_Analytic_Circle_Light( sf::RenderTarget* App, sf::Shader &shader);

        void    Compute_Types_Vertex(const vec2 list_vertex[4], typ_vertex_compared_circle type_intersections_circle[4]);
        type_bv Compute_Type_BV(const typ_vertex_compared_circle type_intersections_circle[4]) const;

    private:
        //
        vec2    pos;
        float   inner_radius;
        float   square_inner_radius;
        float   influence_radius;
        //
        vec2    vertex_wall[2];
        vec2    vertex_edge[4];
        //
        std::vector <Bounding_Volume> m_vector_bv;
};

#endif
