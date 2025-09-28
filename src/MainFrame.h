#pragma once
#include <wx/frame.h>
#include "GLCanvas.h"

class MainFrame : public wxFrame
{
public:
	MainFrame();
private:
	enum
	{
		ID_Open = wxID_HIGHEST + 1,
		ID_SetTriColor,
		ID_SetEdgeColor,
		ID_ToggleEdges,
		ID_QMorph
	};

	void OnOpen( wxCommandEvent& );
	void OnQuit( wxCommandEvent& );
	void OnSetTriColor( wxCommandEvent& );
	void OnSetEdgeColor( wxCommandEvent& );
	void OnToggleEdges( wxCommandEvent& );
	void OnQMorph( wxCommandEvent& );

	inline wxColour ToWx( GLCanvas::Color c )
	{
		return wxColour(
			(unsigned char)std::lround( c.r * 255.0f ),
			(unsigned char)std::lround( c.g * 255.0f ),
			(unsigned char)std::lround( c.b * 255.0f ),
			(unsigned char)std::lround( c.a * 255.0f ) );
	}
	inline void FromWx( const wxColour& w, float c[4] )
	{
		c[0] = w.Red() / 255.0f;
		c[1] = w.Green() / 255.0f;
		c[2] = w.Blue() / 255.0f;
		c[3] = w.Alpha() / 255.0f;
	}

	GLCanvas* canvas_{};
	wxDECLARE_EVENT_TABLE();
};