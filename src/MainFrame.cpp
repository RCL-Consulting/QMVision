#include "MainFrame.h"
#include "GLCanvas.h"
#include <wx/menu.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/colordlg.h>

#include "QMorph.h"

wxBEGIN_EVENT_TABLE( MainFrame, wxFrame )
    EVT_MENU( ID_Open, MainFrame::OnOpen )
    EVT_MENU( ID_SetTriColor, MainFrame::OnSetTriColor )
    EVT_MENU( ID_SetEdgeColor, MainFrame::OnSetEdgeColor )
    EVT_MENU( ID_ToggleEdges, MainFrame::OnToggleEdges )
    EVT_MENU( ID_QMorph, MainFrame::OnQMorph )
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
: wxFrame(nullptr, wxID_ANY, "QMVision (wxWidgets + OpenGL 4.6)", wxDefaultPosition, wxSize(1280, 800))
{
  auto* menuFile = new wxMenu;
  menuFile->Append(ID_Open, "&Open...\tCtrl+O");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT, "E&xit");
  auto* menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");

  auto* mView = new wxMenu;
  mView->Append( ID_SetTriColor, "Set &Triangle Color..." );
  mView->Append( ID_SetEdgeColor, "Set &Edge Color..." );
  mView->AppendCheckItem( ID_ToggleEdges, "Show &Edges" )->Check( true );
  menuBar->Append( mView, "&View" );

  auto* mMesh = new wxMenu;
  mMesh->Append( ID_QMorph, "&QMorph" );
  menuBar->Append( mMesh, "&Mesh" );

  SetMenuBar(menuBar);
  CreateStatusBar();

  canvas_ = new GLCanvas(this);
  auto* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(canvas_, 1, wxEXPAND);
  SetSizer(sizer);
}

void MainFrame::OnOpen(wxCommandEvent&) {
  wxFileDialog dlg(this, "Open mesh", "", "", "Meshes (*.mesh)|*.mesh|All files|*.*",
                   wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (dlg.ShowModal() == wxID_OK) {
    canvas_->LoadMesh(std::string(dlg.GetPath().ToUTF8()));
  }
}

void MainFrame::OnQuit(wxCommandEvent&) { Close(true); }

static inline void ApplyColorDialog( wxWindow* parent,
                                     std::function<void( float, float, float, float )> setter,
                                     const wxColour& initial = *wxWHITE )
{
    wxColourData data; data.SetChooseFull( true ); data.SetColour( initial );
    wxColourDialog dlg( parent, &data );
    if ( dlg.ShowModal() == wxID_OK )
    {
        wxColour c = dlg.GetColourData().GetColour();
        setter( c.Red() / 255.f, c.Green() / 255.f, c.Blue() / 255.f, 1.0f );
    }
}

void MainFrame::OnSetTriColor( wxCommandEvent& )
{
    // optional: ask GLCanvas for current color if you exposed a getter
    ApplyColorDialog( this, [this]( float r, float g, float b, float a )
                      {
                          canvas_->SetTriangleColor( r, g, b, a );
                      }, ToWx( canvas_->GetTriangleColor() ) );
}

void MainFrame::OnSetEdgeColor( wxCommandEvent& )
{
    ApplyColorDialog( this, [this]( float r, float g, float b, float a )
                      {
                          canvas_->SetEdgeColor( r, g, b, a );
                      }, ToWx( canvas_->GetEdgeColor() ) );
}

void
MainFrame::OnToggleEdges( wxCommandEvent& e )
{
    // add this method on GLCanvas if you haven’t yet
    // e.IsChecked() is true/false
    canvas_->SetShowSegments( e.IsChecked() );
}

void 
MainFrame::OnQMorph( wxCommandEvent& )
{
    auto Morph = std::make_shared<QMorph>();
    Morph->init( -1, 0.6, false );
    Morph->run();

    canvas_->RegenerateMeshDisplay();
}
