//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "frmInputPassword.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TInputPassword *InputPassword;
//---------------------------------------------------------------------------
__fastcall TInputPassword::TInputPassword(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TInputPassword::FormShow(TObject *Sender)
{
    this->ActiveControl = Edit1;
}
//---------------------------------------------------------------------------

