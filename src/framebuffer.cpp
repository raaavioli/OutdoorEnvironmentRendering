#include "framebuffer.h"

#include <iostream>
#include <gl_helpers.h>

FrameBuffer::FrameBuffer(uint32_t width, uint32_t height) {
    glGenFramebuffers(1, &this->renderer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, this->renderer_id);

    glGenTextures(1, &this->color_attachment);
    glBindTexture(GL_TEXTURE_2D, this->color_attachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &this->depth_stencil_attachment);
    glBindTexture(GL_TEXTURE_2D, this->depth_stencil_attachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->color_attachment, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, this->depth_stencil_attachment, 0);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR (FrameBuffer): Framebuffer was not successfully created (" 
            << status << ")" << std::endl;
    }
    unbind();
}