//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <wx/app.h>

#include <wx/archive.h>
#include <wx/config.h>		// Do_not_auto_remove (MacOS 10.3, wx 2.7)
#include <wx/confbase.h>	// Do_not_auto_remove (MacOS 10.3, wx 2.7)
#include <wx/html/htmlwin.h>
#include <wx/mimetype.h>	// Do_not_auto_remove (win32)
#include <wx/stattext.h>
#include <wx/stdpaths.h>
#include <wx/textfile.h>	// Do_not_auto_remove (win32)
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/wupdlock.h>	// Needed for wxWindowUpdateLocker

#include <common/EventIDs.h>

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for SVNDATE, PACKAGE, VERSION
#else
#include <common/ClientVersion.h>
#endif // HAVE_CONFIG_H

#include "amuleDlg.h"		// Interface declarations.

#include <common/Format.h>	// Needed for CFormat
#include "amule.h"		// Needed for theApp
#include "ChatWnd.h"		// Needed for CChatWnd
#include "ClientListCtrl.h"	// Needed for CClientListCtrl
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "KadDlg.h"		// Needed for CKadDlg
#include "Logger.h"
#include "MuleTrayIcon.h"
#include "muuli_wdr.h"		// Needed for ID_BUTTON*
#include "Preferences.h"	// Needed for CPreferences
#include "PrefsUnifiedDlg.h"
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "Server.h"		// Needed for CServer
#include "ServerConnect.h"	// Needed for CServerConnect
#include "ServerWnd.h"		// Needed for CServerWnd
#include "SharedFilesWnd.h"	// Needed for CSharedFilesWnd
#include "Statistics.h"		// Needed for theStats
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "TerminationProcess.h"	// Needed for CTerminationProcess
#include "TransferWnd.h"	// Needed for CTransferWnd
#ifndef CLIENT_GUI
#include "PartFileConvert.h"
#endif

#ifndef __WXMSW__
#include "aMule.xpm"
#endif

#include "kademlia/kademlia/Kademlia.h"

#ifdef ENABLE_IP2COUNTRY
	#include "IP2Country.h"		// Needed for IP2Country
#endif

BEGIN_EVENT_TABLE(CamuleDlg, wxFrame)

	EVT_TOOL(ID_BUTTONNETWORKS, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSEARCH, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONTRANSFER, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSHARED, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONMESSAGES, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSTATISTICS, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_ABOUT, CamuleDlg::OnAboutButton)

	EVT_TOOL(ID_BUTTONNEWPREFERENCES, CamuleDlg::OnPrefButton)
#ifndef CLIENT_GUI
	EVT_TOOL(ID_BUTTONIMPORT, CamuleDlg::OnImportButton)
#endif

	EVT_TOOL(ID_BUTTONCONNECT, CamuleDlg::OnBnConnect)

	EVT_CLOSE(CamuleDlg::OnClose)
	EVT_ICONIZE(CamuleDlg::OnMinimize)

	EVT_BUTTON(ID_BUTTON_FAST, CamuleDlg::OnBnClickedFast)
	EVT_BUTTON(IDC_SHOWSTATUSTEXT, CamuleDlg::OnBnStatusText)

	EVT_TIMER(ID_GUI_TIMER_EVENT, CamuleDlg::OnGUITimer)

	EVT_SIZE(CamuleDlg::OnMainGUISizeChange)

	EVT_KEY_UP(CamuleDlg::OnKeyPressed)

	EVT_MENU(wxID_EXIT, CamuleDlg::OnExit)
	
END_EVENT_TABLE()

#ifndef wxCLOSE_BOX
	#define wxCLOSE_BOX 0
#endif

CamuleDlg::CamuleDlg(
	wxWindow* pParent,
	const wxString &title,
	wxPoint where,
	wxSize dlg_size)
:
wxFrame(
	pParent, -1, title, where, dlg_size,
	wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxDIALOG_NO_PARENT|
	wxRESIZE_BORDER|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX,
	wxT("aMule")),
m_activewnd(NULL),
m_transferwnd(NULL),
m_serverwnd(NULL),
m_sharedfileswnd(NULL),
m_searchwnd(NULL),
m_chatwnd(NULL),
m_statisticswnd(NULL),
m_kademliawnd(NULL),
m_prefsDialog(NULL),
m_srv_split_pos(0),
m_imagelist(16,16),
m_tblist(32,32),
m_prefsVisible(false),
m_wndToolbar(NULL),
m_wndTaskbarNotifier(NULL),
m_nActiveDialog(DT_NETWORKS_WND),
m_is_safe_state(false),
m_BlinkMessages(false),
m_CurrentBlinkBitmap(24),
m_last_iconizing(0),
m_skinFileName(),
m_clientSkinNames(CLIENT_SKIN_SIZE)
{
	// Initialize skin names
	m_clientSkinNames[Client_Green_Smiley]            = wxT("Transfer");
	m_clientSkinNames[Client_Red_Smiley]              = wxT("Connecting");
	m_clientSkinNames[Client_Yellow_Smiley]           = wxT("OnQueue");
	m_clientSkinNames[Client_Grey_Smiley]             = wxT("A4AFNoNeededPartsQueueFull");
	m_clientSkinNames[Client_White_Smiley]            = wxT("StatusUnknown");
	m_clientSkinNames[Client_ExtendedProtocol_Smiley] = wxT("ExtendedProtocol");
	m_clientSkinNames[Client_SecIdent_Smiley]         = wxT("SecIdent");
	m_clientSkinNames[Client_BadGuy_Smiley]           = wxT("BadGuy");
	m_clientSkinNames[Client_CreditsGrey_Smiley]      = wxT("CreditsGrey");
	m_clientSkinNames[Client_CreditsYellow_Smiley]    = wxT("CreditsYellow");
	m_clientSkinNames[Client_Upload_Smiley]           = wxT("Upload");
	m_clientSkinNames[Client_Friend_Smiley]           = wxT("Friend");
	m_clientSkinNames[Client_eMule_Smiley]            = wxT("eMule");
	m_clientSkinNames[Client_mlDonkey_Smiley]         = wxT("mlDonkey");
	m_clientSkinNames[Client_eDonkeyHybrid_Smiley]    = wxT("eDonkeyHybrid");
	m_clientSkinNames[Client_aMule_Smiley]            = wxT("aMule");
	m_clientSkinNames[Client_lphant_Smiley]           = wxT("lphant");
	m_clientSkinNames[Client_Shareaza_Smiley]         = wxT("Shareaza");
	m_clientSkinNames[Client_xMule_Smiley]            = wxT("xMule");
	m_clientSkinNames[Client_Unknown]                 = wxT("Unknown");
	m_clientSkinNames[Client_InvalidRating_Smiley]    = wxT("InvalidRatingOnFile");
	m_clientSkinNames[Client_PoorRating_Smiley]       = wxT("PoorRatingOnFile");
	m_clientSkinNames[Client_GoodRating_Smiley]       = wxT("GoodRatingOnFile");
	m_clientSkinNames[Client_FairRating_Smiley]       = wxT("FairRatingOnFile");
	m_clientSkinNames[Client_ExcellentRating_Smiley]  = wxT("ExcellentRatingOnFile");
	m_clientSkinNames[Client_CommentOnly_Smiley]      = wxT("CommentOnly");
	m_clientSkinNames[Client_Encryption_Smiley]       = wxT("Encrypted");
	
	// wxWidgets send idle events to ALL WINDOWS by default... *SIGH*
	wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);
	wxUpdateUIEvent::SetMode(wxUPDATE_UI_PROCESS_SPECIFIED);
	wxInitAllImageHandlers();
	Apply_Clients_Skin();
	
	bool override_where = where != wxDefaultPosition;
	bool override_size =
		dlg_size.x != DEFAULT_SIZE_X ||
		dlg_size.y != DEFAULT_SIZE_Y;
	if (!LoadGUIPrefs(override_where, override_size)) {
		// Prefs not loaded for some reason, exit
		AddLogLineM( true, wxT("Error! Unable to load Preferences") );
		return;
	}

	SetIcon(wxICON(aMule));

	srand(time(NULL));

	// Create new sizer and stuff a wxPanel in there.
	wxFlexGridSizer *s_main = new wxFlexGridSizer(1);
	s_main->AddGrowableCol(0);
	s_main->AddGrowableRow(0);

	wxPanel* p_cnt = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize);
	s_main->Add(p_cnt, 0, wxGROW|wxEXPAND, 0);
	muleDlg(p_cnt, false, true);
	SetSizer(s_main, true);

	m_serverwnd = new CServerWnd(p_cnt, m_srv_split_pos);
	AddLogLineM(false, wxEmptyString);
	AddLogLineM(false, wxT(" - ") +
		CFormat(_("This is aMule %s based on eMule.")) % GetMuleVersion());
	AddLogLineM(false, wxT("   ") +
		CFormat(_("Running on %s")) % wxGetOsDescription());
	AddLogLineM(false, wxT(" - ") +
		wxString(_("Visit http://www.amule.org to check if a new version is available.")));
	AddLogLineM(false, wxEmptyString);

#ifdef ENABLE_IP2COUNTRY
	m_IP2Country = new CIP2Country();
#endif
	m_searchwnd = new CSearchDlg(p_cnt);
	m_transferwnd = new CTransferWnd(p_cnt);
	m_sharedfileswnd = new CSharedFilesWnd(p_cnt);
	m_statisticswnd = new CStatisticsDlg(p_cnt, theApp->m_statistics);
	m_chatwnd = new CChatWnd(p_cnt);
	m_kademliawnd = CastChild(wxT("kadWnd"), CKadDlg);

	m_serverwnd->Show(false);
	m_searchwnd->Show(false);
	m_transferwnd->Show(false);
	m_sharedfileswnd->Show(false);
	m_statisticswnd->Show(false);
	m_chatwnd->Show(false);

	// Create the GUI timer
	gui_timer=new wxTimer(this,ID_GUI_TIMER_EVENT);
	if (!gui_timer) {
		AddLogLine(false, _("Fatal Error: Failed to create Timer"));
		exit(1);
	}

	// Set Serverlist as active window
	Create_Toolbar(thePrefs::VerticalToolbar());
	SetActiveDialog(DT_NETWORKS_WND, m_serverwnd);
	m_wndToolbar->ToggleTool(ID_BUTTONNETWORKS, true );

	m_is_safe_state = true;

	// Init statistics stuff, better do it asap
	m_statisticswnd->Init();
	m_kademliawnd->Init();
	m_searchwnd->UpdateCatChoice();
	if (thePrefs::UseTrayIcon()) {
		CreateSystray();
	}

	Show(true);
	// Must we start minimized?
	if (thePrefs::GetStartMinimized()) { 
		DoIconize(true);
	}

	// Set shortcut keys
	wxAcceleratorEntry entries[] = { 
		wxAcceleratorEntry(wxACCEL_CTRL, wxT('Q'), wxID_EXIT)
	};
	
	SetAcceleratorTable(wxAcceleratorTable(itemsof(entries), entries));	
	ShowED2KLinksHandler( thePrefs::GetFED2KLH() );
}


// Madcat - Sets Fast ED2K Links Handler on/off.
void CamuleDlg::ShowED2KLinksHandler( bool show )
{
	// Errorchecking in case the pointer becomes invalid ...
	if (s_fed2klh == NULL) {
		wxLogWarning(wxT("Unable to find Fast ED2K Links handler sizer! Hiding FED2KLH aborted."));
		return;
	}
	
	s_dlgcnt->Show( s_fed2klh, show );
	s_dlgcnt->Layout();
}

// Toogles ed2k link handler.
void CamuleDlg::ToogleED2KLinksHandler()
{
	// Errorchecking in case the pointer becomes invalid ...
	if (s_fed2klh == NULL) {
		wxLogWarning(wxT("Unable to find Fast ED2K Links handler sizer! Toogling FED2KLH aborted."));
		return;
	}
	ShowED2KLinksHandler(!s_dlgcnt->IsShown(s_fed2klh));
}

void CamuleDlg::SetActiveDialog(DialogType type, wxWindow* dlg)
{
	m_nActiveDialog = type;
	
	if ( type == DT_TRANSFER_WND ) {
		if (thePrefs::ShowCatTabInfos()) {
			m_transferwnd->UpdateCatTabTitles();
		}
	}
	
	if ( m_activewnd ) {
		m_activewnd->Show(false);
		contentSizer->Detach(m_activewnd);
	}

	contentSizer->Add(dlg, 1, wxALIGN_LEFT|wxEXPAND);
	dlg->Show(true);
	m_activewnd=dlg;
	s_dlgcnt->Layout();

	// Since we might be suspending redrawing while hiding the dialog
	// we have to refresh it once it is visible again
	dlg->Refresh( true );
	dlg->SetFocus();
}


void CamuleDlg::UpdateTrayIcon(int percent)
{
	// set trayicon-icon
	if(!theApp->IsConnected()) {
		m_wndTaskbarNotifier->SetTrayIcon(TRAY_ICON_DISCONNECTED, percent);
	} else {
		if(theApp->IsConnectedED2K() && theApp->serverconnect->IsLowID()) {
			m_wndTaskbarNotifier->SetTrayIcon(TRAY_ICON_LOWID, percent);
		} else {
			m_wndTaskbarNotifier->SetTrayIcon(TRAY_ICON_HIGHID, percent);					
		}
	}
}

		
void CamuleDlg::CreateSystray()
{
	wxCHECK_RET(m_wndTaskbarNotifier == NULL,
		wxT("Systray already created"));

	m_wndTaskbarNotifier = new CMuleTrayIcon();
	// This will effectively show the Tray Icon.
	UpdateTrayIcon(0);
}	
	

void CamuleDlg::RemoveSystray()
{
	delete m_wndTaskbarNotifier;
	m_wndTaskbarNotifier = NULL;
}


void CamuleDlg::OnToolBarButton(wxCommandEvent& ev)
{
	static int lastbutton = ID_BUTTONNETWORKS;

	// Kry - just if the GUI is ready for it
	if ( m_is_safe_state ) {

		// Rehide the handler if needed
		if ( lastbutton == ID_BUTTONSEARCH && !thePrefs::GetFED2KLH() ) {
			if (ev.GetId() != ID_BUTTONSEARCH) {
				ShowED2KLinksHandler( false );
			} else {
				// Toogle ED2K handler.
				ToogleED2KLinksHandler();
			}
		}

		if ( lastbutton != ev.GetId() ) {
			switch ( ev.GetId() ) {
				case ID_BUTTONNETWORKS:
					SetActiveDialog(DT_NETWORKS_WND, m_serverwnd);
					// Set serverlist splitter position
					CastChild( wxT("SrvSplitterWnd"), wxSplitterWindow )->SetSashPosition(m_srv_split_pos, true);
					break;

				case ID_BUTTONSEARCH:
					// The search dialog should always display the handler
					if ( !thePrefs::GetFED2KLH() )
						ShowED2KLinksHandler( true );

					SetActiveDialog(DT_SEARCH_WND, m_searchwnd);
					break;

				case ID_BUTTONTRANSFER:
					SetActiveDialog(DT_TRANSFER_WND, m_transferwnd);
					// Prepare the dialog, sets the splitter-position
					m_transferwnd->Prepare();
					break;

				case ID_BUTTONSHARED:
					SetActiveDialog(DT_SHARED_WND, m_sharedfileswnd);
					break;

				case ID_BUTTONMESSAGES:
					m_BlinkMessages = false;
					SetActiveDialog(DT_CHAT_WND, m_chatwnd);
					break;

				case ID_BUTTONSTATISTICS:
					SetActiveDialog(DT_STATS_WND, m_statisticswnd);
					break;

				// This shouldn't happen, but just in case
				default:
					AddLogLineM( true, wxT("Unknown button triggered CamuleApp::OnToolBarButton().") );
					break;
			}
		}

		m_wndToolbar->ToggleTool(lastbutton, lastbutton == ev.GetId() );
		lastbutton = ev.GetId();
	}
}


void CamuleDlg::OnAboutButton(wxCommandEvent& WXUNUSED(ev))
{
	wxString msg = wxT(" ");
#ifdef CLIENT_GUI
	msg << _("aMule remote control ") << wxT(VERSION);
#else
	msg << wxT("aMule ") << wxT(VERSION);
#endif
	msg << wxT(" ");
#ifdef SVNDATE
	msg << _("Snapshot:") << wxT("\n ") << wxT(SVNDATE);
#endif
	msg << wxT("\n\n") << _(" 'All-Platform' p2p client based on eMule \n\n") <<
		_(" Website: http://www.amule.org \n") <<
		_(" Forum: http://forum.amule.org \n") << 
		_(" FAQ: http://wiki.amule.org \n\n") <<
		_(" Contact: admin@amule.org (administrative issues) \n") <<
		_(" Copyright (C) 2003-2008 aMule Team \n\n") <<
		_(" Part of aMule is based on \n") <<
		_("Kademlia: Peer-to-peer routing based on the XOR metric.\n") <<
		_(" Copyright (C) 2002 Petar Maymounkov\n") <<
		_(" http://kademlia.scs.cs.nyu.edu\n");
	
	if (m_is_safe_state) {
		wxMessageBox(msg, _("Message"), wxOK | wxICON_INFORMATION, this);
	}
}


void CamuleDlg::OnPrefButton(wxCommandEvent& WXUNUSED(ev))
{
	if (m_is_safe_state) {
		if (m_prefsDialog == NULL) {
			m_prefsDialog = new PrefsUnifiedDlg(this);
		}
		
		m_prefsDialog->TransferToWindow();
		m_prefsDialog->Show(true);
		m_prefsDialog->Raise();
	}
}


#ifndef CLIENT_GUI
void CamuleDlg::OnImportButton(wxCommandEvent& WXUNUSED(ev))
{
	if ( m_is_safe_state ) {
		CPartFileConvert::ShowGUI(NULL);
	}
}
#endif

CamuleDlg::~CamuleDlg()
{
	printf("Shutting down aMule...\n");
	
	SaveGUIPrefs();

	theApp->amuledlg = NULL;
	
	printf("aMule dialog destroyed\n");
}


void CamuleDlg::OnBnConnect(wxCommandEvent& WXUNUSED(evt))
{

	bool disconnect = (theApp->IsConnectedED2K() || theApp->serverconnect->IsConnecting()) 
						#ifndef CLIENT_GUI
						|| (Kademlia::CKademlia::IsRunning())
						#endif
						;	
	if (thePrefs::GetNetworkED2K()) {
		if (disconnect) {
			//disconnect if currently connected
			if (theApp->serverconnect->IsConnecting()) {
				theApp->serverconnect->StopConnectionTry();
			} else {
				theApp->serverconnect->Disconnect();
			}
		} else {		
			//connect if not currently connected
			AddLogLine(true, _("Connecting"));
			theApp->serverconnect->ConnectToAnyServer();
		}
	} else {
		wxASSERT(!theApp->IsConnectedED2K());
	}

	// Connect Kad also
	if (thePrefs::GetNetworkKademlia()) {
		if( disconnect ) {
			theApp->StopKad();
		} else {
			theApp->StartKad();
		}
	} else {
		#ifndef CLIENT_GUI
			wxASSERT(!Kademlia::CKademlia::IsRunning());
		#endif
	}

	ShowConnectionState();
}


void CamuleDlg::OnBnStatusText(wxCommandEvent& WXUNUSED(evt))
{
	wxString line = CastChild(wxT("infoLabel"), wxStaticText)->GetLabel();

	if (!line.IsEmpty()) {
		wxMessageBox(line, wxString(_("Status text")), wxOK|wxICON_INFORMATION, this);
	}
}


void CamuleDlg::ResetLog(int id)
{
	wxTextCtrl* ct = CastByID(id, m_serverwnd, wxTextCtrl);
	wxCHECK_RET(ct, wxT("Resetting unknown log"));

	ct->Clear();
	
	if (id == ID_LOGVIEW) {
		// Also clear the log line
		wxStaticText* text = CastChild(wxT("infoLabel"), wxStaticText);
		text->SetLabel(wxEmptyString);
		text->GetParent()->Layout();
	}
}


void CamuleDlg::AddLogLine(bool addtostatusbar, const wxString& line)
{
	// Remove newspace at end, it causes problems with the layout...
	wxString bufferline = line.Strip(wxString::trailing);

	// Create the timestamp
	wxString stamp = wxDateTime::Now().FormatISODate() + wxT(" ") + wxDateTime::Now().FormatISOTime() + wxT(": ");

	// Add the message to the log-view
	wxTextCtrl* ct = CastByID( ID_LOGVIEW, m_serverwnd, wxTextCtrl );
	if ( ct ) {
		if ( bufferline.IsEmpty() ) {
			// If it's empty we just write a blank line with no timestamp.
			ct->AppendText( wxT("\n") );
		} else {
			// Bold critical log-lines
			wxTextAttr style = ct->GetDefaultStyle();
			wxFont font = style.GetFont();
			font.SetWeight(addtostatusbar ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
			style.SetFont(font);
			ct->SetDefaultStyle(style);
			
			// Split multi-line messages into individual lines
			wxStringTokenizer tokens( bufferline, wxT("\n") );		
			while ( tokens.HasMoreTokens() ) {
				ct->AppendText( stamp + tokens.GetNextToken() + wxT("\n") );
			}
		}
			
		ct->ShowPosition( ct->GetLastPosition() - 1 );
	}
	

	// Set the status-bar if the event warrents it
	if ( addtostatusbar ) {
		// Escape "&"s, which would otherwise not show up
		bufferline.Replace( wxT("&"), wxT("&&") );
		wxStaticText* text = CastChild( wxT("infoLabel"), wxStaticText );
		// Only show the first line if multiple lines
		text->SetLabel( bufferline.BeforeFirst( wxT('\n') ) );
		text->GetParent()->Layout();
	}
	
}


void CamuleDlg::AddServerMessageLine(wxString& message)
{
	wxTextCtrl* cv= CastByID( ID_SERVERINFO, m_serverwnd, wxTextCtrl );
	if(cv) {
		if (message.Length() > 500) {
			cv->AppendText(message.Left(500) + wxT("\n"));
		} else {
			cv->AppendText(message + wxT("\n"));
		}
		cv->ShowPosition(cv->GetLastPosition()-1);
	}
}


void CamuleDlg::ShowConnectionState()
{
	static wxImageList status_arrows(16,16,true,0);
	if (!status_arrows.GetImageCount()) {
		// Generate the image list (This is only done once)
		for (int t = 0; t < 7; ++t) {
			status_arrows.Add(connImages(t));
		}
	}
	
	m_serverwnd->UpdateED2KInfo();
	m_serverwnd->UpdateKadInfo();


	////////////////////////////////////////////////////////////	
	// Determine the status of the networks
	//
	enum ED2KState { ED2KOff = 0, ED2KLowID = 1, ED2KConnecting = 2, ED2KHighID = 3 };
	enum EKadState { EKadOff = 4, EKadFW = 5, EKadConnecting = 5, EKadOK = 6 };

	ED2KState ed2kState = ED2KOff;
	EKadState kadState  = EKadOff;

	////////////////////////////////////////////////////////////	
	// Update the label on the status-bar and determine
	// the states of the two networks.
	//
	wxString msgED2K;
	if (theApp->IsConnectedED2K()) {
		CServer* server = theApp->serverconnect->GetCurrentServer();
		if (server) {
			msgED2K = CFormat(wxT("ED2K: %s")) % server->GetListName();
		}

		if (theApp->serverconnect->IsLowID()) {
			ed2kState = ED2KLowID;
		} else {
			ed2kState = ED2KHighID;
		}
	} else if (theApp->serverconnect->IsConnecting()) {
		msgED2K = _("ED2K: Connecting");

		ed2kState = ED2KConnecting;
	} else if (thePrefs::GetNetworkED2K()) {
		msgED2K = _("ED2K: Disconnected");
	}

	wxString msgKad;
	if (theApp->IsConnectedKad()) {
		if (theApp->IsFirewalledKad()) {
			msgKad = _("Kad: Firewalled");

			kadState = EKadFW;
		} else {
			msgKad = _("Kad: Connected");
			
			kadState = EKadOK;
		}
	} else if (theApp->IsKadRunning()) {
		msgKad = _("Kad: Connecting");

		kadState = EKadConnecting;
	} else if (thePrefs::GetNetworkKademlia()) {
		msgKad = _("Kad: Off");
	}
	
	wxStaticText* connLabel = CastChild( wxT("connLabel"), wxStaticText );
	wxCHECK_RET(connLabel, wxT("'connLabel' widget not found"));

	wxString labelMsg;
	if (msgED2K.Length() && msgKad.Length()) {
		labelMsg = msgED2K + wxT(" | ") + msgKad;
	} else {
		labelMsg = msgED2K + msgKad;
	}

	connLabel->SetLabel(labelMsg);
	connLabel->GetParent()->Layout();


	////////////////////////////////////////////////////////////	
	// Update the connect/disconnect/cancel button.
	//
	enum EConnState {
		ECS_Unknown,
		ECS_Connected,
		ECS_Connecting,
		ECS_Disconnected
	};

	static EConnState s_oldState = ECS_Unknown;
	EConnState currentState = ECS_Disconnected;

	if (theApp->serverconnect->IsConnecting() ||
			(theApp->IsKadRunning() && !theApp->IsConnectedKad())) {
		currentState = ECS_Connecting;
	} else if (theApp->IsConnected()) {
		currentState = ECS_Connected;
	} else {
		currentState = ECS_Disconnected;
	}

	if (currentState != s_oldState) {
		m_wndToolbar->Freeze();
		wxToolBarToolBase* toolbarTool = m_wndToolbar->RemoveTool(ID_BUTTONCONNECT);

		switch (currentState) {
			case ECS_Connecting:
				toolbarTool->SetLabel(_("Cancel"));
				toolbarTool->SetShortHelp(_("Stop the current connection attempts"));
				toolbarTool->SetNormalBitmap(m_tblist.GetBitmap(2));
				break;

			case ECS_Connected:
				toolbarTool->SetLabel(_("Disconnect"));
				toolbarTool->SetShortHelp(_("Disconnect from the currently connected networks"));
				toolbarTool->SetNormalBitmap(m_tblist.GetBitmap(1));
				break;

			default:
				toolbarTool->SetLabel(_("Connect"));
				toolbarTool->SetShortHelp(_("Connect to the currently enabled networks"));
				toolbarTool->SetNormalBitmap(m_tblist.GetBitmap(0));
		}

		m_wndToolbar->InsertTool(0, toolbarTool);
		m_wndToolbar->Realize();
		m_wndToolbar->Thaw();

		s_oldState = currentState;
	}


	////////////////////////////////////////////////////////////	
	// Update the globe-icon in the lower-right corner.
	// 	
	wxStaticBitmap* connBitmap = CastChild( wxT("connImage"), wxStaticBitmap );
	wxCHECK_RET(connBitmap, wxT("'connImage' widget not found"));

	wxBitmap statusIcon = connBitmap->GetBitmap();
	wxMemoryDC bitmapDC(statusIcon);

	status_arrows.Draw(kadState, bitmapDC, 0, 0, wxIMAGELIST_DRAW_TRANSPARENT);
	status_arrows.Draw(ed2kState, bitmapDC, 0, 0, wxIMAGELIST_DRAW_TRANSPARENT);

	connBitmap->SetBitmap(statusIcon);
}


void CamuleDlg::ShowUserCount(const wxString& info)
{
	wxStaticText* label = CastChild( wxT("userLabel"), wxStaticText );
	
	// Update Kad tab
	m_serverwnd->UpdateKadInfo();
	
	label->SetLabel(info);
	label->GetParent()->Layout();
}


void CamuleDlg::ShowTransferRate()
{
	float kBpsUp = theStats::GetUploadRate() / 1024.0;
	float kBpsDown = theStats::GetDownloadRate() / 1024.0;
	wxString buffer;
	if( thePrefs::ShowOverhead() )
	{
		buffer = wxString::Format(_("Up: %.1f(%.1f) | Down: %.1f(%.1f)"), kBpsUp, theStats::GetUpOverheadRate() / 1024.0, kBpsDown, theStats::GetDownOverheadRate() / 1024.0);
	} else {
		buffer = wxString::Format(_("Up: %.1f | Down: %.1f"), kBpsUp, kBpsDown);
	}
	buffer.Truncate(50); // Max size 50

	wxStaticText* label = CastChild( wxT("speedLabel"), wxStaticText );
	label->SetLabel(buffer);
	label->GetParent()->Layout();

	// Show upload/download speed in title
	if (thePrefs::GetShowRatesOnTitle()) {
		wxString UpDownSpeed = wxString::Format(wxT(" -- Up: %.1f | Down: %.1f"), kBpsUp, kBpsDown);
		SetTitle(theApp->m_FrameTitle + UpDownSpeed);
	}

	wxASSERT((bool)m_wndTaskbarNotifier == thePrefs::UseTrayIcon());
	if (m_wndTaskbarNotifier) {
		// set trayicon-icon
		int percentDown = (int)ceil((kBpsDown*100) / thePrefs::GetMaxGraphDownloadRate());
		UpdateTrayIcon( ( percentDown > 100 ) ? 100 : percentDown);
	
		wxString buffer2;
		if ( theApp->IsConnected() ) {
			buffer2 = CFormat(_("aMule (%s | Connected)")) % buffer;
		} else {
			buffer2 = CFormat(_("aMule (%s | Disconnected)")) % buffer;
		}
		m_wndTaskbarNotifier->SetTrayToolTip(buffer2);
	}

	wxStaticBitmap* bmp = CastChild( wxT("transferImg"), wxStaticBitmap );
	bmp->SetBitmap(dlStatusImages((kBpsUp>0.01 ? 2 : 0) + (kBpsDown>0.01 ? 1 : 0)));
}

void CamuleDlg::DlgShutDown()
{
	// Are we already shutting down or still on init?
	if ( m_is_safe_state == false ) {
		return;
	}

	// we are going DOWN
	m_is_safe_state = false;

	// Stop the GUI Timer
	delete gui_timer;
	m_transferwnd->downloadlistctrl->DeleteAllItems();

	// We want to delete the systray too!
	RemoveSystray();
}

void CamuleDlg::OnClose(wxCloseEvent& evt)
{
	// This will be here till the core close is != app close
	if (evt.CanVeto() && thePrefs::IsConfirmExitEnabled() ) {
		if (wxNO == wxMessageBox(wxString(_("Do you really want to exit aMule?")),
				wxString(_("Exit confirmation")), wxYES_NO, this)) {
			evt.Veto();
			return;
		}
	}
	
	theApp->ShutDown(evt);
}


void CamuleDlg::OnBnClickedFast(wxCommandEvent& WXUNUSED(evt))
{
	wxTextCtrl* ctl = CastChild( wxT("FastEd2kLinks"), wxTextCtrl );

	for ( int i = 0; i < ctl->GetNumberOfLines(); i++ ) {
		wxString strlink = ctl->GetLineText(i);
		strlink.Trim(true);
		strlink.Trim(false);
		if ( !strlink.IsEmpty() ) {
			theApp->downloadqueue->AddLink( strlink, m_transferwnd->downloadlistctrl->GetCategory() );
		}
	}
	
	ctl->SetValue(wxEmptyString);
}


// Formerly known as LoadRazorPrefs()
bool CamuleDlg::LoadGUIPrefs(bool override_pos, bool override_size)
{
	// Create a config base for loading razor preferences
	wxConfigBase *config = wxConfigBase::Get();
	// If config haven't been created exit without loading
	if (config == NULL) {
		return false;
	}

	// The section where to save in in file
	wxString section = wxT("/Razor_Preferences/");

	// Get window size and position
	int x1 = config->Read(section + wxT("MAIN_X_POS"), 01);
	int y1 = config->Read(section + wxT("MAIN_Y_POS"), 01);
	int x2 = config->Read(section + wxT("MAIN_X_SIZE"), -1);
	int y2 = config->Read(section + wxT("MAIN_Y_SIZE"), -1);

	int maximized = config->Read(section + wxT("Maximized"), 01);

	// Kry - Random usable pos for m_srv_split_pos
	m_srv_split_pos = config->Read(section + wxT("SRV_SPLITTER_POS"), 463l);
	if (!override_size) {
		if (x2 > 0 && y2 > 0) {
			SetSize(x2, y2);
		} else {
#ifndef __WXGTK__
			// Probably first run.
			Maximize();
#endif
		}
	}

	if (!override_pos) {
		// If x1 and y1 != 0 Redefine location
		if(x1 != -1 && y1 != -1) {
			Move(x1, y1);
		}
	}

	if (!override_size && !override_pos && maximized) {
		Maximize();
	}

	return true;
}


bool CamuleDlg::SaveGUIPrefs()
{
	/* Razor 1a - Modif by MikaelB
	   Save client size and position */

	// Create a config base for saving razor preferences
	wxConfigBase *config = wxConfigBase::Get();
	// If config haven't been created exit without saving
	if (config == NULL) {
		return false;
	}
	// The section where to save in in file
	wxString section = wxT("/Razor_Preferences/");

	// Main window location and size
	int x1, y1, x2, y2;
	GetPosition(&x1, &y1);
	GetSize(&x2, &y2);

	// Saving window size and position
	config->Write(section+wxT("MAIN_X_POS"), (long) x1);
	config->Write(section+wxT("MAIN_Y_POS"), (long) y1);
	config->Write(section+wxT("MAIN_X_SIZE"), (long) x2);
	config->Write(section+wxT("MAIN_Y_SIZE"), (long) y2);
	config->Write(section+wxT("Maximized"), (long) (IsMaximized() ? 1 : 0));

	// Saving sash position of splitter in server window
	config->Write(section+wxT("SRV_SPLITTER_POS"), (long) m_srv_split_pos);

	config->Flush(true);

	/* End modif */

	return true;
}


void CamuleDlg::DoIconize(bool iconize) 
{
	// Evil Hack: check if the mouse is inside the window
#ifndef __WINDOWS__
	if (GetScreenRect().Contains(wxGetMousePosition()))
#endif
	{
		if (m_wndTaskbarNotifier && thePrefs::DoMinToTray()) {
			if (iconize) {
				// Skip() will do it.
				//Iconize(true);
				if (SafeState()) {
					Show(false);
				}
			} else {
				Show(true);
				Raise();
			}
		} else {
			// Will be done by Skip();
			//Iconize(iconize);
		}
	}
}

void CamuleDlg::OnMinimize(wxIconizeEvent& evt)
{
	if (m_prefsDialog && m_prefsDialog->IsShown()) {
		// Veto.
	} else {
		if (m_wndTaskbarNotifier) {
			DoIconize(evt.Iconized());
		}
		evt.Skip();
	}
}

void CamuleDlg::OnGUITimer(wxTimerEvent& WXUNUSED(evt))
{
	// Former TimerProc section

	static uint32	msPrev1, msPrev5, msPrevStats;

	uint32 			msCur = theStats::GetUptimeMillis();

	// can this actually happen under wxwin ?
	if (!SafeState()) {
		return;
	}

	bool bStatsVisible = (!IsIconized() && StatisticsWindowActive());
	
#ifndef CLIENT_GUI
	static uint32 msPrevGraph;
	int msGraphUpdate = thePrefs::GetTrafficOMeterInterval() * 1000;
	if ((msGraphUpdate > 0)  && ((msCur / msGraphUpdate) > (msPrevGraph / msGraphUpdate))) {
		// trying to get the graph shifts evenly spaced after a change in the update period
		msPrevGraph = msCur;
		
		GraphUpdateInfo update = theApp->m_statistics->GetPointsForUpdate();
		
		m_statisticswnd->UpdateStatGraphs(bStatsVisible, theStats::GetPeakConnections(), update);
		m_kademliawnd->UpdateGraph(!IsIconized() && (m_activewnd == m_serverwnd), update);
	}
#else
	//#warning TODO: CORE/GUI -- EC needed
#endif
	
	int sStatsUpdate = thePrefs::GetStatsInterval();
	if ((sStatsUpdate > 0) && ((int)(msCur - msPrevStats) > sStatsUpdate*1000)) {
		if (bStatsVisible) {
			msPrevStats = msCur;
			m_statisticswnd->ShowStatistics();
		}
	}

	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		ShowTransferRate();
		if (thePrefs::ShowCatTabInfos() && theApp->amuledlg->m_activewnd == theApp->amuledlg->m_transferwnd) {
			m_transferwnd->UpdateCatTabTitles();
		}
		if (thePrefs::AutoSortDownload()) {
			m_transferwnd->downloadlistctrl->SortList();
		}
	}
	
	if (msCur-msPrev1 > 1000) {  // every second
		msPrev1 = msCur;
		if (m_CurrentBlinkBitmap == 12) {
			m_CurrentBlinkBitmap = 7;
			SetMessagesTool();		
		} else {
			if (m_BlinkMessages) {
				m_CurrentBlinkBitmap = 12;
				SetMessagesTool();
			}
		}
		
	}
}


void CamuleDlg::SetMessagesTool()
{
	int pos = m_wndToolbar->GetToolPos(ID_BUTTONMESSAGES);
	wxASSERT(pos == 6); // so we don't miss a change on wx2.4
	
	wxWindowUpdateLocker freezer(m_wndToolbar);
	wxToolBarToolBase* item = m_wndToolbar->RemoveTool(ID_BUTTONMESSAGES);
	item->SetNormalBitmap(m_tblist.GetBitmap(m_CurrentBlinkBitmap));

	m_wndToolbar->InsertTool(pos, item);
	m_wndToolbar->Realize();
}


/*
	Try to launch the specified url:
	 - Windows: Default or custom browser will be used.
	 - Mac: Currently not implemented
	 - Anything else: Try a number of hardcoded browsers. Should be made configurable...
*/
void CamuleDlg::LaunchUrl( const wxString& url )
{
	wxString cmd;

	cmd = thePrefs::GetBrowser();
	if ( !cmd.IsEmpty() ) {
		wxString tmp = url;
		// Pipes cause problems, so escape them
		tmp.Replace( wxT("|"), wxT("%7C") );

		if (!cmd.Replace(wxT("%s"), tmp)) {
			// No %s found, just append the url
			cmd += wxT(" ") + tmp;
		}

		CTerminationProcess *p = new CTerminationProcess(cmd);
		if (wxExecute(cmd, wxEXEC_ASYNC, p)) {
			printf( "Launch Command: %s\n", (const char *)unicode2char(cmd));
			return;
		} else {
			delete p;
		}
#ifdef __WXMSW__
	} else {
		wxFileType* ft =
			wxTheMimeTypesManager->GetFileTypeFromExtension(wxT("html"));
		if (!ft) {
			wxLogError(wxT("Impossible to determine the file type for extension html. Please edit your MIME types."));
			return;
		}

		bool ok = ft->GetOpenCommand(
			&cmd, wxFileType::MessageParameters(url, wxT("")));
		delete ft;

		if (!ok) {
			wxMessageBox(
				_("Could not determine the command for running the browser."),
				wxT("Browsing problem"), wxOK|wxICON_EXCLAMATION, this);
			return;
		}

		wxPuts(wxT("Launch Command: ") + cmd);
		CTerminationProcess *p = new CTerminationProcess(cmd);
		if (wxExecute(cmd, wxEXEC_ASYNC, p)) {
			return;
		} else {
			delete p;
		}
#endif // __WXMSW__
	}
	// Unable to execute browser. But this error message doesn't make sense,
	// cosidering that you _can't_ set the browser executable path... =/
	wxLogError(wxT("Unable to launch browser. Please set correct browser executable path in Preferences."));
}


wxString CamuleDlg::GenWebSearchUrl(const wxString &filename, WebSearch wsProvider )
{
	wxString URL;
	switch (wsProvider)  {
		case WS_FILEHASH:
			URL = wxT("http://www.filehash.com/search.html?pattern=FILENAME&submit=Find");
			break;
		default:
			wxASSERT(0);
	}
	URL.Replace(wxT("FILENAME"), filename);
	
	return URL;
}


bool CamuleDlg::Check_and_Init_Skin()
{
	bool ret = true;
	wxString skinFileName(thePrefs::GetSkin());

	wxString userDir(JoinPaths(GetConfigDir(), wxT("skins")) + wxFileName::GetPathSeparator());
	
	wxStandardPathsBase &spb(wxStandardPaths::Get());
#ifdef __WXMSW__
	wxString dataDir(spb.GetPluginsDir());
#elif defined(__WXMAC__)
		wxString dataDir(spb.GetDataDir());
#else
	wxString dataDir(spb.GetDataDir().BeforeLast(wxT('/')) + wxT("/amule"));
#endif
	wxString systemDir(JoinPaths(dataDir,wxT("skins")) + wxFileName::GetPathSeparator());

		
	skinFileName.Replace(wxT("User:"), userDir );
	skinFileName.Replace(wxT("System:"), systemDir );

	m_skinFileName.Assign(skinFileName);
	if (!m_skinFileName.FileExists()) {
		AddLogLineM(true, CFormat(
			_("Skin directory '%s' does not exist")) %
			skinFileName );
		ret = false;
	} else if (!m_skinFileName.IsFileReadable()) {
		AddLogLineM(true, CFormat(
			_("Warning: Unable to open skin file '%s' for read")) %
			skinFileName);
		ret = false;
	}

		wxFFileInputStream in(m_skinFileName.GetFullPath());
		wxZipInputStream zip(in);

		while ((entry = zip.GetNextEntry()) != NULL) {
			wxZipEntry*& current = cat[entry->GetInternalName()];
			delete current;
			current = entry;
		}

	return ret;
}


void CamuleDlg::Add_Skin_Icon(
	const wxString &iconName,
	const wxBitmap &stdIcon,
	bool useSkins)
{
	wxImage new_image;
	if (useSkins) {
		wxFFileInputStream in(m_skinFileName.GetFullPath());
		wxZipInputStream zip(in);
		
		it = cat.find(wxZipEntry::GetInternalName(iconName + wxT(".png")));
		if ( it != cat.end() ) {
			zip.OpenEntry(*it->second);
			if ( !new_image.LoadFile(zip,wxBITMAP_TYPE_PNG) ) {
				AddLogLineM(false,
					wxT("Warning: Error loading icon for ") +
						iconName);
				useSkins = false;
			}
		}else {
				AddLogLineM(false,
					wxT("Warning: Can't load icon for ") +
						iconName);
				useSkins = false;
		}
		
	}
	
	wxBitmap bmp(useSkins ? new_image : stdIcon);
	if (iconName.StartsWith(wxT("Client_"))) {
		m_imagelist.Add(bmp);
	} else if (iconName.StartsWith(wxT("Toolbar_"))) {
		m_tblist.Add(bmp);
	}
}


void CamuleDlg::Apply_Clients_Skin()
{
	bool useSkins = thePrefs::UseSkins() && Check_and_Init_Skin();
	
	// Clear the client image list
	m_imagelist.RemoveAll();
	
	// Add the images to the image list
	for (int i = 0; i < CLIENT_SKIN_SIZE; ++i) {
		Add_Skin_Icon(wxT("Client_") + m_clientSkinNames[i],
			clientImages(i), useSkins);
	}
}


void CamuleDlg::Apply_Toolbar_Skin(wxToolBar *wndToolbar)
{
	bool useSkins = thePrefs::UseSkins() && Check_and_Init_Skin();
	
	// Clear the toolbar image list
	m_tblist.RemoveAll();
	
	// Add the images to the image list
	Add_Skin_Icon(wxT("Toolbar_Connect"),    connButImg(0),      useSkins);
	Add_Skin_Icon(wxT("Toolbar_Disconnect"), connButImg(1),      useSkins);
	Add_Skin_Icon(wxT("Toolbar_Connecting"), connButImg(2),      useSkins);
	Add_Skin_Icon(wxT("Toolbar_Network"),    amuleDlgImages(20), useSkins);
	Add_Skin_Icon(wxT("Toolbar_Transfers"),  amuleDlgImages(21), useSkins);
	Add_Skin_Icon(wxT("Toolbar_Search"),     amuleDlgImages(22), useSkins);
	Add_Skin_Icon(wxT("Toolbar_Shared"),     amuleDlgImages(23), useSkins);
	Add_Skin_Icon(wxT("Toolbar_Messages"),   amuleDlgImages(24), useSkins);
	Add_Skin_Icon(wxT("Toolbar_Stats"),      amuleDlgImages(25), useSkins);
	Add_Skin_Icon(wxT("Toolbar_Prefs"),      amuleDlgImages(26), useSkins);
	Add_Skin_Icon(wxT("Toolbar_Import"),     amuleDlgImages(32), useSkins);
	Add_Skin_Icon(wxT("Toolbar_About"),      amuleDlgImages(29), useSkins);
	Add_Skin_Icon(wxT("Toolbar_Blink"),	     amuleDlgImages(33), useSkins);
	
	// Build aMule toolbar
	wndToolbar->SetMargins(0, 0);
		
	// Placeholder. Gets updated by ShowConnectionState
	wndToolbar->AddTool(ID_BUTTONCONNECT, wxT("..."), m_tblist.GetBitmap(0));

	wndToolbar->AddSeparator();
	wndToolbar->AddTool(ID_BUTTONNETWORKS,
		_("Networks"), m_tblist.GetBitmap(3),
		wxNullBitmap, wxITEM_CHECK,
		_("Networks Window"));
	wndToolbar->ToggleTool(ID_BUTTONNETWORKS, true);
	wndToolbar->AddTool(ID_BUTTONSEARCH,
		_("Searches"), m_tblist.GetBitmap(5),
		wxNullBitmap, wxITEM_CHECK,
		_("Searches Window"));
	wndToolbar->AddTool(ID_BUTTONTRANSFER,
		_("Transfers"), m_tblist.GetBitmap(4),
		wxNullBitmap, wxITEM_CHECK,
		_("Files Transfers Window"));
	wndToolbar->AddTool(ID_BUTTONSHARED,
		_("Shared Files"), m_tblist.GetBitmap(6),
		wxNullBitmap, wxITEM_CHECK,
		_("Shared Files Window"));
	wndToolbar->AddTool(ID_BUTTONMESSAGES,
		_("Messages"), m_tblist.GetBitmap(7),
		wxNullBitmap, wxITEM_CHECK,
		_("Messages Window"));
	wndToolbar->AddTool(ID_BUTTONSTATISTICS,
		_("Statistics"), m_tblist.GetBitmap(8),
		wxNullBitmap, wxITEM_CHECK,
		_("Statistics Graph Window"));
	wndToolbar->AddSeparator();
	wndToolbar->AddTool(ID_BUTTONNEWPREFERENCES,
		_("Preferences"), m_tblist.GetBitmap(9),
		wxNullBitmap, wxITEM_NORMAL,
		_("Preferences Settings Window"));
	wndToolbar->AddTool(ID_BUTTONIMPORT,
		_("Import"), m_tblist.GetBitmap(10),
		wxNullBitmap, wxITEM_NORMAL,
		_("The partfile importer tool"));
	wndToolbar->AddTool(ID_ABOUT,
		_("About"), m_tblist.GetBitmap(11),
		wxNullBitmap, wxITEM_NORMAL,
		_("About/Help"));
	
	// Needed for non-GTK platforms, where the
	// items don't get added immediatly.
	wndToolbar->Realize();
	
	// Updates the "Connect" button, and so on.
	ShowConnectionState();
}


void CamuleDlg::Create_Toolbar(bool orientation)
{
	Freeze();
	// Create ToolBar from the one designed by wxDesigner (BigBob)
	wxToolBar *current = GetToolBar();

	wxASSERT(current == m_wndToolbar);

	if (current) {
		bool oldorientation = (current->GetWindowStyle() & wxTB_VERTICAL);
		if (oldorientation != orientation) {
			current->Destroy();
			SetToolBar(NULL); // Remove old one if present
			m_wndToolbar = NULL;
		} else {
			current->ClearTools();
		}
	}

	if (!m_wndToolbar) {
		m_wndToolbar = CreateToolBar(
			(orientation ? wxTB_VERTICAL : wxTB_HORIZONTAL) |
			wxNO_BORDER | wxTB_TEXT | wxTB_3DBUTTONS |
			wxTB_FLAT | wxCLIP_CHILDREN | wxTB_NODIVIDER);


			m_wndToolbar->SetToolBitmapSize(wxSize(32, 32));
	}

	Apply_Toolbar_Skin(m_wndToolbar);		
	#ifdef CLIENT_GUI
		m_wndToolbar->DeleteTool(ID_BUTTONIMPORT);
	#endif

	Thaw();
}


void CamuleDlg::OnMainGUISizeChange(wxSizeEvent& evt)
{
	wxFrame::OnSize(evt);	
	if (m_transferwnd && m_transferwnd->clientlistctrl) {
		// Transfer window's splitter set again if it's hidden.
		if (m_transferwnd->clientlistctrl->GetListView() == vtNone) {
			int height = m_transferwnd->clientlistctrl->GetSize().GetHeight();
			wxSplitterWindow* splitter =
				CastChild(wxT("splitterWnd"), wxSplitterWindow);
			height += splitter->GetWindow1()->GetSize().GetHeight();
			splitter->SetSashPosition( height );
		}
	}
	
}


void CamuleDlg::OnKeyPressed(wxKeyEvent& event)
{
	if (event.GetKeyCode() == WXK_F1) {
		// Ctrl/Alt/Shift must not be pressed, to avoid
		// conflicts with other (global) shortcuts.
		if (!event.HasModifiers() && !event.ShiftDown()) {
			LaunchUrl(wxT("http://wiki.amule.org"));
			return;
		}
	}
	
	event.Skip();
}


void CamuleDlg::OnExit(wxCommandEvent& WXUNUSED(evt))
{
	Close();
}

// File_checked_for_headers
