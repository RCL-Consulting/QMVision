#pragma once
#include <glad/glad.h>
#include <cstdint>
#include <vector>

class GpuMesh
{
public:
    ~GpuMesh() { destroy(); }
    void destroy()
    {
        if ( ebo_ ) glDeleteBuffers( 1, &ebo_ );
        if ( vbo_ ) glDeleteBuffers( 1, &vbo_ );
        if ( vao_ ) glDeleteVertexArrays( 1, &vao_ );
        vao_ = vbo_ = ebo_ = 0; count_ = 0;
    }
    void upload( const std::vector<float>& pos, const std::vector<uint32_t>& idx )
    {
        destroy();
        glCreateVertexArrays( 1, &vao_ );
        glCreateBuffers( 1, &vbo_ );
        glCreateBuffers( 1, &ebo_ );

        GLbitfield flags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glNamedBufferStorage( vbo_, pos.size() * sizeof( float ), pos.data(), flags );
        glNamedBufferStorage( ebo_, idx.size() * sizeof( uint32_t ), idx.data(), flags );

        glVertexArrayVertexBuffer( vao_, 0, vbo_, 0, sizeof( float ) * 3 );
        glEnableVertexArrayAttrib( vao_, 0 );
        glVertexArrayAttribFormat( vao_, 0, 3, GL_FLOAT, GL_FALSE, 0 );
        glVertexArrayAttribBinding( vao_, 0, 0 );
        glVertexArrayElementBuffer( vao_, ebo_ );

        count_ = (GLsizei)idx.size();
    }

	GLuint Vao() { return vao_; }
	GLuint Vbo() { return vbo_; }
	GLsizei IndexCount() const { return count_; }

    void draw() const
    {
        glBindVertexArray( vao_ );
        glDrawElements( GL_TRIANGLES, count_, GL_UNSIGNED_INT, (void*)0 );
    }

    bool valid() const { return vao_ != 0; }

private:
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei count_ = 0;
};
