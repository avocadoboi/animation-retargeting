#ifndef ANIMATION_RETARGETING_TESTING_TEXTURE_HPP
#define ANIMATION_RETARGETING_TESTING_TEXTURE_HPP

#include <glad/glad.h>
#include <stb_image.h>

#include <stdexcept>

namespace testing {

namespace stb {

struct Image {
    stbi_uc* data;
    int width;
    int height;
    int channel_count;
    
    explicit Image(char const* const file_path) :
        data{stbi_load(file_path, &width, &height, &channel_count, 0)}
    {
        if (!data) {
            throw std::runtime_error{"Failed to load image."};
        }
    }
    ~Image() {
        stbi_image_free(data);
    }

    Image(Image const&) = delete;
    Image const& operator=(Image const&) = delete;
    Image(Image&&) = delete;
    Image const& operator=(Image&&) = delete;
};

} // namespace stb

class Texture {
private:
    GLuint id_{};
    
public:
    explicit Texture(char const* const file_path)
    {
        stbi_set_flip_vertically_on_load(true);
        
        stb::Image const image{file_path};

        auto const format = [&] {
            switch (image.channel_count) {
                case 1: return GL_RED;
                case 3: return GL_RGB;
                case 4: return GL_RGBA;
                default: throw std::runtime_error{"Invalid channel count from loaded image."};
            }
        }();

        glGenTextures(1, &id_);
        glBindTexture(GL_TEXTURE_2D, id_);
        glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    ~Texture() {
        if (id_) { // Zero is an invalid texture name.
            glDeleteTextures(1, &id_);
        }
    }

    Texture(Texture const&) = delete;
    Texture const& operator=(Texture const&) = delete;

    Texture(Texture&& other) : 
        id_{other.id_}
    {
        other.id_ = 0;
    } 
    Texture const& operator=(Texture&& other) {
        if (id_) {
            glDeleteTextures(1, &id_);
        }
        id_ = other.id_;
        other.id_ = 0;
        return *this;
    } 

    GLuint id() const {
        return id_;
    }
};

} // namespace testing

#endif