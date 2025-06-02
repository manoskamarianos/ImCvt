#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "imageio.h"


const char *USAGE = 
  "|-----------------------------------------------------------------------|\n"
  "| ImageConverter (v0.6)  by https://github.com/WangXuan95/              |\n"
  "|-----------------------------------------------------------------------|\n"
  "| Usage:                                                                |\n"
  "|   ImCvt [-switches]  <in1> -o <out1>  [<in2> -o <out2>]  ...          |\n"
  "|                                                                       |\n"
  "|-----------------------------------------------------------------------|\n"
  "| Where <in> and <out> can be:                                          |\n"
  "|--------|---------------------------------|-------|--------|-----|-----|\n"
  "|        |                                 | gray  | RGB    |     |     |\n"
  "| suffix |          format name            | 8-bit | 24-bit | in  | out |\n"
  "|--------|---------------------------------|-------|--------|-----|-----|\n"
  "| .bmp   | Bitmap Image File (BMP)         | yes   | yes    | yes | yes |\n"
  "| .pnm   | Portable Any Map (PNM)          | yes   | yes    | yes | yes |\n"
  "| .pgm   | Portable Gray Map (PGM)         | yes   | no     | yes | yes |\n"
  "| .ppm   | Portable Pix Map (PPM)          | no    | yes    | yes | yes |\n"
  "| .qoi   | Quite OK Image (QOI)            | no    | yes    | yes | yes |\n"
  "| .png   | Portable Network Graphics (PNG) | yes   | yes    | no  | yes |\n"
  "| .jls   | JPEG-LS Image (JLS)             | yes   | yes    | no  | yes |\n"
  "| .jlsx  | JPEG-LS extension (ITU-T T.870) | yes   | no     | yes | yes |\n"
  "| .h265  | H.265/HEVC Image                | yes   | no     | no  | yes |\n"
  "|--------|---------------------------------|-------|--------|-----|-----|\n"
  "|                                                                       |\n"
  "| switches:    -f         : force overwrite of output file              |\n"
  "|              -<number>  : 1. PNG RGB palette quantize color count     |\n"
  "|                           2. JPEG-LS near value                       |\n"
  "|                           3. H.265 (qp-4)/6 value                     |\n"
  "|-----------------------------------------------------------------------|\n"
  "\n";



// return:   1 : match    0 : mismatch
static int fileSuffixMatch (const char *string, const char *suffix) {
    #define  TO_LOWER(c)   ((((c) >= 'A') && ((c) <= 'Z')) ? ((c)+32) : (c))
    const char *p1, *p2;
    for (p1=string; *p1; p1++);
    for (p2=suffix; *p2; p2++);
    while (TO_LOWER(*p1) == TO_LOWER(*p2)) {
        if (p2 <= suffix)
            return 1;
        if (p1 <= string)
            return 0;
        p1 --;
        p2 --;
    }
    return 0;
}


static void fileSuffixReplace (char *p_dst, const char *p_src, const char *p_suffix) {
    char *p , *p_base = p_dst;
    for (; *p_src; p_src++) {       // traversal p_src
        *(p_dst++) = *p_src;        // copy p_src to p_dst
    }
    *p_dst = '\0';
    for (p=p_dst; ; p--) {          // reverse traversal p_dst
        if (p < p_base || *p=='/' || *p=='\\') {
            break;
        }
        if (*p == '.') {
            p_dst = p;
            break;
        }
    }
    *(p_dst++) = '.';
    for (; *p_suffix; p_suffix++) { // traversal p_suffix
        *(p_dst++) = *p_suffix;     // copy p_suffix to p_dst
    }
    *p_dst = '\0';
}


static int fileExist (const char *p_filename) {
    FILE *fp = fopen(p_filename, "rb");
    if (fp) fclose(fp);
    return (fp != NULL);
}


#define  MAX_N_FILE  999


static void parseCommand (
    int   argc, char **argv,
    int  *p_n_file,
    char *src_fnames[MAX_N_FILE],
    char *dst_fnames[MAX_N_FILE],
    int  *p_input_number,
    int  *p_force_write
) {
    int i, next_is_dst=0;

    *p_input_number = *p_force_write = 0;
    
    for (i=0; i<MAX_N_FILE; i++) {
        src_fnames[i] = dst_fnames[i] = NULL;
    }
    
    (*p_n_file) = -1;
    
    for (i=1; i<argc; i++) {
        char *arg = argv[i];
        
        if (arg[0] == '-') {                       // parse switches
            switch (arg[1]) {
                case 'f' : *p_force_write = 1;  break;
                case 'o' : next_is_dst = 1;     break;
                default : if (atoi(&arg[1]) > 0) {
                    *p_input_number = atoi(&arg[1]);
                }
            }
            
        } else if ((*p_n_file)+1 < MAX_N_FILE) {   // parse file names
            
            if (next_is_dst) {
                next_is_dst = 0;
                
                dst_fnames[(*p_n_file)] = arg;
            } else {
                (*p_n_file)++;
                
                src_fnames[(*p_n_file)] = arg;
            }
        }
    }
    
    (*p_n_file)++;
}


#define  ERROR(error_message,fname) {   \
    printf("   ***ERROR: ");            \
    printf((error_message), (fname));   \
    printf("\n");                       \
    continue;                           \
}


int main (int argc, char **argv) {
    int   i_file, n_file, n_success=0;
    char *src_fnames[MAX_N_FILE], *dst_fnames[MAX_N_FILE];
    int   input_number, force_write;
    
    parseCommand(argc, argv, &n_file, src_fnames, dst_fnames, &input_number, &force_write);
    
    if (n_file <= 0) {
        printf(USAGE);
        return -1;
    }
    
    
    for (i_file=0; i_file<n_file; i_file++) {
        char *p_src_fname = src_fnames[i_file];
        char *p_dst_fname = dst_fnames[i_file];
        
        uint8_t *img_buf=NULL;
        uint32_t height=0, width=0;
        int      is_rgb=0;
        int      failed=0;
        
        if (p_dst_fname == NULL) {
            static char dst_fname_buffer [16384];
            p_dst_fname = dst_fname_buffer;
            fileSuffixReplace(p_dst_fname, p_src_fname, "png");   // default: convert to PNG
        }
        
        printf("(%d/%d)  %s -> %s\n", i_file+1, n_file, p_src_fname, p_dst_fname);
        
        if (!fileExist(p_src_fname)) ERROR("%s not exist", p_src_fname);
        
        if (!force_write && fileExist(p_dst_fname)) ERROR("%s already exist", p_dst_fname);
        
        if (img_buf==NULL) img_buf = loadPNMImageFile(p_src_fname, &is_rgb, &height, &width);
        if (img_buf==NULL) img_buf = loadBMPImageFile(p_src_fname, &is_rgb, &height, &width);
        if (img_buf==NULL) img_buf = loadQOIImageFile(p_src_fname, &is_rgb, &height, &width);
        if (img_buf==NULL) img_buf =loadJLSXImageFile(p_src_fname, &is_rgb, &height, &width);
        if (img_buf==NULL) ERROR("open %s failed", p_src_fname);
        
        if        (    fileSuffixMatch(p_dst_fname, "pnm") || fileSuffixMatch(p_dst_fname, "ppm") || fileSuffixMatch(p_dst_fname, "pgm")) {
            failed = writePNMImageFile(p_dst_fname, img_buf, is_rgb, height, width);
        } else if (    fileSuffixMatch(p_dst_fname, "png")) {
            failed = writePNGImageFile(p_dst_fname, img_buf, is_rgb, height, width, (input_number<256?input_number:256));
        } else if (    fileSuffixMatch(p_dst_fname, "bmp")) {
            failed = writeBMPImageFile(p_dst_fname, img_buf, is_rgb, height, width);
        } else if (    fileSuffixMatch(p_dst_fname, "qoi")) {
            failed = writeQOIImageFile(p_dst_fname, img_buf, is_rgb, height, width);
        } else if (    fileSuffixMatch(p_dst_fname, "jls")) {
            failed = writeJLSImageFile(p_dst_fname, img_buf, is_rgb, height, width, (input_number<4?input_number:4));
        } else if (    fileSuffixMatch(p_dst_fname, "jlsx")) {
            failed =writeJLSXImageFile(p_dst_fname, img_buf, is_rgb, height, width, (input_number<4?input_number:4));
        } else if (    fileSuffixMatch(p_dst_fname, "h265") || fileSuffixMatch(p_dst_fname, "265") || fileSuffixMatch(p_dst_fname, "hevc")) {
            failed =writeHEVCImageFile(p_dst_fname, img_buf, is_rgb, height, width, (input_number<4?input_number:4));
        } else {
            free(img_buf);
            ERROR("unsupported output suffix: %s", p_dst_fname);
        }
        
        free(img_buf);
        
        if (failed) ERROR("write %s failed", p_dst_fname);
        
        n_success ++;
    }
    
    
    {
        int n_failed = n_file - n_success;
        
        if (n_file > 1) {
            printf("\nsummary:");
            if (n_success) printf("  %d file converted", n_success);
            if (n_failed)  printf("  %d failed"        , n_failed);
            printf("\n");
        }
        
        return n_failed;
    }
}
