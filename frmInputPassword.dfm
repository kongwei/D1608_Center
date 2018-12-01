object InputPassword: TInputPassword
  Left = 438
  Top = 255
  Width = 609
  Height = 321
  ActiveControl = Edit1
  Caption = #36755#20837#23494#30721
  Color = clBtnFace
  Font.Charset = GB2312_CHARSET
  Font.Color = clWindowText
  Font.Height = -24
  Font.Name = #24494#36719#38597#40657
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  Scaled = False
  OnShow = FormShow
  PixelsPerInch = 144
  TextHeight = 31
  object Label1: TLabel
    Left = 130
    Top = 89
    Width = 96
    Height = 31
    Caption = #36755#20837#23494#30721
  end
  object Edit1: TEdit
    Left = 272
    Top = 83
    Width = 178
    Height = 29
    MaxLength = 12
    PasswordChar = '*'
    TabOrder = 0
    Text = 'Edit1'
  end
  object Button1: TButton
    Left = 142
    Top = 201
    Width = 110
    Height = 37
    Caption = #30830#23450
    Default = True
    ModalResult = 1
    TabOrder = 1
  end
  object Button2: TButton
    Left = 331
    Top = 201
    Width = 110
    Height = 37
    Cancel = True
    Caption = #21462#28040
    ModalResult = 2
    TabOrder = 2
  end
end
