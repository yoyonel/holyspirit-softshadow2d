#include "Light_Wall_Debug.h"
#include "Light_Wall.h"
#include "Light_SV_Tools.h"

const GLuint N = 1;
GLuint queries[N];
GLuint sampleCount;
GLint available;
GLuint bitsSupported;

extern int indice_text_line;


void Light_Wall::Debug(sf::RenderTarget* App, const vec2& P) const
{
    if (App && !m_vector_bv.empty())
    {
        std::vector<sf::Shape>  vector_shapes_debug;
        std::vector<sf::Text>   vector_text_debug;

        const vec2 P_LS = P - pos;

        // - E0_LS et E1_LS sont les positions des deux extremites du mur, relatives au centre de la lumiere
        vec2 E0_LS(vertex_wall[0] - pos);
        vec2 E1_LS(vertex_wall[1] - pos);

        const Bounding_Volume& bv = m_vector_bv.back();

        //std::cout << "m_vector_bv.size(): " << m_vector_bv.size() << std::endl;
        //std::cout << "pos: " << pos.x << "," << pos.y << std::endl;
        //std::cout << "bv.Type()" << bv.Type()<< std::endl;

        //  OUTSIDE_CIRCLE  =0
        //  ON_CIRCLE       =1
        //  INSIDE_CIRCLE   =2
        sf::Color colors_types[3][3] = {
            { sf::Color(255, 0, 0, 255), sf::Color(0, 255, 0, 255), sf::Color(0, 0, 255, 255) },
            { sf::Color(0, 255, 0, 255), sf::Color(255, 255, 0, 255), sf::Color(0, 255, 255, 255) },
            { sf::Color(0, 0, 255, 255), sf::Color(0, 255, 255, 255), sf::Color(255, 255, 255, 255) }
        };

        float vis_light = 1.f;
        indice_text_line = -1;

        ADD_DEBUG_CIRCLE_OUTLINE(vector_shapes_debug, pos, bv.E0(), 2, sf::Color(sf::Color::Red), 2.0);
        ADD_DEBUG_CIRCLE_OUTLINE(vector_shapes_debug, pos, bv.E1(), 2, sf::Color(sf::Color::Red), 2.0);
        ADD_DEBUG_CIRCLE_OUTLINE(vector_shapes_debug, pos, bv.I0(), 2, sf::Color(sf::Color::Red), 2.0);
        ADD_DEBUG_CIRCLE_OUTLINE(vector_shapes_debug, pos, bv.I1(), 2, sf::Color(sf::Color::Red), 2.0);

        //ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.E0(), bv.I0(), 1, colors_types[0][1] );

        switch(bv.Type())
        {
            case PENUMBRAS_WIL:
            {
                const typ_vertex_compared_circle list_types[4] = { bv.TYPE_E0(), bv.TYPE_I0(), bv.TYPE_I1(), bv.TYPE_E1() };
                type_bv t_bv = Compute_Type_BV(list_types);
                switch(t_bv)
                {
                    case PENUMBRAS_WIL_TYPE_0: // OUTSIDE_CIRCLE, ON_CIRCLE, INSIDE_CIRCLE, INSIDE_CIRCLE
                    {
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.E0(), bv.I0(), 1, colors_types[0][1] );
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.I0(), bv.I1(), 1, colors_types[1][2] );
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.I1(), bv.E1(), 1, colors_types[2][2] );
                        //
                        vis_light -= 1 - compute_visibility_light_out_on(P_LS, bv.E0(), bv.I0(), inner_radius, pos, vector_shapes_debug, vector_text_debug);
                        vis_light -= 1 - compute_visibility_light_on_in(P_LS, bv.I0(), bv.E1(), inner_radius, pos, vector_shapes_debug, vector_text_debug);
                    }
                    break;

                    case PENUMBRAS_WIL_TYPE_1: // INSIDE_CIRCLE, INSIDE_CIRCLE, ON_CIRCLE, OUTSIDE_CIRCLE
                    {
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.E0(), bv.I0(), 1, colors_types[2][2] );
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.I0(), bv.I1(), 1, colors_types[2][1] );
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.I1(), bv.E1(), 1, colors_types[1][0] );
                        //
                        vis_light -= 1 - compute_visibility_light_in_on(P_LS, bv.I0(), bv.I1(), inner_radius, pos, vector_shapes_debug, vector_text_debug);
                        vis_light -= 1 - compute_visibility_light_out_on(P_LS, bv.E1(), bv.I1(), inner_radius, pos, vector_shapes_debug, vector_text_debug);
                    }
                    break;

                    case PENUMBRAS_WIL_TYPE_2: // ON_CIRCLE, ON_CIRCLE, ON_CIRCLE, ON_CIRCLE
                    {
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.E0(), bv.I0(), 1, colors_types[1][1] );
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.I0(), bv.I1(), 1, colors_types[1][1] );
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.I1(), bv.E1(), 1, colors_types[1][1] );
                        //
                        vis_light -= (1-compute_visibility_light_on_on(P_LS, bv.E0(), bv.E1(), inner_radius, pos, vector_shapes_debug, vector_text_debug));
                    }
                    break;

                    case PENUMBRAS_WIL_TYPE_3: // OUTSIDE_CIRCLE, ON_CIRCLE, ON_CIRCLE, OUTSIDE_CIRCLE
                    {
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.E0(), bv.I0(), 1, colors_types[0][1] );
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.I0(), bv.I1(), 1, colors_types[1][1] );
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.I1(), bv.E1(), 1, colors_types[1][0] );
                        //
                        vis_light -= 1 - compute_visibility_light_out_out(P_LS, bv.E0(), bv.I0(), bv.I1(), bv.E1(), inner_radius, pos, vector_shapes_debug, vector_text_debug);
                    }
                    break;

                    case PENUMBRAS_WIL_TYPE_4: // INSIDE_CIRCLE, INSIDE_CIRCLE, INSIDE_CIRCLE, INSIDE_CIRCLE
                    {
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.E0(), bv.I0(), 1, colors_types[2][2] );
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.I0(), bv.I1(), 1, colors_types[2][2] );
                        ADD_DEBUG_LINE( vector_shapes_debug, pos, bv.I1(), bv.E1(), 1, colors_types[2][2] );
                        //
                        vis_light -= (1 - compute_visibility_light_in_in(P_LS, bv.E0(), bv.E1(), inner_radius, pos, vector_shapes_debug, vector_text_debug));
                    }
                }
            }
            break;

            break;
            default:
            break;
        }

        ADD_DEBUG_TEXT(vector_text_debug, vec2(0.f, 0.f), "Visibility Light: ", vis_light, sf::Color(32, 32, 255, 255), 15);
        //
        for(std::vector<sf::Text>::iterator IterText=vector_text_debug.begin(); IterText!=vector_text_debug.end(); ++IterText)
            App->Draw(*IterText);

        for(std::vector<sf::Shape>::iterator IterShape=vector_shapes_debug.begin(); IterShape!=vector_shapes_debug.end(); ++IterShape)
            App->Draw(*IterShape);

        vector_shapes_debug.clear();
        vector_text_debug.clear();
    }
}

void Light_Wall::Debug( sf::RenderTarget* App, sf::Shader &shader_debug, const vec2 &P_WS)
{
    if (App && !m_vector_bv.empty())
    {
        const vec2 P = P_WS - pos;
        const float &r = inner_radius;
        const float inv_r = 1.f / r;
        float f_render_disc_portion;
        std::vector<sf::Shape> _vector_shapes_debug;

        const Bounding_Volume& bv = m_vector_bv.back();

        const vec2 E0 = bv.E0();
        const vec2 I0 = bv.I0();
        const vec2 I1 = bv.I1();
        const vec2 E1 = bv.E1();

        shader_debug.SetParameter("u_f_radius_light", r);
        shader_debug.SetParameter("u_v_receiver",     P);
        shader_debug.SetParameter("u_v_e0",           E0);
        shader_debug.SetParameter("u_v_e1",           E1);
        shader_debug.SetParameter("u_v_i0",           I0);
        shader_debug.SetParameter("u_v_i1",           I1);

        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        occlusion_query_begin();
        f_render_disc_portion = 0.f;
        shader_debug.SetParameter("u_f_render_disc_portion", f_render_disc_portion);
        // On rend la portion de disque ombree
        Draw_Analytic_Circle_Light(App, shader_debug);
        const GLuint nb_samples_circle = occlusion_query_end();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        std::cout << "bv.Type(): " << bv.Type() << std::endl;

        switch(bv.Type())
        {
            case PENUMBRAS_WIL_TYPE_0: // OUTSIDE_CIRCLE, ON_CIRCLE, IN_CIRCLE, IN_CIRCLE
            {
                _vector_shapes_debug.clear();
                //
                vec2 Proj_E0;
                vec2 Proj_I0, Proj_I1;
                //
                Proj_E0 = compute_projection_on_circle( E0, P, r, inv_r );
                Proj_I0 = compute_projection_on_circle( I0, P, r, inv_r );
                Proj_I1 = compute_projection_on_circle( I1, P, r, inv_r );
                //

                // Portion de disque
                occlusion_query_begin();
                f_render_disc_portion = 2.f; // Portion de disque: Proj_I0, Proj_I1
                shader_debug.SetParameter("u_f_render_disc_portion", f_render_disc_portion);
                // On rend la portion de disque ombree:
                Draw_Analytic_Circle_Light(App, shader_debug);
                const GLuint nb_samples_disc_portion_0 = occlusion_query_end();

                // Portion de disque
                occlusion_query_begin();
                f_render_disc_portion = 3.f; // Portion de disque: I0, Proj_I0
                shader_debug.SetParameter("u_f_render_disc_portion", f_render_disc_portion);
                // On rend la portion de disque ombree
                Draw_Analytic_Circle_Light(App, shader_debug);
                const GLuint nb_samples_disc_portion_1 = occlusion_query_end();

                // triangles
                occlusion_query_begin();
                {
                    sf::Color color_triangle(255, 255, 255, 255);
                    _vector_shapes_debug.clear();
                    ADD_DEBUG_TRIANGLE_FILL( _vector_shapes_debug, pos, I0, Proj_I0, I1, 1, color_triangle );
                    for(std::vector<sf::Shape>::iterator IterShape=_vector_shapes_debug.begin(); IterShape!=_vector_shapes_debug.end(); ++IterShape)
                        App->Draw(*IterShape);
                }
                const GLuint nb_samples_triangle0 = occlusion_query_end();
                //
                occlusion_query_begin();
                {
                    sf::Color color_triangle(255, 255, 255, 255);
                    _vector_shapes_debug.clear();
                    ADD_DEBUG_TRIANGLE_FILL( _vector_shapes_debug, pos, I1, Proj_I0, Proj_I1, 1, color_triangle );
                    for(std::vector<sf::Shape>::iterator IterShape=_vector_shapes_debug.begin(); IterShape!=_vector_shapes_debug.end(); ++IterShape)
                        App->Draw(*IterShape);
                }
                const GLuint nb_samples_triangle1 = occlusion_query_end();

                const float inv_area_disc = 1/(float)(M_PI*r*r);
                const float inv_nb_samples_disc = 1/(float)(nb_samples_circle);
                float visibility_light = 1 - (nb_samples_disc_portion_0 + nb_samples_disc_portion_1 + nb_samples_triangle0 + nb_samples_triangle1)*inv_nb_samples_disc;

                std::vector<sf::Text> vector_text_debug;
                //
                int i = 0;
                int y = 25;
                //
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire (totale) de recouvrement du disque", nb_samples_circle*inv_area_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement de la portion de disque_0", nb_samples_disc_portion_0*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement de la portion de disque_1", nb_samples_disc_portion_1*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement du triangle0", nb_samples_triangle0*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement du triangle1", nb_samples_triangle1*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                //
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "visibility_light", visibility_light, sf::Color(64, 128, 255, 255), 15);
                //
                for(std::vector<sf::Text>::iterator IterText=vector_text_debug.begin(); IterText!=vector_text_debug.end(); ++IterText)
                    App->Draw(*IterText);
            }
            break;

            case PENUMBRAS_WIL_TYPE_1: // INSIDE_CIRCLE, INSIDE_CIRCLE, ON_CIRCLE, OUTSIDE_CIRCLE
            {
                _vector_shapes_debug.clear();
                //
                vec2 Proj_E1;
                vec2 Proj_I0, Proj_I1;
                //
                Proj_E1 = compute_projection_on_circle( E1, P, r, inv_r );
                Proj_I0 = compute_projection_on_circle( I0, P, r, inv_r );
                Proj_I1 = compute_projection_on_circle( I1, P, r, inv_r );
                //

                // Portion de disque
                occlusion_query_begin();
                f_render_disc_portion = 2.f; // Portion de disque: Proj_I0, Proj_I1
                shader_debug.SetParameter("u_f_render_disc_portion", f_render_disc_portion);
                // On rend la portion de disque ombree:
                Draw_Analytic_Circle_Light(App, shader_debug);
                const GLuint nb_samples_disc_portion_0 = occlusion_query_end();

                // Portion de disque
                occlusion_query_begin();
                f_render_disc_portion = 4.f; // Portion de disque: I1, Proj_I1
                shader_debug.SetParameter("u_f_render_disc_portion", f_render_disc_portion);
                // On rend la portion de d  isque ombree
                Draw_Analytic_Circle_Light(App, shader_debug);
                const GLuint nb_samples_disc_portion_2 = occlusion_query_end();

                // triangles
                occlusion_query_begin();
                {
                    sf::Color color_triangle(255, 255, 255, 255);
                    _vector_shapes_debug.clear();
                    ADD_DEBUG_TRIANGLE_FILL( _vector_shapes_debug, pos, I0, Proj_I0, I1, 1, color_triangle );
                    for(std::vector<sf::Shape>::iterator IterShape=_vector_shapes_debug.begin(); IterShape!=_vector_shapes_debug.end(); ++IterShape)
                        App->Draw(*IterShape);
                }
                const GLuint nb_samples_triangle0 = occlusion_query_end();
                //
                occlusion_query_begin();
                {
                    sf::Color color_triangle(255, 255, 255, 255);
                    _vector_shapes_debug.clear();
                    ADD_DEBUG_TRIANGLE_FILL( _vector_shapes_debug, pos, I1, Proj_I0, Proj_I1, 1, color_triangle );
                    for(std::vector<sf::Shape>::iterator IterShape=_vector_shapes_debug.begin(); IterShape!=_vector_shapes_debug.end(); ++IterShape)
                        App->Draw(*IterShape);
                }
                const GLuint nb_samples_triangle1 = occlusion_query_end();

                const float inv_area_disc = 1/(float)(M_PI*r*r);
                const float inv_nb_samples_disc = 1/(float)(nb_samples_circle);
                float visibility_light = 1 - (nb_samples_disc_portion_0 + nb_samples_disc_portion_2 + nb_samples_triangle0 + nb_samples_triangle1)*inv_nb_samples_disc;

                std::vector<sf::Text> vector_text_debug;
                //
                int i = 0;
                int y = 25;
                //
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire (totale) de recouvrement du disque", nb_samples_circle*inv_area_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement de la portion de disque_0", nb_samples_disc_portion_0*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement de la portion de disque_2", nb_samples_disc_portion_2*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement du triangle0", nb_samples_triangle0*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement du triangle1", nb_samples_triangle1*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                //
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "visibility_light", visibility_light, sf::Color(64, 128, 255, 255), 15);
                //
                for(std::vector<sf::Text>::iterator IterText=vector_text_debug.begin(); IterText!=vector_text_debug.end(); ++IterText)
                    App->Draw(*IterText);
            }
            break;

            case PENUMBRAS_WIL_TYPE_3: // OUTSIDE_CIRCLE, ON_CIRCLE, ON_CIRCLE, OUTSIDE_CIRCLE
            {
                _vector_shapes_debug.clear();
                //
                vec2 Proj_E0, Proj_E1;
                vec2 Proj_I0, Proj_I1;
                Proj_E0 = compute_projection_on_circle( E0, P, r, inv_r );
                Proj_E1 = compute_projection_on_circle( E1, P, r, inv_r );
                Proj_I0 = compute_projection_on_circle( I0, P, r, inv_r );
                Proj_I1 = compute_projection_on_circle( I1, P, r, inv_r );

                // Portion de disque
                occlusion_query_begin();
                f_render_disc_portion = 2.f; // Portion de disque: Proj_I0, Proj_I1
                shader_debug.SetParameter("u_f_render_disc_portion", f_render_disc_portion);
                // On rend la portion de disque ombree:
                Draw_Analytic_Circle_Light(App, shader_debug);
                const GLuint nb_samples_disc_portion_0 = occlusion_query_end();

                // Portion de disque
                occlusion_query_begin();
                f_render_disc_portion = 3.f; // Portion de disque: I0, Proj_I0
                shader_debug.SetParameter("u_f_render_disc_portion", f_render_disc_portion);
                // On rend la portion de disque ombree
                Draw_Analytic_Circle_Light(App, shader_debug);
                const GLuint nb_samples_disc_portion_1 = occlusion_query_end();

                // Portion de disque
                occlusion_query_begin();
                f_render_disc_portion = 4.f; // Portion de disque: I1, Proj_I1
                shader_debug.SetParameter("u_f_render_disc_portion", f_render_disc_portion);
                // On rend la portion de disque ombree
                Draw_Analytic_Circle_Light(App, shader_debug);
                const GLuint nb_samples_disc_portion_2 = occlusion_query_end();

                // triangles
                occlusion_query_begin();
                {
                    sf::Color color_triangle(255, 255, 255, 255);
                    _vector_shapes_debug.clear();
                    ADD_DEBUG_TRIANGLE_FILL( _vector_shapes_debug, pos, I0, Proj_I0, I1, 1, color_triangle );
                    for(std::vector<sf::Shape>::iterator IterShape=_vector_shapes_debug.begin(); IterShape!=_vector_shapes_debug.end(); ++IterShape)
                        App->Draw(*IterShape);
                }
                const GLuint nb_samples_triangle0 = occlusion_query_end();
                //
                occlusion_query_begin();
                {
                    sf::Color color_triangle(255, 255, 255, 255);
                    _vector_shapes_debug.clear();
                    ADD_DEBUG_TRIANGLE_FILL( _vector_shapes_debug, pos, I1, Proj_I0, Proj_I1, 1, color_triangle );
                    for(std::vector<sf::Shape>::iterator IterShape=_vector_shapes_debug.begin(); IterShape!=_vector_shapes_debug.end(); ++IterShape)
                        App->Draw(*IterShape);
                }
                const GLuint nb_samples_triangle1 = occlusion_query_end();

                const float inv_area_disc = 1/(float)(M_PI*r*r);
                const float inv_nb_samples_disc = 1/(float)(nb_samples_circle);
                float visibility_light = 1 - (nb_samples_disc_portion_0 + nb_samples_disc_portion_1 + nb_samples_disc_portion_2 + nb_samples_triangle0 + nb_samples_triangle1)*inv_nb_samples_disc;

                std::vector<sf::Text> vector_text_debug;
                //
                int i = 0;
                int y = 25;
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire (totale) de recouvrement du disque", nb_samples_circle*inv_area_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement de la portion de disque_0", nb_samples_disc_portion_0*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement de la portion de disque_1", nb_samples_disc_portion_1*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement de la portion de disque_2", nb_samples_disc_portion_2*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement du triangle0", nb_samples_triangle0*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement du triangle1", nb_samples_triangle1*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                //
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "visibility_light", visibility_light, sf::Color(64, 128, 255, 255), 15);
                //
                for(std::vector<sf::Text>::iterator IterText=vector_text_debug.begin(); IterText!=vector_text_debug.end(); ++IterText)
                    App->Draw(*IterText);
            }
            break;

            case PENUMBRAS_WIL_TYPE_4: // INSIDE_CIRCLE, INSIDE_CIRCLE, INSIDE_CIRCLE, INSIDE_CIRCLE
            {
                vec2 Proj_E0, Proj_E1;
                Proj_E0 = compute_projection_on_circle( E0, P, r, inv_r );
                Proj_E1 = compute_projection_on_circle( E1, P, r, inv_r );

                // Portion de disque: Proj_E0, Proj_E1
                f_render_disc_portion = 1.f;
                occlusion_query_begin();
                shader_debug.SetParameter("u_f_render_disc_portion", f_render_disc_portion);
                // On rend la portion de disque ombree
                Draw_Analytic_Circle_Light(App, shader_debug);
                const GLuint nb_samples_disc_portion = occlusion_query_end();

                // triangles
                occlusion_query_begin();
                {
                    sf::Color color_triangle(255, 255, 255, 255);
                    _vector_shapes_debug.clear();
                    ADD_DEBUG_TRIANGLE_FILL( _vector_shapes_debug, pos, E0, Proj_E0, E1, 1, color_triangle );
                    for(std::vector<sf::Shape>::iterator IterShape=_vector_shapes_debug.begin(); IterShape!=_vector_shapes_debug.end(); ++IterShape)
                        App->Draw(*IterShape);
                }
                const GLuint nb_samples_triangle0 = occlusion_query_end();
                //
                occlusion_query_begin();
                {
                    sf::Color color_triangle(255, 255, 255, 255);
                    _vector_shapes_debug.clear();
                    ADD_DEBUG_TRIANGLE_FILL( _vector_shapes_debug, pos, E1, Proj_E0, Proj_E1, 1, color_triangle );
                    for(std::vector<sf::Shape>::iterator IterShape=_vector_shapes_debug.begin(); IterShape!=_vector_shapes_debug.end(); ++IterShape)
                        App->Draw(*IterShape);
                }
                const GLuint nb_samples_triangle1 = occlusion_query_end();

                vec2 mid_Proj_E0_E1 = (Proj_E0 + Proj_E1) * 0.5f;
                vec2 v_dir = NORMALIZE(mid_Proj_E0_E1 );
                float d = signed_distance_point_line(vec2(0, 0), Proj_E0, Proj_E1);
                ADD_DEBUG_LINE(_vector_shapes_debug, pos, mid_Proj_E0_E1, mid_Proj_E0_E1 - v_dir*d, 1, sf::Color(0, 0, 255, 255));
                for(std::vector<sf::Shape>::iterator IterShape=_vector_shapes_debug.begin(); IterShape!=_vector_shapes_debug.end(); ++IterShape)
                        App->Draw(*IterShape);

                const float inv_area_disc = 1/(float)(M_PI*r*r);
                const float inv_nb_samples_disc = 1/(float)(nb_samples_circle);
                float visibility_light = 1 - (nb_samples_disc_portion + nb_samples_triangle0 + nb_samples_triangle1)*inv_nb_samples_disc;

                std::vector<sf::Text> vector_text_debug;
                //
                int i = 0;
                int y = 25;
                //
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire (totale) de recouvrement du disque", nb_samples_circle*inv_area_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement de la portion de disque", nb_samples_disc_portion*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement du triangle0", nb_samples_triangle0*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "aire de recouvrement du triangle1", nb_samples_triangle1*inv_nb_samples_disc, sf::Color(255, 255, 255, 255), 15);
                //
                ADD_DEBUG_TEXT(vector_text_debug, vec2(0., y + (++i)*15), "visibility_light", visibility_light, sf::Color(64, 128, 255, 255), 15);
                //
                for(std::vector<sf::Text>::iterator IterText=vector_text_debug.begin(); IterText!=vector_text_debug.end(); ++IterText)
                    App->Draw(*IterText);
            }
            break;

            default:
            break;
        }
    }
}

vec2 compute_projection_on_circle( const vec2& E, const vec2& P, const float r, const float inv_r )
{
    float   f_signed_distance, d, a;
    vec2    x, y, Proj_E_P;

    vec2 pos_light= vec2(0.f, 0.f);

    f_signed_distance = signed_distance_point_line( pos_light, P, E );
    // [TODO]: ÃÂ  simplifier !
    d = f_signed_distance*inv_r;
    a = acos(d);
    y = NORMALIZE(E-P);
    x = NORMAL(y);
    Proj_E_P = pos_light + x*d*r + y*sinf(a)*r;

    return Proj_E_P;
}

bool inside_half_plane(const vec2& A, const vec2& B, const vec2& P)
{
    // Du bon cote du demi-plan dont l'edge est la frontiere (ou sa droite) et oriente pour ne pas contenir la source de lumiere
    // equation parametrique d'une droite: (1) a.x + b.y + c = 0, avec (a,b) normale de la droite
    vec2 v_dir      = B - A;
    vec2 v2_normal   = NORMAL(v_dir);
    // on calcul c => c = -(a*x + b*y), on prend (x, y) = A (1 point de la droite)
    float c = - DOT(v2_normal, A);
    // dans quel sens la normale est orientee pour relier le point P et la droite ?
    float side_of_P = DOT(P, v2_normal) + c;
    // selon le sens (qui indique le sens de la normale), on determine si le point est dans le demi-plan
    return (side_of_P*SIGN(c)<0.);
}

// P0 et P1: sont sur le cercle de position pos_light et (inverse) de rayon inv_r
float compute_disc_portion_area( const vec2& P0, const vec2& P1, const float r )
{
    float f_distance, d, A, aire_secteur_angulaire, aire_triangle_isocele, alpha;
    const vec2 pos_light = vec2(0.f, 0.f);

    // On calcul de l'aire du bout de disque dans la zone d'occultation
    d = signed_distance_point_line( pos_light, P0, P1 );
    d = CLAMP(d, -r, +r);

    alpha = acosf(d/r); // angle
    aire_secteur_angulaire = alpha*r*r;
    aire_triangle_isocele = d*sqrtf(r*r - d*d); // se decompose en deux triangles rectangles dont le rayon du cercle sont les hypothenuses

    // aire de la portion de disque
    A = alpha*r*r - d*sqrtf(r*r - d*d);

    return A;
}

// calcul la visibilite de la lumiere
// pour un point P
// occulte par un segment defini
// par deux points E0 et E1 en dehors ou sur le disque de lumiere
float compute_visibility_light(const vec2& P, const vec2& E0, const vec2& E1, const float r)
{
    float visibilty = 1.;
    const float inv_area_light = 1.f / (M_PI*r*r);

    // Compute visibility of the circle light
    float A0 = compute_disc_portion_area(P, E0, r);
    float A1 = compute_disc_portion_area(P, E1, r);

    A0 *= inv_area_light;
    A1 *= inv_area_light;

    // Do some tests to fix the visibilty (depends of orientations between receiver point, edge, and the circle light source)
    vec2 Edge = E1 - E0;
    //
    vec2 P_E0 = E0 - P;
    vec2 P_E1 = E1 - P;
    //
    vec2 n_P_E0 = NORMAL(P_E0);
    vec2 n_P_E1 = NORMAL(P_E1);
    // Orientation du volume d'ombre
    A0 = DOT(n_P_E0, +1.f*Edge) > 0.f ? (1.f-A0) : A0;
    // A t'on calcule l'aire de visibilite ou d'occultation ?
    A1 = DOT(n_P_E1, -1.f*Edge) > 0.f ? (1.f-A1) : A1;
    // Sens de projection de l'ombre
    A0 = float(DOT(P_E0, E0) < 0.f) * A0;
    A1 = float(DOT(P_E1, E1) < 0.f) * A1;

    visibilty = (A0 + A1);

    return visibilty;
}

// E0: In       Circle
// E1: In       Circle
// [DEBUG: OK]
float compute_visibility_light_in_in(const vec2& P, const vec2& E0, const vec2& E1, const float r, const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug)
{
    float visibility_light;
    float coef_shadow_light;
    float coef_area_triangle_0, coef_area_triangle_1;
    float coef_disc_portion_area;

    const float inv_area_light = 1.f/(M_PI*r*r);
    const float inv_r = 1.f/r;

    vec2 Proj_E0, Proj_E1;
    Proj_E0 = compute_projection_on_circle( E0, P, r, inv_r );
    Proj_E1 = compute_projection_on_circle( E1, P, r, inv_r );

    // occluder triangle (issu de E0)
    coef_area_triangle_0 = area_triangle(E0, Proj_E0, E1)*inv_area_light;

    // occluder triangle (issu de E1)
    coef_area_triangle_1 = area_triangle(E1, Proj_E0, Proj_E1)*inv_area_light;

    // 1 portion de disque
    coef_disc_portion_area = compute_disc_portion_area( Proj_E0, Proj_E1, r)*inv_area_light;
    vec2 N_Proj_E0_Proj_E1 = NORMAL(Proj_E1 - Proj_E0);
    coef_disc_portion_area = DOT( P - Proj_E0, N_Proj_E0_Proj_E1 ) > 0 ? 1 - coef_disc_portion_area : coef_disc_portion_area;

    coef_shadow_light = coef_area_triangle_0 + coef_area_triangle_1 + coef_disc_portion_area;

    visibility_light = 1. - coef_shadow_light;

    //
    sf::Color   color_receiver(64, 96, 196, 255),
                color_edge(64, 196, 32, 255),
                color_proj(196, 32, 64, 255),
                color_triangle(64, 128, 128, 255),
                color_text(32, 255, 32, 255),
                color_disc_portion(196, 164, 64, 255);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, P, 5, color_receiver);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, E0, 3, color_edge);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, E1, 3, color_edge);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, Proj_E0, 3, color_proj);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, Proj_E1, 3, color_proj);
    //
    ADD_DEBUG_TRIANGLE_OUTLINE( _vector_shapes_debug, _pos_light, E0, Proj_E0, E1, 1, color_triangle );
    ADD_DEBUG_TRIANGLE_OUTLINE( _vector_shapes_debug, _pos_light, E1, Proj_E1, Proj_E0, 1, color_triangle );
    //
    ADD_DEBUG_LINE( _vector_shapes_debug, _pos_light, Proj_E0, Proj_E1, 2, color_disc_portion );
    //
    float y = 400, x = 0;
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_in_in => visibility_light: ", visibility_light, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_in_in: coef_area_triangle_0: ", coef_area_triangle_0, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_in_in: coef_area_triangle_1: ", coef_area_triangle_1, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_in_in: coef_disc_portion_area: ", coef_disc_portion_area, color_text, 15 );

    return visibility_light;
}

// E0: Out      Circle
// E1: On       Circle
// [DEBUG: OK]
float compute_visibility_light_out_on(const vec2& P, const vec2& E0, const vec2& E1, const float r, const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug)
{
    vec2 dir_E1     = NORMALIZE(E1);
    vec2 normal_E1  = NORMAL(dir_E1);

    bool b_wall_occlude_light = inside_half_plane(E1, E1 + normal_E1, P);

    float visibility_light = b_wall_occlude_light ? compute_visibility_light(P, E0, E1, r) : 1.f;

    sf::Color   color_receiver(64, 96, 196, 255),
                color_edge(64, 196, 32, 255),
                color_text(32, 255, 32, 255);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, P, 5, color_receiver);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, E0, 3, color_edge);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, E1, 3, color_edge);
    //
    float y = 400, x = 0;
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "=> compute_visibility_light_out_on: ", 1 - visibility_light, color_text, 15 );

    return visibility_light;
}

// E0: On       Circle
// E1: On       Circle
// [DEBUG: OK]
float   compute_visibility_light_on_on(const vec2& P, const vec2& E0, const vec2& E1, const float r, const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug)
{
    float visibility_light;
    float coef_area_triangle_0 = 0, coef_area_triangle_1 = 0;
    float coef_disc_portion_area = 0;
    float coef_shadow_light = 0.;

    const float inv_area_light = 1.f/(M_PI*r*r);
    const float inv_r = 1./r;

    vec2 Proj_E0, Proj_E1;

    vec2 dir_E0     = NORMALIZE(E0);
    vec2 normal_E0  = NORMAL(dir_E0);
    bool b_E0_occlude_light = inside_half_plane(E0, E0 + normal_E0, P);
    Proj_E0 = b_E0_occlude_light ? compute_projection_on_circle( E0, P, r, inv_r ) : E0;
    //
    vec2 dir_E1     = NORMALIZE(E1);
    vec2 normal_E1  = NORMAL(dir_E1);
    bool b_E1_occlude_light = inside_half_plane(E1, E1 + normal_E1, P);
    Proj_E1 = b_E1_occlude_light ? compute_projection_on_circle( E1, P, r, inv_r ) : E1;

    // occluder triangle (issu de E0)
    coef_area_triangle_0 = area_triangle(E0, Proj_E0, E1)*inv_area_light;
    // occluder triangle (issu de E1)
    coef_area_triangle_1 = area_triangle(E1, Proj_E0, Proj_E1)*inv_area_light;

    // 1 portion de disque
    coef_disc_portion_area = compute_disc_portion_area( Proj_E0, Proj_E1, r ) * inv_area_light;;
    vec2 N_Proj_E0_Proj_E1 = NORMAL(Proj_E1 - Proj_E0);
    coef_disc_portion_area = DOT( P - Proj_E0, N_Proj_E0_Proj_E1 ) > 0 ? 1 - coef_disc_portion_area : coef_disc_portion_area;

    coef_shadow_light = coef_area_triangle_0 + coef_area_triangle_1 + coef_disc_portion_area;

    visibility_light = 1. - coef_shadow_light;

    //
    sf::Color   color_receiver(64, 96, 196, 255),
                color_edge(64, 196, 32, 255),
                color_proj(196, 32, 64, 255),
                color_triangle(64, 128, 128, 255),
                color_text(32, 255, 32, 255),
                color_disc_portion(196, 164, 64, 255);
    //
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, P, 5, color_receiver);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, E0, 3, color_edge);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, E1, 3, color_edge);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, Proj_E0, 3, color_proj);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, Proj_E1, 3, color_proj);
    //
    ADD_DEBUG_TRIANGLE_OUTLINE( _vector_shapes_debug, _pos_light, E0, Proj_E0, E1, 1, color_triangle );
    ADD_DEBUG_TRIANGLE_OUTLINE( _vector_shapes_debug, _pos_light, E1, Proj_E1, Proj_E0, 1, color_triangle );
    //
    ADD_DEBUG_LINE( _vector_shapes_debug, _pos_light, Proj_E0, Proj_E1, 2, color_disc_portion );
    //
    float y = 400, x = 0;
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_on_on => visibility_light: ", visibility_light, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_on_on => coef_area_triangle_0: ", coef_area_triangle_0, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_on_on => coef_area_triangle_1: ", coef_area_triangle_1, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_on_on => coef_disc_portion_area: ", coef_disc_portion_area, color_text, 15 );

    return visibility_light;
}

// E0: On       Circle
// E1: Inside   Circle
float compute_visibility_light_on_in(const vec2& P, const vec2& E0, const vec2& E1, const float r, const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug)
{
    float visibility_light;
    float coef_area_triangle_0 = 0, coef_area_triangle_1 = 0;
    float coef_disc_portion_area = 0;
    float coef_shadow_light = 0.;
    const float inv_area_light = 1.f/(M_PI*r*r);
    float inv_r = 1./r;

    vec2 Proj_E0, Proj_E1;

    Proj_E1 = compute_projection_on_circle( E1, P, r, inv_r );

    vec2 dir_E0     = NORMALIZE(E0);
    vec2 normal_E0  = NORMAL(dir_E0);
    bool b_E0_occlude_light = inside_half_plane(E0, E0 + normal_E0, P);
    if (b_E0_occlude_light)
    {
        Proj_E0 = compute_projection_on_circle( E0, P, r, inv_r );
        // occluder triangle (issu de E0)
        coef_area_triangle_0 = area_triangle(E0, Proj_E0, Proj_E1)* inv_area_light;
    }
    else
    {
        Proj_E0 = E0;
    }

    // occluder triangle (issu de E1)
    coef_area_triangle_1 = area_triangle(E1, E0, Proj_E1)* inv_area_light;

    // 1 portion de disque
    coef_disc_portion_area = compute_disc_portion_area( Proj_E0, Proj_E1, r ) * inv_area_light;
    vec2 N_Proj_E0_Proj_E1 = NORMAL(Proj_E1 - Proj_E0);
    coef_disc_portion_area = DOT( P - Proj_E0, N_Proj_E0_Proj_E1 ) > 0 ? 1 - coef_disc_portion_area : coef_disc_portion_area;

    coef_shadow_light = coef_area_triangle_0 + coef_area_triangle_1 + coef_disc_portion_area;

    visibility_light = 1. - coef_shadow_light;

    //
    sf::Color   color_receiver(64, 96, 196, 255),
                color_edge(64, 196, 32, 255),
                color_proj(196, 32, 64, 255),
                color_triangle(64, 128, 128, 255),
                color_text(32, 255, 32, 255),
                color_disc_portion(196, 164, 64, 255);
    //
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, P, 5, color_receiver);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, E0, 3, color_edge);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, E1, 3, color_edge);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, Proj_E0, 3, color_proj);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, Proj_E1, 3, color_proj);
    //
    ADD_DEBUG_TRIANGLE_OUTLINE( _vector_shapes_debug, _pos_light, E0, Proj_E0, E1, 1, color_triangle );
    ADD_DEBUG_TRIANGLE_OUTLINE( _vector_shapes_debug, _pos_light, E1, Proj_E1, Proj_E0, 1, color_triangle );
    //
    ADD_DEBUG_LINE( _vector_shapes_debug, _pos_light, Proj_E0, Proj_E1, 2, color_disc_portion );
    //
    float y = 400, x = 0;
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "visibility_light: ", visibility_light, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24) , "coef_area_triangle_0: ", coef_area_triangle_0, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "coef_area_triangle_1: ", coef_area_triangle_1, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "coef_disc_portion_area: ", coef_disc_portion_area, color_text, 15 );

    return visibility_light;
}

// E0: In       Circle
// E1: On   Circle
float compute_visibility_light_in_on(const vec2& P, const vec2& E0, const vec2& E1, const float r, const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug)
{
    float visibility_light;
    float coef_area_triangle_0 = 0, coef_area_triangle_1 = 0;
    float coef_disc_portion_area = 0;
    float coef_shadow_light = 0.;
    const float inv_area_light = 1.f/(M_PI*r*r);
    float inv_r = 1./r;

    vec2 Proj_E0, Proj_E1;

    Proj_E0 = compute_projection_on_circle( E0, P, r, inv_r );

    vec2 dir_E1     = NORMALIZE(E1);
    vec2 normal_E1  = NORMAL(dir_E1);
    bool b_E1_occlude_light = inside_half_plane(E1, E1 + normal_E1, P);
    if (b_E1_occlude_light)
    {
        Proj_E1 = compute_projection_on_circle( E1, P, r, inv_r );
        // occluder triangle (issu de E0)
        coef_area_triangle_1 = area_triangle(E1, E0, Proj_E1)* inv_area_light;
    }
    else
    {
        Proj_E1 = E1;
    }

    // occluder triangle (issu de E0)
    coef_area_triangle_0 = area_triangle(E0, Proj_E0, Proj_E1)* inv_area_light;

    // 1 portion de disque
    coef_disc_portion_area = compute_disc_portion_area( Proj_E0, Proj_E1, r ) * inv_area_light;
    vec2 N_Proj_E0_Proj_E1 = NORMAL(Proj_E1 - Proj_E0);
    coef_disc_portion_area = DOT( P - Proj_E0, N_Proj_E0_Proj_E1 ) > 0 ? 1 - coef_disc_portion_area : coef_disc_portion_area;

    coef_shadow_light = coef_area_triangle_0 + coef_area_triangle_1 + coef_disc_portion_area;

    visibility_light = 1. - coef_shadow_light;

    //
    sf::Color   color_receiver(64, 96, 196, 255),
                color_edge(64, 196, 32, 255),
                color_proj(196, 32, 64, 255),
                color_triangle(64, 128, 128, 255),
                color_text(32, 255, 32, 255),
                color_disc_portion(196, 164, 64, 255);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, P, 5, color_receiver);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, E0, 3, color_edge);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, E1, 3, color_edge);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, Proj_E0, 3, color_proj);
    ADD_DEBUG_CIRCLE(_vector_shapes_debug, _pos_light, Proj_E1, 3, color_proj);
    //
    ADD_DEBUG_TRIANGLE_OUTLINE( _vector_shapes_debug, _pos_light, E0, Proj_E0, E1, 1, color_triangle );
    ADD_DEBUG_TRIANGLE_OUTLINE( _vector_shapes_debug, _pos_light, E1, Proj_E1, Proj_E0, 1, color_triangle );
    //
    ADD_DEBUG_LINE( _vector_shapes_debug, _pos_light, Proj_E0, Proj_E1, 2, color_disc_portion );
    //
    float y = 400, x = 0;
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_in_on => visibility_light: ", visibility_light, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_in_on: coef_area_triangle_0: ", coef_area_triangle_0, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_in_on: coef_area_triangle_1: ", coef_area_triangle_1, color_text, 15 );
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "compute_visibility_light_in_on: coef_disc_portion_area: ", coef_disc_portion_area, color_text, 15 );

    return visibility_light;
}

// E0: Out       Circle
// E1: Out       Circle
// [DEBUG: OK]
float   compute_visibility_light_out_out(const vec2& P, const vec2& E0, const vec2& I0, const vec2& I1, const vec2& E1, const float r, const vec2& _pos_light, std::vector<sf::Shape> &_vector_shapes_debug, std::vector<sf::Text> &_vector_text_debug)
{
    float shadow_light = 0;

    // Decomposition du segment d'occultation
    shadow_light += 1 - compute_visibility_light_out_on( P, E0, I0, r, _pos_light, _vector_shapes_debug, _vector_text_debug );
    shadow_light += 1 - compute_visibility_light_out_on( P, E1, I1, r, _pos_light, _vector_shapes_debug, _vector_text_debug );
    shadow_light += 1 - compute_visibility_light_on_on(  P, I0, I1, r, _pos_light, _vector_shapes_debug, _vector_text_debug );
    //
    sf::Color color_text(32, 255, 32, 255);
    float y = 400, x = 0;
    ADD_DEBUG_TEXT( _vector_text_debug, vec2(x, y + (++indice_text_line)*24), "=> compute_visibility_light_out_out: ", shadow_light, color_text, 15 );

    return 1 - shadow_light;
}

void occlusion_query_begin()
{
    // check to make sure functionality is supported
    glGetQueryiv(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, (GLint*)(&bitsSupported));
//    std::cout << "# bitsSupported: " << bitsSupported << std::endl;

    if(bitsSupported)
    {
        glGenQueriesARB(N, queries);
        glBeginQueryARB(GL_SAMPLES_PASSED_ARB, queries[0]);
    }
}

GLuint occlusion_query_end()
{
    // check to make sure functionality is supported
    glGetQueryiv(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, (GLint*)(&bitsSupported));
    if(bitsSupported)
    {
        glEndQuery(GL_SAMPLES_PASSED_ARB);
        do {
            glGetQueryObjectivARB(queries[0], GL_QUERY_RESULT_AVAILABLE_ARB, &available);
        } while (!available);
        glGetQueryObjectuivARB(queries[0], GL_QUERY_RESULT_ARB, &sampleCount);
    }

    return sampleCount;
}


//Light_Wall::Compute()
//{
////    ...
//
//    //            type_bv_wil = Compute_Type_BV(type_intersections_circle);
////            //
////            int i = type_bv_wil - PENUMBRAS_WIL_TYPE_0;
////            const sf::Color colors_shapes_bb[5] = {
////                sf::Color(255,  0,      0,      255),   // Type0
////                sf::Color(0,    255,    0,      255),   // Type1
////                sf::Color(0,    255,    255,    255),   // Type2
////                sf::Color(255,  255,    0,      255),   // Type3
////                sf::Color(255,  0,      255,    255)    // Type4
////            };
////            const sf::Color &color_shape_bb = colors_shapes_bb[i];
////            std::cout << "i: " << i << std::endl;
////    ...
//}
