/*!\file window.c
 * \author Farès BELHADJ, amsi@up8.edu
 * \date November 16, 2021.
 */
#include <assert.h>
/* inclusion des entêtes de fonctions de gestion de primitives simples
 * de dessin. La lettre p signifie aussi bien primitive que
 * pédagogique. */
#include <GL4D/gl4dp.h>
/* inclure la bibliothèque de rendu DIY */
#include "rasterize.h"

/* inclusion des entêtes de fonctions de création et de gestion de
 * fenêtres système ouvrant un contexte favorable à GL4dummies. Cette
 * partie est dépendante de la bibliothèque SDL2 */
#include <GL4D/gl4duw_SDL2.h>

/* protos de fonctions locales (static) */
static void init(void);
static void idle(void);
static void draw(void);
static void keyu(int keycode);
static void keyd(int keycode);
static void sortie(void);

/*!\brief une surface représentant un cube */
static surface_t * _cube = NULL;
static float _cubeSize = 4.0f;

/* variable d'état pour activer/désactiver la synchronisation verticale */
static int _use_vsync = 1;

/* on créé une grille de positions où il y aura des cubes */
static int _grille[] = {
  1, 1, 1, 1, 1, 1, 1,
  1, 0, 1, 0, 0, 0, 1,
  1, 0, 1, 0, 0, 0, 1,
  1, 0, 1, 1, 0, 1, 1,
  1, 0, 0, 0, 0, 1, 1,
  1, 0, 0, 1, 0, 0, 1,
  1, 1, 1, 1, 1, 1, 1
};
static int _grilleW = 7;
static int _grilleH = 7;

/* Définition d'un personnage */
typedef struct perso_t {
  float x, y, z;
} perso_t;

/* Définition de nos deux joueurs */
perso_t _herosA = { -10.0f, 0.0f, 0.0f };
perso_t _herosB = { 10.0f, 0.0f, 0.0f };

/* clavier virtuel */
enum {
    /* Héros A */
    VK_RIGHT = 0,
    VK_UP,
    VK_LEFT,
    VK_DOWN,

    /* Héros B */
    VK_d,
    VK_z,
    VK_q,
    VK_s,

    /* toujours à la fin */
    VK_SIZEOF
};

int _vkeyboard[VK_SIZEOF] = {0, 0, 0, 0, 0, 0, 0, 0};

/*!\brief paramètre l'application et lance la boucle infinie. */
int main(int argc, char ** argv) {
    /* tentative de création d'une fenêtre pour GL4Dummies */
    if(!gl4duwCreateWindow(argc, argv, /* args du programme */
             "Bomberman ⋅ A. KENNEL L2-A", /* titre */
             10, 10, 800, 600, /* x, y, largeur, heuteur */
             GL4DW_SHOWN) /* état visible */) {
        /* ici si échec de la création souvent lié à un problème d'absence
         * de contexte graphique ou d'impossibilité d'ouverture d'un
         * contexte OpenGL (au moins 3.2) */
        return 1;
    }
    init();

    /* mettre en place la fonction d'interception clavier touche pressée */
    gl4duwKeyDownFunc(keyd);

    /* mettre en place la fonction d'interception clavier touche relachée */
    gl4duwKeyUpFunc(keyu);

    /* mettre en place la fonction idle (simulation, au sens physique du terme) */
    gl4duwIdleFunc(idle);

    /* mettre en place la fonction de display */
    gl4duwDisplayFunc(draw);

    /* boucle infinie pour éviter que le programme ne s'arrête et ferme
     * la fenêtre immédiatement */
    gl4duwMainLoop();
    return 0;
}

/*!\brief init de nos données, spécialement le plateau */
void init(void) {
    GLuint id;
    /* création d'un screen GL4Dummies (texture dans laquelle nous
     * pouvons dessiner) aux dimensions de la fenêtre.  IMPORTANT de
     * créer le screen avant d'utiliser les fonctions liées au
     * textures */
    gl4dpInitScreen();

    /* on créé le cube */
    _cube = mk_cube(); /* ça fait 2x6 triangles      */

    /* on rajoute la texture */
    id = get_texture_from_BMP("images/tex.bmp");
    set_texture_id(_cube, id);

    /* Affichage des textures */
    enable_surface_option(_cube, SO_USE_TEXTURE);

    /* Affichage des ombres */
    enable_surface_option(_cube, SO_USE_LIGHTING);

    /* si _use_vsync != 0, on active la synchronisation verticale */
    if(_use_vsync)
        SDL_GL_SetSwapInterval(1);

    /* mettre en place la fonction à appeler en cas de sortie */
    atexit(sortie);
}

/*!\brief la fonction appellée à chaque idle */
void idle(void) {
    /* on récupère le delta-temps */
    static double t0 = 0.0; // le temps à la frame précédente
    double t, dt;
    t = gl4dGetElapsedTime();
    dt = (t - t0) / 1000.0; // diviser par mille pour avoir des secondes
    // pour le frame d'après, je mets à jour t0
    t0 = t;

    /* Mouvements du héros A */
    if(_vkeyboard[VK_RIGHT])
        _herosA.x += 10.f * dt;
    if(_vkeyboard[VK_UP])
        _herosA.z -= 10.f * dt;
    if(_vkeyboard[VK_LEFT])
        _herosA.x -= 10.f * dt;
    if(_vkeyboard[VK_DOWN])
        _herosA.z += 10.f * dt;

    /* Mouvements du héros B */
    if(_vkeyboard[VK_d])
        _herosB.x += 10.f * dt;
    if(_vkeyboard[VK_z])
        _herosB.z -= 10.f * dt;
    if(_vkeyboard[VK_q])
        _herosB.x -= 10.f * dt;
    if(_vkeyboard[VK_s])
        _herosB.z += 10.f * dt;

}

/*!\brief la fonction appelée à chaque display. */
void draw(void) {
    vec4 couleurPlateau = {0.20, 0.20, 0.20, 1}, g = {0, 1, 0, 1}, b = {0, 0, 1, 1};

    /* on va récupérer le delta-temps */
    static double t0 = 0.0; // le temps à la frame précédente
    double t, dt;
    t = gl4dGetElapsedTime();
    dt = (t - t0) / 1000.0; // diviser par mille pour avoir des secondes
    // pour le frame d'après, je mets à jour t0
    t0 = t;

    static float a = 0.0f;
    float model_view_matrix[16], projection_matrix[16], nmv[16];

    /* effacer l'écran et le buffer de profondeur */
    gl4dpClearScreen();
    clear_depth_map();

    /* des macros facilitant le travail avec des matrices et des
     * vecteurs se trouvent dans la bibliothèque GL4Dummies, dans le
     * fichier gl4dm.h */
    /* charger un frustum dans projection_matrix */
    MFRUSTUM(projection_matrix, -0.05f, 0.05f, -0.05f, 0.05f, 0.1f, 1000.0f);
    /* charger la matrice identité dans model-view */
    MIDENTITY(model_view_matrix);
    /* on place la caméra en arrière-haut, elle regarde le centre de la scène */
    lookAt(model_view_matrix, 0, 25 + 50 /* fabs(cos(a * M_PI / 180.0f)) */, 5, 0, 0, 0, 0, 0, -1);

    /* pour centrer la grille par rapport au monde */
    float cX = -_cubeSize * _grilleW / 2.0f;
    float cZ = -_cubeSize * _grilleH / 2.0f;

    /* on change la couleur */
    _cube->dcolor = couleurPlateau;

    /* pour toutes les cases de la grille, afficher un cube quand il y a
     * un 1 dans la grille */
    for(int i = 0; i < _grilleW; ++i)
        for(int j = 0; j < _grilleH; ++j)
            if(_grille[i * _grilleW + j] == 1) {
                /* copie model_view_matrix dans nmv */
                memcpy(nmv, model_view_matrix, sizeof nmv);

                /* pour convertir les coordonnées i, j de la grille en x, z du monde */
                translate(nmv, _cubeSize * j + cX, 0.0f, _cubeSize * i + cZ);
                scale(nmv, _cubeSize / 2.0f, _cubeSize / 2.0f, _cubeSize / 2.0f);
                transform_n_rasterize(_cube, nmv, projection_matrix);
            }

    /* on dessine le héros A */
    _cube->dcolor = g;
    memcpy(nmv, model_view_matrix, sizeof nmv);
    translate(nmv, _herosA.x, _herosA.y, _herosA.z);
    scale(nmv, _cubeSize / 2.0f, _cubeSize / 2.0f, _cubeSize / 2.0f);
    transform_n_rasterize(_cube, nmv, projection_matrix);

    /* on dessine le héros B */
    _cube->dcolor = b;
    memcpy(nmv, model_view_matrix, sizeof nmv);
    translate(nmv, _herosB.x, _herosB.y, _herosB.z);
    scale(nmv, _cubeSize / 2.0f, _cubeSize / 2.0f, _cubeSize / 2.0f);
    transform_n_rasterize(_cube, nmv, projection_matrix);

    /* déclarer qu'on a changé des pixels du screen (en bas niveau) */
    gl4dpScreenHasChanged();

    /* fonction permettant de raffraîchir l'ensemble de la fenêtre*/
    gl4dpUpdateScreen(NULL);
    a += 0.1f * 360.0f * dt;
}

/*!\brief intercepte l'événement clavier pour modifier les options (à l'appuie d'une touche). */
void keyd(int keycode) {
    switch(keycode) {
        /* 'v' utiliser la sync Verticale */
        case GL4DK_v:
            _use_vsync = !_use_vsync;
            if(_use_vsync)
                SDL_GL_SetSwapInterval(1);
            else
                SDL_GL_SetSwapInterval(0);
            break;

        /* Héros A */
        case GL4DK_RIGHT:
            _vkeyboard[VK_RIGHT] = 1;
            break;

        case GL4DK_UP:
            _vkeyboard[VK_UP] = 1;
            break;

        case GL4DK_LEFT:
            _vkeyboard[VK_LEFT] = 1;
            break;

        case GL4DK_DOWN:
            _vkeyboard[VK_DOWN] = 1;
            break;

        /* Héros B */
        case GL4DK_d:
            _vkeyboard[VK_d] = 1;
            break;

        case GL4DK_z:
            _vkeyboard[VK_z] = 1;
            break;

        case GL4DK_q:
            _vkeyboard[VK_q] = 1;
            break;

        case GL4DK_s:
            _vkeyboard[VK_s] = 1;
            break;

        /* Par défaut on ne fais rien */
        default: break;
    }
}

/*!\brief intercepte l'évènement clavier pour modifier les options (au relâchement d'une touche). */
void keyu(int keycode) {
    switch(keycode) {
        /* Cas où l'héros A on arrête de bouger */
        case GL4DK_RIGHT:
            _vkeyboard[VK_RIGHT] = 0;
            break;

        case GL4DK_UP:
            _vkeyboard[VK_UP] = 0;
            break;

        case GL4DK_LEFT:
            _vkeyboard[VK_LEFT] = 0;
            break;

        case GL4DK_DOWN:
            _vkeyboard[VK_DOWN] = 0;
            break;

        /* Cas où l'héros A on arrête de bouger */
        case GL4DK_d:
            _vkeyboard[VK_d] = 0;
            break;

        case GL4DK_z:
            _vkeyboard[VK_z] = 0;
            break;

        case GL4DK_q:
            _vkeyboard[VK_q] = 0;
            break;

        case GL4DK_s:
            _vkeyboard[VK_s] = 0;
            break;

        /* Par défaut on ne fais rien */
        default: break;
    }
}

/*!\brief à appeler à la sortie du programme. */
void sortie(void) {
    /* on libère le cube */
    if(_cube) {
        free_surface(_cube);
        _cube = NULL;
    }

    /* libère tous les objets produits par GL4Dummies, ici
     * principalement les screen */
    gl4duClean(GL4DU_ALL);
}
