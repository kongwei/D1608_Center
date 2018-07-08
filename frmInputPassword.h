//---------------------------------------------------------------------------

#ifndef frmInputPasswordH
#define frmInputPasswordH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TInputPassword : public TForm
{
__published:	// IDE-managed Components
    TEdit *Edit1;
    TLabel *Label1;
    TButton *Button1;
    TButton *Button2;
    void __fastcall FormShow(TObject *Sender);
private:	// User declarations
public:		// User declarations
    __fastcall TInputPassword(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TInputPassword *InputPassword;
//---------------------------------------------------------------------------
#endif

