#pragma once
#include <glad/glad.h>
#include <cstdint>

class Picker
{
public:
    ~Picker() { destroy(); }
    void create( int w, int h )
    {
        if ( w == w_ && h == h_ && fbo_ ) return;
        destroy();
        w_ = w; h_ = h;
        glCreateTextures( GL_TEXTURE_2D, 1, &tex_ );
        glTextureStorage2D( tex_, 1, GL_RGBA8, w_, h_ );
        glCreateRenderbuffers( 1, &rbo_ );
        glNamedRenderbufferStorage( rbo_, GL_DEPTH24_STENCIL8, w_, h_ );
        glCreateFramebuffers( 1, &fbo_ );
        glNamedFramebufferTexture( fbo_, GL_COLOR_ATTACHMENT0, tex_, 0 );
        glNamedFramebufferRenderbuffer( fbo_, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_ );
    }
    void destroy()
    {
        if ( fbo_ ) glDeleteFramebuffers( 1, &fbo_ );
        if ( rbo_ ) glDeleteRenderbuffers( 1, &rbo_ );
        if ( tex_ ) glDeleteTextures( 1, &tex_ );
        fbo_ = rbo_ = tex_ = 0; w_ = h_ = 0;
    }
    void begin()
    {
        glBindFramebuffer( GL_FRAMEBUFFER, fbo_ );
        glViewport( 0, 0, w_, h_ );
        glClearColor( 0, 0, 0, 0 );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }
    void end() { glBindFramebuffer( GL_FRAMEBUFFER, 0 ); }
    uint32_t read( int x, int y )
    { // window coords mapped to FBO size beforehand
        unsigned char rgba[4]{};
        glBindFramebuffer( GL_FRAMEBUFFER, fbo_ );
        glReadPixels( x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba );
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
        return (uint32_t( rgba[0] ) << 16) | (uint32_t( rgba[1] ) << 8) | uint32_t( rgba[2] );
    }
    int w() const { return w_; } int h() const { return h_; }
private:
    GLuint fbo_ = 0, tex_ = 0, rbo_ = 0; int w_ = 0, h_ = 0;
};
