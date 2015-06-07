#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#ifndef LIGHTH
#define LIGHTH

struct Wall
{
    Wall (sf::Vector2f p1,sf::Vector2f p2)
    {
        pt1=p1;
        pt2=p2;
    }

    // Pt1 et Pt2 sont les deux extrÃÂ©mitÃÂ©s du mur
    sf::Vector2f pt1;
    sf::Vector2f pt2;
};

// Wall_Entity est une variable qui permet de reprÃÂ©senter dans le programme un mur
struct Wall_Entity
{
    Wall_Entity (int id)
    {
        m_ID=id;
    }

    int ID() { return m_ID; }

    private:

    int m_ID;
};


// Light_Entity est une variable qui permet de reprÃÂ©senter dans le programme une lumiÃÂ¨re
struct Light_Entity
{
    Light_Entity (){m_Dynamic=false,m_ID=0;}
    Light_Entity (int id,bool d)
    {
        m_ID=id;
        m_Dynamic=d;
    }

    int ID() { return m_ID; }
    bool Dynamic() { return m_Dynamic; }

    private:

    int m_ID;
    bool m_Dynamic;
};

class Light
{
    public :

    // Constructeur et destructeur
    Light();
    Light(sf::Vector2f position, float intensity, float radius, int quality, sf::Color color);
    ~Light();

    // Afficher la lumiÃÂ¨re
    void Draw(sf::RenderTarget *App);

    // Calculer la lumiÃÂ¨re
    virtual void Generate(std::vector <Wall> &m_wall);

    // Ajouter un triangle ÃÂ  la lumiÃÂ¨re, en effet, les lumiÃÂ¨res sont composÃÂ©e de triangles
    void AddTriangle(sf::Vector2f pt1,sf::Vector2f pt2, int minimum_wall,std::vector <Wall> &m_wall);

    // Changer diffÃÂ©rents attributs de la lumiÃÂ¨re
    void SetIntensity(float);
    void SetRadius(float);
    void SetQuality(int);
    void SetColor(sf::Color);
    void SetPosition(sf::Vector2f);

    virtual void SetOtherParameter(unsigned, float);


    // Retourner diffÃÂ©rents attributs de la lumiÃÂ¨re
    float GetIntensity();
    float GetRadius();
    int GetQuality();
    sf::Color GetColor();
    sf::Vector2f GetPosition();

    // Une petite bool pour savoir si la lumiÃÂ¨re est allumÃÂ©e ou ÃÂ©teinte
    bool m_actif;

    protected :
    //Position ÃÂ  l'ÃÂ©cran
    sf::Vector2f m_position;
    //IntensitÃÂ©, gÃÂ¨re la transparence ( entre 0 et 255 )
    float m_intensity;
    //Rayon de la lumiÃÂ¨re
    float m_influence_radius;
    //Couleur de la lumiÃÂ¨re
    sf::Color m_color;


    //Tableau dynamique de Shape, ce sont ces shapes de type triangle qui compose la lumiÃÂ¨re
    std::vector <sf::Shape> m_shape;

    private :

    //QualitÃÂ© de la lumiÃÂ¨re, c'est ÃÂ  dire le nombre de triangles par dÃÂ©faut qui la compose.
    int m_quality;
};

#endif

