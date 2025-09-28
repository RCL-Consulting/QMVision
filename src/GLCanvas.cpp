#include "GLCanvas.h"
#ifdef _WIN32
#include <Windows.h>
#endif

#define STB_EASY_FONT_IMPLEMENTATION
#include "third_party/stb/stb_easy_font.h"

#include <GeomBasics.h>

#include <wx/wx.h>

#include <stdexcept>
#include <filesystem>

static const char* kVS = R"(#version 460 core
layout(location=0) in vec3 aPos;
uniform mat4 uView;
uniform mat4 uProj;
void main(){ gl_Position = uProj * uView * vec4(aPos,1.0); }
)";

static const char* kFS = R"(#version 460 core
out vec4 FragColor;
uniform vec4 uColor;
void main(){ FragColor = uColor; }
)";

static const char* kPickVS = R"(#version 460 core
layout(location=0) in vec3 aPos;
uniform mat4 uView, uProj;
void main(){ gl_Position = uProj * uView * vec4(aPos,1); }
)";

static const char* kPickFS = R"(#version 460 core
uniform uint uId;           // 0..16M
layout(location=0) out vec4 outC;
void main(){
  // pack uId into RGB
  uint r = (uId >> 16u) & 255u;
  uint g = (uId >> 8u)  & 255u;
  uint b = (uId)        & 255u;
  outC = vec4(r,g,b,255)/255.0;
}
)";

// text.vs
static const char* kTextVS = R"(#version 460 core
layout( location = 0 ) in vec2 aPos;
uniform mat4 uProj;
void main() { gl_Position = uProj * vec4( aPos, 0.0, 1.0 ); })";

// text.fs
static const char* kTextFS = R"(#version 460 core
uniform vec4 uColor;
out vec4 FragColor;
void main() { FragColor = uColor; })";


static wxGLAttributes MakeCanvasAttrs()
{
	wxGLAttributes a;
	a.PlatformDefaults()
		.RGBA()
		.DoubleBuffer()
		.Depth( 24 )
		.Stencil( 8 )
		// .SampleBuffers(1).Samples(4)   // enable later if you want MSAA; can cause creation to fail on some drivers
		.EndList();
	return a;
}

wxBEGIN_EVENT_TABLE( GLCanvas, wxGLCanvas )
EVT_PAINT( GLCanvas::OnPaint )
EVT_SIZE( GLCanvas::OnResize )
EVT_MOTION( GLCanvas::onMouse )
EVT_LEFT_DOWN( GLCanvas::onMouse )
EVT_MIDDLE_DOWN( GLCanvas::onMouse )
EVT_MIDDLE_UP( GLCanvas::onMouse )
EVT_LEFT_UP( GLCanvas::onMouse )
EVT_MOUSEWHEEL( GLCanvas::onMouse )
wxEND_EVENT_TABLE()

GLCanvas::GLCanvas( wxWindow* parent )
	: wxGLCanvas( parent, MakeCanvasAttrs(), wxID_ANY )
{
	wxGLContextAttrs want;
	want.CoreProfile()
		.ForwardCompatible()
		.OGLVersion( 4, 6 )
		.EndList();

	ctx_ = std::make_unique<wxGLContext>( this, nullptr, &want );
	if ( !ctx_ || !ctx_->IsOK() )
	{
		// If driver/hardware can’t do 4.6 core, fall back so you still get *a* context
		wxLogWarning( "Requested GL 4.6 core failed; falling back to default context." );
		ctx_ = std::make_unique<wxGLContext>( this );
	}
	wxASSERT( ctx_ && ctx_->IsOK() );

	SetBackgroundStyle( wxBG_STYLE_PAINT );
}

void GLCanvas::OnInitGL()
{
	if ( initialized_ )
		return;

	SetCurrent( *ctx_ );
	if ( !gladLoadGL() )
		throw std::runtime_error( "Failed to load OpenGL" );

	glEnable( GL_DEPTH_TEST );
	glClearColor( 0.1f, 0.1f, 0.12f, 1.f );

	createPipeline();
	pickShader_.build( kPickVS, kPickFS );
	textShader_.build( kTextVS, kTextFS );

	initialized_ = true;
}

void 
GLCanvas::OnPaint( wxPaintEvent& )
{
	wxPaintDC dc( this );
	SetCurrent( *ctx_ );
	if ( !initialized_ ) OnInitGL();

	int w, h; GetClientSize( &w, &h );
	glViewport( 0, 0, w, h );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	/*const float aspect = (h > 0) ? float(w) / float(h) : 1.f;
	auto proj = perspective( 45.f * 3.1415926f / 180.f, aspect, 0.01f, 100.f );
	const float cx = std::cos( pitch_ ) * std::cos( yaw_ ) * dist_;
	const float cy = std::sin( pitch_ ) * dist_;
	const float cz = std::cos( pitch_ ) * std::sin( yaw_ ) * dist_;
	auto view = lookAt( cx, cy, cz, 0, 0, 0, 0, 1, 0 );*/

	// Ortho window computed from center + zoom
	const float halfW = 0.5f * w / camZoom_;
	const float halfH = 0.5f * h / camZoom_;
	const float left = camCenterX_ - halfW;
	const float right = camCenterX_ + halfW;
	const float bottom = camCenterY_ - halfH;
	const float top = camCenterY_ + halfH;

	// Z range can be tight around your mesh plane
	Mat4 proj = ortho( left, right, bottom, top, -1.f, 1.f );
	// View is just identity (or a translate if you want Z offset)
	Mat4 view = translate( 0.f, 0.f, 0.f );

	renderScene( view, proj );

	// For each node, project and draw number
	for ( const auto& n : GeomBasics::nodeList )
	{
		float sx, sy;
		if ( projectToScreen( { (float)n->x, (float)n->y, 0.f }, view, proj, w, h, sx, sy ) )
		{
			auto label = std::to_string( n->GetNumber() );
			drawText2D( sx + 3.f, sy - 3.f, label.c_str() ); // small offset so it doesn’t sit exactly on the point
		}
	}

	if ( wantPick_ )
	{
		// map window coords to FBO coords (same size here)
		int px = pickPos_.x, py = h - 1 - pickPos_.y; // flip Y
		renderPick( view, proj, w, h );
		uint32_t id = picker_.read( px, py );
		pickedTri_ = (id == 0) ? -1 : int( id - 1 );
		wantPick_ = false;

		if ( pickedTri_ >= 0 )
		{
			wxLogMessage( "Picked triangle: %d", pickedTri_ );
		}
		else
		{
			wxLogMessage( "No hit" );
		}
	}

	SwapBuffers();
}


void GLCanvas::OnResize( wxSizeEvent& e )
{
	if ( IsShownOnScreen() )
	{
		SetCurrent( *ctx_ );
		// handle viewport in OnPaint
		Refresh( false );
	}
	e.Skip();
}

void 
GLCanvas::RegenerateMeshDisplay()
{
	uint32_t maxId = 0;
	for ( const auto& n : GeomBasics::nodeList ) maxId = std::max<uint32_t>( maxId, n->GetNumber() );
	std::vector<float> vertices( maxId * 3 );
	for ( const auto& n : GeomBasics::nodeList )
	{
		const uint32_t id = n->GetNumber() - 1;
		vertices[id * 3 + 0] = float( n->x );
		vertices[id * 3 + 1] = float( n->y );
		vertices[id * 3 + 2] = 0.0f;
	}

	auto add_triangle = [&]( const Edge& e1, const Edge& e2,
							 std::vector<uint32_t>& indices,
							 uint32_t start )
		{
			indices[start] = uint32_t( e1.leftNode->GetNumber() - 1 );
			indices[start+1] = uint32_t( e1.rightNode->GetNumber() - 1 );
			if ( !e2.leftNode->equals( e1.leftNode ) && !e2.leftNode->equals( e1.rightNode ) )
				indices[start+2] = uint32_t( e2.leftNode->GetNumber() - 1 );
			else
				indices[start+2] = uint32_t( e2.rightNode->GetNumber() - 1 );

		};

	std::vector<uint32_t> indices( GeomBasics::triangleList.size() * 3 + 6 * GeomBasics::elementList.size(), 0 );
	for ( size_t i = 0; const auto& t : GeomBasics::triangleList )
	{
		const auto e1 = t->edgeList[0];
		const auto e2 = t->edgeList[1];
		add_triangle( *e1, *e2, indices, uint32_t( i * 3 ) );
		++i;
	}

	for ( size_t i = 0; const auto& e : GeomBasics::elementList )
	{
		const auto e1 = e->edgeList[0];
		const auto e2 = e->edgeList[1];
		add_triangle( *e1, *e2, indices, uint32_t( i * 3 ) );
		++i;
	
		if ( e->edgeList.size() == 4 )
		{
			const auto e3 = e->edgeList[2];
			const auto e4 = e->edgeList[3];
			add_triangle( *e3, *e4, indices, uint32_t( i * 3 ) );
			++i;
		}
	}

	mesh_.upload( vertices, indices );

	// After mesh_.upload(vertices, indices);
	pslg_.create( mesh_.Vbo() );  // add a tiny getter in your Mesh to expose the position VBO id

	// Build segments from your QMorph data (example: boundary/constrained edges)
	std::vector<Segment> segs;
	segs.reserve( GeomBasics::edgeList.size() );
	for ( const auto& e : GeomBasics::edgeList )
	{
		// choose your criterion: boundary edges, constrained edges, etc.
		// Example (pseudo): if (e->isConstrained) ...
		// Fallback: push all edges to visualize connectivity:
		segs.push_back( { uint32_t( e->leftNode->GetNumber() - 1 ),
						 uint32_t( e->rightNode->GetNumber() - 1 ) } );
	}
	pslg_.uploadSegments( segs );

	float minx = FLT_MAX, miny = FLT_MAX, maxx = -FLT_MAX, maxy = -FLT_MAX;
	for ( const auto& n : GeomBasics::nodeList )
	{
		minx = std::min( minx, float( n->x ) );
		miny = std::min( miny, float( n->y ) );
		maxx = std::max( maxx, float( n->x ) );
		maxy = std::max( maxy, float( n->y ) );
	}
	fitToMesh( minx, miny, maxx, maxy );

	Refresh( false );
}

void
GLCanvas::LoadMesh( const std::string& path )
{
	std::filesystem::path inputPath( path );

	GeomBasics::clearLists();

	GeomBasics::setParams( inputPath.filename().string(), inputPath.parent_path().string(), false, false );
	GeomBasics::loadMesh();
	GeomBasics::findExtremeNodes();

	RegenerateMeshDisplay();
}

void GLCanvas::destroyPipeline()
{
	if ( ibo_ ) glDeleteBuffers( 1, &ibo_ ), ibo_ = 0;
	if ( vbo_ ) glDeleteBuffers( 1, &vbo_ ), vbo_ = 0;
	if ( vao_ ) glDeleteVertexArrays( 1, &vao_ ), vao_ = 0;
}

void GLCanvas::createPipeline()
{
	// cube vertices & indices
	const float v[] = {
	  -1,-1,-1,  1,-1,-1,  1, 1,-1, -1, 1,-1,
	  -1,-1, 1,  1,-1, 1,  1, 1, 1, -1, 1, 1
	};
	const uint16_t i[] = {
	  0,1,2, 2,3,0,  1,5,6, 6,2,1,
	  5,4,7, 7,6,5,  4,0,3, 3,7,4,
	  3,2,6, 6,7,3,  4,5,1, 1,0,4
	};

	glCreateVertexArrays( 1, &vao_ );
	glCreateBuffers( 1, &vbo_ );
	glNamedBufferStorage( vbo_, sizeof( v ), v, 0 );
	glVertexArrayVertexBuffer( vao_, 0, vbo_, 0, sizeof( float ) * 3 );

	glEnableVertexArrayAttrib( vao_, 0 );
	glVertexArrayAttribFormat( vao_, 0, 3, GL_FLOAT, GL_FALSE, 0 );
	glVertexArrayAttribBinding( vao_, 0, 0 );

	glCreateBuffers( 1, &ibo_ );
	glNamedBufferStorage( ibo_, sizeof( i ), i, 0 );
	glVertexArrayElementBuffer( vao_, ibo_ );

	shader_.build( kVS, kFS );
}

void 
GLCanvas::onMouse( wxMouseEvent& e )
{
	if ( e.MiddleDown() ) 
	{
		panning_ = true; 
		panLast_ = e.GetPosition(); 
		CaptureMouse(); 
	}
	if ( e.MiddleUp() ) { panning_ = false; panLast_ = { -1,-1 }; if ( HasCapture() ) ReleaseMouse(); }

	if ( !m_dragging && (e.LeftDClick() || (e.LeftUp() && e.GetEventType() == wxEVT_LEFT_UP) ))
	{
		// request a pick; store window coords (origin top-left)
		pickPos_ = e.GetPosition();
		wantPick_ = true;
		Refresh( false );
	}	
	else if ( e.LeftDown() ) 
		lastDrag_ = e.GetPosition();
	else if ( e.Dragging() && e.LeftIsDown() && lastDrag_.x >= 0 )
	{
		auto p = e.GetPosition();
		int dx = p.x - lastDrag_.x, dy = p.y - lastDrag_.y;
		lastDrag_ = p;
		yaw_ += dx * 0.005f;
		pitch_ += dy * 0.005f;
		pitch_ = std::clamp( pitch_, -1.3f, 1.3f );
		m_dragging = true;
		Refresh( false );
	}
	else if ( e.LeftUp() )
	{
		lastDrag_ = { -1,-1 };
		m_dragging = false;
	}
	else if ( panning_ && e.Dragging() && e.MiddleIsDown() && panLast_.x >= 0 )
	{
		wxPoint p = e.GetPosition();
		int w, h; GetClientSize( &w, &h );
		const float worldPerPxX = 1.f / camZoom_;
		const float worldPerPxY = 1.f / camZoom_;

		// screen +X is right → move center left to keep scene under cursor = subtract dx
		const int dx = p.x - panLast_.x;
		const int dy = p.y - panLast_.y; // +down in screen space
		camCenterX_ -= dx * worldPerPxX;
		camCenterY_ += dy * worldPerPxY; // screen down means world up is negative Y; here we flip so drag feels natural

		panLast_ = p;
		Refresh( false );
	}

	if ( e.GetWheelRotation() != 0 )
	{
		wxPoint mouse = e.GetPosition();
		// world point under cursor BEFORE zoom
		auto before = screenToWorld( mouse.x, mouse.y );

		float steps = float( e.GetWheelRotation() ) / float( e.GetWheelDelta() );
		float factor = std::pow( 1.1f, steps );     // 10% per wheel detent
		camZoom_ = std::clamp( camZoom_ * factor, 0.0001f, 10000.f );

		// world point under cursor AFTER zoom (with same center)
		auto after = screenToWorld( mouse.x, mouse.y );

		// shift center so 'before' stays under cursor
		camCenterX_ += (before.x - after.x);
		camCenterY_ += (before.y - after.y);

		Refresh( false );
	}
}

void GLCanvas::renderScene( const Mat4& view, const Mat4& proj )
{
	glDisable( GL_CULL_FACE );          // TEMP while debugging
	glDisable( GL_DEPTH_TEST );         // TEMP if all z==0

	shader_.use();
	shader_.setMat4( "uProj", proj.data() );
	shader_.setMat4( "uView", view.data() );
	
	// --- Triangles ---
	{
		const float c[4] = { triColor_.r, triColor_.g, triColor_.b, triColor_.a };
		shader_.setVec4( "uColor", c );
		if ( mesh_.valid() )
		{
			mesh_.draw();
		}
		else
		{
			glBindVertexArray( vao_ );
			glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0 );
		}
	}

	// --- Overlay (segments/arcs) ---
	glDisable( GL_DEPTH_TEST ); // draw on top; remove if you want depth-tested edges
	{
		const float c[4] = { edgeColor_.r, edgeColor_.g, edgeColor_.b, edgeColor_.a };
		shader_.setVec4( "uColor", c );

		if ( showSegments_ && pslg_.hasSegments() )
		{
			pslg_.drawLines();
		}
		if ( showArcs_ && pslg_.hasArcs() )
		{
			pslg_.drawArcs();
		}
	}
	glEnable( GL_DEPTH_TEST );

}

void 
GLCanvas::fitToMesh( float bboxMinX, float bboxMinY, float bboxMaxX, float bboxMaxY )
{
	int w, h; GetClientSize( &w, &h );
	camCenterX_ = 0.5f * (bboxMinX + bboxMaxX);
	camCenterY_ = 0.5f * (bboxMinY + bboxMaxY);

	const float worldW = std::max( 1e-6f, bboxMaxX - bboxMinX );
	const float worldH = std::max( 1e-6f, bboxMaxY - bboxMinY );

	// margin & zoom to fit
	const float margin = 1.1f;
	const float fitZoomX = (w > 0) ? (w / (worldW * margin)) : camZoom_;
	const float fitZoomY = (h > 0) ? (h / (worldH * margin)) : camZoom_;
	camZoom_ = std::min( fitZoomX, fitZoomY );
}

// Convert from screen pixels -> world (XY), using current ortho params
Vec3 GLCanvas::screenToWorld( int px, int py ) const
{
	int w, h; const_cast<GLCanvas*>(this)->GetClientSize( &w, &h );
	const float halfW = 0.5f * w / camZoom_;
	const float halfH = 0.5f * h / camZoom_;

	// NDC in [-1,1]; screen origin is top-left in wx
	const float ndcX = (2.f * px) / float( w ) - 1.f;
	const float ndcY = -(2.f * py) / float( h ) + 1.f;

	const float left = camCenterX_ - halfW;
	const float bottom = camCenterY_ - halfH;

	// linear map NDC -> world
	const float wx = left + (ndcX + 1.f) * halfW;
	const float wy = bottom + (ndcY + 1.f) * halfH;
	return { wx, wy, 0.f };
}

void GLCanvas::drawText2D( float x, float y, const char* text )
{
	// 1) Build quads from stb_easy_font (each vertex = 16 bytes)
	static unsigned char scratch[200000]; // enough for a few thousand chars
	if ( !text ) return;

	char buffer[1024];
	std::strncpy( buffer, text, sizeof( buffer ) );
	buffer[sizeof( buffer ) - 1] = '\0';

	int num_quads = stb_easy_font_print( x, y, buffer, nullptr,
										 scratch, (int)sizeof( scratch ) );
	if ( num_quads <= 0 ) return;

	// 2) Expand quads -> triangles (x,y only)
	std::vector<float> verts;
	verts.reserve( size_t( num_quads ) * 6 /*tri verts*/ * 2 /*xy*/ );

	auto vert_xy = [&]( int q, int i ) -> std::pair<float, float>
		{
			// q-th quad, i-th vertex (0..3)
			unsigned char* base = scratch + q * 4 * 16 + i * 16;
			float* f = reinterpret_cast<float*>(base);
			return { f[0], f[1] }; // x,y (f[2] is z; last 4 bytes are color)
		};

	for ( int q = 0; q < num_quads; ++q )
	{
		auto [x0, y0] = vert_xy( q, 0 );
		auto [x1, y1] = vert_xy( q, 1 );
		auto [x2, y2] = vert_xy( q, 2 );
		auto [x3, y3] = vert_xy( q, 3 );

		// tri 0: 0,1,2
		verts.push_back( x0 ); verts.push_back( y0 );
		verts.push_back( x1 ); verts.push_back( y1 );
		verts.push_back( x2 ); verts.push_back( y2 );
		// tri 1: 0,2,3
		verts.push_back( x0 ); verts.push_back( y0 );
		verts.push_back( x2 ); verts.push_back( y2 );
		verts.push_back( x3 ); verts.push_back( y3 );
	}

	// 3) Upload & draw
	GLuint vao = 0, vbo = 0;
	glCreateVertexArrays( 1, &vao );
	glCreateBuffers( 1, &vbo );
	glNamedBufferData( vbo, verts.size() * sizeof( float ), verts.data(), GL_STREAM_DRAW );
	glVertexArrayVertexBuffer( vao, 0, vbo, 0, sizeof( float ) * 2 );
	glEnableVertexArrayAttrib( vao, 0 );
	glVertexArrayAttribFormat( vao, 0, 2, GL_FLOAT, GL_FALSE, 0 );
	glVertexArrayAttribBinding( vao, 0, 0 );

	// overlay state
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_DEPTH_TEST );

	textShader_.use();
	int w, h; GetClientSize( &w, &h );
	Mat4 proj = orthoPixels( float( w ), float( h ) );  // your pixel-ortho matrix
	textShader_.setMat4( "uProj", proj.data() );
	const float white[4] = { 1,1,1,1 };
	textShader_.setVec4( "uColor", white );

	glBindVertexArray( vao );
	glDrawArrays( GL_TRIANGLES, 0, (GLsizei)(verts.size() / 2) );

	// restore state if needed
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );

	glDeleteBuffers( 1, &vbo );
	glDeleteVertexArrays( 1, &vao );
}


void 
GLCanvas::renderPick( const Mat4& view, const Mat4& proj, int fbw, int fbh )
{
	if ( !mesh_.valid() ) return;
	picker_.create( fbw, fbh );
	picker_.begin();

	pickShader_.use();
	pickShader_.setMat4( "uProj", proj.data() );
	pickShader_.setMat4( "uView", view.data() );

	// draw each triangle with unique color id
	glBindVertexArray( 0 ); // we’ll reuse mesh VAO but issue 3-index draws
	glBindVertexArray( mesh_.Vao() ); // add a small getter returning mesh_.vao_ if you like

	// We don’t have a public getter; simplest: add mesh_.bind() that binds the VAO.
	// For brevity here, imagine mesh_.drawTri(i, firstIndex) – implement if desired.
	// Minimal workable version below using glDrawElements with offset:

	// WARNING: requires GL 4.5+ DSA buffer addressing; here we use index offset.
	// indices are 32-bit, so byte offset = tri*3*sizeof(uint32_t)
	GLuint prog = pickShader_.id();
	GLint loc = glGetUniformLocation( prog, "uId" );

	// (cap number drawn if extremely large during early testing)
	GLsizei tris = mesh_.IndexCount() / 3; // add a helper that returns count_/3
	for ( GLsizei t = 0; t < tris; ++t )
	{
		glUniform1ui( loc, (GLuint)t + 1 ); // 0 = no hit
		const void* offset = (const void*)(size_t( t ) * 3 * sizeof( uint32_t ));
		glDrawElements( GL_TRIANGLES, 3, GL_UNSIGNED_INT, offset );
	}

	picker_.end();
}
