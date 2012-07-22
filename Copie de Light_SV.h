#ifndef __LIGHT_SHADOW_VOLUME_H__
#define __LIGHT_SHADOW_VOLUME_H__

#include "Light.h"
#include "Light_Wall.h"

class Light_SV : public Light
{
    public:
        Light_SV();

        Light_SV(sf::Vector2f position, float intensity, float radius, int quality, sf::Color color);

        virtual ~Light_SV();

        // Afficher la lumière
        void Draw(sf::RenderTarget *App);

        void Generate(std::vector <Wall> &m_wall);

        // Ajouter un triangle à la lumière, en effet, les lumières sont composée de triangles
        void AddTriangle(sf::Vector2f pt1,sf::Vector2f pt2);

        int GetNumberPrimitivesForSV() const { return m_shape_sv.size(); };

        inline float    GetInnerRadius() const          { return m_inner_radius; };
        inline void     SetInnerRadius(float _radius)   { m_inner_radius = _radius; };

    private:
        void clear();

        void Construct();
        void UpdatePosition();

        void Generate_With_Analytic_Computations(std::vector<Wall> &m_wall, const sf::Vector2f& pos_light, const float influence_radius, const float radius, Light_Wall &light_walls, const bool use_slip = true);
        void Generate_With_Projection_At_Infinity(std::vector<Wall> &m_wall);

        void Generate_Shapes_For_SSV(
                                     const sf::Vector2f intersections_segment_circle[2],
                                     const sf::Vector2f proj_intersections[2],
                                     const sf::Vector2f &l1_to_l2,
                                     const sf::Vector2f &pos_light,
                                     const float inner_radius,
                                     const float radius,
                                     const float f_square_radius_light,
                                     Light_Wall &light_walls
                                );

        void Render_SSV(sf::RenderTarget *App, const std::vector <sf::Shape> &shapes_intersections_ssv, const int i, sf::Shader &shader) const;
        void Draw_Analytic_Circle_Light(sf::RenderTarget *App, sf::Shader& shader) const;
        void Draw_Analytic_Circle(sf::RenderTarget *App, const sf::Shader& shader) const;
        void Draw_Analytic_Circle(sf::RenderTarget *App) const;

        void Draw_Full_Light(sf::RenderTarget *App);
        void Draw_Test_Stencil(sf::RenderTarget *App);
        void Draw_GroundTruth(sf::RenderTarget *App);

        void InitEffects();

    private:
        //Tableau dynamique de Shape, ce sont ces shapes de type quad qui compose les volumes d'ombre
        std::vector <sf::Shape> m_shape_sv;
        std::vector <sf::Shape> m_shape_outer_ssv;  // Polygone englobant le volume de pénombre outer
        std::vector <sf::Shape> m_shape_inner_ssv;  // Polygone englobant le volume de pénombre inner
        //
        std::vector <sf::Shape> m_shape_intersections_sv;   // DEBUG
        std::vector <sf::Shape> m_shape_intersections_ssv;  // DEBUG
        std::vector <sf::Shape> m_shape_tests_intersections;   // DEBUG
        std::vector <sf::Text>  m_shape_text;

        std::vector <Light_Wall> m_lights_walls;

        // Rayon de la "sphère"
        float m_inner_radius;

        bool m_b_is_construct;
        bool m_b_init_effects;

        sf::Shader SoftShadowInnerEffect;
        sf::Shader SoftShadowOuterEffect;
        sf::Shader SphereLighting2D;
        sf::Shader CombineEffect;
        sf::Shader CombineEffect2;
        sf::Shader SoftShadowEffect;
        sf::Shader SoftShadowEffect_GroundTruth;

        bool m_b_lb_is_not_create;
        sf::RenderImage *m_renderLightBufferImg;

        sf::Image   img_normal_map;
        sf::Sprite  sprite_normal_map;

        sf::Image   m_img_light_buffer;
};

#endif
