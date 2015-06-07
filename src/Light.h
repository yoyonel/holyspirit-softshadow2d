#ifndef LIGHTH
#define LIGHTH

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

struct Wall
{
    Wall (sf::Vector2f p1,sf::Vector2f p2)
    {
        pt1=p1;
        pt2=p2;
    }

    // Pt1 et Pt2 sont les deux extremites du mur
    sf::Vector2f pt1;
    sf::Vector2f pt2;
};

// Wall_Entity est une variable qui permet de representer dans le programme un mur
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


// Light_Entity est une variable qui permet de representer dans le programme une lumiere
struct Light_Entity
{
    Light_Entity (){m_Dynamic=false,m_ID=0; m_SoftShadow=false; m_NormalMap=false;}
    Light_Entity (int id,bool d, bool ss=false, bool nm=false)
    {
        m_ID=id;
        m_Dynamic=d;
        m_SoftShadow=ss;
        m_NormalMap=nm;
    }

    int     ID()            const { return m_ID; }
    bool    Dynamic()       const { return m_Dynamic; }
    bool    SoftShadow()    const { return m_SoftShadow; }
    bool    NormalMap()     const { return m_NormalMap; }

    private:

    int m_ID;
    bool m_Dynamic;
    bool m_SoftShadow;
    bool m_NormalMap;
};

class Light
{
    public :

    // Constructeur et destructeur
    Light();
    Light(sf::Vector2f position, float intensity, float radius, int quality, sf::Color color);
    ~Light();

    // Afficher la lumiere
    virtual void Draw(sf::RenderTarget *App);

    // Calculer la lumiere
    virtual void Generate(std::vector <Wall> &m_wall);

    // Ajouter un triangle ÃÂ  la lumiere, en effet, les lumieres sont composee de triangles
    virtual void AddTriangle(sf::Vector2f pt1,sf::Vector2f pt2, int minimum_wall,std::vector <Wall> &m_wall);

    // Changer differents attributs de la lumiere
    void SetIntensity(float);
    void SetRadius(float);
    void SetQuality(int);
    void SetColor(sf::Color);
    void SetPosition(sf::Vector2f);

    virtual void SetOtherParameter(unsigned, float);


    // Retourner differents attributs de la lumiere
    float GetIntensity();
    float GetRadius();
    int GetQuality();
    sf::Color GetColor();
    sf::Vector2f GetPosition();

    // Une petite bool pour savoir si la lumiere est allumee ou eteinte
    bool m_actif;

    protected :
    //Position ÃÂ  l'ecran
    sf::Vector2f m_position;
    //Intensite, gere la transparence ( entre 0 et 255 )
    float m_intensity;
    //Rayon de la lumiere
    float m_influence_radius;
    //Couleur de la lumiere
    sf::Color m_color;


    //Tableau dynamique de Shape, ce sont ces shapes de type triangle qui compose la lumiere
    std::vector <sf::Shape> m_shape;

    private :

    //Qualite de la lumiere, c'est ÃÂ  dire le nombre de triangles par defaut qui la compose.
    int m_quality;
};

#endif

