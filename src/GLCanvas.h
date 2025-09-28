#pragma once
#include <glad/glad.h>
#include <wx/glcanvas.h>
#include <string>

#include "gl/Shader.h"
#include "gl/Math.h"
#include "gl/GpuMesh.h"
#include "gl/Picker.h"
#include "gl/PSLGOverlay.h"

class GLCanvas : public wxGLCanvas
{
public:
	GLCanvas( wxWindow* parent );
	void LoadMesh( const std::string& path );
	void RegenerateMeshDisplay();

	void SetShowSegments( bool b ) { showSegments_ = b; Refresh( false ); }
	void SetShowArcs( bool b ) { showArcs_ = b; Refresh( false ); }
	void SetTriangleColor( float r, float g, float b, float a = 1.0f )
	{
		triColor_ = { r,g,b,a }; Refresh( false );
	}
	void SetEdgeColor( float r, float g, float b, float a = 1.0f )
	{
		edgeColor_ = { r,g,b,a }; Refresh( false );
	}

	struct Color { float r{ 0.90f }, g{ 0.85f }, b{ 0.10f }, a{ 1.0f }; };
	Color GetTriangleColor() const { return triColor_; }
	Color GetEdgeColor() const { return edgeColor_; }

private:
	void OnPaint( wxPaintEvent& );
	void OnResize( wxSizeEvent& );
	void OnInitGL();
	bool initialized_ = false;
	std::unique_ptr<wxGLContext> ctx_;
	
	// GPU objects
	GLuint vao_ = 0, vbo_ = 0, ibo_ = 0;
	Shader shader_;
	GpuMesh mesh_;
	Picker  picker_;
	Shader  pickShader_;
	Shader  textShader_;

	// camera
	float yaw_ = 0.6f, pitch_ = 0.3f, dist_ = 3.0f;
	wxPoint lastDrag_{ -1,-1 };
	// Ortho camera state
	float camZoom_ = 1.0f;   // world units per pixel inverse (bigger = zoom in)
	float camCenterX_ = 0.0f;   // world-space view center
	float camCenterY_ = 0.0f;

	bool  panning_ = false;
	wxPoint panLast_ = { -1,-1 };

	bool m_dragging = false;
	
	bool wantPick_ = false;
	wxPoint pickPos_{ 0,0 };
	int pickedTri_ = -1;
	PSLGOverlay pslg_;
	bool showSegments_ = true;
	bool showArcs_ = true;

	// GLCanvas.h (add near other members)
	
	Color triColor_{ 0.45f, 0.8f, 0.85f, 1.0f };
	Color edgeColor_{ 0.15f, 0.45f, 0.5f, 1.0f };

	void renderScene( const Mat4& view, const Mat4& proj );
	void renderPick( const Mat4& view, const Mat4& proj, int fbw, int fbh );
	Vec3 screenToWorld( int px, int py ) const;
	void fitToMesh( float bboxMinX, float bboxMinY, float bboxMaxX, float bboxMaxY );
	void drawText2D( float x, float y, const char* text );

	void createPipeline();
	void destroyPipeline();
	void onMouse( wxMouseEvent& e );

	wxDECLARE_EVENT_TABLE();
};