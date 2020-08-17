//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>
#include <Registry.hpp>
#include <OleCtrls.hpp>
#include "SYNCOMLib_OCX.h"
#include <AppEvnts.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm, public _ISynDeviceEvents
{
__published:	// IDE-verwaltete Komponenten
	TButton *ok;
	TButton *cancel;
	TTrayIcon *TrayIcon1;
	TPopupMenu *PopupMenu1;
	TMenuItem *Settings1;
	TMenuItem *About1;
	TMenuItem *Exit1;
	TPageControl *pages;
	TTabSheet *scrollTab;
	TTabSheet *tapTab;
	TMenuItem *globalActive;
	TMenuItem *N1;
	TButton *defaults;
	TTabSheet *generalTab;
	TTimer *reactivateTimer;
	TGroupBox *GroupBox1;
	TRadioButton *scrollCircular;
	TCheckBox *scrollLinearEdge;
	TRadioButton *scrollLinear;
	TRadioButton *scrollOff;
	TGroupBox *GroupBox2;
	TTrackBar *scrollAcc;
	TGroupBox *GroupBox3;
	TRadioButton *scrollCompatible;
	TRadioButton *scrollSmooth;
	TLabel *Label1;
	TGroupBox *GroupBox4;
	TCheckBox *startWithWindows;
	TGroupBox *GroupBox5;
	TRadioButton *scrollSmart;
	TLabel *tapMaxDistanceLabel;
	TTrackBar *tapMaxDistance;
	TCheckBox *scrollAccEnabled;
	TComboBox *tapOneOne;
	TLabel *Label2;
	TLabel *Label3;
	TComboBox *tapThree;
	TTrackBar *scrollSpeed;
	TLabel *scrollSpeedLabel;
	TComboBox *tapTwo;
	TLabel *Label4;
	TLabel *Label5;
	TComboBox *tapTwoOne;
	TCheckBox *cbReverseScrollDirection;
	void __fastcall Settings1Click(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall scrollLinearClick(TObject *Sender);
	void __fastcall Exit1Click(TObject *Sender);
	void __fastcall okClick(TObject *Sender);
	void __fastcall cancelClick(TObject *Sender);
	void __fastcall About1Click(TObject *Sender);
	void __fastcall defaultsClick(TObject *Sender);
	void __fastcall reactivateTimerTimer(TObject *Sender);
	void __fastcall Label1Click(TObject *Sender);
	void __fastcall scrollAccEnabledClick(TObject *Sender);

protected:
    void __fastcall Dispatch(void *Message);
private:	// Benutzer Deklarationen
	TRegistry *settings;
	HRESULT STDMETHODCALLTYPE OnSynDevicePacket(long);
	TSynAPI *synAPI;
	TSynDevice *synTouchPad;
	ISynPacket *synPacket;
	bool IsPadAcquired;
	bool synTapState;
	bool IsDeviceTapLocked;
	long tapLastNof;
	long tapStartTime, tapTouchTime, scrollTouchTime;
	long tapDistance;
	POINT tapTouchPos, scrollTouchPos;
	long scrollBuffer;
	long scrollLastXDelta, scrollLastYDelta;
	bool scrollNotEdge;
	int scrollMode;
	void __fastcall SettingsLoad(bool=false);
	void __fastcall SettingsSave();
	void __fastcall AcquirePad(bool);
	void __fastcall LockDeviceTap(bool);
	bool __fastcall DoTap(int);
	bool __fastcall DoScroll(long, long);
public:		// Benutzer Deklarationen
	__fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
