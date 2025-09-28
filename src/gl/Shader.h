#pragma once
#include <string>
#include <stdexcept>
#include <glad/glad.h>

class Shader
{
public:
    Shader() = default;
    ~Shader() { if ( prog_ ) glDeleteProgram( prog_ ); }

    // throws on error
    void build( const char* vsSrc, const char* fsSrc )
    {
        GLuint vs = glCreateShader( GL_VERTEX_SHADER );
        GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
        glShaderSource( vs, 1, &vsSrc, nullptr );
        glShaderSource( fs, 1, &fsSrc, nullptr );
        glCompileShader( vs );  checkShader( vs, "vertex" );
        glCompileShader( fs );  checkShader( fs, "fragment" );

        prog_ = glCreateProgram();
        glAttachShader( prog_, vs );
        glAttachShader( prog_, fs );
        glLinkProgram( prog_ );
        glDeleteShader( vs );
        glDeleteShader( fs );
        checkProgram();
    }

    void use() const { glUseProgram( prog_ ); }
    GLuint id() const { return prog_; }

    // Uniform helpers (use DSA when available)
    void setVec4( const char* name, const float* v ) const
    {
        GLint loc = glGetUniformLocation( prog_, name );
        if ( loc != -1 ) glUniform4fv( loc, 1, v );
    }

    void setFloat( const char* name, const float v ) const
    {
        GLint loc = glGetUniformLocation( prog_, name );
        if ( loc != -1 ) glUniform4fv( loc, 1, &v );
    }

    void setMat4( const char* name, const float* m ) const
    {
        GLint loc = glGetUniformLocation( prog_, name );
        if ( loc != -1 ) glUniformMatrix4fv( loc, 1, GL_FALSE, m );
    }

private:
    GLuint prog_ = 0;

    void checkShader( GLuint s, const char* what )
    {
        GLint ok = 0; glGetShaderiv( s, GL_COMPILE_STATUS, &ok );
        if ( !ok )
        {
            GLint len = 0; glGetShaderiv( s, GL_INFO_LOG_LENGTH, &len );
            std::string log( len, '\0' ); glGetShaderInfoLog( s, len, nullptr, log.data() );
            throw std::runtime_error( std::string( "Compile " ) + what + " shader failed:\n" + log );
        }
    }
    void checkProgram()
    {
        GLint ok = 0; glGetProgramiv( prog_, GL_LINK_STATUS, &ok );
        if ( !ok )
        {
            GLint len = 0; glGetProgramiv( prog_, GL_INFO_LOG_LENGTH, &len );
            std::string log( len, '\0' ); glGetProgramInfoLog( prog_, len, nullptr, log.data() );
            throw std::runtime_error( std::string( "Link program failed:\n" ) + log );
        }
    }
};
