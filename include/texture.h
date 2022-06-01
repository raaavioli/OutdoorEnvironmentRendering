#ifndef WR_TEXTURE_H
#define WR_TEXTURE_H

#include <iostream>
#include <vector>

// TODO: Remove, Textures should not do asset loading themselves, 
// should be handled by AssetManager.
#include <filesystem>

#include <glad/glad.h>

/** TODO: Make more generic, do not assume byte-sized data for image */
struct RawImage {
    RawImage(uint32_t width, uint32_t height, GLenum gl_format);
    ~RawImage();

    void set_pixel(uint32_t x, uint32_t y, GLubyte r, GLubyte g, GLubyte b);
    inline uint32_t get_width() { return this->width; }
    inline uint32_t get_height() { return this->height; }
    inline GLenum get_gl_format() { return this->gl_format; }
    inline const char* get_data() { return this->data; }

private:
    char* data;
    uint32_t width, height;
    GLenum gl_format;
};

struct Texture2D {
    Texture2D();
    Texture2D(const std::filesystem::path& file_path);
    Texture2D(RawImage image);
    ~Texture2D();

    inline GLuint get_texture_id() { return m_Handle; };
    inline void bind(uint32_t slot) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_Handle);
    };

    inline void unbind() const { glBindTexture(GL_TEXTURE_2D, 0); };

private:
    GLuint m_Handle;
};

struct TextureCubeMap {
    TextureCubeMap(const std::filesystem::path& path, bool folder = false);

    inline GLuint get_texture_id() { return m_Handle; };
    inline void bind(uint32_t slot) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_Handle);
    };

    inline void unbind() const { glBindTexture(GL_TEXTURE_CUBE_MAP, 0); };

private:
    GLuint m_Handle;

    /**
     * Load cube map images from folder within assets/ directory. Folder should contain images named: 
     *      px, nx, py, ny, pz and nz.
     * Assuming JPG 
     * TODO: Look for file extension
     */
    void from_folder(const std::filesystem::path& folder_path);

    /**
     * Load cube map image from file within assets/ directory. Should contain 6 subimages. 
     * Image dimensions needs to be 4:3, so that all subimages are square.
     * Assuming JPG 
     * TODO: Look for file extension
     * TODO: Enable more than one format
     */
    void from_file(const std::filesystem::path& file_path);
};

#endif // WR_TEXTURE_H