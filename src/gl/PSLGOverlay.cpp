// PSLGOverlay.cpp
#include "PSLGOverlay.h"
#include <cmath>

void PSLGOverlay::destroy()
{
    if ( eboSeg_ ) glDeleteBuffers( 1, &eboSeg_ ), eboSeg_ = 0;
    if ( eboArc_ ) glDeleteBuffers( 1, &eboArc_ ), eboArc_ = 0;
    if ( vao_ )    glDeleteVertexArrays( 1, &vao_ ), vao_ = 0;
    segCount_ = 0; arcIndexCount_ = 0;
    arcStripStarts_.clear(); arcStripCounts_.clear();
}

void PSLGOverlay::create( GLuint sharedVbo )
{
    destroy();

    glCreateVertexArrays( 1, &vao_ );
    // bind the shared position VBO at binding=0, stride=3 floats
    glVertexArrayVertexBuffer( vao_, 0, sharedVbo, 0, sizeof( float ) * 3 );
    glEnableVertexArrayAttrib( vao_, 0 );
    glVertexArrayAttribFormat( vao_, 0, 3, GL_FLOAT, GL_FALSE, 0 );
    glVertexArrayAttribBinding( vao_, 0, 0 );

    glCreateBuffers( 1, &eboSeg_ );
    glCreateBuffers( 1, &eboArc_ );
}

void PSLGOverlay::uploadSegments( const std::vector<Segment>& segs )
{
    if ( !eboSeg_ ) return;
    segCount_ = static_cast<GLsizei>(segs.size());
    // indices are pairs of uint32_t
    glNamedBufferData( eboSeg_, segs.size() * 2 * sizeof( uint32_t ), segs.data(), GL_STATIC_DRAW );
}

void PSLGOverlay::uploadArcs( const std::vector<Arc>& arcs, int tessPerQuad )
{
    if ( !eboArc_ ) return;

    std::vector<uint32_t> idx;
    idx.reserve( arcs.size() * (tessPerQuad * 4 + 1) );

    arcStripStarts_.clear();
    arcStripCounts_.clear();

    for ( const auto& a : arcs )
    {
        // compute angle of endpoints around center
        // NOTE: vertex positions are in the shared VBO; here we only generate indices.
        // We tessellate as *index* sequence along the arc; simplest: generate new polyline indices
        // that reference hypothetical tess points. Since we do NOT create tessellated vertices here,
        // we’ll approximate by interpolating between endpoints using the existing endpoints only.
        // For a proper arc, you either:
        //  (A) generate tessellated intermediate *vertices* and put them in a separate VBO, or
        //  (B) keep endpoints only and draw with a shader/GS (more involved).
        //
        // Quick path for milestone: approximate as straight segment for now (visual overlay),
        // but keep structure ready for real tessellation later.
        arcStripStarts_.push_back( static_cast<GLuint>(idx.size()) );
        idx.push_back( a.a );
        idx.push_back( a.b );
        arcStripCounts_.push_back( 2 );
    }

    arcIndexCount_ = static_cast<GLsizei>(idx.size());
    glNamedBufferData( eboArc_, idx.size() * sizeof( uint32_t ), idx.data(), GL_STATIC_DRAW );
}

void PSLGOverlay::drawLines()
{
    if ( !vao_ || !eboSeg_ || segCount_ == 0 ) return;
    glBindVertexArray( vao_ );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, eboSeg_ );
    glDrawElements( GL_LINES, segCount_ * 2, GL_UNSIGNED_INT, (void*)0 );
}

void PSLGOverlay::drawArcs()
{
    if ( !vao_ || !eboArc_ || arcIndexCount_ == 0 ) return;
    glBindVertexArray( vao_ );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, eboArc_ );
    // We stored each arc as a small strip count; draw each as a line strip
    size_t offset = 0;
    for ( size_t i = 0; i < arcStripCounts_.size(); ++i )
    {
        glDrawElements( GL_LINE_STRIP, arcStripCounts_[i], GL_UNSIGNED_INT,
                        (void*)(offset * sizeof( uint32_t )) );
        offset += arcStripCounts_[i];
    }
}
