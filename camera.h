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

const int WIDTH = 600;
const int HEIGHT = 400;
const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

float frameBuffer[WIDTH][HEIGHT];
float zBuffer[WIDTH][HEIGHT];

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
        vec3 dir(0.0f, 0.0f, -1.0f);
        vec3 componente_ambiente(40, 40, 40);
        float L = -dot(unit_vector(normal), dir);
        float cos = std::max(0.0f, L);
        vec3 componenteDifusa = cos * Kd;
        vec3 R = dir - 2 * (dot(normal, dir)) * dir;
        vec3 componente_especular = Ks * (pow(std::max(dot(axisZ, R), 0.0f), n));
        float aux = 1.0f - textura.y();
        int x = obj.texture_width * textura.x();
        int y = obj.texture_height * aux;
        vec3 cor = obj.texture_buffer[y * obj.texture_width + x];

        return componenteDifusa + componente_ambiente + componente_especular;
    }

    float edge(const vec2 &v0, const vec2 &v1, const vec2 &p)
    {
        return ((p.e[0] - v0.e[0]) * (v1.e[1] - v0.e[1]) - (v1.e[0] - v0.e[0]) * (p.e[1] - v0.e[1]));
    }

    void fill_triangle(SDL_Renderer *renderer, const vec2 &v0, const vec2 &v1, const vec2 &v2, vec3 &z, Vertex vtx)
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
                temp = vec2(o, i);

                wa = edge(v0, v1, temp);
                wb = edge(v1, v2, temp);
                wc = edge(v2, v0, temp);

                vec3 Kd(30, 30, 30);
                vec3 Ks(50, 50, 50);
                float n = 4.0f;

                if (wa >= 0 && wb >= 0 && wc >= 0)
                {
                    wa /= area;
                    wb /= area;
                    wc /= area;

                    float actual_z = (wb * (1.0 / z[0]) + wc * (1.0 / z[1]) + wa * (1.0 / z[2]));
                    actual_z = 1.0 / actual_z;

                    if (actual_z < zBuffer[i][o])
                    {
                        zBuffer[i][o] = actual_z;
                        vec3 color(210, 210, 210);
                        vec2 actual_texture = wb * vtx.text + wc * vtx.text + wa * vtx.text;
                        vec3 actual_normal = wb * vtx.nor + wc * vtx.nor + wa * vtx.nor;
                        SDL_SetRenderDrawColor(renderer, color.r(), color.g(), color.b(), 255);
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

        vec3 light(0.0f, 0.0f, -1.0f);
        light.make_unit_vector();
        for (int i = 0; i < imgWidth; i++)
        {
            for (int o = 0; i < imgHeight; o++)
            {
                zBuffer[i][o] = INFINITY;
            }
        }
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

                if (v1 && v2)
                    SDL_RenderDrawLine(renderer, praster1.x(), praster1.y(), praster2.x(), praster2.y());
                if (v1 && v3)
                    SDL_RenderDrawLine(renderer, praster1.x(), praster1.y(), praster3.x(), praster3.y());
                if (v2 && v3)
                    SDL_RenderDrawLine(renderer, praster2.x(), praster2.y(), praster3.x(), praster3.y());
                if (insideScreen(praster1) && insideScreen(praster2) && insideScreen(praster3))
                    fill_triangle(renderer, praster1, praster2, praster3, z);
            }
        }
    }
};

#endif
