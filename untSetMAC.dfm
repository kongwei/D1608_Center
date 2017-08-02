object frmSetMAC: TfrmSetMAC
  Left = 284
  Top = 281
  BorderStyle = bsDialog
  Caption = #20462#25913'MAC'#22320#22336
  ClientHeight = 187
  ClientWidth = 410
  Color = clBtnFace
  Font.Charset = GB2312_CHARSET
  Font.Color = clWindowText
  Font.Height = -16
  Font.Name = #24494#36719#38597#40657
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  PixelsPerInch = 96
  TextHeight = 21
  object Label1: TLabel
    Left = 88
    Top = 60
    Width = 38
    Height = 21
    Caption = 'MAC'
  end
  object Label2: TLabel
    Left = 104
    Top = 96
    Width = 80
    Height = 21
    Caption = #37325#21551#21518#29983#25928
  end
  object Button1: TButton
    Left = 96
    Top = 136
    Width = 75
    Height = 25
    Caption = #30830#23450
    ModalResult = 1
    TabOrder = 0
  end
  object Button2: TButton
    Left = 224
    Top = 136
    Width = 75
    Height = 25
    Cancel = True
    Caption = #21462#28040
    ModalResult = 2
    TabOrder = 1
  end
  object edtVar: TEdit
    Left = 232
    Top = 56
    Width = 81
    Height = 27
    Ctl3D = False
    ParentCtl3D = False
    TabOrder = 2
    Text = '00:00:00'
  end
  object edtFix: TEdit
    Left = 152
    Top = 56
    Width = 73
    Height = 27
    Color = clScrollBar
    Ctl3D = False
    ParentCtl3D = False
    ReadOnly = True
    TabOrder = 3
    Text = '00:00:00'
  end
end
