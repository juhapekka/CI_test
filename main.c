#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <unistd.h>

#define CI_TABLE_SIZE 16

unsigned int imgwidth = 256;
unsigned int imgheight = 256;
unsigned int testseconds = 1;

typedef struct {
    int width;
    int height;
    unsigned char *data;
} textureImage;


int singleBufferAttributess[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_RED_SIZE,      1,
    GLX_GREEN_SIZE,    1,
    GLX_BLUE_SIZE,     1,
    GLX_ALPHA_SIZE,    1,
    None
};

int doubleBufferAttributes[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_DOUBLEBUFFER,  True,
    GLX_RED_SIZE,      1,
    GLX_GREEN_SIZE,    1,
    GLX_BLUE_SIZE,     1,
    GLX_ALPHA_SIZE,    1,
    None
};


Display  *dpy;
GLXWindow glxWin;
int swapFlag = True;


static Bool ImageLoad(textureImage *image) {
    int c1, c2;

    image->width = imgwidth;
    image->height = imgheight;

    image->data = (unsigned char *) malloc(image->height * image->width);
    if (image->data == NULL) {
        printf("Error allocating memory for image data");
        return False;
    }

    for(c1 = 0; c1 < imgheight; c1++)
    {
        double y0 = ((((c1)*3.0)/imgheight)-1.5 );
        for(c2 = 0; c2 < imgwidth; c2++)
        {
            double x0 = ((((c2)*3.5)/imgwidth)-2.5 );
            double x = 0.0;
            double y = 0.0;

            int iteration = 0;
            int max_iteration = 1024;
            while ( x*x + y*y < 2*2  &&  iteration < max_iteration )
            {
                double xtemp = x*x - y*y + x0;
                y = 2*x*y + y0;
                x = xtemp;
                iteration = iteration + 1;
            }
            image->data[c1*imgwidth + c2] = (char)(((iteration*5))&0xf);
        }
    }
    return True;
}

static void testGL()
{
    textureImage *texti;
    unsigned int c;
    float ci[CI_TABLE_SIZE];

    for (c = 0; c < CI_TABLE_SIZE; c++)
    {
        if (c)
            ci[c] = ((float)(CI_TABLE_SIZE-c))/CI_TABLE_SIZE;
        else
            ci[c] = 0.0;
    }

    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, CI_TABLE_SIZE, ci);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, CI_TABLE_SIZE, ci);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, CI_TABLE_SIZE, ci);
    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

    texti = malloc(sizeof(textureImage));
    if (texti && ImageLoad(texti))
    {
        glClear( GL_COLOR_BUFFER_BIT );
        glRasterPos2i(0, texti->height);
        glPixelZoom(1.0f, -1.0f);
        glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

        /*
         * Following will fail in Mesa where is
         * 84eb402c01962e3ff4b70c9948c85a61ed81678f
         * commit in place.
         */
        glDrawPixels(texti->width, texti->height, GL_COLOR_INDEX,
                     GL_UNSIGNED_BYTE, texti->data);

        glFlush();

        if (swapFlag)
            glXSwapBuffers(dpy, glxWin);

        sleep(3);
    }

    if (texti)
    {
        if (texti->data)
            free(texti->data);
        free(texti);
    }
    return;
}


static Bool WaitForNotify( Display *dpy, XEvent *event, XPointer arg ) {
    return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
    (void)dpy;
}

int main( int argc, char *argv[] )
{
    Window                xWin;
    XEvent                event;
    XVisualInfo          *vInfo;
    XSetWindowAttributes  swa;
    GLXFBConfig          *fbConfigs;
    GLXContext            context;
    int                   swaMask;
    int                   numReturned;

    dpy = XOpenDisplay( NULL );
    if ( dpy == NULL ) {
        printf( "Unable to open a connection to the X server\n" );
        exit( EXIT_FAILURE );
    }

    fbConfigs = glXChooseFBConfig( dpy, DefaultScreen(dpy),
                                   doubleBufferAttributes, &numReturned );

    if ( fbConfigs == NULL ) {
        fbConfigs = glXChooseFBConfig( dpy, DefaultScreen(dpy),
                                       singleBufferAttributess, &numReturned );
        swapFlag = False;
    }

    vInfo = glXGetVisualFromFBConfig( dpy, fbConfigs[0] );

    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;
    swa.colormap = XCreateColormap( dpy, RootWindow(dpy, vInfo->screen),
                                    vInfo->visual, AllocNone );

    swaMask = CWBorderPixel | CWColormap | CWEventMask;

    xWin = XCreateWindow( dpy, RootWindow(dpy, vInfo->screen), 0, 0, 256, 256,
                          0, vInfo->depth, InputOutput, vInfo->visual,
                          swaMask, &swa );

    context = glXCreateNewContext( dpy, fbConfigs[0], GLX_RGBA_TYPE,
                 NULL, True );

    glxWin = glXCreateWindow( dpy, fbConfigs[0], xWin, NULL );

    XMapWindow( dpy, xWin );
    XIfEvent( dpy, &event, WaitForNotify, (XPointer) xWin );

    glXMakeContextCurrent( dpy, glxWin, glxWin, context );
    glEnable( GL_TEXTURE_2D );

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, imgwidth, 0, imgheight, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    testGL();

    glXMakeContextCurrent(dpy, 0, 0, 0);
    glXDestroyContext(dpy, context);
    glXDestroyWindow(dpy, glxWin);
    XDestroyWindow(dpy, xWin);
    XFree(vInfo);
    XCloseDisplay(dpy);
    exit( EXIT_SUCCESS );

    (void)argc;
    (void)argv;
}
