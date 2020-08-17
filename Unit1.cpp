//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit1.h"
#include "SynComDefs.h"
#include <shlwapi.h>
#include <psapi.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "SYNCOMLib_OCX"
#pragma resource "*.dfm"

static const UnicodeString regKey = "Software\\TwoFingerScroll";

struct TTapInfo {
	DWORD eventDown, eventUp, x;
};

static const TTapInfo tapInfo[] = {
	{0, 0, 0}, // disabled
	{MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, 0},
	{MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, 0},
	{MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, 0},
	{MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON1},
	{MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON2}
};

TForm1 *Form1;
//---------------------------------------------------------------------------

UnicodeString __fastcall GetForegroundWindowBaseModuleName()
{
	HWND hWnd;

	hWnd = GetForegroundWindow();
	if (hWnd) {
		DWORD dwProcessId;
		HANDLE hProcess;

		GetWindowThreadProcessId(hWnd, &dwProcessId);
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
			PROCESS_VM_READ, false, dwProcessId);
		if (hProcess != NULL) {
			HMODULE hMod;
			DWORD cbNeeded;

			if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
				char szModule[MAX_PATH];

				GetModuleBaseName(hProcess, hMod, szModule, MAX_PATH);
				CloseHandle(hProcess);
				return UnicodeString(szModule);
			}

			CloseHandle(hProcess);
		}
	}

	return UnicodeString();
}
//---------------------------------------------------------------------------

int __fastcall GetScrollMode()
{
	UnicodeString name = GetForegroundWindowBaseModuleName();
	int mode = 1;

	if (name != "") {
		TRegistry *reg;

		reg = new TRegistry();
		reg->RootKey = HKEY_CURRENT_USER;
		if (reg->OpenKeyReadOnly(regKey + "\\scrollModeApps")) {
			if (reg->ValueExists(name)) {
				mode = reg->ReadInteger(name);
			}
			reg->CloseKey();
		}
		delete reg;
	}
	return mode;
}
//---------------------------------------------------------------------------

void __fastcall SetScrollMode(int mode)
{
	UnicodeString name = GetForegroundWindowBaseModuleName();

	if (name != "") {
		TRegistry *reg;

		reg = new TRegistry();
		reg->RootKey = HKEY_CURRENT_USER;
		reg->Access = KEY_ALL_ACCESS;
		if (reg->OpenKey(regKey + "\\scrollModeApps", true)) {
			if (mode != 1) {
				reg->WriteInteger(name, mode);
			}
			else {
				reg->DeleteValue(name);
            }
			reg->CloseKey();
		}
		delete reg;
	}
}
//---------------------------------------------------------------------------

__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
	synAPI = new TSynAPI(this);
	synTouchPad = new TSynDevice(this);

	synAPI->Initialize();

	long devHandle = -1;

	synAPI->FindDevice(SE_ConnectionAny,
		SE_DeviceTouchPad,
		&devHandle);

	if (devHandle < 0) {
		Application->MessageBox(L"No Synaptics TouchPad device found!",
			L"TwoFingerScroll");
		Application->Terminate();
	}

	synTouchPad->Select(devHandle);
	synTouchPad->CreatePacket(&synPacket);

	long multi;

	synTouchPad->GetProperty(SP_IsMultiFingerCapable, &multi);
	if (!multi) {
		TRegistry *reg = new TRegistry();
		reg->RootKey = HKEY_LOCAL_MACHINE;
		reg->Access = KEY_ALL_ACCESS;
		if (!reg->OpenKey("System\\CurrentControlSet\\Services\\SynTP\\Parameters", false)) {
			Application->MessageBox(L"Synaptics kernel driver registry keys missing. Reinstall drivers.",
				L"TwoFingerScroll");
			Application->Terminate();
		}
		unsigned int mask = 0;
		if (reg->ValueExists("CapabilitiesMask")) {
			mask = reg->ReadInteger("CapabilitiesMask");
			if (mask == 0xFFFFFFFF) {
				Application->MessageBox(L"Driver support for multiple fingers is already enabled but\n"
					"the driver still doesn't report multiple fingers. Either you haven't restarted\n"
					"the system yet or your TouchPad doesn't support multiple fingers.",
					L"TwoFingerScroll");
				Application->Terminate();
			}
		}
		if (mask != 0xFFFFFFFF) {
			reg->WriteInteger("CapabilitiesMask", 0xFFFFFFFF);
			reg->CloseKey();
			Application->MessageBox(L"Driver support for multiple fingers has been enabled.\n"
            	"\n"
				"Restart system for the changes to take effect.",
				L"TwoFingerScroll");
			Application->Terminate();
		}
		reg->CloseKey();
	}

	// activate the OnPacket event
	synTouchPad->SetSynchronousNotification(this);

	settings = new TRegistry();

	SettingsLoad();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::SettingsLoad(bool defaults)
{
	settings->RootKey = HKEY_CURRENT_USER;
	if (!defaults)
		settings->OpenKeyReadOnly(regKey);

	if (settings->ValueExists("scroll")) {
		switch (settings->ReadInteger("scroll")) {
		case 1:
			scrollLinear->Checked = true;
			break;
		//case 2:
		//	scrollCircular->Checked = true;
		//	break;
		default:
			scrollOff->Checked = true;
		}
	}
	else scrollLinear->Checked = true;
	scrollLinearClick(NULL);

	if (settings->ValueExists("scrollLinearEdge")) {
		scrollLinearEdge->Checked =
			settings->ReadInteger("scrollLinearEdge");
	}
	else scrollLinearEdge->Checked = true;

	if (settings->ValueExists("scrollSpeed")) {
		scrollSpeed->Position =
			settings->ReadInteger("scrollSpeed");
	}
	else scrollSpeed->Position = 100;

	if (settings->ValueExists("scrollAccEnabled")) {
		scrollAccEnabled->Checked =
			settings->ReadInteger("scrollAccEnabled");
	}
	else scrollAccEnabled->Checked = true;
	scrollAccEnabledClick(NULL);

	if (settings->ValueExists("scrollAcc")) {
		scrollAcc->Position =
			settings->ReadInteger("scrollAcc");
	}
	else scrollAcc->Position = 75;

	if (settings->ValueExists("reverseScrollDirection") &&
		settings->ReadInteger("reverseScrollDirection") != 0) {
		cbReverseScrollDirection->Checked = true;
	}
	else cbReverseScrollDirection->Checked = false;

	if (settings->ValueExists("scrollMode")) {
		switch (settings->ReadInteger("scrollMode")) {
		case 1:
			scrollSmooth->Checked = true;
			break;
		case 2:
			scrollSmart->Checked = true;
			break;
		default:
			scrollCompatible->Checked = true;
		}
	}
	else scrollCompatible->Checked = true;

	if (settings->ValueExists("tapOneOne")) {
		tapOneOne->ItemIndex =
			settings->ReadInteger("tapOneOne");
	}
	else tapOneOne->ItemIndex = 0;

	if (settings->ValueExists("tapTwo")) {
		tapTwo->ItemIndex =
			settings->ReadInteger("tapTwo");
	}
	else tapTwo->ItemIndex = 0;

	if (settings->ValueExists("tapTwoOne")) {
		tapTwoOne->ItemIndex =
			settings->ReadInteger("tapTwoOne");
	}
	else tapTwoOne->ItemIndex = 0;

	if (settings->ValueExists("tapThree")) {
		tapThree->ItemIndex =
			settings->ReadInteger("tapThree");
	}
	else tapThree->ItemIndex = 0;

	if (settings->ValueExists("tapMaxDistance")) {
		tapMaxDistance->Position =
			settings->ReadInteger("tapMaxDistance");
	}
	else tapMaxDistance->Position = 50;

	settings->CloseKey();

	TRegistry *reg = new TRegistry();
	reg->RootKey = HKEY_CURRENT_USER;
	if (!defaults)
		reg->OpenKeyReadOnly("Software\\Microsoft\\Windows\\CurrentVersion\\Run");

	if (reg->ValueExists("TwoFingerScroll")) {
		 startWithWindows->Checked = (reg->ReadString("TwoFingerScroll") ==
			Application->ExeName);
	}
	else startWithWindows->Checked = false;

	reg->CloseKey();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::SettingsSave()
{
	settings->RootKey = HKEY_CURRENT_USER;
	settings->Access = KEY_ALL_ACCESS;
	if (!settings->OpenKey(regKey, true))
		return;

	settings->WriteInteger("scroll", scrollLinear->Checked ? 1 : (scrollCircular->Checked ? 2 : 0));
	settings->WriteInteger("scrollLinearEdge", scrollLinearEdge->Checked);
	settings->WriteInteger("scrollSpeed", scrollSpeed->Position);
	settings->WriteInteger("scrollAccEnabled", scrollAccEnabled->Checked);
	settings->WriteInteger("scrollAcc", scrollAcc->Position);
	settings->WriteInteger("scrollMode", scrollSmooth->Checked ? 1 : (scrollSmart->Checked ? 2 : 0));
	settings->WriteInteger("reverseScrollDirection", cbReverseScrollDirection->Checked ? 1 : 0);
	settings->WriteInteger("tapOneOne", tapOneOne->ItemIndex);
	settings->WriteInteger("tapTwo", tapTwo->ItemIndex);
	settings->WriteInteger("tapTwoOne", tapTwoOne->ItemIndex);
	settings->WriteInteger("tapThree", tapThree->ItemIndex);
	settings->WriteInteger("tapMaxDistance", tapMaxDistance->Position);

	settings->CloseKey();

	TRegistry *reg = new TRegistry();
	reg->RootKey = HKEY_CURRENT_USER;
	reg->Access = KEY_ALL_ACCESS;
	reg->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Run", false);

	if (startWithWindows->Checked) {
		reg->WriteString("TwoFingerScroll", Application->ExeName);
	}
	else if (reg->ValueExists("TwoFingerScroll")) {
		reg->DeleteValue("TwoFingerScroll");
	}

	reg->CloseKey();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::AcquirePad(bool acquire)
{
	if (acquire && !IsPadAcquired) {
		synTouchPad->Acquire(0);
	}
	else if (!acquire && IsPadAcquired) {
		synTouchPad->Unacquire();
	}
	IsPadAcquired = acquire;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::LockDeviceTap(bool lock)
{
	long gest;

	if (lock != IsDeviceTapLocked) {
		if (lock) {
			synTouchPad->GetProperty(SP_Gestures, &gest);
			if (gest & SF_GestureTap) {
				synTouchPad->SetProperty(SP_Gestures, gest & (~SF_GestureTap));
				synTapState = true;
			}
			else synTapState = false;
		}
		else {
			synTouchPad->GetProperty(SP_Gestures, &gest);
			if (synTapState) gest |= SF_GestureTap;
			else gest &= ~SF_GestureTap;
			synTouchPad->SetProperty(SP_Gestures, gest);
		}
		IsDeviceTapLocked = lock;
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Settings1Click(TObject *Sender)
{
	pages->ActivePageIndex = 0;
	Show();
	ok->SetFocus();
	SetForegroundWindow(Handle);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
	Hide();
	SettingsLoad();
	Action = caNone;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::scrollLinearClick(TObject *Sender)
{
	bool e = scrollLinear->Checked;

	scrollLinearEdge->Enabled = e;
	scrollSpeedLabel->Enabled = e;
	scrollSpeed->Enabled = e;
	scrollAccEnabled->Enabled = e;
	scrollAcc->Enabled = e;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Exit1Click(TObject *Sender)
{
	Application->Terminate();
}
//---------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE TForm1::OnSynDevicePacket(long seqNum)
{
	long nof, fstate, xd, yd;

	// get the pointing data packet
	synTouchPad->LoadPacket(synPacket);

	if (!globalActive->Checked)
		return 0;

	// extract relevant data
	synPacket->GetProperty(SP_ExtraFingerState, &nof);
	nof &= 3;
	synPacket->GetProperty(SP_FingerState, &fstate);
	synPacket->GetProperty(SP_XDelta, &xd);
	synPacket->GetProperty(SP_YDelta, &yd);

	// handle tapping
	if (nof > tapLastNof) {
		if (nof >= 2) {
			synPacket->GetProperty(SP_TimeStamp,
				&tapStartTime);
			tapDistance = 0;
			LockDeviceTap(true);
		}
		if (tapLastNof == 0) {
			synPacket->GetProperty(SP_TimeStamp,
				&tapTouchTime);
			GetCursorPos(&tapTouchPos);
		}
	}
	else if (nof < tapLastNof) {
		if ((tapDistance < tapMaxDistance->Position) &&
			(tapLastNof >= 2))
		{
			bool ok = false;
			long tstamp;
			synPacket->GetProperty(SP_TimeStamp, &tstamp);
			if (tstamp - tapTouchTime < 175) {
				if (tapLastNof == 2)
					ok = DoTap(tapTwo->ItemIndex);
				else if (tapLastNof == 3)
					ok = DoTap(tapThree->ItemIndex);
			}
			else if (tstamp - tapStartTime < 175) {
				if (tapLastNof == 2)
					ok = DoTap(tapOneOne->ItemIndex);
				else if (tapLastNof == 3)
					ok = DoTap(tapTwoOne->ItemIndex);
			}
			if (ok)
				SetCursorPos(tapTouchPos.x, tapTouchPos.y);

			// prevent tap trigger until initiated again
			tapStartTime -= 175;
			tapLastNof = nof;
			return 0;
		}
	}
	if (IsDeviceTapLocked) {
		if (abs(xd) < 800) tapDistance += abs(xd);
		if (abs(yd) < 800) tapDistance += abs(yd);
		if (!(fstate & SF_FingerPresent))
			LockDeviceTap(false);
	}
	tapLastNof = nof;

	// handle scrolling
	if (scrollLinear->Checked) {
		if (fstate & SF_FingerPresent) {
			if (scrollTouchTime == 0) {
				GetCursorPos(&scrollTouchPos);
				synPacket->GetProperty(SP_TimeStamp,
						&scrollTouchTime);
			}
			if (nof == 2) {
				long y, ylo, yhi;
				synPacket->GetProperty(SP_Y, &y);
				synTouchPad->GetProperty(SP_YLoBorder, &ylo);
				synTouchPad->GetProperty(SP_YHiBorder, &yhi);
				if (IsPadAcquired && scrollLinearEdge->Checked) {
					if (ylo <= y && y <= yhi) {
						scrollNotEdge = true;
					}
					else if (scrollNotEdge && ((y < ylo && scrollLastYDelta < 0) ||
							(y > yhi && scrollLastYDelta > 0))) {
						DoScroll(scrollLastXDelta, scrollLastYDelta);
						return 0;
					}
				}
				if (fstate & SF_FingerMotion) {
					if (!IsPadAcquired) {
						AcquirePad(true);
						long tstamp;
						synPacket->GetProperty(SP_TimeStamp, &tstamp);
						if (tstamp - scrollTouchTime < 1000) {
							SetCursorPos(scrollTouchPos.x,
								scrollTouchPos.y);
						}
						if (scrollCompatible->Checked) {
							scrollMode = 0;
						}
						else if (scrollSmooth->Checked) {
							scrollMode = 1;
						}
						else {
							scrollMode = GetScrollMode();
							if ((GetKeyState(VK_SHIFT) & 0x8000) &&
									(GetKeyState(VK_CONTROL) & 0x8000) &&
									(GetKeyState(VK_MENU) & 0x8000)) {
								// toggle scroll mode
								if (scrollMode == 1) scrollMode = 0;
								else scrollMode = 1;
								SetScrollMode(scrollMode);
							}

						}
					}
					if (IsPadAcquired) {
						DoScroll(xd, yd);
						scrollLastXDelta = xd;
						scrollLastYDelta = yd;
					}
				}
			}
			else {
				scrollLastXDelta = scrollLastYDelta = 0;
				AcquirePad(false);
				scrollBuffer = 0;
				scrollNotEdge = false;
			}
		}
		else {
			scrollTouchTime = 0;
			scrollLastXDelta = scrollLastYDelta = 0;
			AcquirePad(false);
			scrollBuffer = 0;
			scrollNotEdge = false;
		}
	}

	return 0;
}
//---------------------------------------------------------------------------

bool __fastcall TForm1::DoTap(int index)
{
	INPUT i[2];
	const TTapInfo *info = &tapInfo[index];

	if (info->eventDown == 0) return false; // tapping disabled

	ZeroMemory(i, sizeof(INPUT)*2);
	i[0].type = INPUT_MOUSE;
	i[0].mi.dwFlags = info->eventDown;
	i[0].mi.mouseData = info->x;
	i[1].type = INPUT_MOUSE;
	i[1].mi.dwFlags = info->eventUp;
	i[1].mi.mouseData = info->x;
	SendInput(2, i, sizeof(INPUT));

	return true;
}
//---------------------------------------------------------------------------

bool __fastcall TForm1::DoScroll(long dx, long dy)
{
	long d;

	if (abs(dy) > 800)
		return false;

	// scrollSpeed
	dy = dy * scrollSpeed->Position / 100;

	// scrollAcc
	if (scrollAccEnabled->Checked) {
		d = dy * dy / (scrollAcc->Max - scrollAcc->Position + scrollAcc->Min);
		if (d < 4)
			d = 4;
		if (dy < 0)
			d = -d;
	}
	else d = dy;

	if (scrollMode == 0) {
		// compatibility mode
		scrollBuffer += d;
		d = scrollBuffer - scrollBuffer % WHEEL_DELTA;
	}

	if (d != 0) {
		INPUT i;

		ZeroMemory(&i, sizeof(INPUT));
		i.type = INPUT_MOUSE;
		i.mi.dwFlags = MOUSEEVENTF_WHEEL;
		i.mi.mouseData = cbReverseScrollDirection->Checked ? -d : d;
		SendInput(1, &i, sizeof(INPUT));

		if (scrollMode == 0) // compatibility mode
			scrollBuffer -= d;
	}

	return true;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::okClick(TObject *Sender)
{
	Hide();
	SettingsSave();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::cancelClick(TObject *Sender)
{
	Hide();
	SettingsLoad();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::About1Click(TObject *Sender)
{
	Application->MessageBox(L"TwoFingerScroll 1.0.6\n"
		"\n"
		"Copyright (c) 2008, 2009 Arkadiusz Wahlig\n"
		"<arkadiusz.wahlig@gmail.com>\n"
		"\n"
		"Published under the Apache 2.0 License.\n"
		"\n"
		"Project home:\n"
		"http://code.google.com/p/two-finger-scroll",
		L"About...");
}
//---------------------------------------------------------------------------

void __fastcall TForm1::defaultsClick(TObject *Sender)
{
	SettingsLoad(true);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Dispatch(void *Message)
{
	// the Synaptics API seems to have a problem with
	// power state changes; in particullar, the events
	// get deactivated after standby or hibernation;
	// here we ensure that they stay activated

	if (((TMessage *)Message)->Msg == WM_POWERBROADCAST) {
		reactivateTimer->Tag = 20;
		reactivateTimer->Enabled = true;
	}

	TForm::Dispatch(Message);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::reactivateTimerTimer(TObject *Sender)
{
	synTouchPad->SetSynchronousNotification(this);
	reactivateTimer->Tag--;
	if (!reactivateTimer->Tag) {
		reactivateTimer->Enabled = false;
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Label1Click(TObject *Sender)
{
	Application->MessageBox(
		L"Compatible:\n"
		"The scroll closely simulates a mouse scroll wheel. This mode works with\n"
		"most applications.\n"
		"\n"
		"Smooth:\n"
		"Smooth scrolling. Some older applications may not work propery in this\n"
		"scroll mode.\n"
		"\n"
		"Smart:\n"
		"Uses smooth scrolling by default. Compatible mode can be enabled for\n"
		"specifc applications by scrolling within them while keeping SHIFT, CTRL\n"
		"and ALT keys pressed down. The setting is remembered, all future scrolls\n"
		"in the same application will use the compatible mode. Scrolling with the\n"
		"keys again reverts back to the smooth mode.",
		L"Scroll mode");
}
//---------------------------------------------------------------------------

void __fastcall TForm1::scrollAccEnabledClick(TObject *Sender)
{
	scrollAcc->Enabled = scrollAccEnabled->Checked;
}
//---------------------------------------------------------------------------

