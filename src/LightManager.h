


#ifndef LIGHTMANAGERH
#define LIGHTMANAGERH

#include "Singleton.h"

#include "Directional_light.h"

#include "Light_SV.h"

class Light_Manager : public CSingleton<Light_Manager>
{
    protected :

    Light_Manager();
    ~Light_Manager();

     // Les tableaux de murs, lumieres statiques et dynamiques
    std::vector <Wall> m_wall;
    std::vector <Light*> m_StaticLight;
    std::vector <Light*> m_DynamicLight;

    public :
    // Constructeur et destructeur
    friend Light_Manager* CSingleton<Light_Manager>::GetInstance();
    friend void CSingleton<Light_Manager>::Kill();


    //Differents moyen d'ajouter une lumiere dynamique, soit on l'ajoute sans aucune valeur par defaut, soit on lui donne une lumiere par defaut, soit on lui donne ses valeurs "a  la main"
    Light_Entity Add_Dynamic_Light();
    Light_Entity Add_Dynamic_Light(Light);
    Light_Entity Add_Dynamic_Light(sf::Vector2f position, float intensity, float radius, int quality, sf::Color color);

    Light_Entity Add_Dynamic_Directional_Light();
    Light_Entity Add_Dynamic_Directional_Light(Directional_light);
    Light_Entity Add_Dynamic_Directional_Light(sf::Vector2f position, float intensity, float radius, float angle, float o_angle, sf::Color color);

    Light_Entity Add_Dynamic_Light_SV();
    Light_Entity Add_Dynamic_Light_SV(Light_SV);
    Light_Entity Add_Dynamic_Light_SV(sf::Vector2f position, float intensity, float radius, int quality, sf::Color color);

    Light_Entity Add_Static_Light(Light);
    Light_Entity Add_Static_Light(sf::Vector2f position, float intensity, float radius, int quality, sf::Color color);

    Light_Entity Add_Static_Directional_Light(Directional_light);
    Light_Entity Add_Static_Directional_Light(sf::Vector2f position, float intensity, float radius, float angle, float o_angle, sf::Color color);

    // Ajouter un mur
    Wall_Entity Add_Wall(sf::Vector2f pt1,sf::Vector2f pt2);

    // Desactiver une lumiere ou supprimer un mur
    void Delete_Light(Light_Entity);
    void Delete_Wall(Wall_Entity);

    void Delete_All_Wall();
    void Delete_All_Light();

    // Calculer toutes les lumieres dynamiques
    void Generate();
    void Generate(Light_Entity);

    // Afficher toutes les lumieres a  l'ecran
    void Draw(sf::RenderWindow *App);

    // Differentes methodes pour modifier les attributs d'une lumiere, ou les recuperer. Il faut a  chaque fois envoyer une Light_Entity en parametre pour
    // savoir de quelle lumiere on parle/

    void SetPosition(Light_Entity, sf::Vector2f );
    void SetQuality(Light_Entity, int );
    void SetRadius(Light_Entity, int );
    void SetColor(Light_Entity, sf::Color );
    void SetIntensity(Light_Entity, int);

    void SetOtherParameter(Light_Entity , unsigned, float);

    float GetIntensity(Light_Entity);
    float GetRadius(Light_Entity);
    int GetQuality(Light_Entity);
    sf::Color GetColor(Light_Entity);
    sf::Vector2f GetPosition(Light_Entity);

    void SetPosition(Wall_Entity, sf::Vector2f );

    sf::Color m_basicLight;
    int m_lightSmooth;

    sf::Shader GetSoftShadowInnerEffect()   const { return SoftShadowInnerEffect; }
    sf::Shader GetSoftShadowOuterEffect()   const { return SoftShadowOuterEffect; }
    sf::Shader GetDiscLightingEffect()      const { return DiscLightingEffect; }

    void SetEffects_For_Light_SV(Light_Entity);

    private:

    sf::Shader BlurEffect;
    sf::Shader RenderBackGroundWithSoftShadowEffect;
    //
    sf::Shader SoftShadowInnerEffect;
    sf::Shader SoftShadowOuterEffect;
    sf::Shader DiscLightingEffect;

    std::vector<Light*>::iterator Iter;
    sf::RenderImage m_renderImg;
};
#endif

