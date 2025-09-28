// PSLGOverlay.h
#pragma once
#include <vector>
#include <cstdint>
#include <glad/glad.h>

struct Segment { uint32_t a, b; };

struct Arc
{
	uint32_t a, b;     // endpoints (indices into vertex array)
	float cx, cy;      // center
	float r;           // radius
	bool ccw;          // direction
};

class PSLGOverlay
{
public:
	void destroy();
	void create( GLuint sharedVbo );      // bind the same position VBO as the mesh
	void uploadSegments( const std::vector<Segment>& segs );
	void uploadArcs( const std::vector<Arc>& arcs, int tessPerQuad = 16 );

	void drawLines();                    // GL_LINES using segments
	void drawArcs();                     // tessellated arcs as GL_LINE_STRIP(s)

	bool hasSegments() const { return segCount_ > 0; }
	bool hasArcs() const { return arcIndexCount_ > 0; }

	GLuint vao() const { return vao_; }

private:
	GLuint vao_ = 0;
	GLuint eboSeg_ = 0;                 // segments (uint32 index pairs)
	GLuint eboArc_ = 0;                 // arcs (tessellated line-strip indices)
	GLsizei segCount_ = 0;              // number of lines (pairs)
	GLsizei arcIndexCount_ = 0;         // total indices for all strips
	std::vector<GLuint> arcStripStarts_; // offsets per arc (optional for debugging)
	std::vector<GLsizei> arcStripCounts_;
};
