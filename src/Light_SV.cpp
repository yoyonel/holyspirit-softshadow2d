#include <GL/glew.h>

#include "Light_SV.h"
#include <iostream>
#include <sstream>
#include <limits>
#include <GL/gl.h>
#include <math.h>
#include "Light_SV_Tools.h"
#include "LightManager.h"

//#define __DRAW_DEBUG__
#define __DRAW_DEBUG_RENDER_INNER_LIGHT__       // Show the disc light
#define __DRAW_DEBUG_RENDER_SHAPES_POINTS__
#define __DRAW_DEBUG_RENDER_SHAPES_OUTLINES__

#define __DRAW_DEBUG_LIGHT_WALL__
#define __DRAW_DEBUG_LIGHT_WALL_TYPE__

#define __RENDER_SSV__
#define __USE_ANALYTIC_GENERATION__
//#define __USE_PROJECTION_AT_INFINY_GENERATION__

//#define __USE_GROUND_TRUTH__ // [MOG]
//
//#define __GROUND_TRUTH_STRATIFIED_SAMPLING__
#define __GROUND_TRUTH_RANDOM_SAMPLING__
//
#define NB_SAMPLES_FOR_GROUNDTRUTH 16

#define RADIUS_SOFT_LIGHT   32 // [MOG]

const float f_coef_projection = 1e9;

const sf::Color color_sv(sf::Color(255, 0, 255, 255));
const sf::Color color_inner_ssv(sf::Color(255, 0, 255, 255));
const sf::Color color_outer_ssv(sf::Color(0, 255, 255, 255));

Light_SV::Light_SV() : m_inner_radius(RADIUS_SOFT_LIGHT), m_b_is_construct(false), m_b_init_effects(false), m_b_lb_is_not_create(true),  m_renderLightBufferImg(NULL)
{
    InitEffects();
}

Light_SV::Light_SV(sf::Vector2f position, float intensity, float radius, int quality, sf::Color color) : m_inner_radius(RADIUS_SOFT_LIGHT), m_b_is_construct(false), m_b_init_effects(false), m_b_lb_is_not_create(true), m_renderLightBufferImg(NULL)
{
    SetPosition(position);
    SetIntensity(intensity);
    SetRadius(radius);
    SetColor(color);
    SetQuality(quality);
    //
    Construct();
    //
    InitEffects();
}

Light_SV::~Light_SV()
{
    clear();
}

void Light_SV::clear()
{
    // On vide la memoire
    m_lights_walls.clear();
}

void Light_SV::InitEffects()
{
    // Chargement des shaders
    std::cout << "Load Shader: data/shaders/light_2d_ssv.glsl, data/shaders/combine.glsl" << std::endl;
    CombineEffect.LoadFromFile("data/shaders/light_2d_ssv.glsl", "data/shaders/combine.glsl");

    std::cout << "Load Shader: data/shaders/light_2d_ssv.glsl, data/shaders/combine_2.glsl" << std::endl;
    CombineEffect2.LoadFromFile("data/shaders/light_2d_ssv.glsl", "data/shaders/combine_2.glsl");

    std::cout << "Load Shader: data/shaders/vertex_ss_2d.glsl, data/shaders/pixel_ss_2d.glsl" << std::endl;
    SoftShadowEffect.LoadFromFile("data/shaders/vertex_ss_2d.glsl", "data/shaders/pixel_ss_2d.glsl");

    SoftShadowEffect_GroundTruth.LoadFromFile("data/shaders/vertex_ss_2d.glsl", "data/shaders/pixel_ss_2d_gt.glsl");
    SoftShadowEffect_WIL.LoadFromFile("data/shaders/vertex_ss_2d.glsl", "data/shaders/pixel_ss_2d_wil.glsl");
    SoftShadowEffect_Debug.LoadFromFile("data/shaders/vertex_ss_2d.glsl", "data/shaders/pixel_ss_2d_debug.glsl");

    // On recupere les shaders charges par le Light_Manager
    const Light_Manager *Manager = Light_Manager::GetInstance();
    //
    SphereLighting2D                = Manager->GetDiscLightingEffect();
    SoftShadowInnerEffect           = Manager->GetSoftShadowInnerEffect();
    SoftShadowOuterEffect           = Manager->GetSoftShadowOuterEffect();

    // Les shaders sont ils utilisables ?
    m_b_init_effects =
        SphereLighting2D.IsAvailable() &&
        SoftShadowInnerEffect.IsAvailable() &&
        SoftShadowOuterEffect.IsAvailable() &&
        CombineEffect.IsAvailable() &&
        CombineEffect2.IsAvailable() &&
        SoftShadowEffect.IsAvailable() &&
        SoftShadowEffect_GroundTruth.IsAvailable() &&
        SoftShadowEffect_WIL.IsAvailable() &&
        SoftShadowEffect_Debug.IsAvailable();

    // Textures
    // Chargement de la texture normal
    img_normal_map.LoadFromFile("data/textures/rockNormal.bmp");
    // MOG
    // Assignation de la texture au shader
    CombineEffect2.SetTexture("u_texture_normal_map", img_normal_map);
}

void Light_SV::Draw(sf::RenderTarget *App)
{
    if (m_b_lb_is_not_create)
    {
        m_renderLightBufferImg = new sf::RenderImage();
        // MOG
        m_renderLightBufferImg->Create(App->GetWidth(), App->GetHeight(), true, true);
        m_img_light_buffer = m_renderLightBufferImg->GetImage();
        m_b_lb_is_not_create = false;
    }

    #ifdef __USE_GROUND_TRUTH__
        Draw_GroundTruth(App);
    #else
        Draw_Full_Light(App);
    #endif

}

// N'utilise pas un systeme de shape englobante (pour le shadow volume, et les penumbra volumes)
void Light_SV::Draw_Full_Light(sf::RenderTarget *App)
{
    if (m_b_init_effects)
    {
        m_renderLightBufferImg->SetView(App->GetView());
        m_renderLightBufferImg->SetActive(true);
        m_renderLightBufferImg->SaveGLStates();

        GLuint mask_stencil = (GLuint)(~0x0);
        GLint clear_stencil = 0;

        glDisable(GL_ALPHA_TEST);    // desactivation du test alpha
        //
        glDisable(GL_DEPTH_TEST);   // desactivation du depth test (L.I.F.O.)
        glDepthMask(GL_FALSE);      // on ecrit pas dans le depthbuffer
        //
        glEnable(GL_STENCIL_TEST);  // activation du depth buffer
        glStencilMask(mask_stencil);// mise en place du mask (activation de tous les bits)
        //
        glClearStencil(clear_stencil);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

        #ifdef __RENDER_SSV__
        {
            GLint ref_stencil = 0;
            //
            sf::Shader *list_ptr_shaders[] = {
                &SoftShadowEffect,
                &SoftShadowEffect_WIL
                };
            unsigned int id_shader = 0;

            for(std::vector<Light_Wall>::iterator IterLight_Wall=m_lights_walls.begin();IterLight_Wall!=m_lights_walls.end();++IterLight_Wall)
            {
                std::vector<Bounding_Volume> list_bv = IterLight_Wall->List_Bounding_Volumes();
                for(std::vector<Bounding_Volume>::iterator IterBV=list_bv.begin();IterBV!=list_bv.end();++IterBV)
                {
                    switch(IterBV->Type())
                    {
                        // Si pas d'intersection entre le segment-mur et le cercle de lumiere
                        case SHADOW_VOLUME:
                        case OUTER_PENUMBRA:
                        case INNER_PENUMBRA:
                        {
                            // On utilise le stencil pour ne pas ecrire calculer l'ombre deux fois au mÃÂªme endroit
                            glStencilFunc( GL_EQUAL, ref_stencil, mask_stencil);
                            glStencilOp( GL_INCR, GL_INCR, GL_INCR);
                            glStencilMask( mask_stencil);
                            //
                            id_shader = 0;
                        }
                        break;
                        // Si intersection
                        case PENUMBRAS_WIL:
                        {
                            // On desactive le stencil (il peut avoir recouvrement (volontaire) des zones de penombres)
                            glStencilFunc( GL_ALWAYS, ref_stencil, mask_stencil);
                            glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP);
                            glStencilMask( ~mask_stencil);
                            //
                            id_shader = 1;
                            sf::Shader &shader = (*list_ptr_shaders[id_shader]);
                            // Position des intersections du segment-mur avec le cercle
                            shader.SetParameter("u_v_i0",     IterBV->I0());
                            shader.SetParameter("u_v_i1",     IterBV->I1());
                            // Types des sommets: OUT, ON, IN
                            shader.SetParameter("u_f_type_e0",     IterBV->TYPE_E0());
                            shader.SetParameter("u_f_type_i0",     IterBV->TYPE_I0());
                            shader.SetParameter("u_f_type_i1",     IterBV->TYPE_I1());
                            shader.SetParameter("u_f_type_e1",     IterBV->TYPE_E1());
                        }
                        break;
                        default:
                        break;
                    }
                    //
                    sf::Shader &shader = (*list_ptr_shaders[id_shader]);
                    shader.SetParameter("u_v_e0",     IterBV->E0());
                    shader.SetParameter("u_v_e1",     IterBV->E1());
                    shader.SetParameter("u_v_position_light",         m_position);
                    shader.SetParameter("u_f_radius_light",           m_inner_radius);
                    shader.SetParameter("u_f_influence_radius_light", m_influence_radius);

                    // On rend le volume d'ombre en mode additif (les ombres se cumulent pour un couple lumiere-occluder)
                    sf::Shape shape_shadow = IterBV->Shape();
                    shape_shadow.SetBlendMode(sf::Blend::Add);
                    m_renderLightBufferImg->Draw(shape_shadow, shader);
                }

                // On efface le stencil
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glStencilMask(mask_stencil);
                glStencilFunc(GL_ALWAYS, ref_stencil, mask_stencil);
                glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
                for(std::vector<Bounding_Volume>::iterator IterBV=list_bv.begin();IterBV!=list_bv.end();++IterBV)
                    m_renderLightBufferImg->Draw(IterBV->Shape());
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            }
        }
        #endif
        /**/
        m_renderLightBufferImg->RestoreGLStates();
        m_renderLightBufferImg->Display();

        sf::Sprite sprite;
        sprite.SetImage(m_renderLightBufferImg->GetImage()); // [MOG]
        sprite.SetBlendMode(sf::Blend::Add); // [MOG]
        //sprite.SetBlendMode(sf::Blend::None);
        //
        const sf::Vector3f  v_color_light   = sf::Vector3f(m_color.r, m_color.g, m_color.b) / 255.f;
        // MOG
        CombineEffect2.SetParameter("u_v_position_light",         m_position);
        CombineEffect2.SetParameter("u_f_influence_radius_light", m_influence_radius);
        CombineEffect2.SetParameter("u_v_color_light",            v_color_light);

        CombineEffect2.SetTexture("u_texture_light_map",            m_renderLightBufferImg->GetImage());

        App->Draw(sprite, CombineEffect2);

        // MOG: bug avec SFML, probleme avec la surface de rendu App si je ne remet pas son shader ÃÂ  NULL
        // Cette remise ÃÂ  NULL est effectuee si on active le debug
        // ou juste en dessinant une shape vide
        // ... bizarre quand meme :/
        Draw_Debug(App);
        App->Draw(sf::Shape());
    }
}

// Echantillonage de la source de lumiere (par un ensemble de source de lumiere ponctuelle)
void Light_SV::Draw_GroundTruth(sf::RenderTarget *App)
{
    if (m_b_init_effects && m_renderLightBufferImg)
    {
        sf::Sprite sprite;
        sprite.SetBlendMode(sf::Blend::None);
        sprite.SetImage(m_img_light_buffer);
        //
        const sf::Vector3f  v_color_light   = sf::Vector3f(m_color.r, m_color.g, m_color.b) / 255.f;
        const Light_Manager *Manager        = Light_Manager::GetInstance();
        const sf::Vector3f  v_ambiant_color = sf::Vector3f(Manager->m_basicLight.r, Manager->m_basicLight.g, Manager->m_basicLight.b) / 255.f;
        // MOG
        CombineEffect2.SetParameter("u_v_position_light",         m_position);
        CombineEffect2.SetParameter("u_f_influence_radius_light", m_influence_radius);
        CombineEffect2.SetParameter("u_v_color_light",            v_color_light);
        CombineEffect2.SetParameter("u_v_ambiant_color",          v_ambiant_color);
        CombineEffect2.SetTexture("u_texture_light_map",            m_img_light_buffer);
        //
        App->Draw(sprite, CombineEffect2);
    }
}

void Light_SV::Draw_Analytic_Circle(sf::RenderTarget *App) const
{
    // Bounding Box de la light
    App->Draw(m_shape[0]);
}

void Light_SV::Draw_Analytic_Circle(sf::RenderTarget *App, const sf::Shader& shader) const
{
    // Bounding Box de la light
    App->Draw(m_shape[0], shader);
}

void Light_SV::Draw_Analytic_Circle_Light(sf::RenderTarget *App, sf::Shader& shader) const
{
    shader.SetParameter("u_v_position_light",         m_position);
    shader.SetParameter("u_f_influence_radius_light", m_influence_radius);
    //
    Draw_Analytic_Circle(App, shader);
}

void Light_SV::Render_SSV(sf::RenderTarget *App, const std::vector <sf::Shape> &shapes_intersections_ssv, const int i, sf::Shader &shader) const
{
    const sf::Vector2f  &intersection0_segment_circle   = shapes_intersections_ssv[i].GetPointPosition(0);
    const sf::Shape     &shape_for_second_vertex        = (i%2)?shapes_intersections_ssv[i-1]:shapes_intersections_ssv[i+1];
    const sf::Vector2f  &intersection1_segment_circle   = shape_for_second_vertex.GetPointPosition(0);
    const sf::Vector2f  &intersection_dir               = intersection0_segment_circle;
    const sf::Vector2f  intersection_normal             = NORMALIZE(NORMAL(intersection_dir));
    //
    shader.SetParameter("u_v_position_light",           m_position);
    shader.SetParameter("u_f_radius_light",             m_inner_radius);
    shader.SetParameter("u_f_influence_radius_light",  m_influence_radius);
    //
    shader.SetParameter("u_v_e0",     intersection0_segment_circle);
    shader.SetParameter("u_v_e1",     intersection1_segment_circle);
    shader.SetParameter("u_v_intersection_normal",              intersection_normal);
    //
    App->Draw(shapes_intersections_ssv[i], shader);
}

void Light_SV::Generate(std::vector<Wall> &_vector_walls)
{
    clear();

    #ifdef __USE_ANALYTIC_GENERATION__
        Light_Wall light_walls( m_position, m_inner_radius, m_influence_radius );
        //Generate_With_Analytic_Computations(m_wall, m_position, m_influence_radius, m_inner_radius, light_walls, false); // [MOG]
        Generate_With_Analytic_Computations(_vector_walls);
    #endif
    #ifdef __USE_PROJECTION_AT_INFINY_GENERATION__
        Generate_With_Projection_At_Infinity(_vector_walls);
    #endif

    #ifdef __USE_GROUND_TRUTH__ // [MOG]
        Generate_Ground_Truth(_vector_walls);
    #endif
}

void Light_SV::Generate_With_Analytic_Computations(std::vector<Wall> &_vector_walls)
{
    if (!m_b_is_construct)
        Construct();

    UpdatePosition();

    // On boucle sur tous les murs
    for(std::vector<Wall>::iterator IterWall=_vector_walls.begin();IterWall!=_vector_walls.end();++IterWall)
    {
        Light_Wall light_wall(m_position, m_inner_radius, m_influence_radius, IterWall->pt1, IterWall->pt2);
        light_wall.Compute();
        m_lights_walls.push_back(light_wall);
    }
}

void Light_SV::Construct()
{
    m_shape.push_back( sf::Shape() );
    m_b_is_construct = true;
}

void Light_SV::UpdatePosition()
{
    m_shape.back() = sf::Shape::Rectangle(m_position.x-m_influence_radius, m_position.y-m_influence_radius, m_influence_radius*2, m_influence_radius*2, sf::Color(255, 10, 0, 128));
}

void Light_SV::Generate_Ground_Truth( std::vector<Wall> &m_wall )
{
    // Boucler sur tous les pixels de m_renderLightBufferImg
    // pour chaque pixel
    //  envoyer des rayons en direction d'un echantillonage (aleatoire) de la source de lumiere
    //  pour chaque rayon
    //      l'intersecter par l'ensemble des murs de la scene
    //      faire la moyenne des rayons non intersectes
    //      calculer le coefficient de visibilite et ecrire le resultat
    if (m_renderLightBufferImg)
    {
        m_img_light_buffer = m_renderLightBufferImg->GetImage();

        // Boucler sur tous les pixels de m_renderLightBufferImg
        for(unsigned int y=0; y<m_img_light_buffer.GetHeight(); ++y)
        {
            for(unsigned int x=0; x<m_img_light_buffer.GetWidth(); ++x)
            {
                // pour chaque pixel
                vec2            v2_pos_receiver(x, y);
                unsigned int    nb_ray_not_intersect = 0;
                //  envoyer des rayons en direction de la source de lumiere aux murs de la scene
                for(unsigned int i=0; i<NB_SAMPLES_FOR_GROUNDTRUTH; ++i)
                {
                    vec2 offset;
                    float rand_number = (float)(rand())/(float)(RAND_MAX);
                    // SIMULATE BILLBOARD LIGHT
                    #ifdef __GROUND_TRUTH_RANDOM_SAMPLING__
                    // strategie d'echantillonnage: RANDOM (=> bruit)
                    {
                        //
                        vec2 rand_vec   = (2.f*rand_number-1.f)*vec2(1, 1);
                        //offset          = rand_vec*m_inner_radius;
                        offset = vec2((float)(rand())/(float)(RAND_MAX), (float)(rand())/(float)(RAND_MAX))*2.f - vec2(1.f, 1.f);
                        offset *= m_inner_radius;
                    }
                    #endif
                    #ifdef __GROUND_TRUTH_STRATIFIED_SAMPLING__
                    // strategie d'echantillonnage: STRATIFIED (+ random => bruit)
                    {
                        float rand_stratified_number = CLAMP((((i/(float)(NB_SAMPLES_FOR_GROUNDTRUTH))*2.f - 1.f) - (rand_number*2.f - 1.f)/(float)(NB_SAMPLES_FOR_GROUNDTRUTH)), -1.f, +1.f);
                        //
                        vec2 v_dir      = m_position - v2_pos_receiver;
                        vec2 v_normal   = NORMALIZE(NORMAL(v_dir));
                        //
                        offset = v_normal * rand_stratified_number * m_inner_radius;
                    }
                    #endif

                    vec2 v2_pos_light = m_position + offset;
                    // l'intersecter par l'ensemble des murs de la scene
                    bool b_not_intersect = true;
                    std::vector<Wall>::iterator IterWall=m_wall.begin();
                    while( b_not_intersect && IterWall!=m_wall.end())
                    {
                        vec2 P_intersection;
                        b_not_intersect = !Compute_Intersection_Segments( v2_pos_receiver, v2_pos_light, IterWall->pt1, IterWall->pt2, P_intersection );
                        ++IterWall;
                    }
                    nb_ray_not_intersect += b_not_intersect;
                }
                // faire la moyenne des rayons non intersectes
                // calculer le coefficient de visibilite
                float coef_shadow = 1 - ((float)(nb_ray_not_intersect)/(float)(NB_SAMPLES_FOR_GROUNDTRUTH));
                // on ecrit le resultat [MOG]
                coef_shadow *= 255;
                sf::Color color(coef_shadow, coef_shadow, coef_shadow);
                m_img_light_buffer.SetPixel(x, y, color);
            }
        }
    }
}

void Light_SV::Draw_Debug(sf::RenderTarget* App)
{
    #ifdef __DRAW_DEBUG__
    {
        std::vector<sf::Shape> vector_shapes_debug;

        #ifdef __DRAW_DEBUG_RENDER_SHAPES_POINTS__
        {
            for(std::vector<Light_Wall>::iterator IterLight_Wall=m_lights_walls.begin();IterLight_Wall!=m_lights_walls.end();++IterLight_Wall)
            {
                std::vector<Bounding_Volume> list_bv = IterLight_Wall->List_Bounding_Volumes();
                for(std::vector<Bounding_Volume>::iterator IterBV=list_bv.begin();IterBV!=list_bv.end();++IterBV)
                {
                    sf::Color color = sf::Color(32, 128, 255, 255);
                    switch(IterBV->Type())
                    {
                        case SHADOW_VOLUME:
                            color = sf::Color(32, 32, 196, 255);
                        break;
                        case OUTER_PENUMBRA:
                            color = sf::Color(196, 64, 32, 255);
                        break;
                        case INNER_PENUMBRA:
                            color = sf::Color(16, 128, 196, 255);
                        break;
//                        case PENUMBRAS_WIL:
//                            color = sf::Color(96, 196, 32, 255);
//                        break;
//                        case PENUMBRAS_WTL:
//                            color = sf::Color(32, 196, 96, 255);
//                        break;
                        default:
                        break;
                    }
                    const sf::Shape &shape = IterBV->Shape();
                    for(unsigned int i=0; i<shape.GetPointsCount(); ++i)
                    {
                        const vec2 &point = shape.GetPointPosition(i);
                        ADD_DEBUG_CIRCLE(vector_shapes_debug, m_position, point, 3, color);
                    }
                }
            }
        }
        #endif

        #ifdef __DRAW_DEBUG_RENDER_SHAPES_OUTLINES__
        {
            for(std::vector<Light_Wall>::iterator IterLight_Wall=m_lights_walls.begin();IterLight_Wall!=m_lights_walls.end();++IterLight_Wall)
            {
                std::vector<Bounding_Volume> list_bv = IterLight_Wall->List_Bounding_Volumes();
                for(std::vector<Bounding_Volume>::iterator IterBV=list_bv.begin();IterBV!=list_bv.end();++IterBV)
                {
                    sf::Color color = sf::Color(32, 128, 255, 255);
                    switch(IterBV->Type())
                    {
                        case SHADOW_VOLUME:
                            color = sf::Color(32, 32, 196, 255);
                        break;
                        case OUTER_PENUMBRA:
                            color = sf::Color(196, 64, 32, 255);
                        break;
                        case INNER_PENUMBRA:
                            color = sf::Color(16, 128, 196, 255);
                        break;
//                        case PENUMBRAS_WIL:
//                            color = sf::Color(96, 196, 32, 255);
//                        break;
//                        case PENUMBRAS_WTL:
//                            color = sf::Color(32, 196, 96, 255);
//                        break;
                        default:
                        break;
                    }
                    sf::Shape shape = IterBV->Shape();
                    // modification des attributs de la shape (juste afficher les contours)
                    shape.EnableFill(false);
                    shape.EnableOutline(true);
                    // MOG
                    shape.SetOutlineThickness(1.0);
                    shape.SetColor(color);
                    //
                    vector_shapes_debug.push_back(shape);
                }
            }
        }
        #endif

        #ifdef __DRAW_DEBUG_RENDER_INNER_LIGHT__
            ADD_DEBUG_CIRCLE_OUTLINE(vector_shapes_debug, m_position, sf::Vector2f(0, 0), m_inner_radius, sf::Color(64,128,196), 1.f);
        #endif

        //
        for(std::vector<sf::Shape>::iterator IterShape=vector_shapes_debug.begin(); IterShape!=vector_shapes_debug.end(); ++IterShape)
            App->Draw(*IterShape);

        #ifdef __DRAW_DEBUG_LIGHT_WALL__
        {
            vec2 v2_receiver = vec2(400, 500);

//            v2_receiver = vec2(App->GetWidth()/2.f, App->GetHeight()/2.f);
//            static float angle = 0;
//            angle += 0.025;
//            float r = 400 * sin(angle)*cos(angle);
//            v2_receiver += vec2(cos(angle), sin(angle))*r;

            for(std::vector<Light_Wall>::iterator IterLight_Wall=m_lights_walls.begin();IterLight_Wall!=m_lights_walls.end();++IterLight_Wall)
            {
                IterLight_Wall->Debug(App, v2_receiver);
                //IterLight_Wall->Debug(App, SoftShadowEffect_Debug, v2_receiver);
            }
        }
        #endif

    }
    #endif
}
