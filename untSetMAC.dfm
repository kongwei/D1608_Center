object frmSetMAC: TfrmSetMAC
  Left = 284
  Top = 281
  BorderStyle = bsDialog
  Caption = #20462#25913'MAC'#22320#22336
  ClientHeight = 276
  ClientWidth = 605
  Color = clBtnFace
  Font.Charset = GB2312_CHARSET
  Font.Color = clWindowText
  Font.Height = -24
  Font.Name = #24494#36719#38597#40657
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  Scaled = False
  PixelsPerInch = 144
  TextHeight = 31
  object Label1: TLabel
    Left = 130
    Top = 89
    Width = 56
    Height = 31
    Caption = 'MAC'
  end
  object Label2: TLabel
    Left = 154
    Top = 142
    Width = 120
    Height = 31
    Caption = #37325#21551#21518#29983#25928
  end
  object Button1: TButton
    Left = 142
    Top = 201
    Width = 110
    Height = 37
    Caption = #30830#23450
    ModalResult = 1
    TabOrder = 0
  end
  object Button2: TButton
    Left = 331
    Top = 201
    Width = 110
    Height = 37
    Cancel = True
    Caption = #21462#28040
    ModalResult = 2
    TabOrder = 1
  end
  object edtVar: TEdit
    Left = 342
    Top = 83
    Width = 120
    Height = 27
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 2
    Text = '00:00:00'
  end
  object edtFix: TEdit
    Left = 224
    Top = 83
    Width = 108
    Height = 27
    Color = clScrollBar
    Ctl3D = False
    ParentCtl3D = False
    ReadOnly = True
    TabOrder = 3
    Text = '00:00:00'
  end
end
