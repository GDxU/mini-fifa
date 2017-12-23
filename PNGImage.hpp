#pragma once

#include <cstdio>
#include <csetjmp>
#include <png.h>

#include "Debug.hpp"
#include "Logger.hpp"
#include "Image.hpp"

namespace img {
struct PNGImage : public Image {
  PNGImage(const char *filename):
    Image(filename)
  {}
  ~PNGImage()
  {}
  void init() {
    unsigned char header[8];    // 8 is the maximum size that can be checked
    format = GL_RGBA;

    /* open file and test for it being a png */
    FILE *fp = fopen(filename.c_str(), "rb");
    ASSERT(fp != NULL);
    fread(header, 1, 8, fp);
    if(png_sig_cmp(header, 0, 8)) {
      Logger::Error("[read_png_file] File %s is not recognized as a PNG file", filename.c_str());
      return;
    }

    /* initialize stuff */
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if(png_ptr == NULL) {
      Logger::Error("[read_png_file] png_create_read_struct failed");
      return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr) {
      Logger::Error("[read_png_file] png_create_info_struct failed");
      return;
    }

    if(setjmp(png_jmpbuf(png_ptr))) {
      Logger::Error("[read_png_file] Error during init_io");
      return;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    int color_type = png_get_color_type(png_ptr, info_ptr);
    int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    int number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    /* read file */
    if(setjmp(png_jmpbuf(png_ptr))) {
      Logger::Error("[read_png_file] read_image failed");
      return;
    }

    size_t bpp = png_get_channels(png_ptr, info_ptr);

    data = new unsigned char[height * width * bpp];
    png_bytep row_pointers[height];
    for(size_t y = 0; y < height; ++y) {
      row_pointers[y] = &data[y * width * bpp];
    }

    png_read_image(png_ptr, row_pointers);

    fclose(fp);

    if(bpp == 3)
      format = GL_RGB;
    else if(bpp == 4)
      format = GL_RGBA;
    else
      Logger::Error("unknown image format %d for file %s\n", bpp, filename.c_str()),abort();

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  }
};
}