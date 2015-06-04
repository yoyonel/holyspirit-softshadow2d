
#include "LightManager.h"

#include <stdio.h>
#include <GL/glew.h>
#include <GL/gl.h>

//#define     __USE_LIGHT__
#define   __USE_LIGHT_SV__
//#define __USE_1_LIGHT__         // [MOG]
//#define __USE_1_WALL__          // [MOG]

#define     SIZE_RAND_WALLS     0
//
#define     QUALITY_LIGHT       32
//
#define     SIZE_LIGHT_TYPE0    (256+128)
#define     SIZE_LIGHT_TYPE1    (128*2)
//
#define     __USE_BACKGROUND__
//
#define     __DRAW_WALLS__
//
#define DEBUG_PRINT(msg, var)           std::cout << msg << ": " << var << std::endl;

float f_ratio_screen    = 16/9.f;
//float f_ratio_screen    = 4.f/3.f;
//
int i_screen_height     = 720; // HD-720p
//int i_screen_height     = 1024; // HD-1024p
//int i_screen_height     = 600; // 800x600, 4/3
//
int i_screen_width      = (int)(i_screen_height*f_ratio_screen);

int ambient = 3;    // [MOG]

sf::Color ambient_light = sf::Color(ambient, ambient, ambient, 255);

int main()
{
    // Création de la fenêtre de rendu.
    sf::ContextSettings Settings;

    sf::RenderWindow App(sf::VideoMode(i_screen_width, i_screen_height, 32), "Holyspirit Light Test - Dynamic Light Shadow Volume 2D"); // Ecran 16/9, resolution HD 720p

    if (GLEW_OK != glewInit())
    {
        // GLEW failed!
        exit(1);
    }

    // Création de l'horloge pour gérer le temps.
    sf::Clock Clock;

    // Création d'un tableau dynamique qui contient les images des murs.
    std::vector <sf::Shape> Wall;


    // Création du Light_Manager, c'est lui qui va s'occupper de gérer les lumières.
    Light_Manager *Manager;
    Manager=Light_Manager::GetInstance();

    Manager->m_lightSmooth=3.5;
    Manager->m_basicLight=ambient_light;

    Wall.push_back(sf::Shape::Line(125, 425, 125, 475, 5, sf::Color(255,255,255, 255)));
    Wall.push_back(sf::Shape::Line(125, 475, 175, 475, 5, sf::Color(255,255,255, 255)));
    //
    Wall.push_back(sf::Shape::Line(350, 250, 450, 250, 5, sf::Color(255,255,255, 255)));
    Wall.push_back(sf::Shape::Line(350, 350, 450, 350, 5, sf::Color(255,255,255, 255)));
    Wall.push_back(sf::Shape::Line(350, 250, 350, 350, 5, sf::Color(255,255,255, 255)));
    Wall.push_back(sf::Shape::Line(450, 250, 450, 350, 5, sf::Color(255,255,255, 255)));

    Wall.push_back(sf::Shape::Line(500, 200, 600, 300, 5, sf::Color(255,255,255, 255)));
    Wall.push_back(sf::Shape::Line(500, 200, 525,185, 5, sf::Color(255,255,255, 255)));
    Wall.push_back(sf::Shape::Line(525, 185, 625, 285, 5, sf::Color(255,255,255, 255)));
    Wall.push_back(sf::Shape::Line(625, 285, 600, 300, 5, sf::Color(255,255,255, 255)));

    // On ajoute des murs au Light_Manager,
    // ce sont ces murs qui sont pris en compte lors du calcul des lumières
    // (Position du point 1, Position du point 2).
    //
    Manager->Add_Wall(sf::Vector2f(500,200),sf::Vector2f(600,300));
    Manager->Add_Wall(sf::Vector2f(500,200),sf::Vector2f(525,185));
    Manager->Add_Wall(sf::Vector2f(525,185),sf::Vector2f(625,285));
    Manager->Add_Wall(sf::Vector2f(625,285),sf::Vector2f(600,300));

    #ifndef __USE_1_WALL__
        Manager->Add_Wall(sf::Vector2f(500,300),sf::Vector2f(600,200));

        Manager->Add_Wall(sf::Vector2f(450,325),sf::Vector2f(300,200));
        Manager->Add_Wall(sf::Vector2f(100,100),sf::Vector2f(125,125));
        Manager->Add_Wall(sf::Vector2f(125,425),sf::Vector2f(125,475));
        Manager->Add_Wall(sf::Vector2f(125,475),sf::Vector2f(175,475));
        Manager->Add_Wall(sf::Vector2f(250,240),sf::Vector2f(400,365));

        Manager->Add_Wall(sf::Vector2f(125,425),sf::Vector2f(125,475));
        Manager->Add_Wall(sf::Vector2f(125,475),sf::Vector2f(175,475));
        //
        Manager->Add_Wall(sf::Vector2f(350, 250), sf::Vector2f(450, 250));
        Manager->Add_Wall(sf::Vector2f(350, 350), sf::Vector2f(450, 350));
        Manager->Add_Wall(sf::Vector2f(350, 250), sf::Vector2f(350, 350));
        Manager->Add_Wall(sf::Vector2f(450, 250), sf::Vector2f(450, 350));
        //

        // On ajoute des lignes à "Wall" pour représenter les murs de facon graphique.
        Wall.push_back(sf::Shape::Line(450, 325, 300, 200, 5, sf::Color(255,255,255)));
        Wall.push_back(sf::Shape::Line(250, 240, 400, 365, 5, sf::Color(255,255,255)));
        Wall.push_back(sf::Shape::Line(125, 125, 100, 100, 5, sf::Color(255,255,255)));
        Wall.push_back(sf::Shape::Line(500, 200, 600, 300, 5, sf::Color(255,255,255)));
        Wall.push_back(sf::Shape::Line(500, 300, 600, 200, 5, sf::Color(255,255,255)));
    #endif

    for(int i=0; i<SIZE_RAND_WALLS; i++)
    {
        sf::Vector2f    pt1(rand()%800,rand()%600),
                        pt2;
        float rnd_angle     = ((rand()/(float)(RAND_MAX))*2 - 1) * M_PI;
        float rnd_length    = fmax((rand()/(float)(RAND_MAX)) * 500.f, 50) ;
        pt2 = pt1 + sf::Vector2f(cos(rnd_angle), sin(rnd_angle)) * rnd_length;

        Wall.push_back(sf::Shape::Line(pt1.x, pt1.y, pt2.x, pt2.y, 5, sf::Color(255,255,255)));
        Manager->Add_Wall(pt1, pt2);
    }
    // Création des Light_Entity, ce sont elles qui permettent de modifier les lumières par après, comme changer de position, de couleur, etc
    Light_Entity light,light2,light3,directional_light;

    // On ajoute une lumière dynamique au Light_Manager et on dit que c'est "light" qui la représente.
    #ifdef __USE_LIGHT__
        #ifndef __USE_1_LIGHT__
            light=Manager->Add_Dynamic_Light();
            light2=Manager->Add_Dynamic_Light();
        #endif
        // On ajoute une lumière dynamique à la position (600,600), d'intensité 255, de rayon 160, de qualité 16 et de couleur verte.
        light3=Manager->Add_Dynamic_Light(sf::Vector2f(600,200),255,SIZE_LIGHT_TYPE0,QUALITY_LIGHT,sf::Color(0,255,0));
    #endif

    #ifdef __USE_LIGHT_SV__
        #ifndef __USE_1_LIGHT__
            light=Manager->Add_Dynamic_Light_SV();
            light2=Manager->Add_Dynamic_Light_SV();
            // On donne la position (375,275) à la lumière raccordée à "light".
            Manager->SetPosition(light,sf::Vector2f(375,275));
            // On donne une intensité de 255 à la lumière raccordée à "light".
            Manager->SetIntensity(light,255);
            // On donne un rayon de 128 à la lumière raccordée à "light".
            Manager->SetRadius(light,SIZE_LIGHT_TYPE1);
            // On donne une qualité de 16 à la lumière raccordée à "light".
            Manager->SetQuality(light,QUALITY_LIGHT);
            // On donne une couleur rouge à la lumière raccordée à "light".
            Manager->SetColor(light,sf::Color(255,0,0));
            // Même chose que juste au dessus, mais avec "light2"
            Manager->SetPosition(light2,sf::Vector2f(175,50));
            Manager->SetIntensity(light2,255);
            Manager->SetRadius(light2,SIZE_LIGHT_TYPE1);
            Manager->SetQuality(light2,16);
            Manager->SetColor(light2,sf::Color(0,0,255));
            //
            Manager->SetEffects_For_Light_SV(light);
            Manager->SetEffects_For_Light_SV(light2);
        #endif
        // On ajoute une lumière dynamique à la position (600,600), d'intensité 255, de rayon 160, de qualité 16 et de couleur verte.
        light3=Manager->Add_Dynamic_Light_SV(sf::Vector2f(600,200), 255, SIZE_LIGHT_TYPE0, QUALITY_LIGHT, sf::Color(32,128,196));
//        light3=Manager->Add_Dynamic_Light_SV( sf::Vector2f(600,200), 255, SIZE_LIGHT_TYPE0, QUALITY_LIGHT, sf::Color(255, 255, 255));
        //
        Manager->SetEffects_For_Light_SV(light3);
    #endif

    #ifndef __USE_LIGHT_SV__
        #ifndef __USE_1_LIGHT__
            // On ajoute une lumière statique à la position (110,490), d'intensité 160, de rayon 96, de qualité 8 et de couleur blanche.
            // On ne la raccorde pas à une Light_Entity car c'est une lumière statique et qui n'est donc pas modifiable.
            Manager->Add_Static_Light(sf::Vector2f(110,490),160,96,8,sf::Color(255,255,255));
            //
            directional_light = Manager->Add_Dynamic_Directional_Light(sf::Vector2f(750,310),255,384,90,45,sf::Color(255,128,255));
        #endif
    #endif

    #ifdef __USE_BACKGROUND__
        // Création d'une image qui servira d'image de fond, on charge donc l'image "test.png".
        sf::Image Image;
        //Image.LoadFromFile("data/textures/test.png");
//        Image.LoadFromFile("data/textures/rock.png");
//        Image.LoadFromFile("data/textures/texture-sol-telecabine-800x600.png");
        //Image.LoadFromFile("data/textures/ground-512x512.png");
        Image.LoadFromFile("data/textures/rock.bmp");
        //Image.LoadFromFile("data/textures/brickwork-texture.png");
        sf::Sprite background;
        background.SetImage(Image);
        background.Resize(i_screen_width,i_screen_height);
    #endif

    // Création d'une sf::String pour afficher les FPS
    sf::Text FPS;
    // Création d'un buffer pour préparer le texte des FPS
    char buffer[255];

    // Création de variables pour la gestion du temps.
    float AfficherFPS=0,LightRefresh=0;

    //Création de bool pour faire rebondir les lumières
    bool allerX=true,allerY=true;
    bool allerX2=true,allerY2=true;

    float angle = 90;

    // Exécution de la boucle principale
    while (App.IsOpened())
    {
        // Traitement des évènements
        sf::Event Event;
        while (App.GetEvent(Event))
        {
            // Escape key : exit
            if ((Event.Type == sf::Event::KeyPressed) && (Event.Key.Code == sf::Key::Escape))
                App.Close();
            // Fenêtre fermée : on quitte
            if (Event.Type == sf::Event::Closed)
                App.Close();
            // Resize event : adjust viewport
            if (Event.Type == sf::Event::Resized)
            {
            }
        }

        // On ajoute le temps écoulé
        AfficherFPS+=Clock.GetElapsedTime();
        LightRefresh+=Clock.GetElapsedTime();

        angle += Clock.GetElapsedTime() * 20;


        //On déplace la "light" en fonction du temps, avec allerX et allerY pour savoir si on monte ou on descend.
        if(allerX)
            Manager->SetPosition(light,sf::Vector2f(Manager->GetPosition(light).x+100*Clock.GetElapsedTime(),Manager->GetPosition(light).y));
        else
            Manager->SetPosition(light,sf::Vector2f(Manager->GetPosition(light).x-100*Clock.GetElapsedTime(),Manager->GetPosition(light).y));

        if(allerY)
            Manager->SetPosition(light,sf::Vector2f(Manager->GetPosition(light).x,Manager->GetPosition(light).y+100*Clock.GetElapsedTime()));
        else
            Manager->SetPosition(light,sf::Vector2f(Manager->GetPosition(light).x,Manager->GetPosition(light).y-100*Clock.GetElapsedTime()));

        //Si la "light" sort d'une certaine zone, on modifie allerX et/ou allerY pour faire rebondir.
        if(Manager->GetPosition(light).x>800)
            allerX=false;
        if(Manager->GetPosition(light).x<0)
            allerX=true;

        if(Manager->GetPosition(light).y>600)
            allerY=false;
        if(Manager->GetPosition(light).y<0)
            allerY=true;

        // Ce ne sert que pour la démo technique et n'intervient pas dans l'utilisation du moteur de lumières


        // Idem mais avec light2
        if(allerX2)
            Manager->SetPosition(light2,sf::Vector2f(Manager->GetPosition(light2).x+125*Clock.GetElapsedTime(),Manager->GetPosition(light2).y));
        else
            Manager->SetPosition(light2,sf::Vector2f(Manager->GetPosition(light2).x-125*Clock.GetElapsedTime(),Manager->GetPosition(light2).y));

        if(allerY2)
            Manager->SetPosition(light2,sf::Vector2f(Manager->GetPosition(light2).x,Manager->GetPosition(light2).y+125*Clock.GetElapsedTime()));
        else
            Manager->SetPosition(light2,sf::Vector2f(Manager->GetPosition(light2).x,Manager->GetPosition(light2).y-125*Clock.GetElapsedTime()));

        if(Manager->GetPosition(light2).x>800)
            allerX2=false;
        if(Manager->GetPosition(light2).x<0)
            allerX2=true;

        if(Manager->GetPosition(light2).y>600)
            allerY2=false;
        if(Manager->GetPosition(light2).y<0)
            allerY2=true;


        // On récupère la position de la souris (coordonnées window)
        const sf::Input& Input  = App.GetInput();
        sf::Vector2f pos_mouse_in_window(Input.GetMouseX(), Input.GetMouseY());
        pos_mouse_in_window.y   = App.GetHeight()-pos_mouse_in_window.y;
        // [-1, +1]² => [0, with-1]x[0, height-1]
        sf::Matrix3 mat_window  = sf::Matrix3::Transformation(sf::Vector2f(-1, -1), sf::Vector2f(0, 0), 0, sf::Vector2f(App.GetWidth()/2, App.GetHeight()/2));
        const sf::Matrix3 mat_inv_window            = mat_window.GetInverse(); // [0, with_window-1]x[0, height_window-1] -> [-1, +1]x[-1, +1]
        const sf::Matrix3 mat_view                  =  App.GetView().GetMatrix();    // [0, with_viewport]x[0, height_viewport] -> [-1, +1]x[-1, +1]
        const sf::Matrix3 mat_inv_view              =  mat_view.GetInverse();    // [-1, +1]x[-1, +1] -> [0, with_viewport]x[0, height_viewport]
        const sf::Matrix3 mat_window_to_viewport    =  mat_inv_view*mat_inv_window; // [0, with_window-1]x[0, height_window-1] -> [0, with_viewport]x[0, height_viewport]
        sf::Vector2f pos_mouse_in_viewport          = mat_window_to_viewport.Transform(pos_mouse_in_window);
        // On place light 3 à cette position
        Manager->SetPosition(light3, pos_mouse_in_viewport);

        Manager->SetOtherParameter(directional_light,ANGLE,angle);

        Clock.Reset();

        //Vous pouvez changer le 0.025 (=25ms=40hz) par une autre valeur si vous voulez que les lumières ne se rafraichissent que toutes les x secondes, au lieu de le faire à chaque tour de boucle.
        if(LightRefresh>0.025)
        {
            // On re-calcule les lumières
            Manager->Generate();
            LightRefresh=0;
        }

        // Efface l'écran (remplissage avec du noir)
        App.Clear( sf::Color(255, 255, 255, 255) );

        #ifdef __USE_BACKGROUND__
            // On affiche le fond
            App.Draw(background);
        #endif

        #ifdef __DRAW_WALLS__
            // On affiches les murs
            for(int w=0;w<(int)Wall.size();w++)
            {
                Wall[w].SetBlendMode(sf::Blend::Add);
                App.Draw(Wall[w]);
            }
        #endif

        // On affiche les lumières
        Manager->Draw(&App);

        // On affiche les FPS
        if(AfficherFPS>1.0)
        {
            float Framerate = 1.f / App.GetFrameTime();
            sprintf(buffer,"%i",(int)Framerate);
            FPS.SetString(buffer);
            AfficherFPS=0;
        }
        App.Draw(FPS);

        // Affichage du contenu de la fenêtre à l'écran
        App.Display();
    }

     Manager->Kill();

    return EXIT_SUCCESS;
}
