#include "Bounding_Volume.h"

using namespace bv; // bounding volume namespace

Bounding_Volume::Bounding_Volume( const vec2 &_vertex_edge_0,  const vec2 &_vertex_edge_1, const sf::Shape& _shape, type_bv _type )
{
    vertex_edge[0]  = _vertex_edge_0;
    vertex_edge[3]  = _vertex_edge_1;
    m_shape         = _shape;
    type            = _type;
}

Bounding_Volume::Bounding_Volume( const vec2 &_vertex_edge_0,  const vec2 &_vertex_intersection_0, const vec2 &_vertex_intersection_1, const vec2 &_vertex_edge_1, const sf::Shape& _shape, type_bv _type )
{
    //
    vertex_edge[0]  = _vertex_edge_0;
    vertex_edge[1]  = _vertex_intersection_0;
    vertex_edge[2]  = _vertex_intersection_1;
    vertex_edge[3]  = _vertex_edge_1;
    //
    m_shape         = _shape;
    type            = _type;
}

// _vertex_edge[0, 1]: E0, E1
Bounding_Volume::Bounding_Volume( const vec2 _vertex_edge[2], const sf::Shape& _shape, type_bv _type )
{
    vertex_edge[0]  = _vertex_edge[0];
    vertex_edge[3]  = _vertex_edge[1];
    //
    m_shape         = _shape;
    type            = _type;
}

Bounding_Volume::Bounding_Volume( const vec2 _vertex_edge[2], const vec2 _vertex_intersections[2], const sf::Shape& _shape, type_bv _type )
{
    vertex_edge[0]  = _vertex_edge[0];
    vertex_edge[1]  = _vertex_intersections[0];
    vertex_edge[2]  = _vertex_intersections[1];
    vertex_edge[3]  = _vertex_edge[1];

    m_shape         = _shape;
    type            = _type;
}

// _vertex[0 .. 3] = { E0, I0, I1, E1 }
Bounding_Volume::Bounding_Volume( const vec2 _vertex[4], const typ_vertex_compared_circle _types_vertex[4], const sf::Shape& _shape, type_bv _type )
{
    for(int i=0; i<4; ++i)
    {
        vertex_edge[i]  = _vertex[i];
        type_vertex[i]  = _types_vertex[i];
    }
    m_shape         = _shape;
    type            = _type;
}

