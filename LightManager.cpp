
#include "LightManager.h"

#include <GL/gl.h>

//#define __USE_BLUR__

extern int i_screen_width, i_screen_height;

Light_Manager::Light_Manager()
{
    m_lightSmooth=0;

    // Chargement du postFX pour adoucir les bords.
    //BlurEffect.LoadFromFile("data/shaders/blurFX.sfx");
    //BlurEffect.LoadFromFile("data/shaders/blur.sfx");
    BlurEffect.LoadFromFile("data/shaders/blur_vs.sfx", "data/shaders/blur.sfx");


//    BlurEffect.SetTexture("texture", sf::Shader::CurrentTexture);
    std::cout << "Load Shader: data/shaders/render_bg_ss.glsl" << std::endl;
    RenderBackGroundWithSoftShadowEffect.LoadFromFile("data/shaders/render_bg_ss.glsl");

    std::cout << "Load Shader: data/shaders/light_2d_ssv.glsl, data/shaders/light_2d_ssv_inner.glsl" << std::endl;
    SoftShadowInnerEffect.LoadFromFile("data/shaders/light_2d_ssv.glsl", "data/shaders/light_2d_ssv_inner.glsl");

    std::cout << "Load Shader: data/shaders/light_2d_ssv.glsl, data/shaders/light_2d_ssv_outer.glsl" << std::endl;
    SoftShadowOuterEffect.LoadFromFile("data/shaders/light_2d_ssv.glsl", "data/shaders/light_2d_ssv_outer.glsl");

    std::cout << "Load Shader: data/shaders/light_2d_ssv.glsl, data/shaders/light_2d.glsl" << std::endl;
    DiscLightingEffect.LoadFromFile("data/shaders/light_2d_ssv.glsl", "data/shaders/light_2d.glsl");

    bool depthBuffer = true;
    bool stencilBuffer = true; // For Shadow Volume

    // MOG
    m_renderImg.Create(i_screen_width,i_screen_height, depthBuffer, stencilBuffer);
}

Light_Manager::~Light_Manager()
{
    for(unsigned i = 0 ; i < m_DynamicLight.size() ; ++i)
        delete m_DynamicLight[i];

    for(unsigned i = 0 ; i < m_StaticLight.size() ; ++i)
        delete m_StaticLight[i];

    m_DynamicLight.clear();
    m_StaticLight.clear();
}

// Les différents moyens d'ajouter des lumières

Light_Entity Light_Manager::Add_Dynamic_Light()
{
    m_DynamicLight.push_back(new Light());
    return Light_Entity((int)m_DynamicLight.size()-1,true);
}
Light_Entity Light_Manager::Add_Dynamic_Light(Light light)
{
    m_DynamicLight.push_back(new Light(light));
    return Light_Entity((int)m_DynamicLight.size()-1,true);
}
Light_Entity Light_Manager::Add_Dynamic_Light(sf::Vector2f position, float intensity, float radius, int quality, sf::Color color)
{
    m_DynamicLight.push_back(new Light (position,intensity,radius,quality,color));
    return Light_Entity((int)m_DynamicLight.size()-1,true);
}

// Les différents moyens d'ajouter des lumières directionelles

Light_Entity Light_Manager::Add_Dynamic_Directional_Light()
{
    m_DynamicLight.push_back(new Directional_light ());
    return Light_Entity((int)m_DynamicLight.size()-1,true);
}
Light_Entity Light_Manager::Add_Dynamic_Directional_Light(Directional_light light)
{
    m_DynamicLight.push_back(new Directional_light(light));
    return Light_Entity((int)m_DynamicLight.size()-1,true);
}
Light_Entity Light_Manager::Add_Dynamic_Directional_Light(sf::Vector2f position, float intensity, float radius, float angle, float o_angle, sf::Color color)
{
    m_DynamicLight.push_back(new Directional_light (position,intensity,radius,angle, o_angle,color));
    return Light_Entity((int)m_DynamicLight.size()-1,true);
}

Light_Entity Light_Manager::Add_Dynamic_Light_SV()
{
    m_DynamicLight.push_back(new Light_SV());
    return Light_Entity((int)m_DynamicLight.size()-1,true);
}

Light_Entity Light_Manager::Add_Dynamic_Light_SV(Light_SV light)
{
    m_DynamicLight.push_back(new Light_SV(light));
    return Light_Entity((int)m_DynamicLight.size()-1,true);
}

Light_Entity Light_Manager::Add_Dynamic_Light_SV(sf::Vector2f position, float intensity, float radius, int quality, sf::Color color)
{
    m_DynamicLight.push_back(new Light_SV (position,intensity,radius,quality,color));
    return Light_Entity((int)m_DynamicLight.size()-1,true);
}

// Notez bien qu'on calcule les lumières statiques lors de leur ajout.

Light_Entity Light_Manager::Add_Static_Light(Light light)
{
    m_StaticLight.push_back(new Light(light));
    m_StaticLight.back()->Generate(m_wall);
    return Light_Entity((int)m_StaticLight.size()-1,false);
}
Light_Entity Light_Manager::Add_Static_Light(sf::Vector2f position, float intensity, float radius, int quality, sf::Color color)
{
    m_StaticLight.push_back(new Light (position,intensity,radius,quality,color));
    m_StaticLight.back()->Generate(m_wall);
    return Light_Entity((int)m_StaticLight.size()-1,false);
}

// Notez bien qu'on calcule les lumières statiques lors de leur ajout.

Light_Entity Light_Manager::Add_Static_Directional_Light(Directional_light light)
{
    m_StaticLight.push_back(new Directional_light(light));
    m_StaticLight.back()->Generate(m_wall);
    return Light_Entity((int)m_StaticLight.size()-1,false);
}
Light_Entity Light_Manager::Add_Static_Directional_Light(sf::Vector2f position, float intensity, float radius, float angle, float o_angle, sf::Color color)
{
    m_StaticLight.push_back(new Directional_light (position,intensity,radius,angle,o_angle,color));
    m_StaticLight.back()->Generate(m_wall);
    return Light_Entity((int)m_StaticLight.size()-1,false);
}

// Ajouter un mur

Wall_Entity Light_Manager::Add_Wall(sf::Vector2f pt1,sf::Vector2f pt2)
{
//    const float f_delta_wall_to_add = (pt1.y-pt2.y)/(pt1.x-pt2.y);
    const float f_delta_wall_to_add = (pt1.y-pt2.y)/(pt1.x-pt2.x); // pente du segment
    for(int i=0;i<(int)m_wall.size();i++) {
//        float f_delta_current_wall = (m_wall[i].pt1.y-m_wall[i].pt2.y)/(m_wall[i].pt1.x-m_wall[i].pt2.y);
        float f_delta_current_wall = (m_wall[i].pt1.y-m_wall[i].pt2.y)/(m_wall[i].pt1.x-m_wall[i].pt2.x);
        // Meme pente ?
        if(f_delta_wall_to_add == f_delta_current_wall) {
            // Points en commun ?
            if(pt1==m_wall[i].pt1 || pt2==m_wall[i].pt1 || pt1==m_wall[i].pt2 || pt2==m_wall[i].pt2) {
                // Si oui on reconstruit le segment
                sf::Vector2f min=pt1,max=pt2;

                if(pt2.x<min.x)
                    min.x=pt2.x;
                if(pt1.x>max.x)
                    max.x=pt2.x;

                if(m_wall[i].pt1.x<min.x)
                    min.x=m_wall[i].pt1.x;
                if(m_wall[i].pt2.x<min.x)
                    min.x=m_wall[i].pt2.x;

                if(m_wall[i].pt1.x>max.x)
                    max.x=m_wall[i].pt1.x;
                if(m_wall[i].pt2.x>max.x)
                    max.x=m_wall[i].pt2.x;


                if(pt2.y<min.y)
                    min.y=pt2.y;
                if(pt1.y>max.y)
                    max.y=pt2.y;

                if(m_wall[i].pt1.y<min.y)
                    min.y=m_wall[i].pt1.y;
                if(m_wall[i].pt2.y<min.y)
                    min.y=m_wall[i].pt2.y;

                if(m_wall[i].pt1.y>max.y)
                    max.y=m_wall[i].pt1.y;
                if(m_wall[i].pt2.y>max.y)
                    max.y=m_wall[i].pt2.y;

                m_wall[i].pt1=min;
                m_wall[i].pt2=max;

                return Wall_Entity(i);
            }
        }
    }
    m_wall.push_back(Wall (pt1,pt2));
    return Wall_Entity(m_wall.size()-1);
}

// On désactive une lumière
void Light_Manager::Delete_Light(Light_Entity e)
{
    if(e.Dynamic())
    {
        if(e.Dynamic())
        {
            if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
                m_DynamicLight[e.ID()]->m_actif = false;
        }
        else
        {
            if(e.ID()>=0&&e.ID()<(int)m_StaticLight.size())
                m_StaticLight[e.ID()]->m_actif  = false;
        }
    }
}

// On supprime un mur
void Light_Manager::Delete_Wall(Wall_Entity e)
{
    m_wall.erase(m_wall.begin()+e.ID());
}

void Light_Manager::Delete_All_Wall()
{
    m_wall.clear();
}
void Light_Manager::Delete_All_Light()
{
    for(unsigned i = 0 ; i < m_DynamicLight.size() ; ++i)
        delete m_DynamicLight[i];

    for(unsigned i = 0 ; i < m_StaticLight.size() ; ++i)
        delete m_StaticLight[i];

    m_DynamicLight.clear();
    m_StaticLight.clear();
}

// On calcule toutes les lumières dynamiques actives
void Light_Manager::Generate()
{
    m_renderImg.Clear(m_basicLight);

    //BlurEffect.SetParameter("offset",0.005 * m_lightSmooth);
    // Texture Space Coordinates: [0, 1]²
    // Image space Coordinates: [0, width-1]x[0, height-1]
    BlurEffect.SetParameter("offsetX", (float)(m_lightSmooth)/m_renderImg.GetWidth());
    BlurEffect.SetParameter("offsetY", (float)(m_lightSmooth)/m_renderImg.GetHeight());

    for(Iter=m_DynamicLight.begin();Iter!=m_DynamicLight.end();++Iter)
    {
        Light* ptr_light = (*Iter);
        if(ptr_light->m_actif)
        {
            ptr_light->Generate(m_wall);
            ptr_light->Draw(&m_renderImg);
        }
    }

    for(Iter=m_StaticLight.begin();Iter!=m_StaticLight.end();++Iter)
        if((*Iter)->m_actif)
            (*Iter)->Draw(&m_renderImg);

    m_renderImg.Display();
}


void Light_Manager::Generate(Light_Entity e)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            m_DynamicLight[e.ID()]->Generate(m_wall);
}

// On crée une image avec toutes les lumières actives
void Light_Manager::Draw(sf::RenderWindow *App)
{
    sf::Sprite sprite;
    sprite.SetImage(m_renderImg.GetImage());
    sprite.SetBlendMode(sf::Blend::Multiply); // [MOG] with background
    //sprite.SetBlendMode(sf::Blend::None); // [MOG]
    #ifdef __USE_BLUR__
        App->Draw(sprite, BlurEffect);
    #else
        const sf::Vector3f  v_ambiant_color = sf::Vector3f(m_basicLight.r, m_basicLight.g, m_basicLight.b) / 255.f;
        RenderBackGroundWithSoftShadowEffect.SetParameter("u_v_ambiant_color",          v_ambiant_color);
        App->Draw(sprite, RenderBackGroundWithSoftShadowEffect);
    #endif
}

// On modifie ou récupère les attributs de lumières dynamiques

void Light_Manager::SetPosition(Light_Entity e, sf::Vector2f p)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            m_DynamicLight[e.ID()]->SetPosition(p);
}
void Light_Manager::SetQuality(Light_Entity e, int q)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            m_DynamicLight[e.ID()]->SetQuality(q);
}
void Light_Manager::SetRadius(Light_Entity e, int r)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            m_DynamicLight[e.ID()]->SetRadius(r);
}
void Light_Manager::SetColor(Light_Entity e, sf::Color c)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            m_DynamicLight[e.ID()]->SetColor(c);
}
void Light_Manager::SetIntensity(Light_Entity e, int i)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            m_DynamicLight[e.ID()]->SetIntensity(i);
}

void Light_Manager::SetOtherParameter(Light_Entity e, unsigned no, float v)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            m_DynamicLight[e.ID()]->SetOtherParameter(no, v);
}

void Light_Manager::SetEffects_For_Light_SV(Light_Entity e)
{
//    if(e.Dynamic())
//    {
//        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
//            static_cast<Light_SV*>(m_DynamicLight[e.ID()])->SetEffects(SoftShadowInnerEffect, SoftShadowOuterEffect, DiscLightingEffect);
//    }
//    else
//    {
//        if(e.ID()>=0&&e.ID()<(int)m_StaticLight.size())
//            static_cast<Light_SV*>(m_StaticLight[e.ID()])->SetEffects(SoftShadowInnerEffect, SoftShadowOuterEffect, DiscLightingEffect);
//    }
}

float Light_Manager::GetIntensity(Light_Entity e)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            return m_DynamicLight[e.ID()]->GetIntensity();
    return 0;
}
float Light_Manager::GetRadius(Light_Entity e)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            return m_DynamicLight[e.ID()]->GetRadius();
    return 0;
}
int Light_Manager::GetQuality(Light_Entity e)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            return m_DynamicLight[e.ID()]->GetQuality();

    return 0;
}
sf::Color Light_Manager::GetColor(Light_Entity e)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            return m_DynamicLight[e.ID()]->GetColor();

    return sf::Color (0,0,0);
}
sf::Vector2f Light_Manager::GetPosition(Light_Entity e)
{
    if(e.Dynamic())
        if(e.ID()>=0&&e.ID()<(int)m_DynamicLight.size())
            return m_DynamicLight[e.ID()]->GetPosition();

    return sf::Vector2f (0,0);
}



// On modifie ou récupère différents attributs des murs

void Light_Manager::SetPosition(Wall_Entity e, sf::Vector2f p)
{
    if(e.ID()>=0&&e.ID()<(int)m_wall.size())
    {
        sf::Vector2f buffer=m_wall[e.ID()].pt1;
        m_wall[e.ID()].pt1=p;
        m_wall[e.ID()].pt2.x= m_wall[e.ID()].pt2.x + (p.x-buffer.x);
        m_wall[e.ID()].pt2.y= m_wall[e.ID()].pt2.y + (p.y-buffer.y);
    }
}


