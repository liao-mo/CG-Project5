///////////////////////////////////////////////////////////////////////////////
// FrameBuffer.h
// =============
// class for OpenGL Frame Buffer Object (FBO)
// It contains a 32bit color buffer and a depth buffer as GL_DEPTH_COMPONENT24
// Call init() to create/resize a FBO with given width and height params.
// It supports MSAA (Multi Sample Anti Aliasing) FBO. If msaa=0, it creates a
// single-sampled FBO. If msaa > 0 (even number), it creates a multi-sampled
// FBO.
//
// NOTE: This class does not use GL_ARB_texture_multisample extension yet. Call
//       update() function explicitly to blit color/depth buffers from
//       multi-sample FBO to single-sample FBO if you want to get single-sample
//       color and depth data from MSAA FBO.
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2015-03-05
// UPDATED: 2020-05-07
///////////////////////////////////////////////////////////////////////////////

#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#endif
#include "glad/glad.h"

#include <string>

class FrameBuffer
{
public:
    FrameBuffer();
    ~FrameBuffer();

    bool init(int width, int height, int msaa=0);   // create buffer objects
    void bind();                                    // bind fbo
    void unbind();                                  // unbind fbo
    void update();                                  // copy multi-sample to single-sample and generate mipmaps

    void blitColorTo(GLuint fbo, int x=0, int y=0, int w=0, int h=0);   // copy color buffer only
    void blitDepthTo(GLuint fbo, int x=0, int y=0, int w=0, int h=0);   // copy depth buffer only

    void copyColorBuffer();                         // copy color to array
    void copyDepthBuffer();                         // copy depth to array
    const unsigned char* getColorBuffer() const     { return colorBuffer; }
    const float* getDepthBuffer() const             { return depthBuffer; }

    GLuint getId() const;
    GLuint getColorId() const                       { return texId; }   // single-sample texture object
    GLuint getDepthId() const                       { return rboId; }   // single-sample rbo

    int getWidth() const                            { return width; }
    int getHeight() const                           { return height; }
    int getMsaa() const                             { return msaa; }
    std::string getStatus() const;                  // return FBO info
    std::string getErrorMessage() const             { return errorMessage; }

protected:

private:
    // member functions
    void deleteBuffers();
    bool checkFrameBufferStatus();

    static std::string getTextureParameters(GLuint id);
    static std::string getRenderbufferParameters(GLuint id);
    static std::string convertInternalFormatToString(GLenum format);

    // member vars
    int width;                      // buffer width
    int height;                     // buffer height
    int msaa;                       // # of multi samples; 0, 2, 4, 8,...
    unsigned char* colorBuffer;     // color buffer (rgba)
    float* depthBuffer;             // depth buffer
    GLuint fboMsaaId;               // primary id for multisample FBO
    GLuint rboMsaaColorId;          // id for multisample RBO (color buffer)
    GLuint rboMsaaDepthId;          // id for multisample RBO (depth buffer)
    GLuint fboId;                   // secondary id for frame buffer object
    GLuint texId;                   // id for texture object (color buffer)
    GLuint rboId;                   // id for render buffer object (depth buffer)
    std::string errorMessage;
};

#endif
