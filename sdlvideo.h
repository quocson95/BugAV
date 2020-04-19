#ifndef SDLVideo_H
#define SDLVideo_H

#include <QWidget>
#include "SDL2/SDL.h"
#include <QDebug>
class SDLVideo : public QWidget {
    Q_OBJECT

public:
    SDLVideo(QWidget *parent = 0, Qt::WindowFlags f = 0) : QWidget(parent, f), m_Screen(0){
        setAttribute(Qt::WA_PaintOnScreen);
        setUpdatesEnabled(false);

        // Set the new video mode with the new window size
        char variable[64];
        snprintf(variable, sizeof(variable), "SDL_WINDOWID=0x%lx", winId());
        putenv(variable);

        SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);

        // initialize default Video
        if((SDL_Init(SDL_INIT_VIDEO) == -1)) {
            qDebug() << "Could not initialize SDL: " << SDL_GetError();
        }

        m_Screen = SDL_SetVideoMode(640, 480, 8, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (m_Screen == 0) {
            qDebug() << "Couldn't set video mode: " << SDL_GetError();
        }
    }

    virtual ~SDLVideo() {
        if(SDL_WasInit(SDL_INIT_VIDEO) != 0) {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            m_Screen = 0;
        }
    }
private:
    SDL_Surface *m_Screen;
};

#endif // VWIDGET_H
