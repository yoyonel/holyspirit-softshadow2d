#ifndef __LIGHT_WALL_DEBUG__
#define __LIGHT_WALL_DEBUG__

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Bounding_Volume.h"

#include <GL/glew.h>

#ifndef vec2
#define vec2 sf::Vector2f
#endif

float area_triangle( const vec2& A, const vec2& B, const vec2& C );
float compute_disc_portion_area( const vec2& P0, const vec2& P1, const float inv_r );
float distance_point_line( const vec2& P, const vec2& A, const vec2& B );
bool inside_half_plane(const vec2& A, const vec2& B, const vec2& P);
vec2 compute_projection_on_circle( const vec2& E, const vec2& P, const float r, const float inv_r );

float compute_visibility_light(const vec2& P, const vec2& E0, const vec2& E1, const float r);
float compute_visibility_light(const vec2& P, const vec2& E, const float r);

float compute_visibility_light_in_in(const vec2& P, const vec2& E0, const vec2& E1, const float r,  const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug);
float compute_visibility_light_out_on(const vec2& P, const vec2& E0, const vec2& E1, const float r, const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug);
float compute_visibility_light_on_on(const vec2& P, const vec2& E0, const vec2& E1, const float r,  const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug);
float compute_visibility_light_out_out(const vec2& P, const vec2& E0, const vec2& I0, const vec2& I1, const vec2& E1, const float r, const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug);
float compute_visibility_light_on_in(const vec2& P, const vec2& E0, const vec2& E1, const float r, const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug);
float compute_visibility_light_in_on(const vec2& P, const vec2& E0, const vec2& E1, const float r, const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug);

void    occlusion_query_begin();
GLuint  occlusion_query_end();
#endif
