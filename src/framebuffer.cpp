#include "framebuffer.h"

#include <iostream>
#include <gl_helpers.h>

FrameBuffer::FrameBuffer(uint32_t width, uint32_t height, uint32_t attachment_bits, ColorFormat color_format) : width(width), height(height) {
    glGenFramebuffers(1, &this->renderer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, this->renderer_id);

    if (attachment_bits & AttachmentType::COLOR)
    {
        glGenTextures(1, &this->color_attachment);
        glBindTexture(GL_TEXTURE_2D, this->color_attachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GetGLInternalFormat(color_format), width, height, 0, GetGLDataFormat(color_format), GetGLDataType(color_format), NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->color_attachment, 0);
    }

    if (attachment_bits & AttachmentType::DEPTH)
    {
        glGenTextures(1, &this->depth_attachment);
        glBindTexture(GL_TEXTURE_2D, this->depth_attachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depth_attachment, 0);
    }

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR (FrameBuffer): Framebuffer was not successfully created (" 
            << status << ")" << std::endl;
    }
    unbind();
}

uint32_t GetGLInternalFormat(ColorFormat fmt)
{
    switch (fmt)
    {
    case RGB8:
        return GL_RGB8;
    case RGBA8:
        return GL_RGBA8;
    case RG16:
        return GL_RG16;
    case RGBA32F:
        return GL_RGBA32F;
    default:
        std::cout << "Error: Color format '" << fmt << "' is invalid" << std::endl;
        return 0;
    }
}

uint32_t GetGLDataFormat(ColorFormat fmt)
{
    switch (fmt)
    {
    case RGB8:
        return GL_RGB;
    case RGBA8:
        return GL_RGBA;
    case RG16:
        return GL_RG;
    case RGBA32F:
        return GL_RGBA;
    default:
        std::cout << "Error: Color format '" << fmt << "' is invalid" << std::endl;
        return 0;
    }
}

uint32_t GetGLDataType(ColorFormat fmt)
{
    switch (fmt)
    {
    case RGB8:
    case RGBA8:
        return GL_BYTE;
    case RG16:
        return GL_FLOAT;
    case RGBA32F:
        return GL_FLOAT;
    default:
        std::cout << "Error: Color format '" << fmt << "' is invalid" << std::endl;
        return 0;
    }
}