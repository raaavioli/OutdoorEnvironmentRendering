#ifndef WR_FRAMEBUFFER_H
#define WR_FRAMEBUFFER_H

#include <glad/glad.h>

#include "texture.h"

enum AttachmentType : uint32_t
{
    COLOR   = 1,
    DEPTH   = 2,
    // TODO: STENCIL = 4
};

enum ColorFormat : uint32_t
{
    NONE    = 0,
    RGB8    = 1,
    RGBA8   = 2,
    RG16    = 3,
    RGBA32F = 4,
    // Add as needed
};

uint32_t GetGLInternalFormat(ColorFormat fmt);
uint32_t GetGLDataFormat(ColorFormat fmt);
uint32_t GetGLDataType(ColorFormat fmt);

struct FrameBuffer {
    FrameBuffer(uint32_t width, uint32_t height, uint32_t attachment_bits, ColorFormat color_format);

    inline void bind() { 
      glViewport(0, 0, width, height);
      glBindFramebuffer(GL_FRAMEBUFFER, this->renderer_id); 
    };
    inline void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    inline GLuint get_color_attachment() { return this->color_attachment; };
    inline GLuint get_depth_attachment() { return this->depth_attachment; };

private:
    GLuint renderer_id;
    GLuint color_attachment;
    GLuint depth_attachment;
    int width, height;
};

#endif // WR_FRAMEBUFFER_H