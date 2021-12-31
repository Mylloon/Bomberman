/*!\file window.c
 * \author Farès BELHADJ, amsi@up8.edu
 * \student Anri KENNEL, anri.kennel@etud.univ-paris8.fr
 * \date November 16, 2021.
 */
#include <assert.h>

/* Inclusion des entêtes de fonctions de gestion de primitives simples
 * de dessin. La lettre p signifie aussi bien primitive que
 * pédagogique. */
#include <GL4D/gl4dp.h>

/* Inclure la bibliothèque de rendu du cours */
#include "rasterize.h"

/* Inclusion des entêtes de fonctions de création et de gestion de
 * fenêtres système ouvrant un contexte favorable à GL4dummies. Cette
 * partie est dépendante de la bibliothèque SDL2 */
#include <GL4D/gl4duw_SDL2.h>

/* Fonctions locales (static) */
static void init(void);
static void idle(void);
static void draw(void);
static void keyu(int keycode);
static void keyd(int keycode);
static void sortie(void);

/*!\brief Surface représentant un cube */
static surface_t * _cube = NULL;
static float _cubeSize = 4.f;

/* Variable d'état pour activer/désactiver la synchronisation verticale */
static int _use_vsync = 1;

/* Grille de positions où il y aura des cubes */
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
perso_t _herosA = { 6.f, 0.f, -6.f }; // à droite
perso_t _herosB = { -10.f, 0.f, 0.f }; // à gauche

/* Clavier virtuel */
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

    /* Toujours à la fin */
    VK_SIZEOF
};

int _vkeyboard[VK_SIZEOF] = {0, 0, 0, 0, 0, 0, 0, 0};

/*!\brief Paramètre l'application et lance la boucle infinie. */
int main(int argc, char ** argv) {
    /* Tentative de création d'une fenêtre pour GL4Dummies */
    if(!gl4duwCreateWindow(argc, argv, /* args du programme */
             "Bomberman ⋅ A. KENNEL L2-A", /* titre */
             10, 10, 800, 600, /* x, y, largeur, heuteur */
             GL4DW_SHOWN) /* état visible */) {
        /* Ici si échec de la création souvent lié à un problème d'absence
         * de contexte graphique ou d'impossibilité d'ouverture d'un
         * contexte OpenGL (au moins 3.2) */
        return 1;
    }
    init();

    /* Mets en place la fonction d'interception clavier touche pressée */
    gl4duwKeyDownFunc(keyd);

    /* Mets en place la fonction d'interception clavier touche relachée */
    gl4duwKeyUpFunc(keyu);

    /* Mets en place la fonction idle (simulation, au sens physique du terme) */
    gl4duwIdleFunc(idle);

    /* Mets en place la fonction de display */
    gl4duwDisplayFunc(draw);

    /* Boucle infinie pour éviter que le programme ne s'arrête et ferme
     * la fenêtre immédiatement */
    gl4duwMainLoop();
    return 0;
}

/*!\brief init de nos données, spécialement le plateau */
void init(void) {
    GLuint id;
    /* Création d'un screen GL4Dummies (texture dans laquelle nous
     * pouvons dessiner) aux dimensions de la fenêtre. IMPORTANT de
     * créer le screen avant d'utiliser les fonctions liées au
     * textures */
    gl4dpInitScreen();

    /* Création du cube */
    _cube = mk_cube(); /* ça fait 2x6 triangles */

    /* Rajoute la texture */
    id = get_texture_from_BMP("images/tex.bmp");
    set_texture_id(_cube, id);

    /* Affichage des textures */
    enable_surface_option(_cube, SO_USE_TEXTURE);

    /* Affichage des ombres */
    enable_surface_option(_cube, SO_USE_LIGHTING);

    /* Si _use_vsync != 0, on active la synchronisation verticale */
    SDL_GL_SetSwapInterval(_use_vsync);

    /* Mets en place la fonction à appeler en cas de sortie */
    atexit(sortie);
}

/*!\brief Fonction appellée à chaque idle (entre chaque frame) */
void idle(void) {
    /* on récupère le delta-temps */
    static double t0 = 0.0; // le temps à la frame précédente
    double t, dt;
    t = gl4dGetElapsedTime();
    dt = (t - t0) / 1000.0; // diviser par mille pour avoir des secondes
    /* pour le frame d'après, mets à-jour t0 */
    t0 = t;

    float vitesse = 10.f;
    /* Mouvements du héros A */
    float zA = (float)((_herosA.z + _cubeSize * _grilleH / 2) / _cubeSize); // ligne - hauteur
    float xA = (float)((_herosA.x + _cubeSize * _grilleW / 2) / _cubeSize); // colonne - longueur
    if(_vkeyboard[VK_RIGHT])
        if(_grille[(int)(zA + .5f) * _grilleH + (int)(xA + 1.f)] == 0) // collision à droite du plateau
            _herosA.x += vitesse * dt;
    if(_vkeyboard[VK_UP])
        if(_grille[(int)zA * _grilleH + (int)(xA + .5f)] == 0) // collision en haut plateau
            _herosA.z -= vitesse * dt;
    if(_vkeyboard[VK_LEFT])
        if(_grille[(int)(zA + .5f) * _grilleH + (int)xA] == 0) // collision à gauche du plateau
            _herosA.x -= vitesse * dt;
    if(_vkeyboard[VK_DOWN])
        if(_grille[(int)(zA + 1.f) * _grilleH + (int)(xA + .5f)] == 0) // collision en bas du plateau
            _herosA.z += vitesse * dt;
    printf("\n=========== Héros A ===========\n li = %d, col = %d\n", (int)(zA + .5f), (int)(xA + .5f));

    /* Mouvements du héros B */
    float zB = (float)((_herosB.z + _cubeSize * _grilleH / 2) / _cubeSize); // ligne - hauteur
    float xB = (float)((_herosB.x + _cubeSize * _grilleW / 2) / _cubeSize); // colonne - longueur
    if(_vkeyboard[VK_d])
        if(_grille[(int)(zB + .5f) * _grilleH + (int)(xB + 1.f)] == 0) // collision à droite du plateau
            _herosB.x += vitesse * dt;
    if(_vkeyboard[VK_z])
        if(_grille[(int)zB * _grilleH + (int)(xB + .5f)] == 0) // collision en haut plateau
            _herosB.z -= vitesse * dt;
    if(_vkeyboard[VK_q])
        if(_grille[(int)(zB + .5f) * _grilleH + (int)xB] == 0) // collision à gauche du plateau
            _herosB.x -= vitesse * dt;
    if(_vkeyboard[VK_s])
        if(_grille[(int)(zB + 1.f) * _grilleH + (int)(xB + .5f)] == 0) // collision en bas du plateau
            _herosB.z += vitesse * dt;
    printf("=========== Héros B ===========\n li = %d, col = %d\n===============================\n", (int)(zB + .5f), (int)(xB + .5f));
}

/*!\brief Fonction appelée à chaque display. */
void draw(void) {
    vec4 couleurPlateau = {0.2, 0.2, 0.2, 1} /* Gris */, couleurHerosA = {0.15, 0.5, 0.15, 1} /* Vert */, couleurHerosB = {0.2, 0.2, 0.7, 1} /* Bleu */;

    /* On va récupérer le delta-temps */
    static double t0 = 0.0; // le temps à la frame précédente
    double t, dt;
    t = gl4dGetElapsedTime();
    dt = (t - t0) / 1000.0; // diviser par mille pour avoir des secondes
    // Pour le frame d'après, mets à-jour t0
    t0 = t;

    static float a = 0.0f;
    float model_view_matrix[16], projection_matrix[16], nmv[16];

    /* Efface l'écran et le buffer de profondeur */
    gl4dpClearScreen();
    clear_depth_map();

    /* Des macros facilitant le travail avec des matrices et des
     * vecteurs se trouvent dans la bibliothèque GL4Dummies, dans le
     * fichier gl4dm.h */
    /* Charger un frustum dans projection_matrix */
    MFRUSTUM(projection_matrix, -0.05f, 0.05f, -0.05f, 0.05f, 0.1f, 1000.0f);
    /* Charger la matrice identité dans model-view */
    MIDENTITY(model_view_matrix);
    /* On place la caméra en arrière-haut, elle regarde le centre de la scène */
    lookAt(model_view_matrix, 0, 30 /* zoom */, 30 /* inclinaison */, 0, 0, 0, 0, 0, -1);

    /* Pour centrer la grille par rapport au monde */
    float cX = -_cubeSize * _grilleW / 2.0f;
    float cZ = -_cubeSize * _grilleH / 2.0f;

    /* On change la couleur */
    _cube->dcolor = couleurPlateau;

    /* Pour toutes les cases de la grille, afficher un cube quand il y a
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

    /* Dessine le héros A */
    _cube->dcolor = couleurHerosA;
    memcpy(nmv, model_view_matrix, sizeof nmv);
    translate(nmv, _herosA.x, _herosA.y, _herosA.z);
    scale(nmv, _cubeSize / 2.0f, _cubeSize / 2.0f, _cubeSize / 2.0f);
    transform_n_rasterize(_cube, nmv, projection_matrix);

    /* Dessine le héros B */
    _cube->dcolor = couleurHerosB;
    memcpy(nmv, model_view_matrix, sizeof nmv);
    translate(nmv, _herosB.x, _herosB.y, _herosB.z);
    scale(nmv, _cubeSize / 2.0f, _cubeSize / 2.0f, _cubeSize / 2.0f);
    transform_n_rasterize(_cube, nmv, projection_matrix);

    /* Déclarer qu'on a changé des pixels du screen (en bas niveau) */
    gl4dpScreenHasChanged();

    /* Fonction permettant de raffraîchir l'ensemble de la fenêtre*/
    gl4dpUpdateScreen(NULL);
    a += 0.1f * 360.0f * dt;
}

/*!\brief Intercepte l'événement clavier pour modifier les options (à l'appuie d'une touche). */
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

/*!\brief Intercepte l'évènement clavier pour modifier les options (au relâchement d'une touche). */
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

/*!\brief Appeler à la sortie du programme
 * pour libérer les objets de la mémoire. */
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
