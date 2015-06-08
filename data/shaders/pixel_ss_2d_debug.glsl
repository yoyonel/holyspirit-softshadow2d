// --------------------------------------------------------------------------------
// file: pixel_ss_2d_debug.glsl
// --------------------------------------------------------------------------------
// action:
// --------------------------------------------------------------------------------

// wil: wall in light, le segment-mur est a  l'interieur de la source de lumiere
#define NORMAL(V2)                  vec2(-(V2).y, (V2).x)
#define EQUAL0_EPS(_f, _eps)        ((_f)>=-(_eps) && (_f)<=+(_eps))
#define EQUAL_EPS(_f1, _f2, _eps)   (((_f1)-(_f2))>=-(_eps) && ((_f1)-(_f2))<=+(_eps))
#define EPSILON                     0.001
#define M_PI                        3.14159

#define NORMALIZE                   normalize
#define sinf                        sin
#define DOT                         dot
#define M_PI                        3.14159
#define SIGN                        sign

#define __DRAW_VERTEX_DEBUG__

uniform vec2    u_v_position_light;
uniform float   u_f_radius_light;
uniform float   u_f_influence_radius_light;
//
uniform vec2    u_v_receiver;
uniform vec2    u_v_e0;
uniform vec2    u_v_e1;
uniform vec2    u_v_i0;
uniform vec2    u_v_i1;
//
uniform float    u_f_render_disc_portion;

//
varying vec2 v_v_position;


void    tests_for_discard_texel( in vec2 P );
vec2    compute_projection_on_circle( in vec2 E, in vec2 P, in float r, in float inv_r );
float   signed_distance_point_line( in vec2 P, in vec2 A, in vec2 B );
bool    inside_half_plane(in vec2 A, in vec2 B, in vec2 P);

vec4    render_disc_portion_proj_i0_proj_i1(in vec2 I0, in vec2 I1, in vec2 receiver_in_ls, in vec2 pixel_in_ls, in float r, in float inv_r);
vec4    render_disc_portion_proj_e0_proj_e1(in vec2 E0, in vec2 E1, in vec2 receiver_in_ls, in vec2 pixel_in_ls, in float r, in float inv_r);
vec4    render_disc_portion_i0_proj_i0(in vec2 I0, in vec2 I1, in vec2 E0, in vec2 receiver_in_ls, in vec2 pixel_in_ls, in float r, in float inv_r);
vec4    render_disc_portion_i1_proj_i1(in vec2 I0, in vec2 I1, in vec2 E1, in vec2 receiver_in_ls, in vec2 pixel_in_ls, in float r, in float inv_r);

void main()
{
    vec2    v_pos_in_ls         = v_v_position - u_v_position_light; // position du vertex edge dans le repere lumiere (Light Space)
    vec2    v_pos_light_in_ls   = vec2(0.); // origine du repere

    float   r = u_f_radius_light;
    float   inv_r = 1./r;
    //
    vec2    E0 = u_v_e0;
    vec2    E1 = u_v_e1;
    vec2    I0 = u_v_i0;
    vec2    I1 = u_v_i1;
    //
    vec2    P  = u_v_receiver;

    // Tests pour rejeter rapidement (le plus possible)
    // les points non inclus dans la zone d'influence de penombre de l'araªte par rapport au cercle de lumiere
    tests_for_discard_texel( v_pos_in_ls );

    vec4 color = vec4(0);

    if (u_f_render_disc_portion == 0.0)
    {
        color = vec4(0, 1, 0, 1);
    }
    else if (u_f_render_disc_portion == 1.0) // Portion de disque: Proj_E0, Proj_E1
    {
        color = render_disc_portion_proj_e0_proj_e1(E0, E1, P, v_pos_in_ls, r, inv_r);
    }
    else if (u_f_render_disc_portion == 2.0) // Portion de disque: Proj_I0, Proj_I1
    {
        color = render_disc_portion_proj_i0_proj_i1(I0, I1, P, v_pos_in_ls, r, inv_r);
    }
    else if (u_f_render_disc_portion == 3.0) // Portion de disque: I0, Proj_I0
    {
        color = render_disc_portion_i0_proj_i0(I0, I1, E0, P, v_pos_in_ls, r, inv_r);
    }
    else if (u_f_render_disc_portion == 4.0) // Portion de disque: I1, Proj_I1
    {
        color = render_disc_portion_i1_proj_i1(I0, I1, E1, P, v_pos_in_ls, r, inv_r);
    }

    // write result
    gl_FragColor = color;
}

void tests_for_discard_texel( in vec2 P )
{
    // inside influence circle ?
    if ( dot(P, P) > u_f_influence_radius_light*u_f_influence_radius_light )
        discard;
    if ( dot(P, P) > (u_f_radius_light*u_f_radius_light))
        discard;
}

vec2 compute_projection_on_circle( in vec2 E, in vec2 P, in float r, in float inv_r )
{
    float   f_signed_distance, d, a;
    vec2    x, y, Proj_E_P;

    vec2 pos_light = vec2(0., 0.);

    f_signed_distance = signed_distance_point_line( pos_light, P, E );
    // [TODO]: a  simplifier !
    d = clamp(f_signed_distance*inv_r, -1, +1);
    a = acos(d);
    y = NORMALIZE(E-P);
    x = NORMAL(y);
    Proj_E_P = pos_light + x*d*r + y*sinf(a)*r;

    return Proj_E_P;
}

// Repere main droite pour retrouver le sens de la normale de la droite (son 'Z')
float signed_distance_point_line( in vec2 P, in vec2 A, in vec2 B )
{
    vec2 AB = B - A;

    vec2 v = NORMALIZE(NORMAL(AB));
    vec2 r = A - P;

    return DOT(v, r);
}

bool inside_half_plane(in vec2 A, in vec2 B, in vec2 P)
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
    return (-side_of_P < 0.0);
}

vec4 render_disc_portion_proj_i0_proj_i1(in vec2 I0, in vec2 I1, in vec2 receiver_in_ls, in vec2 pixel_in_ls, in float r, in float inv_r)
{
    vec4 color;

    vec2 Proj_I0, Proj_I1;
    //
    Proj_I0 = compute_projection_on_circle( I0, receiver_in_ls, r, inv_r );
    Proj_I1 = compute_projection_on_circle( I1, receiver_in_ls, r, inv_r );
    //
    bool b_inside_half_plane_0 = inside_half_plane(Proj_I0, Proj_I1, receiver_in_ls) ? inside_half_plane(Proj_I1, Proj_I0, pixel_in_ls): inside_half_plane(Proj_I0, Proj_I1, pixel_in_ls) ;

    if (b_inside_half_plane_0)
        color+=vec4(0.8, 0.4, 0.1, 1);
    else
        discard;

    // debug
    #ifdef __DRAW_VERTEX_DEBUG__
        color += vec4(1, 0, 0, 1) * 1/(distance(Proj_I0, pixel_in_ls)*0.250);
        color += vec4(0, 1, 0, 1) * 1/(distance(Proj_I1, pixel_in_ls)*0.250);
    #endif

    return color;
}

vec4 render_disc_portion_i0_proj_i0(in vec2 I0, in vec2 I1, in vec2 E0, in vec2 receiver_in_ls, in vec2 pixel_in_ls, in float r, in float inv_r)
{
    vec4 color;
    bool b_inside_half_plan[3];

    vec2 I0_I1 = I1 - I0;
    vec2 n_I0_I1 = normalize(NORMAL(I0_I1));
    vec2 n_I0 = NORMAL(I0);
    vec2 Proj_I0 = compute_projection_on_circle( I0, receiver_in_ls, r, inv_r );
    vec2 Proj_E0 = compute_projection_on_circle( E0, receiver_in_ls, r, inv_r );
    vec2 n_I0_Proj_I0 = NORMAL(Proj_I0 - I0);
    vec2 n_E0_Proj_E0 = NORMAL(Proj_E0 - E0);

    // Est ce que l'araªte E0 I0 occlude la light par rapport au texel receiver ?
    b_inside_half_plan[0] = dot(I0 - receiver_in_ls, I0) < 0;

    n_I0_Proj_I0 = dot(n_I0_Proj_I0, I1 - I0) < 0 ? n_I0_Proj_I0 : -n_I0_Proj_I0;
    b_inside_half_plan[1] = dot( pixel_in_ls - I0, n_I0_Proj_I0) > 0;

    n_E0_Proj_E0 = dot(n_E0_Proj_E0, I1 - I0) > 0 ? n_E0_Proj_E0 : -n_E0_Proj_E0;
    b_inside_half_plan[2] = abs(signed_distance_point_line(vec2(0, 0), E0, receiver_in_ls)) > r ? true : dot( pixel_in_ls - E0, n_E0_Proj_E0) > 0;

    if (b_inside_half_plan[0] && b_inside_half_plan[1] && b_inside_half_plan[2])
        color+= vec4(0.4, 0.8, 0.1, 1);
    else
        discard;

    // debug
    #ifdef __DRAW_VERTEX_DEBUG__
        color += vec4(1, 0, 0, 1) * 1/(distance(Proj_I0, pixel_in_ls)*0.250);
        color += vec4(1, 0, 0, 1) * 1/(distance(Proj_E0, pixel_in_ls)*0.250);
        color += vec4(1, 0, 0, 1) * 1/(distance(E0, pixel_in_ls)*0.250);
    #endif

    return color;
}

vec4 render_disc_portion_proj_e0_proj_e1(in vec2 E0, in vec2 E1, in vec2 receiver_in_ls, in vec2 pixel_in_ls, in float r, in float inv_r)
{
    return render_disc_portion_proj_i0_proj_i1(E0, E1, receiver_in_ls, pixel_in_ls, r, inv_r);
}

vec4 render_disc_portion_i1_proj_i1(in vec2 I0, in vec2 I1, in vec2 E1, in vec2 receiver_in_ls, in vec2 pixel_in_ls, in float r, in float inv_r)
{
    vec4 color;
    bool b_inside_half_plan[3];

    vec2 I0_I1 = I1 - I0;
    vec2 n_I0_I1 = normalize(NORMAL(I0_I1));
    vec2 n_I1 = NORMAL(I1);
    vec2 Proj_I1 = compute_projection_on_circle( I1, receiver_in_ls, r, inv_r );
    vec2 Proj_E1 = compute_projection_on_circle( E1, receiver_in_ls, r, inv_r );
    vec2 n_I1_Proj_I1 = NORMAL(Proj_I1 - I1);
    vec2 n_E1_Proj_E1 = NORMAL(Proj_E1 - E1);

    // Est ce que l'araªte E0 I0 occlude la light par rapport au texel receiver ?
    b_inside_half_plan[0] = dot(I1 - receiver_in_ls, I1) < 0;

    n_I1_Proj_I1 = dot(n_I1_Proj_I1, I1 - I0) < 0 ? n_I1_Proj_I1 : -n_I1_Proj_I1;
    b_inside_half_plan[1] = dot( pixel_in_ls - I1, n_I1_Proj_I1) < 0;

    n_E1_Proj_E1 = dot(n_E1_Proj_E1, I1 - I0) > 0 ? n_E1_Proj_E1 : -n_E1_Proj_E1;
    b_inside_half_plan[2] = abs(signed_distance_point_line(vec2(0, 0), E1, receiver_in_ls)) > r ? true : dot( pixel_in_ls - E1, n_E1_Proj_E1) < 0;

    if (b_inside_half_plan[0] && b_inside_half_plan[1] && b_inside_half_plan[2])
        color+= vec4(0.4, 0.8, 0.1, 1);
    else
        discard;

    // debug
    #ifdef __DRAW_VERTEX_DEBUG__
//        color += vec4(1, 0, 0, 1) * 1/(distance(Proj_I1, pixel_in_ls)*0.250);
        color += vec4(1, 0, 0, 1) * 1/(distance(Proj_E1, pixel_in_ls)*0.250);
        color += vec4(1, 0, 0, 1) * 1/(distance(E1, pixel_in_ls)*0.250);
    #endif

    return color;
}
