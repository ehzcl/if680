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

    int GetOutcode(vec2 &point, float x_min, float x_max, float y_min, float y_max) {
        int outcode = 0;
        if(point.y() > y_max) {
            outcode += 8;
        }
        if(point.y() < y_min) {
            outcode += 4;
        }
        if(point.x() > x_max) {
            outcode += 2;
        }
        if(point.x() < x_min) {
            outcode += 1;
        }
        return outcode;
    }

    void calculate_intersection(vec2 &p0, vec2 &p1, vec2 &dst, float xm, float ym) {
        float slope;

        slope = (p1.y() - p0.y())/(p1.x() - p0.x());

        dst.e[0] = p0.x() + (1.0/slope)*(ym - p0.y());
        dst.e[1] = p0.y() + slope * (xm - p0.x());
    }

    bool ClipLine(vec2 &p0, vec2 &p1) {
        bool seen = false;
        int ocp0 = GetOutcode(p0, left, imgWidth, bottom, imgHeight);
        int ocp1 = GetOutcode(p0, left, imgWidth, bottom, imgHeight);
        int OutsideSection;
        vec2 dst;

        while(true) {
            if(ocp0 | ocp1 == 0) {
                seen = true;
                break;
            }
            else {
                if(ocp0 & ocp1 != 0){
                    seen = false;
                    break;
                }
                else {
                    OutsideSection = (ocp1 != 0)? ocp1 : ocp0;
                    
                    if(OutsideSection & 8) { // TOP
                        calculate_intersection(p0, p1, dst, left,imgHeight);
                    }
                    if(OutsideSection & 4) { // BOTTOM
                        calculate_intersection(p0, p1, dst, left,bottom);
                    }
                    if(OutsideSection & 2) { // RIGHT
                        calculate_intersection(p0, p1, dst, imgWidth,imgHeight);
                    }
                    if(OutsideSection & 1) { // LEFT
                        calculate_intersection(p0, p1, dst, left,imgHeight);
                    }
                    if(OutsideSection == ocp0) {
                        p0.e[0] = dst.e[0];
                        p0.e[1] = dst.e[1];
                        ocp0 = GetOutcode(p0, left, imgWidth, bottom, imgHeight);
                    } else {
                        p1.e[0] = dst.e[0];
                        p1.e[1] = dst.e[1];
                        ocp1 = GetOutcode(p0, left, imgWidth, bottom, imgHeight);
                    }
                }
            }
        }
        return seen;
    }

    // nova função para desenhar na tela
    void DrawLine(SDL_Renderer *renderer, vec2 &p0, vec2 &p1) {
        vec2 tmp = p0 - p1;
        int tmp_size = tmp.length();
        tmp.make_unit_vector();
        vec2 start = p1;

        if(ClipLine(p0, p1)) {
            for(int i = 0 ; i <= tmp_size; i++) {
                SDL_RenderDrawPoint(renderer, start.x(),start.y());
                start += tmp;
            }
        }
        
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

                // if (v1 && v2)
                //     SDL_RenderDrawLine(renderer, praster1.x(), praster1.y(), praster2.x(), praster2.y());
                // if (v1 && v3)
                //     SDL_RenderDrawLine(renderer, praster1.x(), praster1.y(), praster3.x(), praster3.y());
                // if (v2 && v3)
                //     SDL_RenderDrawLine(renderer, praster2.x(), praster2.y(), praster3.x(), praster3.y());
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
