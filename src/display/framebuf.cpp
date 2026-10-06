#include <algorithm>
#include "framebuf.h"
#include "font_petme128_8x8.h"

framebuf::framebuf(uint16_t width_, uint16_t heigth_) : width(width_), height(heigth_) {

}

void framebuf::line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color) {
    int dx = x2 - x1;
    int sx;
    if (dx > 0) {
        sx = 1;
    } else {
        dx = -dx;
        sx = -1;
    }

    int dy = y2 - y1;
    int sy;
    if (dy > 0) {
        sy = 1;
    } else {
        dy = -dy;
        sy = -1;
    }

    bool steep;
    if (dy > dx) {
        int temp;
        temp = x1;
        x1 = y1;
        y1 = temp;
        temp = dx;
        dx = dy;
        dy = temp;
        temp = sx;
        sx = sy;
        sy = temp;
        steep = true;
    } else {
        steep = false;
    }

    int e = 2 * dy - dx;
    for (int i = 0; i < dx; ++i) {
        if (steep) {
            if (0 <= y1 && y1 < width && 0 <= x1 && x1 < height) {
                setpixel(y1, x1, color);
            }
        } else {
            if (0 <= x1 && x1 < width && 0 <= y1 && y1 < height) {
                setpixel(x1, y1, color);
            }
        }
        while (e >= 0) {
            y1 += sy;
            e -= 2 * dx;
        }
        x1 += sx;
        e += 2 * dy;
    }

    if (0 <= x2 && x2 < width && 0 <= y2 && y2 < height) {
        setpixel(x2, y2, color);
    }

}

void framebuf::hline(uint16_t x, uint16_t y, uint16_t w, uint32_t color) {
    fill_rect(x, y, w, 1, color);
}

void framebuf::vline(uint16_t x, uint16_t y, uint16_t h, uint32_t color) {
    fill_rect(x, y, 1, h, color);
}

void framebuf::rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color, bool fill) {
    if (fill) {
        fill_rect(x, y, w, h, color);
    } else {
        fill_rect(x, y, w, 1, color);
        fill_rect(x, y + h - 1, w, 1, color);
        fill_rect(x, y, 1, h, color);
        fill_rect(x + w - 1, y, 1, h, color);
    }

}

void framebuf::text(const std::string &str, uint16_t x, uint16_t y, uint32_t color) {
    text(str.c_str(), x, y, color);
}

void framebuf::text(const char *str, uint16_t x, uint16_t y, uint32_t color) {

    for (; *str; ++str) {

        int chr = *(uint8_t *)str;
        if (chr < 32 || chr > 127) {
            chr = 127;
        }

        const uint8_t *chr_data = &font_petme128_8x8[(chr - 32) * 8];

        for (int j = 0; j < 8; j++, x++) {
            if (0 <= x && x < width) {
                uint32_t vline_data = chr_data[j];
                for (int y1 = y; vline_data; vline_data >>= 1, y1++) {
                    if (vline_data & 1) {
                        if (0 <= y1 && y1 < height) {
                            setpixel(x, y1, color);
                        }
                    }
                }
            }
        }
    }

}

void framebuf::fill(uint32_t color) {
    fill_rect(0, 0, width, height, color);
}

void framebuf::blit(framebuf &source, int16_t x, int16_t y, uint32_t key, const framebuf *palette) {

    if (
            (x >= width) ||
            (y >= height) ||
            (-x >= source.width) ||
            (-y >= source.height)
            ) {

        return;
    }

    int32_t x0 = std::max(0, (int)x);
    int32_t y0 = std::max(0, (int)y);
    int32_t x1 = std::max(0, -x);
    int32_t y1 = std::max(0, -y);
    int32_t x0end = std::min((int32_t)width, (int32_t)x + source.width);
    int32_t y0end = std::min((int32_t)height, (int32_t) y + source.height);

    for (; y0 < y0end; ++y0) {
        int cx1 = x1;
        for (int cx0 = x0; cx0 < x0end; ++cx0) {
            uint32_t col = source.getpixel(cx1, y1);
            if (palette) {
                col = palette->getpixel(col, 0);
            }
            if (col != key) {
                setpixel(cx0, y0, col);
            }
            ++cx1;
        }
        ++y1;
    }

}

void framebuf::scroll(int16_t xstep, int16_t ystep) {
    int sx, y, xend, yend, dx, dy;
    if (xstep < 0) {
        sx = 0;
        xend = width + xstep;
        if (xend <= 0) {
            return;
        }
        dx = 1;
    } else {
        sx = width - 1;
        xend = xstep - 1;
        if (xend >= sx) {
            return;
        }
        dx = -1;
    }
    if (ystep < 0) {
        y = 0;
        yend = height + ystep;
        if (yend <= 0) {
            return;
        }
        dy = 1;
    } else {
        y = height - 1;
        yend = ystep - 1;
        if (yend >= y) {
            return;
        }
        dy = -1;
    }
    for (; y != yend; y += dy) {
        for (int x = sx; x != xend; x += dx) {
            setpixel(x, y, getpixel(x - xstep, y - ystep));
        }
    }

}
