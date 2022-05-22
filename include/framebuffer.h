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
    R32UI   = 5,
    // Add as needed
};

uint32_t GetGLInternalFormat(ColorFormat fmt);
uint32_t GetGLDataFormat(ColorFormat fmt);
uint32_t GetGLDataType(ColorFormat fmt);

struct FrameBufferTextureCreateInfo
{
    ColorFormat color_format;
    uint32_t min_filter = GL_LINEAR;
    uint32_t mag_filter = GL_LINEAR;
};

struct FrameBufferCreateInfo
{
    uint32_t width;
    uint32_t height;
    uint32_t attachment_bits;
    int num_color_attachments = 0;
    FrameBufferTextureCreateInfo* color_attachment_infos;
};

struct FrameBuffer {
    FrameBuffer(FrameBufferCreateInfo create_info, bool clamp_to_border = false);

    inline void bind() { 
      glViewport(0, 0, m_Width, m_Height);
      glBindFramebuffer(GL_FRAMEBUFFER, this->m_RendererId);
      GLuint attachments[8] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };
      glDrawBuffers(m_NumColorAttachments, attachments);
    };
    inline void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    inline GLuint get_color_attachment(int i) {
        if (i < m_NumColorAttachments)
            return this->m_ColorAttachments[i];
        return -1;
    };
    inline GLuint get_depth_attachment() { return this->m_DepthAttachment; };

private:
    GLuint m_RendererId;

    int m_NumColorAttachments;
    GLuint m_ColorAttachments[8];
    GLuint m_DepthAttachment;
    int m_Width, m_Height;
};

#endif // WR_FRAMEBUFFER_H