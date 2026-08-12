#define DOOM_IMPLEMENTATION
#define DOOM_IMPLEMENT_FILE_IO
#define DOOM_IMPLEMENT_PRINT
#define DOOM_IMPLEMENT_MALLOC
#include "PureDOOM.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define TARGET_FRAME_US 28500 // 35 FPS 原版标准

static int fb_fd = -1;
static int kbd_fd = -1;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static size_t fb_size = 0;
static uint8_t* fb_mem = NULL;
static uint16_t* pixel_index_map = NULL;

// --- 终极官方宏映射（基于你头文件的确切定义） ---
void handle_input() {
    if (kbd_fd < 0) return;
    struct input_event ev;
    while (read(kbd_fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type != EV_KEY) continue;
        int doom_key = 0;
        switch (ev.code) {
            // ------- 移动控制：物理上下左右 (官方宏) -------
            case KEY_UP: doom_key = DOOM_KEY_UP_ARROW; break;       // ↑ 前进
            case KEY_DOWN: doom_key = DOOM_KEY_DOWN_ARROW; break;   // ↓ 后退
            case KEY_LEFT: doom_key = DOOM_KEY_LEFT_ARROW; break;   // ← 左转
            case KEY_RIGHT: doom_key = DOOM_KEY_RIGHT_ARROW; break; // → 右转

            // ------- 战斗与交互 (现在全都有了官方定义) -------
            case KEY_LEFTCTRL: doom_key = DOOM_KEY_CTRL; break;     // 左Ctrl -> 开火
            case KEY_SPACE: doom_key = DOOM_KEY_SPACE; break;       // 空格 -> 使用/开门 (绝不会变成暂停)

            // ------- 26个英文字母：保留存档打字功能 -------
            case KEY_Q: doom_key = DOOM_KEY_Q; break;
            case KEY_W: doom_key = DOOM_KEY_W; break;
            case KEY_E: doom_key = DOOM_KEY_E; break;
            case KEY_R: doom_key = DOOM_KEY_R; break;
            case KEY_T: doom_key = DOOM_KEY_T; break;
            case KEY_Y: doom_key = DOOM_KEY_Y; break;
            case KEY_U: doom_key = DOOM_KEY_U; break;
            case KEY_I: doom_key = DOOM_KEY_I; break;
            case KEY_O: doom_key = DOOM_KEY_O; break;
            case KEY_P: doom_key = DOOM_KEY_P; break;
            case KEY_A: doom_key = DOOM_KEY_A; break;
            case KEY_S: doom_key = DOOM_KEY_S; break;
            case KEY_D: doom_key = DOOM_KEY_D; break;
            case KEY_F: doom_key = DOOM_KEY_F; break;
            case KEY_G: doom_key = DOOM_KEY_G; break;
            case KEY_H: doom_key = DOOM_KEY_H; break;
            case KEY_J: doom_key = DOOM_KEY_J; break;
            case KEY_K: doom_key = DOOM_KEY_K; break;
            case KEY_L: doom_key = DOOM_KEY_L; break;
            case KEY_Z: doom_key = DOOM_KEY_Z; break;
            case KEY_X: doom_key = DOOM_KEY_X; break;
            case KEY_C: doom_key = DOOM_KEY_C; break;
            case KEY_V: doom_key = DOOM_KEY_V; break;
            case KEY_B: doom_key = DOOM_KEY_B; break;
            case KEY_N: doom_key = DOOM_KEY_N; break;
            case KEY_M: doom_key = DOOM_KEY_M; break;

            // ------- 0-9 数字键：切枪 / 存档输数字 -------
            case KEY_1: doom_key = DOOM_KEY_1; break;
            case KEY_2: doom_key = DOOM_KEY_2; break;
            case KEY_3: doom_key = DOOM_KEY_3; break;
            case KEY_4: doom_key = DOOM_KEY_4; break;
            case KEY_5: doom_key = DOOM_KEY_5; break;
            case KEY_6: doom_key = DOOM_KEY_6; break;
            case KEY_7: doom_key = DOOM_KEY_7; break;
            case KEY_8: doom_key = DOOM_KEY_8; break;
            case KEY_9: doom_key = DOOM_KEY_9; break;
            case KEY_0: doom_key = DOOM_KEY_0; break;

            // ------- 菜单与控制 (官方宏完全支持) -------
            case KEY_LEFTSHIFT: doom_key = DOOM_KEY_SHIFT; break;   // 左Shift -> 加速跑
            case KEY_ESC: doom_key = DOOM_KEY_ESCAPE; break;        // ESC -> 暂停
            case KEY_ENTER: doom_key = DOOM_KEY_ENTER; break;       // 回车 -> 确认
        }
        if (doom_key) {
            if (ev.value == 1) doom_key_down(doom_key);
            else if (ev.value == 0) doom_key_up(doom_key);
        }
    }
}

int main(int argc, char** argv) {
    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0) { perror("open fb0"); return 1; }
    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo)) { perror("finfo"); close(fb_fd); return 1; }
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) { perror("vinfo"); close(fb_fd); return 1; }
    fb_size = finfo.line_length * vinfo.yres;
    printf("Screen: %dx%d, bpp=%d, line_len=%d, size=%zu\n", 
           vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, finfo.line_length, fb_size);
    
    int is_16bpp = (vinfo.bits_per_pixel == 16);
    printf("Screen mode: %s\n", is_16bpp ? "16-bit RGB565" : "32-bit RGB888");
    
    fb_mem = malloc(fb_size);
    if (!fb_mem) { perror("malloc fb"); close(fb_fd); return 1; }

    // 锁定键盘
    kbd_fd = open("/dev/input/event6", O_RDONLY | O_NONBLOCK);
    if (kbd_fd < 0) printf("⚠️ Keyboard not found, demo only.\n");
    else printf("✅ BY Tech Keyboard locked! Arrows move, Letters type, Ctrl fire, Space open!\n");

    int screen_w = vinfo.xres;   // 170
    int screen_h = vinfo.yres;   // 320
    int doom_w = 320;
    int doom_h = 200;

    pixel_index_map = malloc(screen_w * screen_h * sizeof(uint16_t));
    if (!pixel_index_map) { perror("index map malloc"); return 1; }
    int idx = 0;
    for (int y = 0; y < screen_h; y++) {
        for (int x = 0; x < screen_w; x++) {
            int src_y = x + 15;   // 裁切
            int src_x = 319 - y;  // 旋转
            if (src_y < 0) src_y = 0;
            if (src_y >= doom_h) src_y = doom_h - 1;
            if (src_x < 0) src_x = 0;
            if (src_x >= doom_w) src_x = doom_w - 1;
            pixel_index_map[idx++] = (uint16_t)(src_y * doom_w + src_x);
        }
    }

    doom_set_getenv(getenv);
    doom_set_resolution(doom_w, doom_h);
    doom_init(argc, argv, 1);

    printf("Warming up...\n");
    for (int i = 0; i < 100; i++) {
        doom_force_update();
        usleep(10000);
    }
    printf("Ready! Game locked at 35 FPS.\n");

    int pixels_per_line = finfo.line_length / 4;
    struct timespec start, end;

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        handle_input();
        doom_update();
        doom_force_update();

        const uint8_t* doom_fb = doom_get_framebuffer(4);
        if (!doom_fb) continue;
        const uint32_t* doom_pixels = (const uint32_t*)doom_fb;

        const uint16_t* lut_ptr = pixel_index_map;
        if (is_16bpp) {
            uint16_t* fb16 = (uint16_t*)fb_mem;
            for (int y = 0; y < screen_h; y++) {
                uint16_t* dst_row = fb16 + y * screen_w;
                for (int x = 0; x < screen_w; x++) {
                    uint32_t pix = doom_pixels[*lut_ptr++];
                    uint16_t r = (pix >> 16) & 0xFF;
                    uint16_t g = (pix >> 8) & 0xFF;
                    uint16_t b = pix & 0xFF;
                    dst_row[x] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                }
            }
        } else {
            uint32_t* fb32 = (uint32_t*)fb_mem;
            for (int y = 0; y < screen_h; y++) {
                uint32_t* dst_row = fb32 + y * pixels_per_line;
                for (int x = 0; x < screen_w; x++) {
                    uint32_t pix = doom_pixels[*lut_ptr++];
                    dst_row[x] = (pix & 0xFF00FF00) | ((pix & 0x00FF0000) >> 16) | ((pix & 0x000000FF) << 16);
                }
            }
        }

        lseek(fb_fd, 0, SEEK_SET);
        write(fb_fd, fb_mem, fb_size);
        ioctl(fb_fd, FBIOBLANK, FB_BLANK_UNBLANK);

        clock_gettime(CLOCK_MONOTONIC, &end);
        long long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_nsec - start.tv_nsec) / 1000;
        long long sleep_us = TARGET_FRAME_US - elapsed_us;
        if (sleep_us > 0) usleep(sleep_us);
    }

    free(pixel_index_map);
    free(fb_mem);
    close(fb_fd);
    if (kbd_fd > 0) close(kbd_fd);
    return 0;
}