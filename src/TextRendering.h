#pragma once

#include <string>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <learnopengl/shader.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>



/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

class TextRender {
public:
    TextRender();
    std::map<GLchar, Character> Characters;
    void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);
    unsigned int VAO, VBO;
    int src_width;
    int src_height;
    void setScreenSize(int width, int height);
    Shader* shader;

    // FreeType
    // --------
    FT_Library ft;
    std::string font_name = "../resources/fonts/Antonio-Bold.ttf";
    FT_Face face;
};