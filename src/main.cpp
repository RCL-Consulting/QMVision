#include <wx/wx.h>
#include "MainFrame.h"

class App : public wxApp
{
public:
	bool OnInit() override
	{
#ifdef _WIN32
#ifdef _DEBUG
        // Open a console window for stdout/stderr in Debug GUI builds
        if ( AllocConsole() )
        {
            FILE* f;
            freopen_s( &f, "CONOUT$", "w", stdout );
            freopen_s( &f, "CONOUT$", "w", stderr );
            // optional: ensure C++ iostreams are tied to C stdio
            std::ios::sync_with_stdio();
            // optional: print a banner
            printf( "QMVision debug console ready.\n" );
        }
#endif
#endif
		auto* frame = new MainFrame();
		frame->Show( true );
		return true;
	}

    int OnExit() override
    {
#ifdef _WIN32
#ifdef _DEBUG
        // Close the console when the app exits
        FreeConsole();
#endif
#endif
        return wxApp::OnExit();
    }
};
wxIMPLEMENT_APP( App );
