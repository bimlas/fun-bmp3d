/* bmp3d.c - BMP keppontjainak szamerteke szerint kialakitott 3D.
 *
 * ================================  BMP3D ===================================
 *
 * Hasznalat: bmp3d [FAJL]
 *
 * A bemenet a 'terkep.bmp', vagy a FAJL 256 szinu kep; iranyitas a NumPad-on,
 * vagy WASD:
 *
 *                                      +    -
 *                                x     6 D  4 A
 *                                y     8 W  2 S
 *                                z     7 R  1 F
 *                                sugar + T  - G
 *
 *              Lekepzo feluletek kozti valtas (sik / gomb): 5, vagy SPACE
 *              Miniterkep bekapcsolasa:                     Enter
 *              Gyors mozgas:                                Caps Lock
 *
 * ==================== BIMBALASZLO(.WBH.HU|GMAIL.COM) =======================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

/* .......................... VIDEO BEALLITASAI ........................... */

#define PROGRAM_NAME "bmp3d"
#define AUTHORS      "Bimba Laszlo"

#define SZELESSEG    1024
#define MAGASSAG     768
#define SZINMELYSEG  32
#define SDL_FLAGS    SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN

/* A rajz szinei. */

#define SZIN         0xFFFFFF
#define HATTERSZIN   0x444444

/* .......................... GLOBALIS VALTOZOK ........................... */

int
    fps      = 0,                     /* Az FPS meresehez.       */
    kilepes  = 0;                     /* Kilepest aktivalo flag. */

char
    *utvonal = (char *) "terkep.bmp"; /* Bemeneti fajl.          */

/* ............................ FO SURFACE-EK ............................. */

struct _video         /* Foablak */
{
    int
        szelesseg,
        magassag;

    Uint32
        *pix;         /* (Uint32 *) video.surface->pixels, hogy ne kelljen fuggvenyenkent cast-olni. */

    SDL_Surface
        *surface;     /* Mutato az eredeti surface-re. */

} video = { 0,
            0,
            NULL,
            NULL
          };

struct _terkep        /* Bemeneti fajl. */
{
    int
        szelesseg,
        magassag;

    float
        szin_egyseg;  /* A terkep magassagaval aranyos, a tavolsag erzekeltetesehez kell. */

    Uint8
        *pix;

    SDL_Surface
        *surface;

} terkep = { 0,
             0,
             0.0,
             NULL,
             NULL
           };

/* ........................... NEZOPONT ADATAI ............................ */

struct _nezo
{
    int
        x, y, z,        /* A nezo koordinatai. */
        felulet,        /* felulet % 2 == 0 ? SIKRA : GOMBRE kepezzuk le a kepet. */
        sugar;          /* A gombre valo lekepezesnel a gomb sugara, egyebkent a sik tavolsaga a nezotol. */

} nezo = { 0, 0, 128,
           0,
           SZELESSEG / 2
         };

/* ......................... ALLAPOTJELZO ADATAI .......................... */

struct _hud
{
    SDL_Surface
        *szoveg;        /* Surface, amire a szoveget kiirjuk. */

    SDL_Rect
        koordinata;     /* A szoveg bal-felso sarkanak koordinataja - automatikusan szamolja ki a video magassagabol. */

    SDL_Color
        szin;           /* Szoveg szine. */

    TTF_Font
        *font;          /* Betutipusa. */

    struct _mini        /* Miniterkep. */
    {
        int
            szelesseg,
            magassag,
            pitch,
            lathato;    /* Ki/be kapcsolas. */

        Uint8
            alpha,      /* Atlatszosag szine. */
            *pix;

        SDL_Surface
            *surface;

    } mini;

} hud = { NULL,
          { 0, 0, 0, 0 },
          { 0x00, 0xFF, 0xFF },
          NULL,
          { 0,
            0,
            0,
            0,
            0x00,
            NULL,
            NULL
          }
        };

/* _________________________________ HIBA ____________________________________
 *
 * Az uzenet kiirasa utan azonnal befejezi a programot.
 */

void
hiba( const char *fuggveny, const char *uzenet )
{
    fprintf( stderr, "%s: %s\n", fuggveny, uzenet );
    exit( EXIT_FAILURE );
}

/* ____________________________ SDL BEALLITAS ________________________________
 *
 * Hibakezelessel kiegeszitett SDL_Init.
 */

void
sdl_beallitas( void )
{
    /* SDL inditasa, a program befejezesere utemezzuk az SDL lezarasat is. */

    if( SDL_Init( (SDL_INIT_VIDEO | SDL_INIT_TIMER) ) < 0 )
    {
        hiba( "SDL_Init", SDL_GetError() );
    }
    atexit( SDL_Quit );

    /* Ablak cimenek es ikonjanak beallitasa. */

    SDL_WM_SetCaption( PROGRAM_NAME, "" );

    /* Video beallitasa. */

    video.surface = SDL_SetVideoMode( SZELESSEG, MAGASSAG, SZINMELYSEG, SDL_FLAGS );

    if( video.surface == NULL )
    {
        hiba( "SDL_SetVideoMode", SDL_GetError() );
    }

    video.pix       = (Uint32 *) video.surface->pixels;
    video.szelesseg = video.surface->w;
    video.magassag  = video.surface->h;

    /* Az eger lathatatlanna tetele. */

    SDL_ShowCursor( 0 );

    /* Minden esemeny figyelmen kivul hagyasa kiveve azokat, amik minket
     * erintenek.
     */

    SDL_EventState( ~0,              SDL_IGNORE );
    SDL_EventState( SDL_QUIT,        SDL_ENABLE );
    SDL_EventState( SDL_KEYDOWN,     SDL_ENABLE );
    SDL_EventState( SDL_MOUSEMOTION, SDL_ENABLE );

    /* Folyamatos billentyulenyomas engedelyezese. */

    SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );

    return;
}

/* ___________________________ TERKEP BETOLTES _______________________________
 *
 * A bemeneti bmp fajlt elmenti egy surface-re es a miniterkepet is
 * letrehozza.
 */

void
terkep_betoltes( void )
{
    terkep.surface = SDL_LoadBMP( utvonal );

    if( terkep.surface == NULL )
    {
        hiba( "SDL_LoadBMP", SDL_GetError() );
    }

    terkep.pix         = (Uint8 *) terkep.surface->pixels;
    terkep.szelesseg   = terkep.surface->w;
    terkep.magassag    = terkep.surface->h;
    terkep.szin_egyseg = 0x7F / (float) terkep.magassag;

    return;
}

/* ____________________________ HUD BEALLITAS ________________________________
 *
 * A hud helyes mukodesehez szukseges beallitasok.
 */

void
hud_beallitas( void )
{
    SDL_Surface
        *str = NULL;        /* Ideiglenes surface a szoveg magassaganak meresehez. */

    if( TTF_Init() < 0 )
    {
        hiba( "TTF_Init", TTF_GetError() );
    }
    atexit( TTF_Quit );

    hud.font = TTF_OpenFont( "freemono.ttf", 14 );
    if( hud.font == NULL )
    {
        hiba( "TTF_OpenFont", TTF_GetError() );
    }

    /* Megmerjuk a karaktermagassagot: a sztringben kulonbozo karakterek
     * szerepelnek, amelyek valamelyike valoszinuleg eleri a betutipus
     * legalacsonyabb es legmagasabb pontjat, igy megkaphatjuk az adott
     * betutipussal kiirhato szoveg maximalis magassagat.
     */

    str = TTF_RenderText_Blended( hud.font, "y;|#X", hud.szin );

    hud.koordinata.x = 10;
    hud.koordinata.y = MAGASSAG - str->h;

    SDL_FreeSurface( str );

    /* Miniterkep letrehozasa. */

    hud.mini.surface = SDL_ConvertSurface( terkep.surface, terkep.surface->format, SDL_SRCALPHA );

    if( hud.mini.surface == NULL )
    {
        hiba( "SDL_ConvertSurface", SDL_GetError() );
    }

    /* Atlatszo szin megadasa. */

    if( SDL_SetColorKey( hud.mini.surface, SDL_SRCCOLORKEY, (Uint32) hud.mini.alpha ) < 0 )
    {
        hiba( "SDL_SetColorKey", SDL_GetError() );
    }

    /* A surface atlatszosaga. */

    if( SDL_SetAlpha( hud.mini.surface, SDL_SRCALPHA, 128 ) < 0 )
    {
        hiba( "SDL_SetAlpha", SDL_GetError() );
    }

    hud.mini.pix       = (Uint8 *) hud.mini.surface->pixels;
    hud.mini.szelesseg = hud.mini.surface->w;
    hud.mini.magassag  = hud.mini.surface->h;
    hud.mini.pitch     = hud.mini.surface->pitch;

    return;
}

/* ____________________________ HUD FRISSITES ________________________________
 *
 * A megjelenitett kepet befolyasolo fobb ertekek kiiratasa, a miniterkep
 * megjelenitese.
 */

void
hud_frissites( void )
{
    char
        str[100] = { 0 };   /* Atmeneti valtozo, ebbe kerul a kiirando szoveg. */

    sprintf( str,
             "x: %+04d y: %+04d z: %+04d | sugar: %+04d | fps: %04d",
             nezo.x, nezo.y, nezo.z, nezo.sugar, fps
    );

    hud.szoveg = TTF_RenderText_Blended( hud.font, str, hud.szin );

    if( hud.szoveg == NULL )
    {
        hiba( "TTF_RenderText", TTF_GetError() );
    }

    SDL_BlitSurface( hud.szoveg, NULL, video.surface, &hud.koordinata );
    SDL_FreeSurface( hud.szoveg );

    /* Miniterkep. */

    if( (hud.mini.lathato % 2) != 0 )
    {
        SDL_BlitSurface( terkep.surface,   NULL, video.surface, NULL );
        SDL_BlitSurface( hud.mini.surface, NULL, video.surface, NULL );
    }

    return;
}

/* ____________________________ SPEED LIMIT __________________________________
 *
 * A fuggvenynek atadott ertek a maximalis sebesseget hatarozza meg, azaz hogy
 * a fuggveny 1 masodperc alatt maximum ennyiszer engedelyezze a program
 * tovabbhaladasat (gyk. FPS limiter), ha max == 0, akkor nem korlatozza a
 * sebeseget. Visszateresi ertek a valodi FPS, vagy -1, ha meg nem telt el a
 * minimalis ido.
 */

int
speed_limit( unsigned int max )
{
    static Uint32
        utolso_frissites = 0;

    int
        tmp = SDL_GetTicks();

    if( (max == 0) || (utolso_frissites + (1000 / max) <= tmp) )
    {
        tmp = tmp - utolso_frissites;
        utolso_frissites = SDL_GetTicks();
        return( 1000 / ((tmp > 0) ? tmp : 1) );
    }
    else
    {
        return( -1 );
    }
}

/* ______________________________ INTERAKCIO _________________________________
 *
 * A felhasznalo interakciojanak azonositasa, mint pl. melyik gombot nyomta
 * le.
 */

void
interakcio( SDL_Event esemeny )
{
    int
         mertek = ((esemeny.key.keysym.mod & KMOD_CAPS) > 0) ? 3 : 1;

    switch( esemeny.type )
    {
        case SDL_KEYDOWN:

            /* Ugyan itt is lehetne switch-case parost alkalmazni, de arra a
             * gcc egy rakas figyelmeztetest dob, mert nem vizsgalunk meg
             * minden lehetoseget az enum listaban, csak azokhoz a gombokhoz
             * kotunk feltetelt, ami minket erdekel.
             */

            if(      (esemeny.key.keysym.sym == SDLK_KP6)      || (esemeny.key.keysym.sym == 'd') )
            {
                nezo.x += mertek;
            }

            else if( (esemeny.key.keysym.sym == SDLK_KP4)      || (esemeny.key.keysym.sym == 'a') )
            {
                nezo.x -= mertek;
            }

            else if( (esemeny.key.keysym.sym == SDLK_KP8)      || (esemeny.key.keysym.sym == 'w') )
            {
                nezo.y += mertek;
            }

            else if( (esemeny.key.keysym.sym == SDLK_KP2)      || (esemeny.key.keysym.sym == 's') )
            {
                nezo.y -= mertek;
            }

            else if( (esemeny.key.keysym.sym == SDLK_KP7)      || (esemeny.key.keysym.sym == 'r') )
            {
                nezo.z += mertek;
            }

            else if( (esemeny.key.keysym.sym == SDLK_KP1)      || (esemeny.key.keysym.sym == 'f') )
            {
                nezo.z -= mertek;
            }

            else if( (esemeny.key.keysym.sym == SDLK_KP_PLUS)  || (esemeny.key.keysym.sym == 't') )
            {
                nezo.sugar += mertek;
            }

            else if( (esemeny.key.keysym.sym == SDLK_KP_MINUS) || (esemeny.key.keysym.sym == 'g') )
            {
                nezo.sugar -= mertek;
            }

            else if( (esemeny.key.keysym.sym == SDLK_KP5)      || (esemeny.key.keysym.sym == SDLK_SPACE) )
            {
                ++nezo.felulet;
            }

            else if( (esemeny.key.keysym.sym == SDLK_KP_ENTER) || (esemeny.key.keysym.sym == SDLK_RETURN) )
            {
                ++hud.mini.lathato;
            }

            else if( (esemeny.key.keysym.sym == SDLK_ESCAPE)   || (esemeny.key.keysym.sym == 'q') )

            {
                kilepes = 1;
            }

            break;

        case SDL_MOUSEMOTION:

            nezo.x += esemeny.motion.xrel;
            nezo.y -= esemeny.motion.yrel;

            break;

        case SDL_QUIT:

            /* Az ablak bezarasara kilepunk. */

            kilepes = 1;

            break;
    }

    return;
}

/* _________________________________ RAJZ ____________________________________
 *
 * A lenyegi munkat vegzo fuggveny.
 */

void
rajz( void )
{
    int
        bmp_x    = 0,        /* Az aktualisan szamolt koordinata a bmp fajlban.       */
        bmp_y    = 0,
        x        = 0,        /* Az aktualisan szamolt koordinata helye a kepernyon.   */
        y        = 0,
        z        = 0;        /* Az aktualis keppont szamerteke.                       */

    Uint32
        szin     = 0x000000; /* A tavolsaggal aranyos vilagossagu alap szin.          */

    float
        tavolsag = 0.0;      /* A nezo es az aktualisan szamolt koordinata tavolsaga. */

    /* Video torlese. */

    SDL_FillRect( video.surface, NULL, HATTERSZIN );

    /* Ha be van kapcsolva, a miniterkepet is toroljuk. */

    if( (hud.mini.lathato % 2) != 0 )
    {
        SDL_FillRect( hud.mini.surface, NULL, hud.mini.alpha );
    }

    for( bmp_y = terkep.magassag ; bmp_y >= 1 ; --bmp_y )
    {
        /* A tavolsaggal aranyosan lesz a keppontban egyre tobb a kek es egyre
         * kevesebb a zold.
         */

        szin = 0x00FF00 - (0x000100 * (int) (bmp_y * terkep.szin_egyseg)) + bmp_y * terkep.szin_egyseg;

        for( bmp_x = 0 ; bmp_x < terkep.szelesseg ; ++bmp_x )
        {
            /* A BMP formatum a legalso kepsortol kezdve balrol-jobbra tarolja
             * a keppontokat.
             */

            z = terkep.pix[ ((terkep.magassag - bmp_y) * terkep.szelesseg) + bmp_x ];
            tavolsag = bmp_y - nezo.y;

            if( tavolsag <= 0 )
            {
                continue;
            }

            x = bmp_x - nezo.x;
            y = nezo.z - z;

            /* Ha gomb feluletre kell lekepeznunk, akkor Pitagorasz-tetellel
             * kiszamoljuk a nezo es a pont tavolsagat. A tavolsag a lekepzo
             * sik felulet es a nezo tavolsagaval, illetve a gomb sugaraval
             * forditottan aranyos: minel tavolabb helyezzuk a nezotol a
             * lekepezo feluletet, annal nagyobbnak latszik az objektum.
             */

            if( nezo.felulet % 2 == 1 )
            {
                tavolsag *= tavolsag;                        /* pont es nezo.y tavolsag negyzete */
                tavolsag += x * x;                           /* pont es nezo.x tavolsag negyzete */
                tavolsag += y * y;                           /* pont es nezo.z tavolsag negyzete */
                tavolsag =  sqrtf( tavolsag ) / nezo.sugar;  /* abszolut tavolsag / nezo.sugar   */
            }
            else
            {
                tavolsag /= nezo.sugar;
            }

            /* A kepernyo kozepere helyezzuk a koordinatarendszer kozepet. */

            x = (x / tavolsag) + (video.szelesseg / 2);
            if( (x < 0) || (x >= video.szelesseg) )
            {
                continue;
            }

            y = (y / tavolsag) + (video.magassag / 2);
            if( (y < 0) || (y >= (hud.koordinata.y)) )
            {
                continue;
            }

            /* A keppont szinet befolyasolja meg a 'vilagossaga' is. */

            video.pix[ y * video.szelesseg + x ] = (0x010000 * z) + szin;

            /* Ha be van kapcsolva, akkor a miniterkepen is jeloljuk a latott
             * teruletet.
             */

            if( (hud.mini.lathato % 2) != 0 )
            {
                hud.mini.pix[ ((hud.mini.magassag - bmp_y) * hud.mini.pitch) + bmp_x ] = 0x80;
            }
        }
    }

    return;
}

/* _________________________________ MAIN _________________________________ */

int
main( int argc, char **argv )
{
    SDL_Event
        esemeny             = { 0 }; /* Az interakcio azonositasahoz szukseges. */

    if( argc > 2 )
    {
        hiba( "Hasznalat", PROGRAM_NAME " [FAJL]" );
    }

    if( argc == 2 )
    {
        utvonal = argv[ 1 ];
    }

    sdl_beallitas();
    terkep_betoltes();
    hud_beallitas();

    /* A nezot a kep kozepere allitjuk. */

    nezo.x = terkep.szelesseg / 2;

    while( kilepes == 0 )
    {
        fps = speed_limit( 0 );

        /* Esemenyfigyeles. */

        if( SDL_PollEvent( &esemeny ) != 0 )
        {
            interakcio( esemeny );
        }

        /* SDL_DOUBLEBUF eseten zarolni kell a surface-t. */

        if( SDL_MUSTLOCK( video.surface ) )
        {
            if( SDL_LockSurface( video.surface ) < 0 )
            {
                hiba( "SDL_LockSurface", SDL_GetError() );
            }
        }

        /* A kepernyo frissitese. */

        rajz();
        hud_frissites();

        if( SDL_MUSTLOCK( video.surface ) )
        {
            SDL_UnlockSurface( video.surface );
        }

        SDL_Flip( video.surface );
    }

    TTF_CloseFont( hud.font );
    SDL_FreeSurface( hud.mini.surface );
    SDL_FreeSurface( terkep.surface );
    SDL_FreeSurface( video.surface );

    return( EXIT_SUCCESS );
}
