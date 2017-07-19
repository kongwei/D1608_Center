//---------------------------------------------------------------------------

#ifndef untSetMACH
#define untSetMACH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TfrmSetMAC : public TForm
{
__published:	// IDE-managed Components
    TButton *Button1;
    TButton *Button2;
    TEdit *edtVar;
    TLabel *Label1;
    TEdit *edtFix;
private:	// User declarations
public:		// User declarations
    __fastcall TfrmSetMAC(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmSetMAC *frmSetMAC;
//---------------------------------------------------------------------------
#endif
