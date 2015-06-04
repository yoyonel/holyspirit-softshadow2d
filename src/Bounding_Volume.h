#ifndef __BOUNDING_VOLUME__
#define __BOUNDING_VOLUME__

//#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
//#include <SFML/Window.hpp>

#ifndef vec2
#define vec2 sf::Vector2f
#endif

namespace bv
{
    typedef enum {
        //
        SHADOW_VOLUME = 0,
        //
        OUTER_PENUMBRA,
        INNER_PENUMBRA,
        //
        PENUMBRAS_WIL,
        //
        PENUMBRAS_WIL_TYPE_0,
        PENUMBRAS_WIL_TYPE_1,
        PENUMBRAS_WIL_TYPE_2,
        PENUMBRAS_WIL_TYPE_3,
        PENUMBRAS_WIL_TYPE_4
    } type_bv;

    typedef enum {
        OUTSIDE_CIRCLE=0,
        ON_CIRCLE,
        INSIDE_CIRCLE
    } typ_vertex_compared_circle;

    class Bounding_Volume
    {
        public:
            Bounding_Volume( const vec2 &_vertex_edge_0, const vec2 &_vertex_edge_1, const sf::Shape& _shape, type_bv _type = SHADOW_VOLUME );
            Bounding_Volume( const vec2 &_vertex_edge_0,  const vec2 &_vertex_intersection_0, const vec2 &_vertex_intersection_1, const vec2 &_vertex_edge_1, const sf::Shape& _shape, type_bv _type );
            Bounding_Volume( const vec2 _vertex_edge[2], const sf::Shape& _shape, type_bv _type = SHADOW_VOLUME );
            Bounding_Volume( const vec2 _vertex_edge[2], const vec2 _vertex_intersections[2], const sf::Shape& _shape, type_bv _type = SHADOW_VOLUME );
            Bounding_Volume( const vec2 _vertex[4], const typ_vertex_compared_circle _types[4], const sf::Shape& _shape, type_bv _type = SHADOW_VOLUME );
            //
            inline const vec2      &E0()     const { return vertex_edge[0]; };
            inline const vec2      &I0()     const { return vertex_edge[1]; };
            inline const vec2      &I1()     const { return vertex_edge[2]; };
            inline const vec2      &E1()     const { return vertex_edge[3]; };
            //
            inline const sf::Shape &Shape()  const { return m_shape; };
            inline const type_bv   &Type()   const { return type; };
            //
            inline const typ_vertex_compared_circle TYPE_E0() const { return type_vertex[0]; };
            inline const typ_vertex_compared_circle TYPE_I0() const { return type_vertex[1]; };
            inline const typ_vertex_compared_circle TYPE_I1() const { return type_vertex[2]; };
            inline const typ_vertex_compared_circle TYPE_E1() const { return type_vertex[3]; };

        private:
            vec2                        vertex_edge[4];
            //
            typ_vertex_compared_circle  type_vertex[4];
            type_bv                     type;

            //
            sf::Shape   m_shape;
    };

}

#endif
