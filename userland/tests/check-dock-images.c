/* Decode assets with the runtime image library; no display is opened. */
#include <Imlib2.h>
#include <stdio.h>
int main(int argc, char **argv) {
    int i;
    for(i=1;i<argc;i++) {
        Imlib_Image image=imlib_load_image(argv[i]);
        if(!image){fprintf(stderr,"Cannot decode %s\n",argv[i]);return 1;}
        imlib_context_set_image(image);
        printf("Decoded %s: %dx%d\n",argv[i],imlib_image_get_width(),imlib_image_get_height());
        imlib_free_image();
    }
    Imlib_Font font=imlib_load_font("/usr/share/felix/theme/DejaVuSans.ttf/10");
    if(!font){fputs("Cannot load wbar label font\n",stderr);return 1;}
    imlib_context_set_font(font);imlib_free_font();puts("Dock label font loaded");
    return argc>1?0:1;
}
