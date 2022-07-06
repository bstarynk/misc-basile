#include "../include/window.h"
#include "../include/shader.h"
#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


struct rps_window__ {
        GLFWwindow *wnd;
        rps_shader_t *shd;
};


static void rps_window_eventhnd__(rps_window_t *);
static void rps_window_resize__(GLFWwindow *, int, int);


// Creates new GLFW window
rps_window_t *rps_window_new(rps_shader_t *shd)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int width = 800;
    const int height = 600;
    const char *title = "RefPerSys";

    rps_window_t *ctx = (rps_window_t *) malloc(sizeof (*ctx));
    if (!ctx) {
            printf("Failed to allocated memory for rps_window_t!\n");
            abort();
    }

    ctx->wnd = glfwCreateWindow(width, height, title, NULL, NULL);

    if (!ctx->wnd) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        abort();
    }

    glfwMakeContextCurrent(ctx->wnd);
    glfwSetFramebufferSizeCallback(ctx->wnd, rps_window_resize__);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        glfwTerminate();
        abort();
    }

    ctx->shd = shd;
    rps_shader_init(shd);
    return ctx;
}


// Closes window
void
rps_window_free(rps_window_t **ctx)
{
        rps_window_t *w;

        if (ctx && (w = *ctx)) {
            glfwTerminate();
            free(w);
        }

        *ctx = NULL;
}


// Main window event loop
void
rps_window_run(rps_window_t *ctx)
{
        while (!glfwWindowShouldClose(ctx->wnd)) {
                rps_window_eventhnd__(ctx);

                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                rps_shader_render(ctx->shd);
                glfwSwapBuffers(ctx->wnd);
                glfwPollEvents();
        }
}


// Callback to handle window events
void
rps_window_eventhnd__(rps_window_t *ctx)
{
    if (glfwGetKey(ctx->wnd, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(ctx->wnd, true);
}


// Callback on window resize
void
rps_window_resize__(GLFWwindow *ctx, int w, int h)
{
    (void) ctx; // will be used later
    glViewport(0, 0, w, h);
}
