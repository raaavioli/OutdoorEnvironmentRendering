#include "framebuffer.h"

#include <iostream>
#include <gl_helpers.h>

FrameBuffer::FrameBuffer(FrameBufferCreateInfo create_info, bool clamp_to_border) : m_Width(create_info.width), m_Height(create_info.height) {
    glGenFramebuffers(1, &this->m_RendererId);
    glBindFramebuffer(GL_FRAMEBUFFER, this->m_RendererId);

    m_NumColorAttachments = create_info.num_color_attachments;

    if (create_info.attachment_bits & AttachmentType::COLOR)
    {
        for (int i = 0; i < m_NumColorAttachments; i++)
        {
            glGenTextures(1, &this->m_ColorAttachments[i]);
            glBindTexture(GL_TEXTURE_2D, this->m_ColorAttachments[i]);

            FrameBufferTextureCreateInfo texture_info = create_info.color_attachment_infos[i];
            ColorFormat color_format = texture_info.color_format;
            glTexImage2D(GL_TEXTURE_2D, 0, GetGLInternalFormat(color_format), m_Width, m_Height, 0, GetGLDataFormat(color_format), GetGLDataType(color_format), NULL);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_info.min_filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_info.mag_filter);
            if (clamp_to_border)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, this->m_ColorAttachments[i], 0);
        }
    }

    if (create_info.attachment_bits & AttachmentType::DEPTH)
    {
        glGenTextures(1, &this->m_DepthAttachment);
        glBindTexture(GL_TEXTURE_2D, this->m_DepthAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->m_DepthAttachment, 0);
    }

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR (FrameBuffer): Framebuffer was not successfully created (0x" 
            << std::hex << status << ")" << std::endl;
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
    case R32UI:
        return GL_R32UI;
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
    case R32UI:
        return GL_RED_INTEGER;
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
    case R32UI:
        return GL_UNSIGNED_INT;
    default:
        std::cout << "Error: Color format '" << fmt << "' is invalid" << std::endl;
        return 0;
    }
}