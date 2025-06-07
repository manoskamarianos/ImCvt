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


static uint8_t * put_value_bigendian (uint8_t *p_buf, uint32_t value, int n) {
    n --;
    n *= 8;
    for (; n>=0; n-=8) {
        *(p_buf++) = (value >> n);
    }
    return p_buf;
}


static uint8_t * put_png_chunk (uint8_t *p_dst_start, uint8_t *p_dst_end, char *p_name) {
    const static uint32_t crc_table[] = {0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };
    uint32_t crc=0xFFFFFFFF;
    put_value_bigendian(p_dst_start, ((p_dst_end-p_dst_start)-8), 4);  // write chunk len (only payload)
    p_dst_start += 4;
    memcpy(p_dst_start, p_name, 4);                                    // write chunk name
    for (; p_dst_start<p_dst_end; p_dst_start++) {                     // calculate CRC (name + payload)
        crc ^= p_dst_start[0];
        crc = (crc >> 4) ^ crc_table[crc & 15];
        crc = (crc >> 4) ^ crc_table[crc & 15];
    }
    crc = ~crc;
    put_value_bigendian(p_dst_start, crc, 4);
    return p_dst_start + 4;
}


static uint8_t * put_uncompressed_deflate (uint8_t *p_dst, const uint8_t *p_img, uint32_t height, uint32_t width, size_t byte_per_row, uint8_t color_type, uint8_t bit_depth) {
    size_t h, w, deflate_block_pos=0;
    uint32_t adler_a=1, adler_b=0;
    *(p_dst++) = 0x78;
    *(p_dst++) = 0x01;
    for (h=0; h<height; h++) {
        const uint8_t *p_pixel = p_img;
        for (w=0; w<byte_per_row; w++) {
            if (deflate_block_pos == 0) {
                p_dst[0] = 0x00;  // deflate block start (5bytes)
                p_dst[1] = 0xFF;
                p_dst[2] = 0xFF;
                p_dst[3] = 0x00;
                p_dst[4] = 0x00;
                p_dst += 5;
            }
            if (w == 0) {
                p_dst[0] = 0;     // filter at each start of line
            } else if (bit_depth == 1) {
                p_dst[0] = (p_pixel[0]<<7) | (p_pixel[1]<<6) | (p_pixel[2]<<5) | (p_pixel[3]<<4) | (p_pixel[4]<<3) | (p_pixel[5]<<2) | (p_pixel[6]<<1) | (p_pixel[7]);
                p_pixel += 8;
            } else if (bit_depth == 2) {
                p_dst[0] = (p_pixel[0]<<6) | (p_pixel[1]<<4) | (p_pixel[2]<<2) | (p_pixel[3]);
                p_pixel += 4;
            } else if (bit_depth == 4) {
                p_dst[0] = (p_pixel[0]<<4) | (p_pixel[1]);
                p_pixel += 2;
            } else {
                p_dst[0] = p_pixel[0];
                p_pixel ++;
            }
            adler_a = (adler_a + p_dst[0]) % 65521;
            adler_b = (adler_b + adler_a)  % 65521;
            p_dst ++;
            deflate_block_pos = (deflate_block_pos + 1) % 0xFFFF;
        }
        p_img += (color_type==2) ? (3*width) : width;
    }
    p_dst[-deflate_block_pos-5] = 0x01;
    if (deflate_block_pos != 0) {
        p_dst[-deflate_block_pos-4] = (( deflate_block_pos)   ) & 0xFF;
        p_dst[-deflate_block_pos-3] = (( deflate_block_pos)>>8) & 0xFF;
        p_dst[-deflate_block_pos-2] = ((~deflate_block_pos)   ) & 0xFF;
        p_dst[-deflate_block_pos-1] = ((~deflate_block_pos)>>8) & 0xFF;
    }
    p_dst = put_value_bigendian(p_dst, adler_b, 2);
    p_dst = put_value_bigendian(p_dst, adler_a, 2);
    return p_dst;
}


// return:   0: failed    1: dst_size
// n_cluster:    0:do not cluster (color_type=0 or 2)    >0:cluster to palette (color_type=3)
static size_t png_encode (uint8_t *p_dst, const uint8_t *p_img, int is_rgb, uint32_t height, uint32_t width, int n_cluster) {
    uint8_t *p_dst_base=p_dst, *p_img_palette=NULL;
    uint8_t  color_type, bit_depth;
    size_t   byte_per_row;
    
    if (width < 1 || height < 1) {   // if width and height invalid, write a 1x1 gray image
        return 0;
    }
    
    if (((size_t)height)*width  <= 512) n_cluster = 0;
    if (n_cluster < 2  ) n_cluster = 0;
    if (n_cluster > 256) n_cluster = 256;
    
    if (!is_rgb) {
        color_type = 0;                       // gray, 8-bit per pixel
        bit_depth  = 8;
        byte_per_row = 1 + width;
    } else if (n_cluster == 0) {
        color_type = 2;                       // RGB, 24-bit per pixel
        bit_depth  = 8;
        byte_per_row = 1 + width * 3;
    } else {
        color_type = 3;                       // RGB, 8-bit palette index per pixel
        if      (n_cluster <= 2 ) bit_depth = 1;
        else if (n_cluster <= 4 ) bit_depth = 2;
        else if (n_cluster <= 16) bit_depth = 4;
        else                                     bit_depth = 8;
        byte_per_row = 1 + (width + (8/bit_depth) - 1) / (8/bit_depth);
    }
    
    // write 8-bit PNG magic
    memcpy(p_dst, "\x89PNG\r\n\32\n", 8);
    p_dst += 8;
    
    {   // write IHDR
        uint8_t *p_chunk_start = p_dst;
        p_dst += 8;
        p_dst = put_value_bigendian(p_dst, width, 4);
        p_dst = put_value_bigendian(p_dst, height, 4);
        p_dst = put_value_bigendian(p_dst, bit_depth, 1);
        p_dst = put_value_bigendian(p_dst, color_type, 1);
        p_dst = put_value_bigendian(p_dst, 0, 3);
        p_dst = put_png_chunk(p_chunk_start, p_dst, "IHDR");
    }
    
    if (color_type == 3) {
        uint8_t *p_chunk_start=p_dst, *p_tmp_buffer;
        p_dst += 8;
        p_img_palette = (uint8_t*)malloc((size_t)height*width + 12*256*sizeof(uint32_t));
        p_tmp_buffer  = p_img_palette +  (size_t)height*width;
        if (p_img_palette == NULL) {
            return 0;
        }
        palette_kmeans_quantize(p_img, (((size_t)width)*height), n_cluster, p_dst, p_img_palette, p_tmp_buffer);
        p_dst += (3 * n_cluster);
        p_dst = put_png_chunk(p_chunk_start, p_dst, "PLTE");
        p_img = p_img_palette;
    }
    
    {   // write IDAT
        uint8_t *p_chunk_start = p_dst;
        p_dst += 8;
        p_dst = put_uncompressed_deflate(p_dst, p_img, height, width, byte_per_row, color_type, bit_depth);
        p_dst = put_png_chunk(p_chunk_start, p_dst, "IDAT");
    }
    
    // write IEND
    p_dst = put_png_chunk(p_dst, p_dst+8, "IEND");
    
    if (color_type == 3) {
        free(p_img_palette);
    }
    
    return (p_dst - p_dst_base);
}


// return:   0 : success    1 : failed
int writePNGImageFile (const char *p_filename, const uint8_t *p_buf, int is_rgb, uint32_t height, uint32_t width, int n_cluster) {
    FILE *fp;
    size_t png_size;
    uint8_t *p_png_buf = (uint8_t*)malloc(((size_t)(is_rgb?3:1)) * ((size_t)height + 16) * ((size_t)width + 16) + 1048576);
    if (p_png_buf == NULL) {
        return 1;
    }
    png_size = png_encode(p_png_buf, p_buf, is_rgb, height, width, n_cluster);
    if (png_size <= 0) {
        free(p_png_buf);
        return 1;
    }
    fp = fopen(p_filename, "wb");
    if (fp == NULL) {
        free(p_png_buf);
        return 1;
    }
    if (fwrite(p_png_buf, sizeof(uint8_t), png_size, fp) != png_size) {
        free(p_png_buf);
        fclose(fp);
        return 1;
    }
    free(p_png_buf);
    fclose(fp);
    return 0;
}
