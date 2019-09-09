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

class camera
{
public:
    int imgWidth, imgHeight;
    float fov, _near;
    float bottom, left, top, right;
    matrix44 camToWorld;
    matrix44 worldToCamera;

	vec3 _from, _at, _up;
    vec3 axisX, axisY, axisZ;

public:
    camera();
    camera(const vec3 &from, const vec3 &at, const vec3 &up,
           const float &f, const float &n,
           const int &iwidth, const int &iheight): 
           fov(f), _near(n), imgWidth(iwidth), imgHeight(iheight),
		   _from(from), _at(at), _up(up)
           {
                look_at(from, at, up);
           }

    /*  DOT = Produto interno
        CROSS = Produto vetorial
        NORMALIZAR = make_unit_vector */
    void look_at(const vec3 &from, const vec3 &at, const vec3 &up)
    {
        axisZ = from - at;
        axisZ.make_unit_vector();
    }

    bool compute_pixel_coordinates(const vec3 &pWorld, vec2 &pRaster) 
    { 
        return false; // Retornar verdadeiro se o ponto pode ser visto
    }

    void render_scene( std::vector<Obj> objs, SDL_Renderer* renderer) {

        vec3 light(0.0f, 0.0f, -1.0f);
        light.make_unit_vector();

        for (auto obj : objs){
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

                if(v1 && v2)
                    SDL_RenderDrawLine(renderer, praster1.x(), praster1.y(), praster2.x(), praster2.y());
                if(v1 && v3)
                    SDL_RenderDrawLine(renderer, praster1.x(), praster1.y(), praster3.x(), praster3.y());
                if(v2 && v3)
                    SDL_RenderDrawLine(renderer, praster2.x(), praster2.y(), praster3.x(), praster3.y());
            }
        }
    }
};


#endif