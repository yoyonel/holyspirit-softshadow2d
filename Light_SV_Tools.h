#ifndef __LIGHT_SV_TOOLS__
#define __LIGHT_SV_TOOLS__

#include <iostream>
#include <sstream>
#include <limits>
#include "LightManager.h"

#define vec2                        sf::Vector2f

#define DOT(_v1, _v2)               ((_v1).x*(_v2).x + (_v1).y*(_v2).y)
#define ADD(_v1, _v2)               vec2((_v1).x+(_v2).x, (_v1).y+(_v2).y)
#define SUB(_v1, _v2)               vec2((_v1).x-(_v2).x, (_v1).y-(_v2).y)
#define PROD_SCALE(_float, _vec2)   vec2(_float*(_vec2).x, _float*(_vec2).y)
#define PROD_VEC(_v1, _v2)          sf::Vector3f((_v1).y*(_v2).z-(_v1).z*(_v2).y, (_v1).z*(_v2).x-(_v1).x*(_v2).z, (_v1).x*(_v2).y-(_v1).y*(_v2).x)
#define NORM2(_v1)                  DOT(_v1, _v1)
#define NORM(_v1)                   sqrt(NORM2(_v1))
#define NORMALIZE(_v1)              PROD_SCALE(1.f/NORM((_v1)), (_v1))
#define NORMAL(_v)                  vec2(-(_v).y, (_v).x)
#define DET(_v1, _v2)               ((_v1).x*(_v2).y - (_v2).x*(_v1).y)
#define COMPUTE_MIDDLE(_v1, _v2)    PROD_SCALE(0.5f, ADD(_v1, _v2))
#define TO_VEC2(_v)                 vec2((_v).x, (_v).y)
#define TO_VEC3(_v)                 sf::Vector3f((_v).x, (_v).y, 0)
//
#define EQUAL_EPS(_f1, _f2, _eps)   (((_f1)-(_f2))>=-(_eps) && ((_f1)-(_f2))<=+(_eps))
#define EQUAL0_EPS(_f, _eps)        ((_f)>=-(_eps) && (_f)<=+(_eps))
//
#define CLAMP(f, m, M)              fmin(M, fmax(m, f))

#define MAX(a, b)                   (a>b?a:b)
#define SIGN(a)                     (a<0?-1:+1)

#define WHITE   sf::Color(255, 255, 255)
#define RED     sf::Color(255, 0, 0)
#define GREEN   sf::Color(0, 255, 0)
#define BLUE    sf::Color(0, 0, 255)
#define BLACK   sf::Color(0, 0, 0, 255)

#define ADD_DEBUG_LINE(_shape, _pos, _v1, _v2, _linewidth, _color)        \
    _shape.push_back(sf::Shape());                                  \
    _shape.back() = sf::Shape::Line( _v1, _v2, _linewidth, _color); \
    _shape.back().SetPosition(_pos);                          \
    _shape.back().SetBlendMode(sf::Blend::None);

#define ADD_DEBUG_TRIANGLE(_vector_shapes, _pos, _v1, _v2, _v3, _linewidth, _color)                     \
    { \
        _vector_shapes.push_back(sf::Shape());                                  \
        sf::Shape &shape = _vector_shapes.back(); \
        shape.AddPoint( _v1, _color, _color);  \
        shape.AddPoint( _v2, _color, _color);  \
        shape.AddPoint( _v3, _color, _color);  \
        shape.SetPosition(_pos);                          \
        shape.SetBlendMode(sf::Blend::None); \
    }

#define ADD_DEBUG_TRIANGLE_FILL(_vector_shapes, _pos, _v1, _v2, _v3, _linewidth, _color)                     \
    {                                                                                           \
        ADD_DEBUG_TRIANGLE(_vector_shapes, _pos, _v1, _v2, _v3, _linewidth, _color) \
        sf::Shape &shape = _vector_shapes.back(); \
        shape.EnableFill(true);                                                \
        shape.EnableOutline(false);     \
    }

// MOG
// shape.SetOutlineWidth(_linewidth); \
//

#define ADD_DEBUG_TRIANGLE_OUTLINE(_vector_shapes, _pos, _v1, _v2, _v3, _linewidth, _color)        \
    {                                                                                           \
        ADD_DEBUG_TRIANGLE(_vector_shapes, _pos, _v1, _v2, _v3, _linewidth, _color) \
        sf::Shape &shape = _vector_shapes.back(); \
        shape.EnableFill(false);                                                \
        shape.EnableOutline(true);     \
    }

#define ADD_DEBUG_CIRCLE(_shape, _pos, _center, _radius, _color)              \
    {                                                                   \
        _shape.push_back(sf::Shape());                                  \
        _shape.back() = sf::Shape::Circle( _center, _radius, _color);   \
        _shape.back().SetPosition(_pos);                          \
        _shape.back().SetBlendMode(sf::Blend::None);                    \
    }

#define ADD_DEBUG_CIRCLE_OUTLINE(_shape, _pos, _center, _radius, _color, _width)              \
    {                                                                                   \
        _shape.push_back(sf::Shape());                                                  \
        _shape.back() = sf::Shape::Circle( _center, _radius, _color, _width, _color);   \
        _shape.back().SetPosition(_pos);                                          \
        _shape.back().SetBlendMode(sf::Blend::None);                                    \
        _shape.back().EnableFill(false);                                                \
    }

#define ADD_DEBUG_TEXT(_shape, _pos, _str, _value, _color, _size)      \
    {                                                           \
            _shape.push_back(sf::Text());                                                  \
            std::ostringstream stm;                             \
            stm << _str << ": " << _value;                      \
            _shape.back().SetString(stm.str());                        \
            _shape.back().SetFont(sf::Font::GetDefaultFont());         \
            _shape.back().SetPosition(_pos);                           \
            _shape.back().SetColor(_color);                     \
            _shape.back().SetBlendMode(sf::Blend::None);               \
            _shape.back().SetCharacterSize(_size); \
    }

#define DEBUG_PRINT(msg, var)           std::cout << msg << ": " << var << std::endl;

#define EPSILON                         std::numeric_limits<float>::epsilon()

//#define Compute_Intersection_Circles    Compute_Intersection_Circles_0
#define Compute_Intersection_Circles    Compute_Intersection_Circles_1

typedef struct
{
    bool b_has_solutions;
    float f_u0;
    float f_u1;
} Typ_Solutions_Quadratic_Equation;

Typ_Solutions_Quadratic_Equation Solve_Quadratic_Equation(float A, float B, float C);
Typ_Solutions_Quadratic_Equation Solve_Quadratic_Equation(vec2 _A, vec2 _AB, float _radius);
//
bool        Intersect_BoundingBox(const vec2& _v00, const vec2& _v01, const vec2& _v10, const vec2& _v11);
bool        Quadratic_Equation_Has_Solutions(vec2 _A, vec2 _AB, float _radius);
vec2        Compute_Intersection_Lines( const vec2& P1, const vec2& P3);
vec2        Compute_Intersection_Lines( const vec2& P1, const vec2& P2, const vec2& P3, const vec2& P4);
bool        Compute_Intersection_Segments( const vec2& P1, const vec2& P2, const vec2& P3, const vec2& P4, vec2& P);
bool        Compute_Intersection_Circles_1(const vec2& O0, const float R0, const vec2& O1, const float R1, vec2 P[2]);
bool        Compute_Intersection_Circles_0(const vec2& O0, const float R0, const vec2& O1, const float R1, vec2 P[2]);
float       signed_distance_point_line( const vec2& P, const vec2& A, const vec2& B );
float       distance_point_line( const vec2& P, const vec2& A, const vec2& B );
float       area_triangle( const vec2& A, const vec2& B, const vec2& C );
bool        Compute_Intersection_Segment_Circle( const vec2& pos_light, const float r, const vec2 &E0, const vec2 &E1, vec2 intersections_segment_circle[2] );
sf::Shape   construct_shadow_volume_shape( const vec2 &pos_light, const float r, const vec2 intersections_segment_circle[2], const sf::Color &color, sf::Vector2f proj_intersections[2], sf::Vector2f bounding_vertex_sv[2] );

#endif
