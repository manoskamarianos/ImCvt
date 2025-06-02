#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// return:   0 : success    1 : failed
// note: tmp_buffer size should be (12*n_cluster*sizeof(uint32_t))
static void palette_kmeans_quantize (const uint8_t *p_src, size_t n_samples, size_t n_cluster, uint8_t *p_palette, uint8_t *p_index, uint8_t *tmp_buffer) {
    size_t i, j, iter;
    uint32_t *sums           = (uint32_t*)(tmp_buffer);
    uint32_t *counts         = (uint32_t*)(tmp_buffer + 3*n_cluster*sizeof(uint32_t));
    uint8_t  *centroids      =  (uint8_t*)(tmp_buffer + 6*n_cluster*sizeof(uint32_t));
    uint8_t  *prev_centroids =  (uint8_t*)(tmp_buffer + 9*n_cluster*sizeof(uint32_t));
    memset(tmp_buffer, 0, (12*n_cluster*sizeof(uint32_t)));
    for (i = 0; i < n_cluster; i++) {
        size_t idx = rand() % n_samples;
        uint8_t r = p_src[3*idx], g = p_src[3*idx+1], b = p_src[3*idx+2];
        for (j = 0; j < i; j++) {
            if (centroids[3*j] == r && centroids[3*j+1] == g && centroids[3*j+2] == b) {
                break;
            }
        }
        centroids[3*i] = r;
        centroids[3*i+1] = g;
        centroids[3*i+2] = b;
    }
    for (iter = 0; iter < 100; iter++) {
        int changed = 0;
        memcpy(prev_centroids, centroids, 3 * n_cluster * sizeof(uint8_t));
        memset(sums, 0, 3 * n_cluster * sizeof(uint32_t));
        memset(counts, 0, n_cluster * sizeof(uint32_t));
        for (i = 0; i < n_samples; i++) {
            uint8_t r = p_src[3*i], g = p_src[3*i+1], b = p_src[3*i+2];
            size_t best_cluster = 0;
            uint32_t best_dist = UINT32_MAX;
            for (j = 0; j < n_cluster; j++) {
                int dr = r - centroids[3*j];
                int dg = g - centroids[3*j+1];
                int db = b - centroids[3*j+2];
                uint32_t dist = dr*dr + dg*dg + db*db;
                if (dist < best_dist) {
                    best_dist = dist;
                    best_cluster = j;
                }
            }
            p_index[i] = (uint8_t)best_cluster;
            sums[3*best_cluster]   += r;
            sums[3*best_cluster+1] += g;
            sums[3*best_cluster+2] += b;
            counts[best_cluster]++;
        }
        for (j = 0; j < n_cluster; j++) {
            if (counts[j] > 0) {
                uint8_t r = (uint8_t)((sums[3*j]   + counts[j]/2) / counts[j]);
                uint8_t g = (uint8_t)((sums[3*j+1] + counts[j]/2) / counts[j]);
                uint8_t b = (uint8_t)((sums[3*j+2] + counts[j]/2) / counts[j]);
                if (r != centroids[3*j] || g != centroids[3*j+1] || b != centroids[3*j+2]) {
                    changed = 1;
                }
                centroids[3*j]   = r;
                centroids[3*j+1] = g;
                centroids[3*j+2] = b;
            } else {
                size_t idx = rand() % n_samples;
                centroids[3*j]   = p_src[3*idx];
                centroids[3*j+1] = p_src[3*idx+1];
                centroids[3*j+2] = p_src[3*idx+2];
                changed = 1;
            }
        }
        if (!changed) break;
    }
    memcpy(p_palette, centroids, 3 * n_cluster * sizeof(uint8_t));
}


static void write_png_chunk (char *p_name, uint8_t *p_data, uint32_t len, FILE *fp) {
    const static uint32_t crc_table[] = {0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };
    uint32_t i, crc=0xFFFFFFFF;
    fputc(((len>>24) & 0xFF), fp);
    fputc(((len>>16) & 0xFF), fp);
    fputc(((len>> 8) & 0xFF), fp);
    fputc(((len    ) & 0xFF), fp);
    fwrite(p_name, sizeof(uint8_t),   4, fp);
    if (len > 0) {
        fwrite(p_data, sizeof(uint8_t), len, fp);
    }
    for (i=0; i<4; i++) {
        crc ^= p_name[i];
        crc = (crc >> 4) ^ crc_table[crc & 15];
        crc = (crc >> 4) ^ crc_table[crc & 15];
    }
    for (i=0; i<len; i++) {
        crc ^= p_data[i];
        crc = (crc >> 4) ^ crc_table[crc & 15];
        crc = (crc >> 4) ^ crc_table[crc & 15];
    }
    crc = ~crc;
    fputc(((crc>>24) & 0xFF), fp);
    fputc(((crc>>16) & 0xFF), fp);
    fputc(((crc>> 8) & 0xFF), fp);
    fputc(((crc    ) & 0xFF), fp);
}


// return:   0 : success    1 : failed
// n_rgb_cluster_to_palette:    0:do not cluster (color_type=0 or 2)    >0:cluster to palette (color_type=3)
int writePNGImageFile (const char *p_filename, const uint8_t *p_buf, int is_rgb, uint32_t height, uint32_t width, int n_rgb_cluster_to_palette) {
    FILE    *fp;
    uint8_t *p_dst, *p_palette_index=NULL, *p_tmp_buffer_for_kmeans=NULL;
    size_t   byte_per_row;
    uint8_t  color_type, bit_depth=8;
    
    if (width < 1 || height < 1)
        return 1;
    
    if (!is_rgb) {
        color_type   = 0;                       // gray, 8-bit per pixel
        byte_per_row = 1 + width;
    } else if (n_rgb_cluster_to_palette <= 0 || ((size_t)height*width <= 256)) {
        n_rgb_cluster_to_palette = 0;
        color_type   = 2;                       // RGB, 24-bit per pixel
        byte_per_row = 1 + width * 3;
    } else {
        color_type   = 3;                       // RGB, 8-bit palette index per pixel
        if (n_rgb_cluster_to_palette < 2  ) n_rgb_cluster_to_palette = 2;
        if (n_rgb_cluster_to_palette > 256) n_rgb_cluster_to_palette = 256;
        if      (n_rgb_cluster_to_palette <= 2 ) bit_depth = 1;
        else if (n_rgb_cluster_to_palette <= 4 ) bit_depth = 2;
        else if (n_rgb_cluster_to_palette <= 16) bit_depth = 4;
        byte_per_row = 1 + (width + (8/bit_depth) - 1) / (8/bit_depth);
    }
    
    if (color_type == 3) {
        p_dst = (uint8_t*)malloc((size_t)(byte_per_row+8)*height + 1048576 + (size_t)height*width + 64 + 12*256*sizeof(uint32_t));
        p_palette_index = p_dst+ (size_t)(byte_per_row+8)*height + 1048576;
        p_tmp_buffer_for_kmeans = p_palette_index +                          (size_t)height*width + 64;
    } else {
        p_dst = (uint8_t*)malloc((size_t)(byte_per_row+8)*height + 1048576);
    }

    if (p_dst == NULL)
        return 1;
    
    if ((fp = fopen(p_filename, "wb")) == NULL) {
        free(p_dst);
        return 1;
    }
    
    fwrite("\x89PNG\r\n\32\n", sizeof(char), 8, fp);    // 8-bit PNG magic
    
    {
        p_dst[ 0] = (uint8_t)( width>>24);
        p_dst[ 1] = (uint8_t)( width>>16);
        p_dst[ 2] = (uint8_t)( width>> 8);
        p_dst[ 3] = (uint8_t)( width    );
        p_dst[ 4] = (uint8_t)(height>>24);
        p_dst[ 5] = (uint8_t)(height>>16);
        p_dst[ 6] = (uint8_t)(height>> 8);
        p_dst[ 7] = (uint8_t)(height    );
        p_dst[ 8] = bit_depth;
        p_dst[ 9] = color_type;
        p_dst[10] = 0;
        p_dst[11] = 0;
        p_dst[12] = 0;
        write_png_chunk("IHDR", p_dst, 13, fp);
    }

    if (color_type == 3) {
        palette_kmeans_quantize(p_buf, (((size_t)width)*height), n_rgb_cluster_to_palette, p_dst, p_palette_index, p_tmp_buffer_for_kmeans);
        p_buf = p_palette_index;
        write_png_chunk("PLTE", p_dst, (3*n_rgb_cluster_to_palette), fp);
    }
    
    {
        size_t h, w, i=0;
        uint32_t adler_a=1, adler_b=0;
        uint8_t *p=p_dst , *p_last_blk=p_dst;
        *p++ = 0x78;
        *p++ = 0x01;
        for (h=0; h<height; h++) {
            const uint8_t *p_pixel = p_buf;
            for (w=0; w<byte_per_row; w++) {
                if (i%0xFFFF == 0) {
                    *p++ = 0;                 // deflate block start (5bytes)
                    *p++ = 0xFF;
                    *p++ = 0xFF;
                    *p++ = 0x00;
                    *p++ = 0x00;
                    p_last_blk = p;
                }
                if (w == 0) {
                    *p = 0;                   // filter at each start of line
                } else if (bit_depth == 1) {
                    *p = (p_pixel[0]<<7) | (p_pixel[1]<<6) | (p_pixel[2]<<5) | (p_pixel[3]<<4) | (p_pixel[4]<<3) | (p_pixel[5]<<2) | (p_pixel[6]<<1) | (p_pixel[7]);
                    p_pixel += 8;
                } else if (bit_depth == 2) {
                    *p = (p_pixel[0]<<6) | (p_pixel[1]<<4) | (p_pixel[2]<<2) | (p_pixel[3]);
                    p_pixel += 4;
                } else if (bit_depth == 4) {
                    *p = (p_pixel[0]<<4) | (p_pixel[1]);
                    p_pixel += 2;
                } else {
                    *p = p_pixel[0];
                    p_pixel ++;
                }
                adler_a = (adler_a + *p)      % 65521;
                adler_b = (adler_b + adler_a) % 65521;
                p ++;
                i ++;
            }
            p_buf += (color_type==2) ? (3*width) : width;
        }
        {
            uint32_t len_last_block = p - p_last_blk;
            p_last_blk[-5] = 1;
            p_last_blk[-4] = (  len_last_block    ) & 0xFF;
            p_last_blk[-3] = (  len_last_block >>8) & 0xFF;
            p_last_blk[-2] = ((~len_last_block)   ) & 0xFF;
            p_last_blk[-1] = ((~len_last_block)>>8) & 0xFF;
            adler_a |= (adler_b << 16);
            *p++ = (adler_a>>24) & 0xFF;
            *p++ = (adler_a>>16) & 0xFF;
            *p++ = (adler_a>> 8) & 0xFF;
            *p++ = (adler_a    ) & 0xFF;
        }
        write_png_chunk("IDAT", p_dst, (size_t)(p-p_dst), fp);
    }
    
    write_png_chunk("IEND", NULL, 0, fp);
    
    free(p_dst);
    fclose(fp);
    return 0;
}







// #include "uPNG/uPNG.h"


// // return:  NULL     : failed
// //          non-NULL : pointer to image pixels, allocated by malloc(), need to be free() later
// uint8_t* loadPNGImageFile (const char *p_filename, int *p_is_rgb, uint32_t *p_height, uint32_t *p_width) {
//     upng_t     *p_upng;
//     upng_error  err;
//     upng_format png_format;
//     static const char *upng_format_names[] = {
//         (const char*)"BADFORMAT",
//         (const char*)"RGB8",
//         (const char*)"RGB16",
//         (const char*)"RGBA8",
//         (const char*)"RGBA16",
//         (const char*)"LUMA1",
//         (const char*)"LUMA2",
//         (const char*)"LUMA4",
//         (const char*)"LUMA8",
//         (const char*)"LUMA_ALPHA1",
//         (const char*)"LUMA_ALPHA2",
//         (const char*)"LUMA_ALPHA4",
//         (const char*)"LUMA_ALPHA8"
//     };
//     size_t img_size;
//     uint8_t *p_dst_base, *p_dst;
//     const uint8_t *p_src;
//     p_upng = upng_new_from_file(p_filename);
//     if (p_upng == NULL) {
//         return NULL;
//     }
//     err = upng_decode(p_upng);
//     if (err != UPNG_EOK) {
//         if (err==UPNG_EUNSUPPORTED || err==UPNG_EUNINTERLACED || err==UPNG_EUNFORMAT)
//             printf("   ***ERROR: this PNG format is not-yet supported, error code = %d\n", err);
//         upng_free(p_upng);
//         return NULL;
//     }
//     png_format = upng_get_format(p_upng);
//     if (png_format != UPNG_RGBA8 && png_format != UPNG_RGB8 && png_format != UPNG_LUMINANCE8) {
//         printf("   ***ERROR: only support LUMA8, RGB8, and RGBA8. But this PNG is %s\n", upng_format_names[png_format]);
//         upng_free(p_upng);
//         return NULL;
//     }
//     *p_is_rgb = (png_format != UPNG_LUMINANCE8);
//     *p_height = upng_get_height(p_upng);
//     *p_width  = upng_get_width(p_upng);
//     img_size = (size_t)((*p_is_rgb)?3:1) * (*p_height) * (*p_width);
//     p_dst_base = p_dst = (uint8_t*)malloc(img_size);
//     if (p_dst_base) {
//         size_t i;
//         p_src = upng_get_buffer(p_upng);
//         if (png_format == UPNG_RGBA8) {
//             printf("   *warning: disard alpha channel of this PNG\n");
//             for (i=(size_t)(*p_height)*(*p_width); i>0; i--) {
//                 p_dst[0] = p_src[0];
//                 p_dst[1] = p_src[1];
//                 p_dst[2] = p_src[2];
//                 p_dst += 3;
//                 p_src += 4;
//             }
//         } else {
//             for (i=img_size; i>0; i--) {
//                 *(p_dst++) = *(p_src++);
//             }
//         }
//     }
//     upng_free(p_upng);
//     return p_dst_base;
// }
