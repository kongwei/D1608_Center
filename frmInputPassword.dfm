object InputPassword: TInputPassword
  Left = 438
  Top = 255
  Width = 426
  Height = 225
  ActiveControl = Edit1
  Caption = #36755#20837#23494#30721
  Color = clBtnFace
  Font.Charset = GB2312_CHARSET
  Font.Color = clWindowText
  Font.Height = -16
  Font.Name = #24494#36719#38597#40657
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 21
  object Label1: TLabel
    Left = 88
    Top = 60
    Width = 64
    Height = 21
    Caption = #36755#20837#23494#30721
  end
  object Edit1: TEdit
    Left = 184
    Top = 56
    Width = 121
    Height = 29
    MaxLength = 12
    PasswordChar = '*'
    TabOrder = 0
    Text = 'Edit1'
  end
  object Button1: TButton
    Left = 96
    Top = 136
    Width = 75
    Height = 25
    Caption = #30830#23450
    Default = True
    ModalResult = 1
    TabOrder = 1
  end
  object Button2: TButton
    Left = 224
    Top = 136
    Width = 75
    Height = 25
    Cancel = True
    Caption = #21462#28040
    ModalResult = 2
    TabOrder = 2
  end
end
