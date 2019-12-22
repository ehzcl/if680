#ifndef CAMERAH
#define CAMERAH
#include <limits.h>
#include "vec3.h"
#include "vec2.h"
#include "matrix44.h"
#include "object.h"
#include "algorithm"
#ifdef _WIN32 || WIN32
#include <SDL.h>
#elif defined(__unix__)
#include <SDL2/SDL.h>
#endif

#define INF (unsigned)!((int)0)

const int WIDTH = 600;
const int HEIGHT = 400;
const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

float frameBuffer[WIDTH][HEIGHT];
float zBuffer[WIDTH * HEIGHT];

class camera
{
public:
    int imgWidth, imgHeight;
    float fov, _near, _far;
    float bottom, left, top, right;
    matrix44 camToWorld;
    matrix44 worldToCamera;

    vec3 _from, _at, _up;
    vec3 axisX, axisY, axisZ;

public:
    camera();
    camera(const vec3 &from, const vec3 &at, const vec3 &up,
           const float &f, const float &n, const float &far,
           const int &iwidth, const int &iheight) : fov(f), _near(n), imgWidth(iwidth), imgHeight(iheight),
                                                    _from(from), _at(at), _up(up), _far(far)
    {
        look_at(from, at, up);
    }

    void look_at(const vec3 &from, const vec3 &at, const vec3 &up)
    {
        axisZ = from - at;
        axisZ.make_unit_vector();

        axisY = up - ((dot(up, axisZ) / dot(axisZ, axisZ)) * axisZ);
        axisY.make_unit_vector();

        axisX = cross(axisY, axisZ);
        axisX.make_unit_vector();

        float i = 1;

        camToWorld = matrix44(
            axisX.x(), axisX.y(), axisX.z(), 0,
            axisY.x(), axisY.y(), axisY.z(), 0,
            axisZ.x(), axisZ.y(), axisZ.z(), 0,
            from.x(), from.y(), from.z(), 1);

        worldToCamera = camToWorld.inverse();
    }

    bool compute_pixel_coordinates(const vec3 &pWorld, vec2 &pRaster, float &z)
    {
        vec3 screen, projPerspectiva, pJanela;

        if (pWorld.z() >= _from.z())
        { // pois o Z é negativo
            return false;
        }

        worldToCamera.mult_point_matrix(pWorld, screen);

        z = screen.e[2];

        projPerspectiva = vec3(
            screen.x() * (_near / screen.z()),
            screen.y() * (_near / screen.z()),
            _near);

        matrix44 applicationWindowMatrix = matrix44(
            (2 * _near) / (right - left), 0, 0, 0,
            0, (2 * _near) / (bottom - top), 0, 0,
            -(right + left) / (right - left), -(bottom + top) / (bottom - top), (_far + _near) / (_far - _near), 1,
            0, 0, -(2 * _near) / (_far - _near), 0);

        applicationWindowMatrix.mult_point_matrix(projPerspectiva, pJanela); // normalizando o ponto para mapear para janela da aplicação

        float aspect_ratio = (float)imgWidth / (float)imgHeight;
        top = tan((fov / 2) * (M_PI / 180.0));
        right = top * aspect_ratio;
        left = -right;
        bottom = -top;

        pRaster = vec2(
            (1 + pJanela.e[0]) / 2 * imgWidth,
            (1 - pJanela.e[1]) / 2 * imgHeight);

        if (pJanela.x() >= left && pJanela.x() <= right && pJanela.y() >= bottom && pJanela.y() <= top)
        {
            return true; // o ponto pode ser visto
        }
        return false; // o ponto não pode ser visto
    }

    void DrawLine(SDL_Renderer *renderer, vec2 &p0, vec2 &p1)
    {
        vec2 director = p1 - p0;
        vec2 start = p0;
        int iterations = (int)director.length();
        director.make_unit_vector();

        if (ClipLine(p0, p1))
        {
            for (int i = 0; i < iterations; i++)
            {
                SDL_RenderDrawPoint(renderer, start.e[0], start.e[1]);
                start += director;
            }
        }
    }

    int GetOutCode(vec2 &p0)
    {
        int outcode = 0;

        if (p0.e[1] > HEIGHT)
        {
            outcode |= TOP;
        }
        else if (p0.e[1] < 0)
        {
            outcode |= BOTTOM;
        }
        if (p0.e[0] > WIDTH)
        {
            outcode |= RIGHT;
        }
        else if (p0.e[0] < 0)
        {
            outcode |= LEFT;
        }

        return outcode;
    }

    bool intersecao(vec3 N, vec3 RayDirection)
    {
        double EPSILON = 0.00001;
        double i = dot(N, RayDirection);

        if (i >= EPSILON)
            return true; // houve intersecao
        else
            return false; // nao houve intersecao
    }

    bool ClipLine(vec2 &p0, vec2 &p1)
    {

        int outp0 = GetOutCode(p0);
        int outp1 = GetOutCode(p1);
        int slope = 0;
        int tmp_x, tmp_y = 0;

        bool accept = false;
        int outcodeOutside = 0;

        while (true)
        {
            if (outp0 | outp1 == 0)
            {
                accept = true;
                break;
            }
            else if (outp0 & outp1)
            {
                break;
            }
            else
            {
                outcodeOutside = outp1 != 0 ? outp1 : outp0;

                if (outcodeOutside & TOP)
                {
                    tmp_x = p0.e[0] + (p1.e[0] - p0.e[0]) * (HEIGHT - p0.e[1]) / (p1.e[1] - p0.e[1]);
                    tmp_y = HEIGHT;
                }
                else if (outcodeOutside & BOTTOM)
                {
                    tmp_x = p0.e[0] + (p1.e[0] - p0.e[0]) * (0 - p0.e[1]) / (p1.e[1] - p0.e[1]);
                    tmp_y = 0;
                }
                if (outcodeOutside & RIGHT)
                {
                    tmp_y = p0.e[1] + (p1.e[1] - p0.e[1]) * (WIDTH - p0.e[0]) / (p1.e[0] - p0.e[0]);
                    tmp_x = WIDTH;
                }
                else if (outcodeOutside & LEFT)
                {
                    tmp_y = p0.e[1] + (p1.e[1] - p0.e[1]) * (0 - p0.e[0]) / (p1.e[0] - p0.e[0]);
                    tmp_x = 0;
                }

                if (outcodeOutside == outp0)
                {
                    p0.e[0] = tmp_x;
                    p0.e[1] = tmp_y;
                    outp0 = GetOutCode(p0);
                }
                else
                {
                    p1.e[0] = tmp_x;
                    p1.e[1] = tmp_y;
                    outp1 = GetOutCode(p1);
                }
            }
        }
        return accept;
    }

    vec3 phong(vec3 normal, vec3 Kd, vec3 Ks, float n, Obj obj, vec2 textura)
    {
        vec3 dir(0.0, 0.0f, -1.0f);
        vec3 componente_ambiente(120, 120, 120);
        dir.make_unit_vector();
        float L = dot(-normal, dir);
        float cos = L;
        vec3 componente_difusa = cos * Kd;
        vec3 R = dir - 2 * (dot(normal, dir)) * normal;
        vec3 componente_especular = Ks * (pow(dot(axisZ, R), n));
        if (textura.length() != 0)
        {
            float aux = 1.0f - textura.y();
            int x = obj.texture_width * textura.x();
            int y = obj.texture_height * aux;
            vec3 cor = obj.texture_buffer[y * obj.texture_width + x];
        }

        return (componente_difusa + componente_ambiente + componente_especular);
    }

    float edge(vec2 p1, vec2 p2, vec2 p3)
    {
        vec2 p = p2 - p1;
        vec2 r = p3 - p1;
        return (p.x() * r.y()) - (r.x() * p.y());
    }

    int value_at(int x, int y)
    {
        return y * WIDTH + x;
    }

    void fill_triangle(SDL_Renderer *renderer, const vec2 &v0, const vec2 &v1, const vec2 &v2, vec3 &z, Triangle tr, Obj objeto)
    {
        vec2 min, max, temp;
        int min_Y, min_X, max_Y, max_X;
        float wa, wb, wc, area;

        max_X = (v0.e[0] > v1.e[0]) ? ((v0.e[0] > v2.e[0]) ? v0.e[0] : v2.e[0]) : ((v1.e[0] > v2.e[0]) ? v1.e[0] : v2.e[0]);
        max_Y = (v0.e[1] > v1.e[1]) ? ((v0.e[1] > v2.e[1]) ? v0.e[1] : v2.e[1]) : ((v1.e[1] > v2.e[1]) ? v1.e[1] : v2.e[1]);
        min_X = (v0.e[0] < v1.e[0]) ? ((v0.e[0] < v2.e[0]) ? v0.e[0] : v2.e[0]) : ((v1.e[0] < v2.e[0]) ? v1.e[0] : v2.e[0]);
        min_Y = (v0.e[1] < v1.e[1]) ? ((v0.e[1] < v2.e[1]) ? v0.e[1] : v2.e[1]) : ((v1.e[1] < v2.e[1]) ? v1.e[1] : v2.e[1]);

        area = edge(v0, v1, v2);
        for (int o = min_Y; o <= max_Y; o++)
        {
            for (int i = min_X; i <= max_X; i++)
            {
                temp = vec2(i, o);

                wc = edge(v0, v1, temp);
                wa = edge(v1, v2, temp);
                wb = edge(v2, v0, temp);

                vec3 Kd(30, 30, 30);
                vec3 Ks(50, 50, 50);
                float n = 4;

                if (wa >= 0 && wb >= 0 && wc >= 0)
                {
                    wa /= area;
                    wb /= area;
                    wc /= area;

                    float actual_z = (wa * (1.0 / z[0]) + wb * (1.0 / z[1]) + wc * (1.0 / z[2]));

                    if (actual_z < zBuffer[value_at(i, o)])
                    {
                        zBuffer[value_at(i, o)] = actual_z;
                        vec3 normal = wa * tr.vertex[0].nor + wb * tr.vertex[1].nor + wc * tr.vertex[2].nor;
                        vec2 text = wa * tr.vertex[0].text + wb * tr.vertex[1].text + wc * tr.vertex[2].text;
                        vec3 color = phong(normal, Kd, Ks, n, objeto, text);
                        color[0] = std::min(color[0], 255.f);
                        color[1] = std::min(color[1], 255.f);
                        color[2] = std::min(color[2], 255.f);
                        SDL_SetRenderDrawColor(renderer, color.x(), color.y(), color.z(), 255);
                        SDL_RenderDrawPoint(renderer, i, o);
                    }
                }
            }
        }
    }

    bool insideScreen(const vec2 &v0)
    {
        return ((v0.e[0] >= 0) && (v0.e[0] <= WIDTH) && (v0.e[1] >= 0) && (v0.e[1] <= HEIGHT));
    }

    void render_scene(std::vector<Obj> objs, SDL_Renderer *renderer)
    {
        std::fill(std::begin(zBuffer), std::begin(zBuffer) + 240000, 5000);
        for (auto obj : objs)
        {
            for (int i = 0; i < obj.mesh.tris.size(); i++)
            {
                vec2 praster1;
                vec2 praster2;
                vec2 praster3;

                vec3 col(255, 255, 255);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

                bool v1, v2, v3;
                vec3 z;

                v1 = compute_pixel_coordinates(obj.mesh.tris[i].vertex[0].pos, praster1, z.e[0]);
                v2 = compute_pixel_coordinates(obj.mesh.tris[i].vertex[1].pos, praster2, z.e[1]);
                v3 = compute_pixel_coordinates(obj.mesh.tris[i].vertex[2].pos, praster3, z.e[2]);

                if (praster1.x() >= 0 & praster1.x() <= WIDTH & praster1.y() >= 0 & praster1.y() <= HEIGHT & praster2.x() >= 0 & praster2.x() <= WIDTH & praster2.y() >= 0 & praster2.y() <= HEIGHT & praster3.x() >= 0 & praster3.x() <= WIDTH & praster3.y() >= 0 & praster3.y() < HEIGHT)
                    fill_triangle(renderer, praster1, praster2, praster3, z, obj.mesh.tris[i], obj);
            }
        }
    }
};

#endif
