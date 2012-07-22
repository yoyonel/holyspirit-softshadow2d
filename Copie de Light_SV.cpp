#include <GL/glew.h>

#include "Light_SV.h"
#include <iostream>
#include <sstream>
#include <limits>
#include <GL/gl.h>
#include <math.h>
#include "Light_SV_tools.h"
#include "LightManager.h"

#define __DRAW_DEBUG__                  // [MOG]
//#define __DRAW_DEBUG_SV__
//#define __DRAW_DEBUG_OUTER_SSV__
//#define __DRAW_DEBUG_INNER_SSV__
//#define __DRAW_DEBUG_LIGHTS_WALLS__
#define __DRAW_DEBUG_INTERSECTIONS_SV__
#define __DRAW_DEBUG_INTERSECTIONS_SSV__
#define __DRAW_DEBUG_TEST_INTERSECTION__
#define __DRAW_TEXT__
//
#define __RENDER_SSV__  // <= render Soft Shadow Volume
#define __USE_SSV__

#define __USE_ANALYTIC_GENERATION__
//#define __USE_PROJECTION_AT_INFINY_GENERATION__

//#define __USE_GROUND_TRUTH__ // [MOG]
//
#define __DRAW_LIGHT__

#define RADIUS_SOFT_LIGHT   32 // [MOG]

#define NB_SAMPLES_FOR_GROUNDTRUTH 32

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
    // On vide la mémoire
    m_shape_sv.clear();
    m_shape_inner_ssv.clear();
    m_shape_outer_ssv.clear();
    m_shape_intersections_sv.clear();
    m_shape_intersections_ssv.clear();
    m_shape_tests_intersections.clear();
    m_shape_text.clear();
    m_lights_walls.clear();
}

void Light_SV::InitEffects()
{
    // On récupère les shaders chargés par le Light_Manager
    const Light_Manager *Manager = Light_Manager::GetInstance();
    //
    SphereLighting2D                = Manager->GetDiscLightingEffect();
    SoftShadowInnerEffect           = Manager->GetSoftShadowInnerEffect();
    SoftShadowOuterEffect           = Manager->GetSoftShadowOuterEffect();
    //
    CombineEffect.LoadFromFile(                 "data/shaders/light_2d_ssv.glsl", "data/shaders/combine.glsl");

    CombineEffect2.LoadFromFile(                "data/shaders/light_2d_ssv.glsl", "data/shaders/combine_2.glsl");
    SoftShadowEffect.LoadFromFile(              "data/shaders/vertex_ss_2d.glsl", "data/shaders/pixel_ss_2d.glsl");
    SoftShadowEffect_GroundTruth.LoadFromFile(  "data/shaders/vertex_ss_2d.glsl", "data/shaders/pixel_ss_2d_gt.glsl");

    // Les shaders sont ils utilisables ?
    m_b_init_effects                = SphereLighting2D.IsAvailable() && SoftShadowInnerEffect.IsAvailable() && SoftShadowOuterEffect.IsAvailable();

    img_normal_map.LoadFromFile("data/textures/rockNormal.bmp");
//    img_normal_map.LoadFromFile("data/textures/brickwork-bump-map.png");
//    img_normal_map.LoadFromFile("data/textures/brickwork_normal-map.png");

    CombineEffect2.SetTexture("texture_normal_map", img_normal_map);
}

void Light_SV::Draw_Test_Stencil(sf::RenderTarget *App)
{
    m_renderLightBufferImg->SaveGLStates();
    m_renderLightBufferImg->SetView(App->GetView());
    m_renderLightBufferImg->SetActive(true);

//    PRINT_STENCIL_INFO

    GLuint mask_stencil = (GLuint)(~0x0);
    GLint ref_stencil = 0;
    GLint clear_stencil = 0;

//    DEBUG_PRINT("mask_stencil", mask_stencil);

    glDisable(GL_ALPHA_TEST); // le test alpha est activé par défaut
    glAlphaFunc(GL_ALWAYS, 0);
    //
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    //
    glEnable(GL_STENCIL_TEST);
    glStencilMask(mask_stencil);
    //
    glClearStencil(clear_stencil);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);


    {
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glStencilFunc(GL_EQUAL, ref_stencil, mask_stencil);
        glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
        Draw_Analytic_Circle(m_renderLightBufferImg);
    }

    {
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glStencilFunc(GL_EQUAL, (ref_stencil+1), mask_stencil);
        glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
        Draw_Analytic_Circle(m_renderLightBufferImg);
    }

    {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilFunc(GL_EQUAL, ref_stencil+2, mask_stencil);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        Draw_Analytic_Circle(m_renderLightBufferImg);
    }

    m_renderLightBufferImg->RestoreGLStates();
    m_renderLightBufferImg->Display();

    sf::Sprite sprite;
    sprite.SetImage(m_renderLightBufferImg->GetImage());
    sprite.SetBlendMode(sf::Blend::None);
    App->Draw(sprite);
}

// N'utilise pas un système de shape englobante (pour le shadow volume, et les penumbra volumes)
void Light_SV::Draw_Full_Light(sf::RenderTarget *App)
{
    if (m_b_init_effects)
    {
        m_renderLightBufferImg->SetView(App->GetView());
        m_renderLightBufferImg->SetActive(true);
        m_renderLightBufferImg->SaveGLStates();

        GLuint mask_stencil = (GLuint)(~0x0);
        GLint ref_stencil = 0;
        GLint clear_stencil = 0;

        glDisable(GL_ALPHA_TEST); // le test alpha est activé par défaut
        glAlphaFunc(GL_ALWAYS, 0);
        //
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        //
        glEnable(GL_STENCIL_TEST);
        glStencilMask(mask_stencil);
        //
        glClearStencil(clear_stencil);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);


        #ifdef __RENDER_SSV__
        {
            glAlphaFunc(GL_ALWAYS, 0);

            //
            SoftShadowEffect.SetParameter("u_v_position_light",             m_position);
            SoftShadowEffect.SetParameter("u_f_radius_light",               m_inner_radius);
            SoftShadowEffect.SetParameter("u_f_influence_radius_light",     m_influence_radius);

            // On utilise le stencil pour ne pas écrire calculer l'ombre deux fois au même endroit
            glStencilFunc( GL_EQUAL, ref_stencil, mask_stencil);
            glStencilOp( GL_INCR, GL_INCR, GL_INCR);

            for(std::vector<Light_Wall>::iterator IterLight_Wall=m_lights_walls.begin();IterLight_Wall!=m_lights_walls.end();++IterLight_Wall)
            {
                std::vector<Bounding_Volume> list_bv = IterLight_Wall->List_Bounding_Volumes();
                for(std::vector<Bounding_Volume>::iterator IterBV=list_bv.begin();IterBV!=list_bv.end();++IterBV)
                {
                    switch(IterBV->Type())
                    {
                        case SHADOW_VOLUME:
                        case OUTER_PENUMBRA:
                        case INNER_PENUMBRA:
                        case PENUMBRAS:
                        {
                            sf::Shape shape_sv = IterBV->Shape();
                            //
                            const sf::Vector2f  &intersection0_segment_circle   = IterBV->E0();
                            const sf::Vector2f  &intersection1_segment_circle   = IterBV->E1();
                            //
                            SoftShadowEffect.SetParameter("u_v_e0",     intersection0_segment_circle);
                            SoftShadowEffect.SetParameter("u_v_e1",     intersection1_segment_circle);
                            // On rend le volume d'ombre en mode additif (les ombres se cumulent pour un couple lumière-occluder)
                            shape_sv.SetBlendMode(sf::Blend::Add);
                            m_renderLightBufferImg->Draw(shape_sv, SoftShadowEffect);
                        }
                        break;
                    }
                }
            }

//            for(int i=0;i<(int)m_shape_sv.size();++i)
//            {
//                // Shadow-Volume
//                {
//                    //
//                    const sf::Vector2f  &intersection0_segment_circle   = m_shape_sv[i].GetPointPosition(0);
//                    const sf::Vector2f  &intersection1_segment_circle   = m_shape_sv[i].GetPointPosition(5);
//                    //
//                    SoftShadowEffect.SetParameter("u_v_e0",     intersection0_segment_circle);
//                    SoftShadowEffect.SetParameter("u_v_e1",     intersection1_segment_circle);
//                    // On rend le volume d'ombre en mode additif (les ombres se cumulent pour un couple lumière-occluder)
//                    m_shape_sv[i].SetBlendMode(sf::Blend::Add);
//                    m_renderLightBufferImg->Draw(m_shape_sv[i], SoftShadowEffect);
//                }
//
//                // PENUMBRA
//                for(int j=0; j<2; ++j)
//                {
//                    int indice = (i*2) + j;
//                    for(int k=0; k<2; ++k)
//                    {
//                        // OUTER ou INNER Penumbras
//                        std::vector<sf::Shape> &shape = k ? m_shape_outer_ssv : m_shape_inner_ssv;
//                        // On récupère les points formant le segment-mur
//                        const sf::Vector2f  &intersection0_segment_circle   = shape[indice].GetPointPosition(0);
//                        const sf::Shape     &shape_for_second_vertex        = (indice%2)? shape[indice-1] : shape[indice+1];
//                        const sf::Vector2f  &intersection1_segment_circle   = shape_for_second_vertex.GetPointPosition(0);
//                        // On les transmet au shader
//                        SoftShadowEffect.SetParameter("u_v_e0",     intersection0_segment_circle);
//                        SoftShadowEffect.SetParameter("u_v_e1",     intersection1_segment_circle);
//                        // On rend le volume d'ombre en mode additif (les ombres se cumulent pour un couple lumière-occluder)
//                        shape[indice].SetBlendMode(sf::Blend::Add);
//                        m_renderLightBufferImg->Draw(shape[indice], SoftShadowEffect);
//                    }
//                }
//
//                // On efface le stencil
//                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//                glStencilMask(mask_stencil);
//                glStencilFunc(GL_ALWAYS, ref_stencil, mask_stencil);
//                glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
//                // Shadow-Volume
//                m_renderLightBufferImg->Draw(m_shape_sv[i]);
//                // PENUMBRA
//                for(int j=0; j<2; ++j)
//                {
//                    int indice = (i*2) + j;
//                    for(int k=0; k<2; ++k)
//                    {
//                        std::vector<sf::Shape> &shape = k ? m_shape_outer_ssv : m_shape_inner_ssv;
//                        m_renderLightBufferImg->Draw(shape[indice]);
//                    }
//                }
//                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//            }


//            // On boucle sur la liste des couples lumières murs
//            for(int i=0;i<(int)m_lights_walls.size();i++)
//            {
//                glClear(GL_STENCIL_BUFFER_BIT);
//
//                glStencilFunc( GL_EQUAL, 0, ~0);
//                glStencilOp( GL_INCR, GL_INCR, GL_INCR);
//
//                const Light_Wall &light_walls = m_lights_walls[i];
//
//                // On place les attributs de la lumière pour les calculs d'occlusion
//                SoftShadowEffect.SetParameter("u_v_position_light",             light_walls.Position());
//                SoftShadowEffect.SetParameter("u_f_radius_light",               light_walls.InnerRadius());
//                SoftShadowEffect.SetParameter("u_f_influence_radius_light",     light_walls.InfluenceRadius());
//
//                // On recupère les shapes associées à ce couple lumière-murs
//                const std::vector<sf::Shape> &list_shapes = light_walls.ListShapes();
//                std::cout << "list_shapes.size(): " << list_shapes.size() << std::endl;
//
//                for(int j=0; j<(int)list_shapes.size(); ++j)
//                {
//                    sf::Shape shape = list_shapes[j];
//                    sf::Vector2f  intersection0_segment_circle, intersection1_segment_circle;
//
//                    // 4 shapes sur 5 (indices: 1, 2, 3, 4 (%5)), sont des shapes outer/inner
//                    if (j%5)
//                    {
//                        // (j%5) == 1 | 2 | 3 | 4
//                        const int indice = (j%5)-1; // indice = 0 | 1 | 2 | 3
//                        const int next_indice = indice%2 ? j-1:j+1;
//                        const sf::Shape &shape_for_second_vertex = list_shapes[next_indice];
//                        std::cout << "next_indice: " << next_indice << std::endl;
//                        // shape outer/inner
//                        intersection0_segment_circle    = shape.GetPointPosition(0);
//                        intersection1_segment_circle    = shape_for_second_vertex.GetPointPosition(0);
//                        //
//                        SoftShadowEffect.SetParameter("u_v_e0",     intersection0_segment_circle);
//                        SoftShadowEffect.SetParameter("u_v_e1",     intersection1_segment_circle);
//                        //
//                        shape.SetBlendMode(sf::Blend::Add);
//                        m_renderLightBufferImg->Draw(shape, SoftShadowEffect);
//                    }
//                    else // indice: 0 (%5), SV shape
//                    {
//                        // shape sv
//                        intersection0_segment_circle   = shape.GetPointPosition(0);
//                        intersection1_segment_circle   = shape.GetPointPosition(5);
////                        //
////                        SoftShadowEffect.SetParameter("u_v_e0",     intersection0_segment_circle);
////                        SoftShadowEffect.SetParameter("u_v_e1",     intersection1_segment_circle);
////                        //
////                        shape.SetBlendMode(sf::Blend::Add);
////                        m_renderLightBufferImg->Draw(shape, SoftShadowEffect);
//                    }
//                }
//            }
        }
        #endif

        m_renderLightBufferImg->RestoreGLStates();
        m_renderLightBufferImg->Display();

        sf::Sprite sprite;
        #ifdef __USE_GROUND_TRUTH__
            sprite.SetImage(m_img_light_buffer);
        #else
            sprite.SetImage(m_renderLightBufferImg->GetImage()); // [MOG]
        #endif
        sprite.SetBlendMode(sf::Blend::Add); // [MOG]
//        sprite.SetBlendMode(sf::Blend::None);
        //
        const sf::Vector3f  v_color_light   = sf::Vector3f(m_color.r, m_color.g, m_color.b) / 255.f;
        //
        CombineEffect2.SetParameter("u_v_position_light",         m_position);
        CombineEffect2.SetParameter("u_f_influence_radius_light", m_influence_radius);
        CombineEffect2.SetParameter("u_v_color_light",            v_color_light);

        App->Draw(sprite, CombineEffect2);    // [MOG]

//        App->Draw(sprite);

        #ifdef __DRAW_DEBUG__
            #ifdef __DRAW_DEBUG_INTERSECTIONS_SSV__
            {
//                ADD_DEBUG_CIRCLE(           m_shape_intersections_ssv, m_position, sf::Vector2f(0, 0), m_inner_radius, sf::Color(1-m_color.r,1-m_color.g,1-m_color.b,255));
                ADD_DEBUG_CIRCLE_OUTLINE(   m_shape_intersections_ssv, m_position, sf::Vector2f(0, 0), m_inner_radius, sf::Color(255, 224, 32, 255), 2);

//                for(int i=0;i<(int)m_shape_outer_ssv.size();i++)
//                    App->Draw(m_shape_outer_ssv[i]);
//                for(int i=0;i<(int)m_shape_sv.size();i++)
//                    App->Draw(m_shape_sv[i]);
                for(int i=0;i<(int)m_shape_intersections_ssv.size();i++)
                    App->Draw(m_shape_intersections_ssv[i]);
            }
            #endif
            #ifdef __DRAW_DEBUG_INTERSECTIONS_SV__
                for(int i=0;i<(int)m_shape_intersections_sv.size();i++)
                    App->Draw(m_shape_intersections_sv[i]);
            #endif
            #ifdef __DRAW_DEBUG_SV__
                // On boucle sur m_shape_sv pour afficher tous les quads.
                for(int i=0;i<(int)m_shape_sv.size();i++)
                {
                    // copie de la shape pour la modifier
                    sf::Shape shape = m_shape_sv[i];
                    // modification des attributs de la shape (juste afficher les contours)
                    shape.EnableFill(false);
                    shape.EnableOutline(true);
                    shape.SetOutlineWidth(2);
                    // rendu de la shape
                    App->Draw(shape);
                }
            #endif
            #ifdef __DRAW_DEBUG_OUTER_SSV__
                // On boucle sur m_shape_outer_ssv pour afficher tous les quads.
                for(int i=0;i<(int)m_shape_outer_ssv.size();i++)
                {
                    // copie de la shape pour la modifier
                    sf::Shape shape = m_shape_outer_ssv[i];
                    // modification des attributs de la shape (juste afficher les contours)
                    shape.EnableFill(false);
                    shape.EnableOutline(true);
                    shape.SetOutlineWidth(1);
                    // rendu de la shape
                    App->Draw(shape);
                }
            #endif
            #ifdef __DRAW_DEBUG_INNER_SSV__
                // On boucle sur m_shape_outer_ssv pour afficher tous les quads.
                for(int i=0;i<(int)m_shape_inner_ssv.size();i++)
                {
                    // copie de la shape pour la modifier
                    sf::Shape shape = m_shape_inner_ssv[i];
                    // modification des attributs de la shape (juste afficher les contours)
                    shape.EnableFill(false);
                    shape.EnableOutline(true);
                    shape.SetOutlineWidth(1);
                    // rendu de la shape
                    App->Draw(shape);
                }
            #endif
            #ifdef __DRAW_DEBUG_LIGHTS_WALLS__
                // On boucle sur
                for(int i=0;i<(int)m_lights_walls.size();i++)
                {
                    const Light_Wall &light_walls = m_lights_walls[i];
                    const std::vector<sf::Shape> &list_shapes = light_walls.ListShapes();
                    for(int j=0; j<(int)list_shapes.size(); ++j)
                    {
                        // copie de la shape pour la modifier
                        sf::Shape shape = list_shapes[j];
                        // modification des attributs de la shape (juste afficher les contours)
                        shape.EnableFill(false);
                        shape.EnableOutline(true);
                        shape.SetOutlineWidth(1);
                        //shape.SetColor(sf::Color(0, 255, 0));
                        // rendu de la shape
                        App->Draw(shape);
                    }
                }
            #endif

            #ifdef __DRAW_DEBUG_TEST_INTERSECTION__
                // On boucle sur
                for(int i=0;i<(int)m_shape_tests_intersections.size();i++)
                    App->Draw(m_shape_tests_intersections[i]);
            #endif

            #ifdef __DRAW_TEXT__
                // On boucle sur
                for(int i=0;i<(int)m_shape_text.size();i++)
                    App->Draw(m_shape_text[i]);
            #endif
        #endif
    }
}

// Echantillonage de la source de lumière (par un ensemble de source de lumière ponctuelle)
void Light_SV::Draw_GroundTruth(sf::RenderTarget *App)
{
    if (m_b_init_effects)
    {
        // generate()
        sf::Sprite sprite;
        sprite.SetImage(m_img_light_buffer);
        sprite.SetBlendMode(sf::Blend::None);
        //
        const sf::Vector3f  v_color_light   = sf::Vector3f(m_color.r, m_color.g, m_color.b) / 255.f;
        const Light_Manager *Manager        = Light_Manager::GetInstance();
        const sf::Vector3f  v_ambiant_color = sf::Vector3f(Manager->m_basicLight.r, Manager->m_basicLight.g, Manager->m_basicLight.b) / 255.f;
        //
        CombineEffect2.SetParameter("u_v_position_light",         m_position);
        CombineEffect2.SetParameter("u_f_influence_radius_light", m_influence_radius);
        CombineEffect2.SetParameter("u_v_color_light",            v_color_light);
        CombineEffect2.SetParameter("u_v_ambiant_color",          v_ambiant_color);
        //
        App->Draw(sprite, CombineEffect2);

        #ifdef __DRAW_DEBUG__
        #endif
    }
}

void Light_SV::Draw(sf::RenderTarget *App)
{
    if (m_b_lb_is_not_create)
    {
        m_renderLightBufferImg = new sf::RenderImage();
        m_renderLightBufferImg->Create(App->GetWidth(), App->GetHeight(), true, true);
        m_b_lb_is_not_create = false;
    }

    Draw_Full_Light(App);
//    Draw_Test_Stencil(App);
//    Draw_GroundTruth(App);
    return;

    m_renderLightBufferImg->SetView(App->GetView());

    m_renderLightBufferImg->SetActive(true);
//    m_renderLightBufferImg->Clear( sf::Color(0, 0, 0, 1) );
    glClearStencil(0);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (m_b_init_effects)
    {
        m_renderLightBufferImg->SaveGLStates();

        /*
        {
            glEnable(GL_STENCIL_TEST);

            // (1): on écrit 1 dans le stencil buffer la zone d'influence de la lumière
            {
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glStencilMask(GL_TRUE);
                glStencilFunc(GL_ALWAYS, 1, 3);
                glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
                Draw_Analytic_Circle_Light(m_renderLightBufferImg, SphereLighting2D);
            }

            #ifdef __USE_PROJECTION_AT_INFINY_GENERATION__
                // (1-b): Set view and viewport to use frustum culling of OpenGL (and clip shadow-volumes by the boundingbox of the light)
                {
                    sf::View view = m_renderLightBufferImg->GetView();
                    sf::Vector2f top_right_in_view(m_position.x-m_influence_radius, m_position.y-m_influence_radius);
                    sf::Vector2f size_in_view(m_influence_radius*2, m_influence_radius*2);
                    view.Reset(sf::FloatRect(top_right_in_view.x, top_right_in_view.y, size_in_view.x, size_in_view.y));
                    sf::Vector2f size_default_view = m_renderLightBufferImg->GetDefaultView().GetSize();
                    sf::Vector2f top_right_in_viewport(top_right_in_view.x/size_default_view.x, top_right_in_view.y/size_default_view.y);
                    sf::Vector2f size_in_viewport(size_in_view.x/size_default_view.x, size_in_view.y/size_default_view.y);
                    view.SetViewport(sf::FloatRect(top_right_in_viewport.x, top_right_in_viewport.y, size_in_viewport.x, size_in_viewport.y));
                    m_renderLightBufferImg->SetView(view);
                }
            #endif

            // (2)  On décremente le stencil-buffer avec les shadow-volume sur la zone de la lumière (égale à 1 dans le stencil buffer)
            //      Les pixels incluent dans les shadow volume auront comme valeur 0 (car décrementés avec saturation)
            {
                // color mask: false^4
                // stencil mask: true
                glStencilFunc(GL_EQUAL, 1, 3);
                glStencilOp(GL_DECR, GL_DECR, GL_DECR);
                // On boucle sur m_shape_sv pour afficher tous les quads.
                for(int i=0;i<(int)m_shape_sv.size();i++)
                    m_renderLightBufferImg->Draw(m_shape_sv[i]);
            }

            #ifdef __DRAW_LIGHT__
                // (3) On rend la source de lumière uniquement aux endroits ou il n'y a pas de shadow-volume détectés dans le stencil-buffer (texel égal à 0 dans le stencil)
                {
                    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                    glStencilMask(GL_FALSE);
                    glStencilFunc(GL_EQUAL,1, 3);
                    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                    //
                    SphereLighting2D.SetParameter("u_v_position_light", m_position);
                    SphereLighting2D.SetParameter("u_f_influence_radius_light",   m_influence_radius);
                    //
                    m_shape[0].SetBlendMode(sf::Blend::Add);
                    Draw_Analytic_Circle_Light(m_renderLightBufferImg, SphereLighting2D);
                }
            #endif

            // (4) On efface la trace des opérations du stencil en recouvrant la zone de travail (BB de la light)
            {
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glStencilMask(GL_TRUE);
                glStencilFunc(GL_ALWAYS, 0, 3);
                glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
                Draw_Analytic_Circle(m_renderLightBufferImg);
            }

            #ifdef __RENDER_SSV__
            {
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glDisable(GL_STENCIL_TEST);
                for(int i=0;i<(int)m_shape_outer_ssv.size();i++)
                {
                    m_shape_outer_ssv[i].SetBlendMode(sf::Blend::Add);
                    m_shape_inner_ssv[i].SetBlendMode(sf::Blend::Add);
                    // Outer Penumbra [0.5, 1]
                    Render_SSV(m_renderLightBufferImg, m_shape_outer_ssv, i, SoftShadowOuterEffect);
                    // Inner Penumbra [0.5, 0]
                    Render_SSV(m_renderLightBufferImg, m_shape_inner_ssv, i, SoftShadowInnerEffect);
                }
            }
            #endif
        }
        */

        glEnable(GL_STENCIL_TEST);
        glDisable(GL_DEPTH_TEST);

        // (1): on écrit 1 dans le stencil buffer la zone d'influence de la lumière
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glStencilMask(~0);
            glStencilFunc(GL_ALWAYS, 0x1, 0xF);
            glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
            Draw_Analytic_Circle_Light(m_renderLightBufferImg, SphereLighting2D);
        }

        // (2)  On décremente le stencil-buffer avec les shadow-volume sur la zone de la lumière (égale à 1 dans le stencil buffer)
        //      Les pixels incluent dans les shadow volume auront comme valeur 0 (car décrementés avec saturation)
        {
            // color mask: true^4
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            // stencil mask: true
            glStencilFunc(GL_EQUAL, 0x1, 0xF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            // On boucle sur m_shape_sv pour afficher tous les quads.
            for(int i=0;i<(int)m_shape_sv.size();i++)
            {
                m_shape_sv[i].SetBlendMode(sf::Blend::Add);
                m_shape_sv[i].SetColor(sf::Color(0, 1, 0, 1)); // write +1/255 in green component
                m_renderLightBufferImg->Draw(m_shape_sv[i]);
            }
        }

//        #ifdef __DRAW_LIGHT__
//            // (3) On rend la source de lumière uniquement aux endroits ou il n'y a pas de shadow-volume détectés dans le stencil-buffer (texel égal à 0 dans le stencil)
//            {
//                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//                glStencilMask(GL_FALSE);
//                glStencilFunc(GL_EQUAL,1, 3);
//                glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
//                //
//                SphereLighting2D.SetParameter("u_v_position_light", m_position);
//                SphereLighting2D.SetParameter("u_f_influence_radius_light",   m_influence_radius);
//                //
//                m_shape[0].SetBlendMode(sf::Blend::Add);
//                Draw_Analytic_Circle_Light(m_renderLightBufferImg, SphereLighting2D);
//            }
//        #endif

//        // (4) On efface la trace des opérations du stencil en recouvrant la zone de travail (BB de la light)
//        {
//            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//            glStencilMask(GL_TRUE);
//            glStencilFunc(GL_ALWAYS, 0, ~0);
//            glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
//            //Draw_Analytic_Circle(m_renderLightBufferImg);
//            Draw_Analytic_Circle_Light(m_renderLightBufferImg, SphereLighting2D);
//        }

        #ifdef __RENDER_SSV__
        {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDisable(GL_STENCIL_TEST);
            for(int i=0;i<(int)m_shape_outer_ssv.size();i++)
            {
                m_shape_outer_ssv[i].SetBlendMode(sf::Blend::Add);
                m_shape_inner_ssv[i].SetBlendMode(sf::Blend::Add);
                // Outer Penumbra [0.5, 1]
                Render_SSV(m_renderLightBufferImg, m_shape_outer_ssv, i, SoftShadowOuterEffect);
                // Inner Penumbra [0.5, 0]
                Render_SSV(m_renderLightBufferImg, m_shape_inner_ssv, i, SoftShadowInnerEffect);
            }
        }
        #endif

        m_renderLightBufferImg->RestoreGLStates();
        //App->RestoreGLStates();
    }

    m_renderLightBufferImg->Display();

    sf::Sprite sprite;
    sprite.SetImage(m_renderLightBufferImg->GetImage());
    sprite.SetBlendMode(sf::Blend::Add);
    //
    const sf::Vector3f  v_color_light   = sf::Vector3f(m_color.r, m_color.g, m_color.b) / 255.f;
//    const Light_Manager *Manager        = Light_Manager::GetInstance();
//    const sf::Vector3f  v_ambiant_color = sf::Vector3f(Manager->m_basicLight.r, Manager->m_basicLight.g, Manager->m_basicLight.b) / 255.f;
    //
    CombineEffect.SetParameter("u_v_position_light",         m_position);
    CombineEffect.SetParameter("u_f_influence_radius_light", m_influence_radius);
    CombineEffect.SetParameter("u_v_color_light",            v_color_light);
    //
    App->Draw(sprite, CombineEffect);

    #ifdef __DRAW_DEBUG__
    {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_STENCIL_TEST);
        #ifdef __DRAW_DEBUG_SV__
        {
            // On boucle sur m_shape_sv pour afficher tous les quads.
            for(int i=0;i<(int)m_shape_sv.size();i++)
                App->Draw(m_shape_sv[i]);
        }
        #endif

        #ifdef __DRAW_DEBUG_INTERSECTIONS_SV__
//        {
//            for(int i=0;i<(int)m_shape_intersections_sv.size();i++)
//                App->Draw(m_shape_intersections_sv[i]);
//        }
        #endif
        #ifdef __DRAW_DEBUG_INTERSECTIONS_SSV__
        {
            for(int i=0;i<(int)m_shape_intersections_ssv.size();i++)
                App->Draw(m_shape_intersections_ssv[i]);
        }
        #endif
    }
    #endif

    #ifdef __USE_PROJECTION_AT_INFINY_GENERATION__
        m_renderLightBufferImg->SetView(m_renderLightBufferImg->GetDefaultView());
    #endif
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

void Light_SV::Generate(std::vector<Wall> &m_wall)
{
    clear();

    #ifdef __USE_ANALYTIC_GENERATION__
        Light_Wall light_walls( m_position, m_inner_radius, m_influence_radius );
        Generate_With_Analytic_Computations(m_wall, m_position, m_influence_radius, m_inner_radius, light_walls, false); // [MOG]
        //m_lights_walls.push_back(light_walls);
    #endif
    #ifdef __USE_PROJECTION_AT_INFINY_GENERATION__
        Generate_With_Projection_At_Infinity(m_wall);
    #endif

    #ifdef __USE_GROUND_TRUTH__ // [MOG]
        // Boucler sur tous les pixels de m_renderLightBufferImg
        // pour chaque pixel
        //  envoyer des rayons en direction d'un échantillonage (aléatoire) de la source de lumière
        //  pour chaque rayon
        //      l'intersecter par l'ensemble des murs de la scène
        //      faire la moyenne des rayons non intersectés
        //      calculer le coefficient de visibilité et écrire le résultat
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
                    //  envoyer des rayons en direction de la source de lumière aux murs de la scène
                    for(unsigned int i=0; i<NB_SAMPLES_FOR_GROUNDTRUTH; ++i)
                    {
                        vec2 offset;
                        // stratégie d'échantillonnage: RANDOM (=> bruit)
//                        {
//                            float rand_number = (float)(rand())/(float)(RAND_MAX);
//                            vec2 rand_vec = (2.f*rand_number-1.f)*vec2(1, 1);
//                            offset = rand_vec*m_inner_radius;
//                        }
                        // stratégie d'échantillonnage: STRATIFIED (+ random => bruit)
                        {
                            float rand_number = (float)(rand())/(float)(RAND_MAX);
                            vec2 v_dir      = m_position - v2_pos_receiver;
                            vec2 v_normal   = NORMALIZE(NORMAL(v_dir));
                            float rand_stratified_number = CLAMP((((i/(float)(NB_SAMPLES_FOR_GROUNDTRUTH))*2.f - 1.f) - (rand_number*2.f - 1.f)/(float)(NB_SAMPLES_FOR_GROUNDTRUTH)), -1.f, +1.f);
                            offset = v_normal *  rand_stratified_number * m_inner_radius;
                        }
                        vec2 v2_pos_light = m_position + offset;
                        // l'intersecter par l'ensemble des murs de la scène
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
                    // faire la moyenne des rayons non intersectés
                    // calculer le coefficient de visibilité
                    float coef_shadow = 1 - ((float)(nb_ray_not_intersect)/(float)(NB_SAMPLES_FOR_GROUNDTRUTH));

                    // on écrit le résultat [MOG]
                    sf::Color color;
                    color.r = coef_shadow * 255;
//                    color.g = coef_shadow * 255;
//                    color.b = coef_shadow * 255;
//                    color.a = 255;
//                    sf::Color color( coef_shadow*255, coef_shadow*255, coef_shadow*255, 0 );
//                    sf::Color color(
//                                    float(coef_shadow<0.25) * coef_shadow * 255,
//                                    float(coef_shadow>=0.25 && coef_shadow<0.50) * coef_shadow * 255,
//                                    float(coef_shadow>=0.50 && coef_shadow<0.75) * coef_shadow * 255,
//                                    float(coef_shadow>=0.75) * coef_shadow * 255
//                                    );

                    m_img_light_buffer.SetPixel(x, (m_img_light_buffer.GetHeight()-1)-y, color);
                    sf::Color write_color = m_img_light_buffer.GetPixel(x, (m_img_light_buffer.GetHeight()-1)-y);
                }
            }
        }
    #endif
}

void Light_SV::Generate_With_Analytic_Computations(
                                                   std::vector<Wall>    &m_wall,
                                                   const sf::Vector2f   &pos_light,
                                                   const float          influence_radius,
                                                   const float          inner_radius,
                                                   Light_Wall           &light_walls,
                                                   const bool           use_slip
                                                   )
{
    if (!m_b_is_construct)
        Construct();

    UpdatePosition();

//    light_walls.Set(pos_light, inner_radius, influence_radius);

//    const float f_square_radius_light=influence_radius*influence_radius;

    // On boucle sur tous les murs
    for(std::vector<Wall>::iterator IterWall=m_wall.begin();IterWall!=m_wall.end();++IterWall)
    {
        Light_Wall light_wall(pos_light, inner_radius, influence_radius, IterWall->pt1, IterWall->pt2);
        light_wall.Compute();
        m_lights_walls.push_back(light_wall);
/*
        // - l1 et l2 sont les positions des deux extrémités du mur, relatives au centre de la lumière
        sf::Vector2f l1(IterWall->pt1-pos_light);
        sf::Vector2f l2(IterWall->pt2-pos_light);

        sf::Vector2f intersections_segment_circle[2];

        if (Compute_Intersection_Segment_Circle(pos_light, influence_radius, IterWall->pt1, IterWall->pt2, intersections_segment_circle))
        {
            sf::Vector2f intersections_segment_inner_circle[2];
            if (Compute_Intersection_Segment_Circle( pos_light, inner_radius, IterWall->pt1, IterWall->pt2, intersections_segment_inner_circle))
            {
                if (!EQUAL0_EPS(NORM2(intersections_segment_inner_circle[0] - l1), EPSILON))
                {
                    // Nouveau segment wall dont: 1 point est sur le cercle et le second extérieur (le segment n'intersect plus le cercle)

                    #ifdef __DRAW_DEBUG__
                        #ifdef __DRAW_DEBUG_INTERSECTIONS_SV__
                            ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, intersections_segment_inner_circle[0], 3, sf::Color(128,255,128));
                            ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, l1, 3, sf::Color(128,255,128));
                            ADD_DEBUG_LINE(m_shape_intersections_sv, pos_light, intersections_segment_inner_circle[0], l1, 3, sf::Color(128,255,128));
                        #endif
                    #endif
                }
                if (!EQUAL0_EPS(NORM2(intersections_segment_inner_circle[1] - l2), EPSILON))
                {
                    // Nouveau segment wall dont: 1 point est sur le cercle et le second extérieur (le segment n'intersect plus le cercle)
                    #ifdef __DRAW_DEBUG__
                        #ifdef __DRAW_DEBUG_INTERSECTIONS_SV__
                            ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, intersections_segment_inner_circle[1], 3, sf::Color(128,255,128));
                            ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, l2, 3, sf::Color(128,255,128));
                            ADD_DEBUG_LINE(m_shape_intersections_sv, pos_light, intersections_segment_inner_circle[1], l2, 3, sf::Color(128,255,128));
                        #endif
                    #endif
                }

                #ifdef __DRAW_DEBUG__
                    #ifdef __DRAW_DEBUG_INTERSECTIONS_SV__
                        ADD_DEBUG_LINE(m_shape_intersections_sv, pos_light, intersections_segment_inner_circle[0], intersections_segment_inner_circle[1], 3, sf::Color(255,0,255));
                    #endif
                #endif
            }
            else
            {
                sf::Vector2f    proj_intersections[2],
                                bounding_vertex_sv[2];

                sf::Shape shape_shadow_volume = construct_shadow_volume_shape(pos_light, influence_radius, intersections_segment_circle, color_sv, proj_intersections, bounding_vertex_sv);
                m_shape_sv.push_back(shape_shadow_volume);

                #ifdef __USE_SSV__
                    sf::Vector2f l1_to_l2(l2-l1);

                    // SOFT SHADOW
                    Generate_Shapes_For_SSV(
                                        intersections_segment_circle,
                                        proj_intersections,
                                        l1_to_l2,
                                        pos_light,//sf::Vector2f(0, 0),
                                        inner_radius,
                                        influence_radius,
                                        f_square_radius_light,
                                        light_walls
                                        );
                #endif
                #ifdef __DRAW_DEBUG__
                {
                    #ifdef __DRAW_DEBUG_INTERSECTIONS_SV__
                        // - DEBUG
    //                    ADD_DEBUG_LINE(m_shape_intersections_sv, pos_light, intersections_line_circle[0], intersections_line_circle[1], 3, sf::Color(255,255,0));
                        ADD_DEBUG_LINE(m_shape_intersections_sv, pos_light, intersections_segment_circle[0], intersections_segment_circle[1], 3, sf::Color(255,0,255));
    //                                ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, proj_mid_i0_i1, 2, sf::Color(255,128,64));
                        ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, sf::Vector2f(0, 0), 3, sf::Color(128,255,128));
                        ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, proj_intersections[0], 3, sf::Color(128,255,128));
                        ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, proj_intersections[1], 3, sf::Color(128,255,128));
                        ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, bounding_vertex_sv[0], 5, sf::Color(255,64,64));
                        ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, bounding_vertex_sv[1], 5, sf::Color(255,64,64));
                    #endif
                }
                #endif
            }
        }
//
//        // Test d'intersection entres les BBox du cercle et du segment/wall
//        if (Intersect_BoundingBox(l1, l2, sf::Vector2f(-influence_radius,-influence_radius), sf::Vector2f(influence_radius,+influence_radius)))
//        {
//            sf::Vector2f l1_to_l2(l2-l1);
//
//            // (1-b)  système quadratique pour trouver les intersections de la droite support du mur et le cercle d'influence à l'origine
//            Typ_Solutions_Quadratic_Equation solutions = Solve_Quadratic_Equation(l1, l1_to_l2, f_square_radius_light);
//            // - si intersection (la ligne support avec le cercle de lumière)
//            if (solutions.b_has_solutions)
//            {
//                // icl: influence circle ligh (cercle d'influence de la lumière)
//                bool b_wall_intersect_icl = true;
//
//                const float &f_u1 = solutions.f_u0;
//                const float &f_u2 = solutions.f_u1;
//                // On reconstruit les intersections
//                sf::Vector2f intersections_line_circle[2] = {
//                    sf::Vector2f(l1 + l1_to_l2*f_u1),
//                    sf::Vector2f(l1 + l1_to_l2*f_u2)
//                };
//                // On calcul les distances au carré
//                //  du segment [l1, l2]
//                //  du segment [O, l1] (O: centre de la source de lumière => O=(0, 0))
//                //  du segment [O, l2] (O: centre de la source de lumière => O=(0, 0))
//                //  du segment [l1, i1]
//                //  du segment [l1, i2]
//                float f_square_l1l2 = DOT(l1_to_l2, l1_to_l2);
//                float f_square_l1   = DOT(l1, l1);
//                float f_square_l2   = DOT(l2, l2);
//                float f_square_l1i1 = f_u1*f_u1*f_square_l1l2;
//                float f_square_l1i2 = f_u2*f_u2*f_square_l1l2;
//
//                // 4 cas d'intersections/inclusion possibles pour le segment et le cercle
//                // Est ce que ...
//                bool test10 = (f_square_l1<=f_square_radius_light);                                                         // ... le 1er point du segment/wall est dans le cercle ?
//                bool test11 = (f_square_l2<=f_square_radius_light);                                                         // ... le 2nd point du segment/wall est dans le cercle ?
//                bool test01 = !((f_u1<0 && f_u2<0) || (f_square_l1i1>f_square_l1l2 && f_square_l1i2>f_square_l1l2));        // ... le segment d'intersection intersecte le segment/wall ?
//                bool test00 = (!test10 && !test11);                                                                         // ... les deux points du segment sont en dehors du cercle ?
//                bool test20 = !test10;                                                                                      // ... le 1er point du segment/wall n'est pas dans le cercle ?
//                bool &test21 = test11;                                                                                      // ... le 2nd point du segment/wall est dans le cercle ?
//                bool &test30 = test10;                                                                                      // ... le 1er point du segment/wall est dans le cercle ?
//                bool test31 = !test11;                                                                                      // ... le 2nd point du segment/wall n'est pas dans le cercle ?
//                //
//                if (test00&&test01) // cas 1: les vertex du segment/wall ne sont pas inclus dans le cercle et il y a intersection
//                {
//                    intersections_segment_circle[0] = intersections_line_circle[0];
//                    intersections_segment_circle[1] = intersections_line_circle[1];
//                }
//                else if (test10&&test11) // cas 2: les vertex du segment/wall sont inclus (tous les 2) dans le cercle
//                {
//                    intersections_segment_circle[0] = l1;
//                    intersections_segment_circle[1] = l2;
//                }
//                else if (test20&&test21) // cas 3: Un des deux sommets du segment/wall est inclu dans le cercle (le 1er sommet dans ce cas)
//                {
//                    intersections_segment_circle[0] = intersections_line_circle[0];
//                    intersections_segment_circle[1] = l2;
//                }
//                else if (test30&&test31) // cas 4: Un des deux sommets du segment/wall est inclu dans le cercle (le 2nd sommet dans ce cas)
//                {
//                    intersections_segment_circle[0] = l1;
//                    intersections_segment_circle[1] = intersections_line_circle[1];
//
//                }
//                else
//                {
//                    // sinon le segment/wall n'intersecte pas le cercle de lumière (donc ne projette pas d'ombre)
//                    // Ce cas correspond à la présence du segment dans un coin de la boundingbox du cercle (mais non inclut dans le cercle)
//                    b_wall_intersect_icl = false;
//                }
//
//                if (b_wall_intersect_icl)
//                {
//                    // TEST: Split de la light en deux si intersection avec la ligne support de l'edge
//                    Typ_Solutions_Quadratic_Equation solutions_intersection_edge_light;
//                    solutions_intersection_edge_light.b_has_solutions = false;
//
//                    if (use_slip)
//                    {
//                        //
//                        // Système quadratique pour trouver les intersections de la droite support du mur et le cercle de la lumière à l'origine
//                        float f_square_inner_radius_light = inner_radius*inner_radius;
//                        solutions_intersection_edge_light = Solve_Quadratic_Equation(l1, l1_to_l2, f_square_inner_radius_light);
//                        if (solutions_intersection_edge_light.b_has_solutions)
//                        {
//                            const float& u0 = solutions_intersection_edge_light.f_u0;
//                            const float& u1 = solutions_intersection_edge_light.f_u1;
//                            sf::Vector2f intersections_line_light[2] = {
//                                sf::Vector2f(l1 + l1_to_l2*u0),
//                                sf::Vector2f(l1 + l1_to_l2*u1)
//                            };
//                            sf::Vector2f mid_intersections = COMPUTE_MIDDLE(intersections_line_light[0], intersections_line_light[1]);
//                            sf::Vector2f v_dir = NORMAL(NORMALIZE(intersections_line_light[0] - intersections_line_light[1]));
//                            sf::Vector2f centers[2] = {
//                                COMPUTE_MIDDLE(mid_intersections, (+inner_radius)*v_dir),
//                                COMPUTE_MIDDLE(mid_intersections, (-inner_radius)*v_dir)
//                            };
//                            float radius[2] = {
//                                NORM(mid_intersections - centers[0])
//                                //NORM(mid_intersections - centers[1])
//                            };
//                            radius[1] = inner_radius - radius[0];
//
//                            std::vector<Wall> list_1_wall;
//                            list_1_wall.push_back(*IterWall);
//                            //
//                            Light_Wall lights_walls[2];
//                            Generate_With_Analytic_Computations(list_1_wall, centers[0] + pos_light, influence_radius + radius[0], radius[0], lights_walls[0], false);
//                            Generate_With_Analytic_Computations(list_1_wall, centers[1] + pos_light, influence_radius + radius[1], radius[1], lights_walls[1], false);
//                            m_lights_walls.push_back(lights_walls[0]);
//                            m_lights_walls.push_back(lights_walls[1]);
//                        }
//                    }
//
//                    if (!use_slip || !solutions_intersection_edge_light.b_has_solutions)
//                    {
//                        // projection des sommets clippés sur le cercle d'influence de la lumière
//                        sf::Vector2f proj_intersections[2] = {
//                            NORMALIZE(intersections_segment_circle[0]) * influence_radius,
//                            NORMALIZE(intersections_segment_circle[1]) * influence_radius
//                        };
//                        // Milieu du segment clippé
//                        sf::Vector2f mid_i0_i1       = COMPUTE_MIDDLE(proj_intersections[0], proj_intersections[1]);
//                        // Projection du milieu (a) sur le cercle
//                        sf::Vector2f proj_mid_i0_i1  = NORMALIZE(mid_i0_i1) * influence_radius;
//                        // Calcul des vertex englobant
//                        // Ils correspondent aux intersections des lignes formées par:
//                        //  les sommets de projection des intersections
//                        //  et leurs normales associées, tangentes au cercle.
//                        const sf::Vector2f bounding_vertex_sv[2] = {
//                            Compute_Intersection_Lines(proj_intersections[0], proj_mid_i0_i1),
//                            Compute_Intersection_Lines(proj_intersections[1], proj_mid_i0_i1)
//                        };
//
//                        // Construction de la shape ShadowVolume
//                        {
//                            sf::Shape shape;
//                            shape.AddPoint( intersections_segment_circle[0],    color_sv, WHITE);
//                            shape.AddPoint( proj_intersections[0],              color_sv, WHITE);
//                            shape.AddPoint( bounding_vertex_sv[0],              color_sv, WHITE);
//                            shape.AddPoint( bounding_vertex_sv[1],              color_sv, WHITE);
//                            shape.AddPoint( proj_intersections[1],              color_sv, WHITE);
//                            shape.AddPoint( intersections_segment_circle[1],    color_sv, WHITE);
//                            // On regle le shape sans blending et on fournit la position
//                            shape.SetBlendMode(sf::Blend::None);
//                            shape.SetPosition(pos_light);
//                            // - ajout dans la shape SV des quads (sv) ainsi générés
//                            m_shape_sv.push_back(shape);
//
//                            light_walls.Add(shape);
//                        }
//
//                        #ifdef __USE_SSV__
//                            // SOFT SHADOW
//                            Generate_Shapes_For_SSV(
//                                                intersections_segment_circle,
//                                                proj_intersections,
//                                                l1_to_l2,
//                                                pos_light,//sf::Vector2f(0, 0),
//                                                inner_radius,
//                                                influence_radius,
//                                                f_square_radius_light,
//                                                light_walls
//                                                );
//                        #endif
//
//                        #ifdef __DRAW_DEBUG__
//                        {
//                            #ifdef __DRAW_DEBUG_INTERSECTIONS_SV__
//                                // - DEBUG
//                                ADD_DEBUG_LINE(m_shape_intersections_sv, pos_light, intersections_line_circle[0], intersections_line_circle[1], 3, sf::Color(255,255,0));
//                                ADD_DEBUG_LINE(m_shape_intersections_sv, pos_light, intersections_segment_circle[0], intersections_segment_circle[1], 3, sf::Color(255,0,255));
//    //                                ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, proj_mid_i0_i1, 2, sf::Color(255,128,64));
//                                ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, sf::Vector2f(0, 0), 3, sf::Color(128,255,128));
//                                ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, proj_intersections[0], 3, sf::Color(128,255,128));
//                                ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, proj_intersections[1], 3, sf::Color(128,255,128));
//                                ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, bounding_vertex_sv[0], 5, sf::Color(255,64,64));
//                                ADD_DEBUG_CIRCLE(m_shape_intersections_sv, pos_light, bounding_vertex_sv[1], 5, sf::Color(255,64,64));
//                            #endif
//
//                            #ifdef __DRAW_DEBUG_TEST_INTERSECTION__
//                                vec2 test_point = vec2(400, 200);
//
////                                vec2 P;
////                                bool intersect = Compute_Intersection_Segments( test_point, pos_light, IterWall->pt1, IterWall->pt2, P);
////                                ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), pos_light,   5, sf::Color(255, 255, 64));
////                                ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), test_point,  5, sf::Color(255, 64, 255));
////                                if (intersect)
////                                {
////                                    ADD_DEBUG_LINE(m_shape_tests_intersections, vec2(0, 0), vec2(0, 0), vec2(400, 200), pos_light, 3, sf::Color(128, 64, 196));
////                                    ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), P, 5, sf::Color(255-128, 255-64, 255-196));
////                                }
//
//                                float A_E0 = 1;
//                                float A_E1 = 1;
//                                vec2 E;
//
//                                float d_pt1 = NORM(IterWall->pt1 - pos_light);
//                                float d_pt2 = NORM(IterWall->pt2 - pos_light);
//
//                                bool b_pt1_inside_circle = d_pt1 <= inner_radius;
//                                bool b_pt2_inside_circle = d_pt2 <= inner_radius;
//
//                                float dot_pt1_pt2_light = DOT(IterWall->pt1 - pos_light, IterWall->pt2 - pos_light);
//
//                                if (!(b_pt1_inside_circle || b_pt2_inside_circle) && dot_pt1_pt2_light>0)
//                                {
//                                    E = IterWall->pt1;
//                                    {
//                                        float f_signed_distance = signed_distance_point_line( pos_light, test_point, E );
//                                        float f_distance = abs(f_signed_distance);
//
//                                        float d = f_signed_distance / inner_radius;
//                                        d = CLAMP(d, -1, +1);
//                                        // Aire de visibilité de la source
//                                        A_E0 = (1 - d) / 2;
//                                        if (f_distance < inner_radius)
//                                        {
//                                            // reconstruction des points d'intersections (pas utile, juste pour le debug)
//                                            float a = acos(d);
//                                            vec2 y = NORMALIZE(E-test_point);
//                                            vec2 x = NORMAL(y);
//                                            vec2 P_intersection_0 = pos_light + x*d*inner_radius + y*sinf(a)*inner_radius;
//                                            vec2 P_intersection_1 = pos_light + x*d*inner_radius - y*sinf(a)*inner_radius;
//
//                                            ADD_DEBUG_LINE(m_shape_tests_intersections, vec2(0, 0), test_point, test_point + (E-test_point)*100.0f, 1, f_signed_distance > 0 ? sf::Color(128, 0, 0) : sf::Color(0, 128, 0));
//                                            ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), P_intersection_0, 5, sf::Color(255-128, 255-64, 255-196));
//                                            ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), P_intersection_1, 5, sf::Color(255-128, 255-196, 255-196));
//                                        }
//
//                                        ADD_DEBUG_TEXT(m_shape_text, E, "A_E0", A_E0, sf::Color(0, 0, 255));
//                                        ADD_DEBUG_LINE(m_shape_tests_intersections, vec2(0, 0), test_point, test_point + (E-test_point)*100.0f, 1, f_distance <= inner_radius ? sf::Color(128, 64, 196) : sf::Color(255-128, 255-64, 255-196));
//                                    }
//                                    E = IterWall->pt2;
//                                    {
//                                        float f_signed_distance = signed_distance_point_line( pos_light, test_point, E );
//                                        float f_distance = abs(f_signed_distance);
//
//                                        float d = f_signed_distance / inner_radius;
//                                        d = CLAMP(d, -1, +1);
//                                        // Aire de visibilité de la source
//                                        A_E1 = (1 - d) / 2;
//                                        A_E1 = 1 - A_E1;
//                                        if (f_distance < inner_radius)
//                                        {
//                                            // reconstruction des points d'intersections (pas utile, juste pour le debug)
//                                            float a = acos(d);
//                                            vec2 y = NORMALIZE(E-test_point);
//                                            vec2 x = NORMAL(y);
//                                            vec2 P_intersection_0 = pos_light + x*d*inner_radius + y*sinf(a)*inner_radius;
//                                            vec2 P_intersection_1 = pos_light + x*d*inner_radius - y*sinf(a)*inner_radius;
//
//                                            ADD_DEBUG_LINE(m_shape_tests_intersections, vec2(0, 0), test_point, test_point + (E-test_point)*100.0f, 1, f_signed_distance > 0 ? sf::Color(128, 0, 0) : sf::Color(0, 128, 0));
//                                            ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), P_intersection_0, 5, sf::Color(255-128, 255-64, 255-196));
//                                            ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), P_intersection_1, 5, sf::Color(255-128, 255-64, 255-196));
//                                        }
//
//                                        ADD_DEBUG_TEXT(m_shape_text, E, "A_E1", A_E1, sf::Color(0, 0, 255));
//                                        ADD_DEBUG_LINE(m_shape_tests_intersections, vec2(0, 0), test_point, test_point + (E-test_point)*100.0f, 1, f_distance <= inner_radius ? sf::Color(128, 64, 196) : sf::Color(255-128, 255-64, 255-196));
//                                    }
//                                }
//                                else if (b_pt1_inside_circle && b_pt2_inside_circle)
//                                {
//                                    float   f_signed_distance,
//                                            f_distance,
//                                            d,
//                                            a;
//                                    vec2    y, x, P_intersection_0, P_intersection_1;
//
//                                    // Projection de l'edge sur le cercle de lumière
//                                    // sommet 1: E1
//                                    f_signed_distance = signed_distance_point_line( pos_light, test_point, IterWall->pt1 );
//                                    f_distance = abs(f_signed_distance);
//                                    d = f_signed_distance / inner_radius;
//                                    a = acos(d);
//                                    y = NORMALIZE(IterWall->pt1-test_point);
//                                    x = NORMAL(y);
//                                    P_intersection_0 = pos_light + x*d*inner_radius + y*sinf(a)*inner_radius;
//                                    // sommet 2: E2
//                                    f_signed_distance = signed_distance_point_line( pos_light, test_point, IterWall->pt2 );
//                                    f_distance = abs(f_signed_distance);
//                                    d = f_signed_distance / inner_radius;
//                                    a = acos(d);
//                                    y = NORMALIZE(IterWall->pt2-test_point);
//                                    x = NORMAL(y);
//                                    P_intersection_1 = pos_light + x*d*inner_radius + y*sinf(a)*inner_radius;
//
//                                    // On calcul de l'aire du bout de disque dans la zone d'occultation
//                                    f_distance = distance_point_line( pos_light, P_intersection_0, P_intersection_1 );
//                                    d = f_distance / inner_radius;
//                                    float A = (1 - d) / 2;
//
//                                    float inv_disc_area = 1.f / (M_PI*inner_radius*inner_radius);
//                                    // Calcul de l'aire des triangles dans la zone d'occultation
//                                    A += area_triangle( IterWall->pt1, IterWall->pt2, P_intersection_0 )    * inv_disc_area;
//                                    A += area_triangle( IterWall->pt2, P_intersection_0, P_intersection_1 ) * inv_disc_area;
//
//                                    // Calcul de l'aire de visibilité de la source de lumière
//                                    A = 1 - A;
//
//                                    ADD_DEBUG_LINE(m_shape_tests_intersections, vec2(0, 0), test_point, test_point + (IterWall->pt1-test_point)*100.0f, 1, f_signed_distance > 0 ? sf::Color(128, 0, 0) : sf::Color(0, 128, 0));
//                                    ADD_DEBUG_LINE(m_shape_tests_intersections, vec2(0, 0), test_point, test_point + (IterWall->pt2-test_point)*100.0f, 1, f_signed_distance > 0 ? sf::Color(128, 0, 0) : sf::Color(0, 128, 0));
//                                    ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), P_intersection_0, 5, sf::Color(255-128, 255-64, 255-196));
//                                    ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), P_intersection_1, 5, sf::Color(255-128, 255-64, 255-196));
//                                    ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), IterWall->pt1, 5, sf::Color(255-128, 255-64, 255-196));
//                                    ADD_DEBUG_CIRCLE(m_shape_tests_intersections, vec2(0, 0), IterWall->pt2, 5, sf::Color(255-128, 255-64, 255-196));
//                                    ADD_DEBUG_TEXT(m_shape_text, (IterWall->pt1+IterWall->pt2+P_intersection_0+P_intersection_1)*0.25f, "A", A, sf::Color(0, 255, 255));
//                                }
//                            #endif
//                        }
//                        #endif
//                    }
//                }
//            }
//        }
        */
    }
}

void Light_SV::Generate_With_Projection_At_Infinity(std::vector<Wall> &m_wall)
{
    // On vide la mémoire
    m_shape_sv.clear();

    if (!m_b_is_construct)
        Construct();

    UpdatePosition();

    const float f_square_radius_light=m_influence_radius*m_influence_radius;
    const sf::Color color_sv(0, 0, 255, 255);

    // On boucle sur tous les murs
    for(std::vector<Wall>::iterator IterWall=m_wall.begin();IterWall!=m_wall.end();++IterWall)
    {
        // - l1,l2: positions relatives des deux extrémités du mur (origine: centre de la source de lumière)
        sf::Vector2f l1(SUB(IterWall->pt1, m_position));
        sf::Vector2f l2(SUB(IterWall->pt2, m_position));

        // Test d'intersection entres les BBox du cercle et du segment/wall
        if (Intersect_BoundingBox(l1, l2, sf::Vector2f(-m_influence_radius,-m_influence_radius), sf::Vector2f(m_influence_radius,+m_influence_radius)))
        {
            // (1-b)  résolution du système quadratique pour trouver les intersections de
            //      la droite support du mur et le cercle à l'origine
            // - si intersection (la ligne support avec le cercle de lumière)
            if (Quadratic_Equation_Has_Solutions(l1, SUB(l2,l1), f_square_radius_light))
            {
                sf::Vector2f intersections_segment_circle[2]={l1, l2};
                // - projection du segment par rapport au centre de la source de lumière (projection "naive")
                // - faudrait mettre en place une matrice de projection à l'infini (projective matrice 4x4 avec composante homogène (.w=0 pour décrire une ligne fuyant à l'infini))
                // - ajout dans la shape SV des quads (sv) ainsi générés
                m_shape_sv.push_back(sf::Shape());
                m_shape_sv.back().AddPoint( intersections_segment_circle[0],                    color_sv, WHITE);
                m_shape_sv.back().AddPoint( intersections_segment_circle[0]*f_coef_projection,  color_sv, WHITE);
                m_shape_sv.back().AddPoint( intersections_segment_circle[1]*f_coef_projection,  color_sv, WHITE);
                m_shape_sv.back().AddPoint( intersections_segment_circle[1],                    color_sv, WHITE);
                // On regle le shape sans blending et on fournit la position
                m_shape_sv.back().SetBlendMode(sf::Blend::None);
                m_shape_sv.back().SetPosition(m_position);
            }
        }
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

// Entrées: Sommets clippés (par le cercle* d'influence), source de lumière (position, rayon)
// Sorties: Shape des volumes englobant (outer/innter penumbra)
void Light_SV::Generate_Shapes_For_SSV(
                                       const sf::Vector2f intersections_segment_circle[2], const sf::Vector2f proj_intersections[2], const sf::Vector2f &l1_to_l2,
                                       const sf::Vector2f &pos_light, const float inner_radius, const float influence_radius, const float f_square_influence_radius_light,
                                       Light_Wall &light_walls
                                       )
{
    sf::Vector2f intersections_dir[2];
    sf::Vector2f intersections_normals[2];
    sf::Vector2f penumbra_proj_centers[2][2];
    sf::Vector2f proj_intersections_penumbra[2][2];
    Typ_Solutions_Quadratic_Equation solutions[2][2];
    sf::Vector2f isc_ppc[2][2];
    sf::Vector2f isc_ppc_normal[2][2]; // pas unitaire, juste la direction de la normale
    sf::Vector2f bounding_vertex_penumbra[2][2];


    // - vecteur Z orienté par rapport au sens de [l1,l2]
    // - ce vecteur sert pour la suite à orienter les volumes inner/outer de pénombre
    sf::Vector3f z = PROD_VEC(TO_VEC3(intersections_segment_circle[0]), TO_VEC3(l1_to_l2));
    z.z = z.z<0?-1:1;

    // Centre et rayon du 1er cercle: cercle de lumière
    sf::Vector2f O0 = sf::Vector2f(0, 0);
    float R0 = inner_radius;

    // On boucle sur: les deux sommets formant le segment/wall clippé
    for(int i=0; i<2; ++i)
    {
        sf::Vector2f intersections_circles[2];
        {
            // Centre et rayon du 2nd cercle:
            //  O1: Milieu du segment reliant un des points clippé du segment/wall et le centre de la source de lumière
            //  Distance de O1 (par rapport au centre du cercle d'influence)
            sf::Vector2f O1 = (O0 + intersections_segment_circle[i]) * 0.5f;
            float R1 = NORM(O1 - O0);
            // On calcul les intersections des cercles qui sont les centres de projection (non orientés, pour l'instant) pour les volumes outer/inner penumbra
            Compute_Intersection_Circles(O0, R0, O1, R1, intersections_circles);
            // Vecteur directeur de ces centres de projection
            intersections_dir[i] = intersections_segment_circle[i] - O0;

            #ifdef __DRAW_DEBUG__
                #ifdef __DRAW_DEBUG_INTERSECTIONS_SSV__
//                    ADD_DEBUG_CIRCLE_OUTLINE(m_shape_intersections_ssv, m_position, O0, R0, sf::Color(196, 32, 64), 1);
//                    ADD_DEBUG_CIRCLE_OUTLINE(m_shape_intersections_ssv, m_position, O0, 3,  sf::Color(255-196, 255-32, 255-64), 1);
//                    ADD_DEBUG_CIRCLE_OUTLINE(m_shape_intersections_ssv, m_position, O1, R1, sf::Color(196, 32, 64), 1);
//                    ADD_DEBUG_CIRCLE_OUTLINE(m_shape_intersections_ssv, m_position, O1, 3,  sf::Color(255-196, 255-32, 255-64), 1);
                #endif
            #endif
        }

        // On calcul la normale (orientée) du vecteur directeur
        sf::Vector3f normal         = PROD_VEC(z, TO_VEC3(intersections_dir[i]));
        intersections_normals[i]    = TO_VEC2(normal);

        // On boucle sur:
        for(int j=0; j<2; ++j)
        {
            // (1-a) Calcul les centres de projections pour les zones de pénombre
            {
                // on détermine l'orientation du segment pour être sure de caractériser les zones: outer et inner distinctement
                float f_sign_orientation        = j ? -1.f : +1.f;
                sf::Vector2f oriented_normal    = f_sign_orientation*intersections_normals[i];
                float orientation               = DOT(oriented_normal, intersections_circles[j]);
                penumbra_proj_centers[i][j]     = orientation >= 0 ? intersections_circles[j]:intersections_circles[1-j];
            }
            // (1-b) vecteur directeur reliant
            //          un des sommets clippés (par le cercle d'influence) de l'arête
            //          un des centres de projection (caractérisant les volumes de pénombres)
            isc_ppc[i][j]                       = (intersections_segment_circle[i] - penumbra_proj_centers[i][j]);
            // (1-c) Normal du segment orienté isc_ppc
            isc_ppc_normal[i][j]                = NORMAL(isc_ppc[i][j]);

            // (2) Résolution d'un système d'équation quadratique pour retrouver l'intersection de
            //      la ligne (orientée) support du segment isc_ppc
            //      et le cercle d'influence de la lumière
            solutions[i][j]                     = Solve_Quadratic_Equation(penumbra_proj_centers[i][j], isc_ppc[i][j], f_square_influence_radius_light);
            // On récupère la solution max (qui correspond à projection orientée qui nous intéresse)
            // et on l'utilise pour retrouver le point d'intersection
            proj_intersections_penumbra[i][j]   = penumbra_proj_centers[i][j] + isc_ppc[i][j]*(float)(MAX(solutions[i][j].f_u0, solutions[i][j].f_u1));

            // (3) Calcul d'un des sommet englobant les volumes de pénombres (outer/inner)
            //      Ce sommet correspond à l'intersection des lignes
            //          possèdant le sommet (2) et de normale (1-c)
            //          possèdant la projection d'un sommet de l'arête et la normale qui est tangente au cercle en ce point
            bounding_vertex_penumbra[i][j]      = Compute_Intersection_Lines(
                                                    proj_intersections_penumbra[i][j],  proj_intersections_penumbra[i][j]+isc_ppc_normal[i][j],
                                                    proj_intersections[i],              proj_intersections[i]+intersections_normals[i]
                                                    );

            // (4) Construct Shape: outer ou inner penumbra (volume englobant)
            sf::Shape shape;
//            shape.AddPoint( intersections_segment_circle[i],    color_sv, sf::Color(255, 0, 255, 255) );
//            shape.AddPoint( proj_intersections_penumbra[i][j],  (j==0)?color_outer_ssv:color_inner_ssv, sf::Color(255, 0, 255, 255) );
//            shape.AddPoint( bounding_vertex_penumbra[i][j],     (j==0)?color_outer_ssv:color_inner_ssv, sf::Color(255, 0, 255, 255) );
//            shape.AddPoint( proj_intersections[i],              color_sv, sf::Color(255, 0, 255, 255) );
            sf::Color shape_color = (i+j)%2 ? color_inner_ssv : color_outer_ssv;
            shape.AddPoint( intersections_segment_circle[i],    shape_color, shape_color );
            shape.AddPoint( proj_intersections_penumbra[i][j],  shape_color, shape_color );
            shape.AddPoint( bounding_vertex_penumbra[i][j],     shape_color, shape_color );
            shape.AddPoint( proj_intersections[i],              shape_color, shape_color );
            shape.SetPosition(pos_light);
            shape.SetBlendMode(sf::Blend::None);
            (j+i)%2 ? m_shape_inner_ssv.push_back(shape) : m_shape_outer_ssv.push_back(shape);
//            light_walls.Add(shape);
        }
    }

    #ifdef __DRAW_DEBUG__
        #ifdef __DRAW_DEBUG_INTERSECTIONS_SSV__
            // DEBUG Soft Shadow Volume
            for(int i=0; i<2; ++i)
            {
                for(int j=0; j<2; ++j)
                {
//                    ADD_DEBUG_CIRCLE(m_shape_intersections_ssv, pos_light, penumbra_proj_centers[i][j], CLAMP(m_inner_radius/5.f, 2, 5), sf::Color(255, 64, 255));
//
//                    ADD_DEBUG_LINE(m_shape_intersections_ssv, pos_light, intersections_segment_circle[i], (intersections_segment_circle[i]-penumbra_proj_centers[i][j])*f_coef_projection, 1, sf::Color(128, 16, 16));
//                    ADD_DEBUG_LINE(m_shape_intersections_ssv, pos_light, intersections_segment_circle[i], proj_intersections_penumbra[i][j], 2, sf::Color(255, 32, 32));
//                    ADD_DEBUG_LINE(m_shape_intersections_ssv, pos_light, intersections_segment_circle[i], penumbra_proj_centers[i][j], 1, sf::Color(196, 64, 32));

//                    ADD_DEBUG_LINE(m_shape_intersections_ssv, pos_light, proj_intersections_penumbra[i][j],  proj_intersections_penumbra[i][j]+isc_ppc_normal[i][j], 2, sf::Color(64, 196, 96));
//                    ADD_DEBUG_LINE(m_shape_intersections_ssv, pos_light, proj_intersections[i], proj_intersections[i]+intersections_normals[i], 2, sf::Color(64, 196, 96));
//
//                    ADD_DEBUG_CIRCLE_OUTLINE(m_shape_intersections_ssv, pos_light, intersections_segment_circle[i],     3, sf::Color(16, 16, 255), 2);
//                    ADD_DEBUG_CIRCLE_OUTLINE(m_shape_intersections_ssv, pos_light, proj_intersections_penumbra[i][j],   3, sf::Color(16, 16, 255), 2);
//                    ADD_DEBUG_CIRCLE_OUTLINE(m_shape_intersections_ssv, pos_light, bounding_vertex_penumbra[i][j],      3, sf::Color(16, 16, 255), 2);
//                    ADD_DEBUG_CIRCLE_OUTLINE(m_shape_intersections_ssv, pos_light, proj_intersections[i],               3, sf::Color(16, 16, 255), 2);
                }
//                ADD_DEBUG_CIRCLE(m_shape_intersections_ssv, pos_light, proj_intersections[i], 3, WHITE);
//                ADD_DEBUG_CIRCLE(m_shape_intersections_ssv, pos_light, intersections_segment_circle[i], 3, WHITE);
            }

//            ADD_DEBUG_CIRCLE(           m_shape_intersections_ssv, pos_light, sf::Vector2f(0, 0), m_inner_radius,   sf::Color(64,128,196));
            ADD_DEBUG_CIRCLE_OUTLINE(   m_shape_intersections_ssv, pos_light, sf::Vector2f(0, 0), inner_radius, sf::Color(64,128,196), 2.f);
//            ADD_DEBUG_CIRCLE_OUTLINE(   m_shape_intersections_ssv, pos_light, sf::Vector2f(0, 0), influence_radius,       sf::Color(64,128,196), 2.f);
        #endif
    #endif
}
