/**
* Author: Michael Sanfilippo
* Assignment: Simple 2D Scene
* Date due: 2023-09-30, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define GL_GLEXT_PROTOTYPE 1
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.0f,
BG_BLUE = 0.0f,
BG_GREEN = 0.0f,
BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const int TRIANGLE_RED = 1.0,
TRIANGLE_BLUE = 0.4,
TRIANGLE_GREEN = 0.4,
TRIANGLE_OPACITY = 1.0;

const float MILLISECONDS_IN_SECOND = 1000.0f;
float VERTICAL_SPEED = 1.0f;  // Vertical movement speed

// Constants for circular motion of the orbiting triangle
const float CIRCULAR_SPEED = 200.0f;  // Speed of the circular motion

//TEXTURE VARIABLES
const char EARTH_SPRITE[] = "2dEarth.png";
const char MOON_SPRITE[] = "2dMoon.png";
GLuint earth_texture_id;
GLuint moon_texture_id;
const int num_of_textures = 1;
const GLint level_of_detail = 0;
const GLint texture_border = 0; 

SDL_Window* g_display_window;

bool g_game_is_running = true;
ShaderProgram g_program;
glm::mat4 g_view_matrix;
glm::mat4 g_model_matrix1; // Model matrix for the first triangle (vertically triangle)
glm::mat4 g_model_matrix2; // Model matrix for the second triangle (orbiting triangle)
glm::mat4 g_projection_matrix;

float g_triangle_y = 0.0f;  // Vertical position of the vertical triangle
float g_previous_ticks = 0.0f;
float orbiting_triangle_angle = 0.0f; // Angle for circular motion

//Load textures taht are to be rendered 
GLuint load_texture(const char* filepath)
{
    //loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    //If image is not found
    if (image == NULL) {
        LOG("Unable to load image. Make sure path is correct");
        LOG(filepath);
        assert(false);
    }

    //Generatign and Binding Texture ID to images 
    GLuint textureID;
    glGenTextures(num_of_textures, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, level_of_detail, GL_RGBA, width, height, texture_border, GL_RGBA, GL_UNSIGNED_BYTE, image);

    //Sets Texture Filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Release file from memory and return texture id
    stbi_image_free(image);

    return textureID;

}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Circular Motion",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    g_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);
    g_program.set_colour(TRIANGLE_RED, TRIANGLE_BLUE, TRIANGLE_GREEN, TRIANGLE_OPACITY);
    glUseProgram(g_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    //Load shaders and image
    earth_texture_id = load_texture(EARTH_SPRITE);
    moon_texture_id = load_texture(MOON_SPRITE);

    //Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_game_is_running = false;
        }
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_triangle_y += VERTICAL_SPEED * delta_time;

    if (g_triangle_y >= 3.75f) {
        g_triangle_y = 3.75f;
        VERTICAL_SPEED = -1.0f;
    }
    else if (g_triangle_y <= -3.75f) {
        g_triangle_y = -3.75f;
        VERTICAL_SPEED = 1.0f;
    }

    g_model_matrix1 = glm::mat4(1.0f);
    g_model_matrix1 = glm::translate(g_model_matrix1, glm::vec3(0.0f, g_triangle_y, 0.0f));
    float rotation_angle = 90.0f * ticks; // Rotate at 90 degrees per second
    g_model_matrix1 = glm::rotate(g_model_matrix1, glm::radians(rotation_angle), glm::vec3(0.0f, 0.0f, 1.0f));


    orbiting_triangle_angle += delta_time * CIRCULAR_SPEED;

    // Calculate the position of the triangle relative to the moving triangle
    float circle_radius = 1.5f; // Adjust this value to control the radius of the circle
    float orbiting_triangle_x = circle_radius * cos(glm::radians(orbiting_triangle_angle));
    float orbiting_triangle_y = circle_radius * sin(glm::radians(orbiting_triangle_angle));


    g_model_matrix2 = glm::mat4(1.0f);
    g_model_matrix2 = glm::translate(g_model_matrix2, glm::vec3(0.0f, g_triangle_y, 0.0f));
    g_model_matrix2 = glm::translate(g_model_matrix2, glm::vec3(orbiting_triangle_x, orbiting_triangle_y, 0.0f));
}


void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_program.set_model_matrix(g_model_matrix1);
    g_program.set_model_matrix(g_model_matrix2);

    float vertices[] = {
       -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
       -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };


    glVertexAttribPointer(g_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_program.get_position_attribute());


    glVertexAttribPointer(g_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_program.get_tex_coordinate_attribute());

    draw_object(g_model_matrix1, earth_texture_id);
    draw_object(g_model_matrix2, moon_texture_id);

    glDisableVertexAttribArray(g_program.get_position_attribute());
    glDisableVertexAttribArray(g_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
