#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <stdlib.h>
#include <algorithm>

#define DPS 1000
#define WIND_SECONDS 10
#define FPS 60




std::vector<double> wind = {0, 0, 0};
unsigned int last_wind_change;

double gravity = 2;

void compose_screen();

void main_loop();

void event_handler();

template<typename T>
void vec_add(const std::vector<T> & v1, const std::vector<T> & v2, std::vector<T> & vres)
{
    for(size_t i = 0; i<v1.size(); i++)
    {
        vres[i] = v1[i] + v2[i];
    }
}

template<typename T>
void vec_sub(const std::vector<T> & v1, const std::vector<T> & v2, std::vector<T> & vres)
{
    for(size_t i = 0; i<v1.size(); i++)
    {
        vres[i] = v1[i] - v2[i];
    }
}

template<typename T>
void vec_mult(const std::vector<T> & v1, const T & scalar, std::vector<T> & vres)
{
    for(size_t i = 0; i<v1.size(); i++)
    {
        vres[i] = v1[i] * scalar;
    }
}

template<typename T>
double vec_dist(const std::vector<T> & v1, const std::vector<T> & v2)
{
    double res = 0;
    for(size_t i = 0; i<v1.size(); i++)
    {

        res += (v2[i] -v1[i])*(v2[i]-v1[i]);
    }
    return sqrt(res);
}

inline double to_rad(double deg)
{
    return (3.14159*deg)/180.0;
}

inline double randangle()
{
    return (double) (rand() % 356);
}




class Globals
{
public:

    bool end;
    int iter;

    SDL_Renderer * r;
    SDL_Window * w;
    SDL_Texture * screen;
    const Uint8 * k;

    int width, height;

    unsigned int ticks_begin;
    unsigned int last_ticks;
    unsigned int ticks_passed;
    double time_mult;




    void update_ticks()
    {
        auto t = SDL_GetTicks();
        ticks_passed = t-last_ticks;
        last_ticks = t;
        time_mult = (double) ticks_passed / 1000.0;
    }

};

Globals g;



class Drop
{
public:
    std::vector<double> pos;
    std::vector<double> speed;
    bool alive;
    Drop()
    {
        alive = true;
        pos = {0, 0, 0};
        speed = {0, 0, 0};
        auto well_radius = 0.2;
        auto angle   = to_rad(randangle());
        this->pos[0] = cos(angle)*well_radius;
        this->pos[1] = 0;
        this->pos[2] = sin(angle)*well_radius;

        auto hspeed = 0.5;
        auto vspeed = 2;

        this->speed[0] = cos(angle)*hspeed;
        this->speed[1] = vspeed + (((rand()%100)-50)*0.001);
        this->speed[2] = sin(angle)*hspeed;
    }

    void update()
    {
        if(!alive)
            return;

        // gravity
        double grav_change = g.time_mult*gravity;
        speed[1] -= grav_change;

        // wind
        std::vector<double> wmod(3);
        vec_mult(wind, 0.1, wmod);
        vec_mult(wmod, g.time_mult, wmod);
        vec_add(speed, wmod, speed);

        // speed
        std::vector<double>smod(3);
        vec_mult(speed, g.time_mult, smod);
        vec_add(pos, smod, pos);

        if(pos[1]<0)
            alive = false;
    }

    void render()
    {
        if(!alive)
            return;
        double mult = 100; // how many pixels are 1 unit
        double dx = 320/2; // how many pixels to move to the right
        double dy = 240; // how many pixels to move down

        int brightness = 100 + (pos[2]*50);
        if(brightness < 0) brightness = 0;
        if(brightness > 255) brightness = 255;

        int x, y;

        x = (mult*pos[0]) + dx;
        y = (-1*mult*pos[1]) + dy;

        SDL_SetRenderDrawColor(g.r, brightness, brightness, 255, 255);
        SDL_RenderDrawPoint(g.r, x, y);
    }

};


std::vector<Drop> drops;

class Cloth
{
public:
    std::vector<std::vector<double>> points;
    std::vector<std::vector<double>> speed;
    std::vector<bool> fixed;
    double width = 1.2;
    size_t sx;
    size_t sy;
    size_t to_id(size_t x, size_t y)
    {
        return x + (y*sx);
    }

    Cloth()
    {
        sx = 11;
        sy = 10;

        for(int y = 0; y<sy; y++)
        {
            for(int x = 0; x<sx; x++)
            {
                points.push_back({(double)x*width, (double)y*1, 0});
                speed.push_back({0, 0, 0});
                fixed.push_back(false);
            }
        }
        fixed[to_id(0, 0)] = true;
        fixed[to_id(5, 0)] = true;
        fixed[to_id(10, 0)] = true;
    }

    void update()
    {
        for(int y = 0; y<sy; y++)
        {
            for(int x = 0; x<sx; x++)
            {
                int id = to_id(x, y);

                if(!fixed[id])
                {

                    auto clothmult = 20.0;


                    if(y > 0)
                    {
                        double dist = vec_dist(points[to_id(x, y)], points[to_id(x, y-1)]);
                        if(dist > 1)
                        {
                            std::vector<double> tmov(3, 0);
                            vec_sub(points[to_id(x, y)], points[to_id(x, y-1)], tmov);
                            double mult = dist;
                            mult *= mult*g.time_mult*clothmult;
                            vec_mult(tmov, 1.0/dist, tmov);
                            vec_mult(tmov, -mult, tmov);
                            vec_add(tmov, speed[id], speed[id]);
                        }
                    }


                    if(y < sy-1)
                    {
                        double dist = vec_dist(points[to_id(x, y)], points[to_id(x, y+1)]);
                        if(dist > 1)
                        {
                            std::vector<double> tmov(3, 0);
                            vec_sub(points[to_id(x, y)], points[to_id(x, y+1)], tmov);
                            double mult = dist;
                            mult *= mult*g.time_mult*clothmult;
                            vec_mult(tmov, 1.0/dist, tmov);
                            vec_mult(tmov, -mult, tmov);
                            vec_add(tmov, speed[id], speed[id]);
                        }
                    }

                    if(x > 0)
                    {
                        double dist = vec_dist(points[to_id(x, y)], points[to_id(x-1, y)]);
                        if(dist > 1)
                        {
                            std::vector<double> tmov(3, 0);
                            vec_sub(points[to_id(x, y)], points[to_id(x-1, y)], tmov);
                            double mult = dist;
                            mult *= mult*g.time_mult*clothmult;
                            vec_mult(tmov, 1.0/dist, tmov);
                            vec_mult(tmov, -mult, tmov);
                            vec_add(tmov, speed[id], speed[id]);
                        }
                    }


                    if(x < sx-1)
                    {
                        double dist = vec_dist(points[to_id(x, y)], points[to_id(x+1, y)]);
                        if(dist > 1)
                        {
                            std::vector<double> tmov(3, 0);
                            vec_sub(points[to_id(x, y)], points[to_id(x+1, y)], tmov);
                            double mult = dist;
                            mult *= mult*g.time_mult*clothmult;
                            vec_mult(tmov, 1.0/dist, tmov);
                            vec_mult(tmov, -mult, tmov);
                            vec_add(tmov, speed[id], speed[id]);
                        }
                    }


                    // Gravity

                    speed[id][1] += g.time_mult*gravity;

                    // wind

                    std::vector<double> wmod(3, 0);
                    vec_add(wmod, wind, wmod);
                    vec_mult(wmod, g.time_mult*0.7, wmod);
                    vec_add(wmod, speed[id], speed[id]);
                }

                double sp = vec_dist({0, 0, 0}, speed[id]);
                double spmult = 1-(0.3*g.time_mult);
                spmult = sp*spmult;
                if(sp != 0)
                    vec_mult(speed[id], spmult/sp, speed[id]);





            }

        }
        for(int y = 0; y<sy; y++)
        {
            for(int x = 0; x<sx; x++)
            {
                int id = to_id(x, y);

                std::vector<double> tspeed(3);
                vec_mult(speed[id], g.time_mult, tspeed);
                vec_add(points[id], tspeed, points[id]);

            }

        }

    }
    void render()
    {
        for(int y = 0; y<sy; y++)
        {
            for(int x = 0; x<sx; x++)
            {
                int px, py;
                double mult = 12;
                double dx = (320-(mult*11*width))/2;
                double dy = 0;
                int brightness = 150+(points[to_id(x, y)][2]*10);
                if(brightness < 50) brightness = 50;
                if(brightness > 255) brightness = 255;
                SDL_SetRenderDrawColor(g.r, brightness, brightness, brightness, 255);

                px = (points[to_id(x, y)][0]*mult)+dx;
                py = (points[to_id(x, y)][1]*mult)+dy;

                SDL_RenderDrawPoint(g.r, px, py);

                if(x>0)
                {
                    int px2, py2;
                    px2 = (points[to_id(x-1, y)][0]*mult)+dx;
                    py2 = (points[to_id(x-1, y)][1]*mult)+dy;
                    SDL_RenderDrawLine(g.r, px, py, px2, py2);
                }
                if(y>0)
                {
                    int px2, py2;
                    px2 = (points[to_id(x, y-1)][0]*mult)+dx;
                    py2 = (points[to_id(x, y-1)][1]*mult)+dy;
                    SDL_RenderDrawLine(g.r, px, py, px2, py2);
                }
            }
        }
    }

};

Cloth cloth;

int main()
{
    g.end = false;
    g.iter = 0;
    g.width = 320*3;
    g.height = 240*3;
    last_wind_change = SDL_GetTicks();

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_CreateWindowAndRenderer(g.width, g.height, 0, &g.w, &g.r);
    g.screen = SDL_CreateTexture(g.r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 320, 240);
    g.k = SDL_GetKeyboardState(0);

    g.ticks_begin = SDL_GetTicks();
    g.update_ticks();

    drops.push_back(Drop());

    while(!g.end)
    {
        main_loop();
    }

    return 0;
}

void main_loop()
{
    int loops = 0;
    for(int ticks_begin = g.last_ticks; (g.last_ticks-ticks_begin)<(1000/FPS); loops++)
    {
        event_handler();
        g.update_ticks();
        {
            for (int i = 0; i<(int) DPS*g.time_mult; i++)
                drops.push_back(Drop());
            for(auto & d : drops)
            {
                d.update();
            }
            auto rem = std::remove_if(drops.begin(),drops.end(), [](auto const & i) {
                return !i.alive;
            });
            drops.erase(rem, drops.end());
        }
        cloth.update();

        // wind
        {
            if(g.last_ticks - last_wind_change > 1000*WIND_SECONDS)
            {
                last_wind_change = g.last_ticks;
                auto angle   = to_rad(randangle());
                double wmult = rand()%100;
                wmult /= 100.0;
                wmult *= 3;
                wmult += 1;
                wind[0] = cos(angle)*wmult;
                wind[2] = sin(angle)*wmult;
                std::cout<<"Wind change!"<<std::endl;
            }
        }
    }

    compose_screen();
}

void compose_screen()
{
    SDL_SetRenderTarget(g.r, g.screen);
    SDL_SetRenderDrawColor(g.r, 0, 0, 0, 255);
    SDL_RenderClear(g.r);

    // render stuff here
    //
    cloth.render();

    for(auto & d : drops)
    {
        d.render();
    }





    // end rendering

    SDL_SetRenderTarget(g.r, NULL);
    SDL_RenderCopy(g.r, g.screen, NULL, NULL);
    SDL_RenderPresent(g.r);

}

void event_handler()
{
    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        if(e.type == SDL_QUIT || g.k[SDL_SCANCODE_ESCAPE])
            g.end = true;
    }
}

