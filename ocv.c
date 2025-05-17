#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "raylib.h"

#define DEVICE      "/dev/video0"
#define WIDTH       640
#define HEIGHT      480
#define PIXEL_FORMAT V4L2_PIX_FMT_YUYV

typedef struct {
    void   *start;
    size_t length;
} Buffer;

static int  fd = -1;
static Buffer buffers[4];

// Simple YUYV → RGB888 converter
static void yuyv_to_rgb(const unsigned char *yuyv, unsigned char *rgb) {
    for (int i = 0; i < WIDTH * HEIGHT * 2; i += 4) {
        int y0 = yuyv[i + 0], u = yuyv[i + 1], y1 = yuyv[i + 2], v = yuyv[i + 3];
        int c0 = y0 - 16, c1 = y1 - 16, d = u - 128, e = v - 128;
        // Pixel 0
        int r = (298 * c0 + 409 * e + 128) >> 8;
        int g = (298 * c0 - 100 * d - 208 * e + 128) >> 8;
        int b = (298 * c0 + 516 * d + 128) >> 8;
        rgb[0] = (unsigned char) (r < 0 ? 0 : r > 255 ? 255 : r);
        rgb[1] = (unsigned char) (g < 0 ? 0 : g > 255 ? 255 : g);
        rgb[2] = (unsigned char) (b < 0 ? 0 : b > 255 ? 255 : b);
        // Pixel 1
        r = (298 * c1 + 409 * e + 128) >> 8;
        g = (298 * c1 - 100 * d - 208 * e + 128) >> 8;
        b = (298 * c1 + 516 * d + 128) >> 8;
        rgb[3] = (unsigned char) (r < 0 ? 0 : r > 255 ? 255 : r);
        rgb[4] = (unsigned char) (g < 0 ? 0 : g > 255 ? 255 : g);
        rgb[5] = (unsigned char) (b < 0 ? 0 : b > 255 ? 255 : b);
        rgb += 6;
    }
}

int main(void) {
    // 1. Open V4L2 device
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) { perror("Open device"); return EXIT_FAILURE; }

    // 2. Query capabilities
    struct v4l2_capability cap = {0};
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        perror("QueryCap");
        return EXIT_FAILURE;
    } // :contentReference[oaicite:5]{index=5}

    // 3. Set image format
    struct v4l2_format fmt = {0};
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = WIDTH;
    fmt.fmt.pix.height      = HEIGHT;
    fmt.fmt.pix.pixelformat = PIXEL_FORMAT;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        perror("SetFmt");
        return EXIT_FAILURE;
    } // :contentReference[oaicite:6]{index=6}

    // 4. Request buffers
    struct v4l2_requestbuffers req = {0};
    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        perror("ReqBufs");
        return EXIT_FAILURE;
    }

    // 5. Map buffers
    for (int i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            perror("QueryBuf");
            return EXIT_FAILURE;
        }
        buffers[i].length = buf.length;
        buffers[i].start  = mmap(NULL, buf.length,
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            perror("MMAP");
            return EXIT_FAILURE;
        }
    } // :contentReference[oaicite:7]{index=7}

    // 6. Queue buffers and start streaming
    for (int i = 0; i < 4; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        ioctl(fd, VIDIOC_QBUF, &buf);
    }
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMON, &type);

    // 7. Raylib initialization
    InitWindow(WIDTH, HEIGHT, "V4L2 Camera → Raylib");     // :contentReference[oaicite:8]{index=8}
    SetTargetFPS(60);                                       // :contentReference[oaicite:9]{index=9}
    // Texture2D camTex = LoadTextureFromImage(Image);         // placeholder
    Texture2D camTex;

    // Allocate CPU buffer for RGB data
    unsigned char *rgbBuffer = malloc(WIDTH * HEIGHT * 3);

    // Actually create the Raylib texture
    camTex.width  = WIDTH;
    camTex.height = HEIGHT;
    camTex.mipmaps = 1;
    camTex.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;                   // Raylib enum for RGB
    UpdateTexture(camTex, rgbBuffer);                       // dummy to init GPU texture :contentReference[oaicite:10]{index=10}

    // 8. Main loop
    while (!WindowShouldClose()) {
        // Dequeue frame
        struct v4l2_buffer buf = {0};
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            perror("DQBUF");
            break;
        }

        // Convert YUYV→RGB and upload
        yuyv_to_rgb(buffers[buf.index].start, rgbBuffer);
        UpdateTexture(camTex, rgbBuffer);                   // :contentReference[oaicite:11]{index=11}

        // Re-queue buffer
        ioctl(fd, VIDIOC_QBUF, &buf);

        // Draw
        BeginDrawing();
          ClearBackground(BLACK);
          DrawTexture(camTex, 0, 0, WHITE);
          DrawText(TextFormat("FPS: %02i", GetFPS()), 10, 10, 20, GRAY);
        EndDrawing();
    }

    // 9. Cleanup
    CloseWindow();
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    for (int i = 0; i < 4; ++i) munmap(buffers[i].start, buffers[i].length);
    close(fd);
    free(rgbBuffer);

    return 0;
}
