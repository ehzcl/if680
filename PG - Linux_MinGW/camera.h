#ifndef CAMERAH
#define CAMERAH

#include "vec3.h"
#include "vec2.h"
#include "matrix44.h"
#include "object.h"

#ifdef _WIN32 || WIN32
#include <SDL.h>
#elif defined(__unix__)
#include <SDL2/SDL.h>
#endif

const int WIDTH = 600;
const int HEIGHT = 400;
const int INSIDE = 0;   // 0000
const int LEFT = 1;     // 0001
const int RIGHT = 2;    // 0010
const int BOTTOM = 4;   // 0100
const int TOP = 8;      // 1000

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

    /*  DOT() = Produto interno
        CROSS() = Produto vetorial
        MAKE_UNIT_VECTOR() = Normalização */
    void look_at(const vec3 &from, const vec3 &at, const vec3 &up)
    {
        axisZ = from - at;
        axisZ.make_unit_vector();

        axisY = up - ((dot(up,axisZ)/dot(axisZ,axisZ))*axisZ);
        axisY.make_unit_vector();

        axisX = cross(axisY, axisZ);
        axisX.make_unit_vector();

        float i = 1;
        
        camToWorld = matrix44(
            axisX.x(), axisX.y(), axisX.z(), 0,
            axisY.x(), axisY.y(), axisY.z(), 0,
            axisZ.x(), axisZ.y(), axisZ.z(), 0,
            from.x(), from.y(), from.z(), 1
        );

        worldToCamera = camToWorld.inverse();
    }

    bool compute_pixel_coordinates(const vec3 &pWorld, vec2 &pRaster)
    {
        vec3 screen, projPerspectiva, pJanela;
        
        if(pWorld.z() >= _from.z() ){   // pois o Z é negativo
            return false;
        }

        worldToCamera.mult_point_matrix(pWorld,screen);

        projPerspectiva = vec3(
                                screen.x()*(_near/screen.z()),
                                screen.y()*(_near/screen.z()),
                                _near);

        matrix44 applicationWindowMatrix = matrix44(
            (2*_near)/(right-left), 0,0,0,
            0,(2*_near)/(bottom-top),0,0,
            -(right+left)/(right-left), -(bottom+top)/(bottom-top),(_far+_near)/(_far-_near),1,
            0,0,-(2*_near)/(_far-_near),0
        );
        
        applicationWindowMatrix.mult_point_matrix(projPerspectiva,pJanela); // normalizando o ponto para mapear para janela da aplicação

        float aspect_ratio = (float)imgWidth/(float)imgHeight;
        top = tan((fov/2)*(M_PI/180.0));
        right = top*aspect_ratio;
        left = -right;
        bottom = -top;

        pRaster = vec2(
                        (1+pJanela.x())/2*imgWidth,
                        (1-pJanela.y())/2*imgHeight
        );
        
        if(pJanela.x() >= left && pJanela.x() <= right && pJanela.y() >= bottom && pJanela.y() <= top){
            return true;    // o ponto pode ser visto
        }
        return false; // o ponto não pode ser visto
    }

    void DrawLine(SDL_Renderer* renderer, vec2 &p0, vec2 &p1) {
        vec2 director = p1 - p0;
        vec2 start = p0;
        int iterations =(int)director.length();
        director.make_unit_vector();

        if(ClipLine(p0,p1)){
            for(int i = 0 ; i < iterations; i++) {
                SDL_RenderDrawPoint(renderer, start.e[0], start.e[1]);
                start += director; 
            }
        } 
    }

    bool ClipLine(vec2 &p0, vec2 &p1) {

        int outp0 = GetOutCode(p0);
        int outp1 = GetOutCode(p1);
        int slope = 0;
        int tmp_x, tmp_y = 0;

        bool accept = false;
        int outcodeOutside = 0;

        while(true) {
            if(outp0 | outp1 == 0) {
                accept = true;
                break;
            }
            else if(outp0 & outp1) {
                break;
            }else {
                outcodeOutside = outp1 != 0? outp1: outp0;

                if(outcodeOutside & TOP) {
                    tmp_x = p0.e[0] + (p1.e[0] - p0.e[0]) * (HEIGHT - p0.e[1]) / (p1.e[1] - p0.e[1]);
                    tmp_y = HEIGHT;
                } else if(outcodeOutside & BOTTOM) {
                    tmp_x = p0.e[0] + (p1.e[0] - p0.e[0]) * (0 - p0.e[1]) / (p1.e[1] - p0.e[1]);
                    tmp_y = 0;
                } if(outcodeOutside & RIGHT) {
                    tmp_y = p0.e[1] + (p1.e[1] - p0.e[1]) * (WIDTH - p0.e[0]) / (p1.e[0] - p0.e[0]);
                    tmp_x = WIDTH;
                } else if(outcodeOutside & LEFT) {
                    tmp_y = p0.e[1] + (p1.e[1] - p0.e[1]) * (0 - p0.e[0]) / (p1.e[0] - p0.e[0]);
                    tmp_x = 0;
                }

                if(outcodeOutside == outp0) {
                    p0.e[0] = tmp_x;
                    p0.e[1] = tmp_y;
                    outp0 = GetOutCode(p0);
                } else {
                    p1.e[0] = tmp_x;
                    p1.e[1] = tmp_y;
                    outp1 = GetOutCode(p1);
                }
            }
        }
        return accept;
    }

    int GetOutCode (vec2 &p0) {
        int outcode = 0;

        if(p0.e[1] > HEIGHT) {
            outcode |= TOP;
        } else if(p0.e[1] < 0) {
            outcode |= BOTTOM;
        }
        if(p0.e[0] > WIDTH) {
            outcode |= RIGHT;
        } else if(p0.e[0] < 0) {
            outcode |= LEFT;
        }

        return outcode;
    }

    void render_scene(std::vector<Obj> objs, SDL_Renderer *renderer)
    {

        vec3 light(0.0f, 0.0f, -1.0f);
        light.make_unit_vector();

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
                v1 = compute_pixel_coordinates(obj.mesh.tris[i].vertex[0].pos, praster1);
                v2 = compute_pixel_coordinates(obj.mesh.tris[i].vertex[1].pos, praster2);
                v3 = compute_pixel_coordinates(obj.mesh.tris[i].vertex[2].pos, praster3);

                if (v1 && v2)
                    DrawLine(renderer, praster1, praster2);
                if (v1 && v3)
                    DrawLine(renderer, praster1, praster3);
                if (v2 && v3)
                    DrawLine(renderer, praster2, praster3);
            }
        }
    }
};

#endif
