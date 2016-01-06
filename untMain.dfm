object Form1: TForm1
  Left = 245
  Top = 0
  Width = 1286
  Height = 739
  Caption = 'D1608'
  Color = clBtnFace
  Font.Charset = GB2312_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = #23435#20307
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  OnMouseWheel = FormMouseWheel
  PixelsPerInch = 96
  TextHeight = 13
  object PageControl1: TPageControl
    Left = 0
    Top = 0
    Width = 1270
    Height = 701
    ActivePage = tsOperator
    Align = alClient
    TabIndex = 1
    TabOrder = 0
    object tsSearch: TTabSheet
      Caption = #26597#35810#35774#22791
      DesignSize = (
        1262
        673)
      object Bevel1: TBevel
        Left = 8
        Top = 527
        Width = 1134
        Height = 10
        Anchors = [akLeft, akRight, akBottom]
        Shape = bsBottomLine
      end
      object Label1: TLabel
        Left = 1153
        Top = 52
        Width = 53
        Height = 13
        Anchors = [akTop, akRight]
        Caption = #38388#38548'('#31186')'
      end
      object mmLog: TMemo
        Left = 8
        Top = 546
        Width = 1134
        Height = 126
        Anchors = [akLeft, akRight, akBottom]
        ReadOnly = True
        TabOrder = 0
      end
      object btnRefresh: TButton
        Left = 1152
        Top = 5
        Width = 106
        Height = 25
        Anchors = [akTop, akRight]
        Caption = #31435#21363#21047#26032
        TabOrder = 1
        OnClick = btnRefreshClick
      end
      object lvDevice: TListView
        Left = 8
        Top = 0
        Width = 1134
        Height = 523
        Anchors = [akLeft, akTop, akRight, akBottom]
        Columns = <
          item
            Width = 30
          end
          item
            Caption = 'IP'
            Width = 80
          end
          item
            Caption = #26412#22320#24191#25773'IP'
            Width = 80
          end
          item
            Caption = #35774#22791#31867#22411
            Width = 150
          end
          item
            Caption = #24207#21015#21495
            Width = 100
          end
          item
            Caption = #29256#26412
            Width = 80
          end
          item
            Caption = #26085#26399
            Width = 60
          end>
        GridLines = True
        HideSelection = False
        ReadOnly = True
        RowSelect = True
        TabOrder = 2
        ViewStyle = vsReport
        OnDblClick = lvDeviceDblClick
        OnSelectItem = lvDeviceSelectItem
      end
      object lbIplist: TListBox
        Left = 16
        Top = 168
        Width = 105
        Height = 17
        ItemHeight = 13
        TabOrder = 3
        Visible = False
      end
      object cbAutoRefresh: TCheckBox
        Left = 1153
        Top = 32
        Width = 97
        Height = 17
        Anchors = [akTop, akRight]
        Caption = #33258#21160#21047#26032
        Checked = True
        State = cbChecked
        TabOrder = 4
      end
      object spInterval: TCSpinEdit
        Left = 1209
        Top = 48
        Width = 49
        Height = 22
        Anchors = [akTop, akRight]
        MaxValue = 20
        MinValue = 1
        TabOrder = 5
        Value = 1
      end
      object btnSelect: TButton
        Left = 1152
        Top = 77
        Width = 106
        Height = 25
        Anchors = [akTop, akRight]
        Caption = #36873#25321#35774#22791
        TabOrder = 6
        OnClick = btnSelectClick
      end
    end
    object tsOperator: TTabSheet
      Caption = #25805#20316
      ImageIndex = 1
      object Panel1: TPanel
        Tag = 1
        Left = 0
        Top = 160
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 24
        object SpeedButton8: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton9: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object SpeedButton10: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = 'AUTO'
          OnClick = SpeedButton3Click
        end
        object SpeedButton12: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton13: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton14: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label3: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP1'
        end
        object SpeedButton11: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'Default'
          OnClick = SpeedButton4Click
        end
        object SpeedButton77: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar2: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object ProgressBar1: TProgressBar
        Left = 16
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 0
      end
      object ProgressBar2: TProgressBar
        Left = 64
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 1
      end
      object ProgressBar3: TProgressBar
        Left = 112
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 2
      end
      object ProgressBar4: TProgressBar
        Left = 160
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 3
      end
      object ProgressBar5: TProgressBar
        Left = 208
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 4
      end
      object ProgressBar6: TProgressBar
        Left = 256
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 5
      end
      object ProgressBar7: TProgressBar
        Left = 304
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 6
      end
      object ProgressBar8: TProgressBar
        Left = 352
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 7
      end
      object ProgressBar9: TProgressBar
        Left = 400
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 8
      end
      object ProgressBar10: TProgressBar
        Left = 448
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 9
      end
      object ProgressBar11: TProgressBar
        Left = 496
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 10
      end
      object ProgressBar12: TProgressBar
        Left = 544
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 11
      end
      object ProgressBar13: TProgressBar
        Left = 592
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 12
      end
      object ProgressBar14: TProgressBar
        Left = 640
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 13
      end
      object ProgressBar15: TProgressBar
        Left = 688
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 14
      end
      object ProgressBar16: TProgressBar
        Left = 736
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 15
      end
      object ProgressBar17: TProgressBar
        Left = 888
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 16
      end
      object ProgressBar18: TProgressBar
        Left = 936
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 17
      end
      object ProgressBar19: TProgressBar
        Left = 984
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 18
      end
      object ProgressBar20: TProgressBar
        Left = 1032
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 19
      end
      object ProgressBar21: TProgressBar
        Left = 1080
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 20
      end
      object ProgressBar22: TProgressBar
        Left = 1128
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 21
      end
      object ProgressBar23: TProgressBar
        Left = 1176
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 22
      end
      object ProgressBar24: TProgressBar
        Left = 1224
        Top = 48
        Width = 17
        Height = 105
        Min = 0
        Max = 68
        Orientation = pbVertical
        Smooth = True
        TabOrder = 23
      end
      object Panel2: TPanel
        Tag = 2
        Left = 48
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 25
        object SpeedButton1: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton2: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object SpeedButton3: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = 'AUTO'
          OnClick = SpeedButton3Click
        end
        object SpeedButton4: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton5: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton6: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label2: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP2'
        end
        object SpeedButton7: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'Default'
          OnClick = SpeedButton4Click
        end
        object SpeedButton91: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar1: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel3: TPanel
        Tag = 3
        Left = 96
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 26
        object SpeedButton15: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton16: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object SpeedButton17: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = 'AUTO'
          OnClick = SpeedButton3Click
        end
        object SpeedButton18: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton19: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton20: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label4: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP3'
        end
        object SpeedButton21: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'Default'
          OnClick = SpeedButton4Click
        end
        object SpeedButton94: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar3: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel4: TPanel
        Tag = 4
        Left = 144
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 27
        object SpeedButton22: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton23: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object SpeedButton24: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = 'AUTO'
          OnClick = SpeedButton3Click
        end
        object SpeedButton25: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton26: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton27: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label5: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP4'
        end
        object SpeedButton28: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'Default'
          OnClick = SpeedButton4Click
        end
        object SpeedButton101: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar4: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel5: TPanel
        Tag = 5
        Left = 192
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 28
        object SpeedButton29: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton30: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object SpeedButton31: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = 'AUTO'
          OnClick = SpeedButton3Click
        end
        object SpeedButton32: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton33: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton34: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label6: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP5'
        end
        object SpeedButton35: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'Default'
          OnClick = SpeedButton4Click
        end
        object SpeedButton112: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar5: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel6: TPanel
        Tag = 6
        Left = 240
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 29
        object SpeedButton36: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton37: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object SpeedButton38: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = 'AUTO'
          OnClick = SpeedButton3Click
        end
        object SpeedButton39: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton40: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton41: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label7: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP6'
        end
        object SpeedButton42: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'Default'
          OnClick = SpeedButton4Click
        end
        object SpeedButton117: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar6: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object cbWatch: TCheckBox
        Left = 288
        Top = 24
        Width = 97
        Height = 17
        Caption = 'Watch'
        TabOrder = 30
      end
      object Panel7: TPanel
        Tag = 7
        Left = 288
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 31
        object SpeedButton43: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton44: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object SpeedButton45: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = 'AUTO'
          OnClick = SpeedButton3Click
        end
        object SpeedButton46: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton47: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton48: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label8: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP7'
        end
        object SpeedButton49: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'Default'
          OnClick = SpeedButton4Click
        end
        object SpeedButton124: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar7: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel8: TPanel
        Tag = 8
        Left = 336
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 32
        object SpeedButton50: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton51: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object SpeedButton52: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = 'AUTO'
          OnClick = SpeedButton3Click
        end
        object SpeedButton53: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton54: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton55: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label9: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP8'
        end
        object SpeedButton56: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'Default'
          OnClick = SpeedButton4Click
        end
        object SpeedButton131: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar8: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel9: TPanel
        Tag = 9
        Left = 384
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 33
        object SpeedButton57: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton58: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object SpeedButton59: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = 'AUTO'
          OnClick = SpeedButton3Click
        end
        object SpeedButton60: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton61: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton62: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label10: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP9'
        end
        object SpeedButton63: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'Default'
          OnClick = SpeedButton4Click
        end
        object SpeedButton133: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar9: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel10: TPanel
        Tag = 10
        Left = 432
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 34
        object SpeedButton64: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton65: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object SpeedButton66: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = 'AUTO'
          OnClick = SpeedButton3Click
        end
        object SpeedButton67: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton68: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton69: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label11: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP10'
        end
        object SpeedButton70: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'Default'
          OnClick = SpeedButton4Click
        end
        object SpeedButton138: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar10: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel11: TPanel
        Tag = 11
        Left = 480
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 35
        object SpeedButton71: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton72: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'AGC'
          OnClick = SpeedButton2Click
        end
        object SpeedButton74: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton75: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton76: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label12: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP11'
        end
        object SpeedButton140: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar11: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel12: TPanel
        Tag = 12
        Left = 528
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 36
        object SpeedButton78: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton79: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'AGC'
          OnClick = SpeedButton2Click
        end
        object SpeedButton81: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton82: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton83: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label13: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP12'
        end
        object SpeedButton145: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar12: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel13: TPanel
        Tag = 13
        Left = 576
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 37
        object SpeedButton85: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton86: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'AGC'
          OnClick = SpeedButton2Click
        end
        object SpeedButton88: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton89: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton90: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label14: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP13'
        end
        object SpeedButton147: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar13: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel14: TPanel
        Tag = 14
        Left = 624
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 38
        object SpeedButton92: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton93: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'AGC'
          OnClick = SpeedButton2Click
        end
        object SpeedButton95: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton96: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton97: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label15: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP14'
        end
        object SpeedButton152: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar14: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel15: TPanel
        Tag = 15
        Left = 672
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 39
        object SpeedButton99: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton100: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'AGC'
          OnClick = SpeedButton2Click
        end
        object SpeedButton102: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton103: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton104: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label16: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP15'
        end
        object SpeedButton154: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar15: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel16: TPanel
        Tag = 16
        Left = 720
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 40
        object SpeedButton106: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton1Click
        end
        object SpeedButton107: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'AGC'
          OnClick = SpeedButton2Click
        end
        object SpeedButton109: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton5Click
        end
        object SpeedButton110: TSpeedButton
          Tag = 1
          Left = 5
          Top = 169
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 6
          Caption = 'Noise'
          OnClick = SpeedButton6Click
        end
        object SpeedButton111: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton7Click
        end
        object Label17: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP16'
        end
        object SpeedButton159: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar16: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
      end
      object Panel17: TPanel
        Tag = 1
        Left = 872
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 41
        object SpeedButton113: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton113Click
        end
        object SpeedButton114: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'Limit'
          OnClick = SpeedButton114Click
        end
        object SpeedButton115: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = '1'
          OnClick = SpeedButton115Click
        end
        object SpeedButton116: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton116Click
        end
        object SpeedButton118: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton118Click
        end
        object Label18: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP1'
        end
        object SpeedButton119: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'DO1'
          Visible = False
          OnClick = SpeedButton119Click
        end
        object SpeedButton168: TSpeedButton
          Tag = 2
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar17: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar17Change
        end
      end
      object Panel18: TPanel
        Tag = 2
        Left = 920
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 42
        object SpeedButton120: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton113Click
        end
        object SpeedButton121: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'Limit'
          OnClick = SpeedButton114Click
        end
        object SpeedButton122: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = '2'
          OnClick = SpeedButton115Click
        end
        object SpeedButton123: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton116Click
        end
        object SpeedButton125: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton118Click
        end
        object Label19: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP2'
        end
        object SpeedButton126: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'DO2'
          Visible = False
          OnClick = SpeedButton126Click
        end
        object SpeedButton169: TSpeedButton
          Tag = 2
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar18: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar17Change
        end
      end
      object Panel19: TPanel
        Tag = 3
        Left = 968
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 43
        object SpeedButton127: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton113Click
        end
        object SpeedButton128: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'Limit'
          OnClick = SpeedButton114Click
        end
        object SpeedButton129: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = '3'
          OnClick = SpeedButton115Click
        end
        object SpeedButton130: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton116Click
        end
        object SpeedButton132: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton118Click
        end
        object Label20: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP3'
        end
        object SpeedButton170: TSpeedButton
          Tag = 2
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar19: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar17Change
        end
      end
      object Panel20: TPanel
        Tag = 4
        Left = 1016
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 44
        object SpeedButton134: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton113Click
        end
        object SpeedButton135: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'Limit'
          OnClick = SpeedButton114Click
        end
        object SpeedButton136: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = '4'
          OnClick = SpeedButton115Click
        end
        object SpeedButton137: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton116Click
        end
        object SpeedButton139: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton118Click
        end
        object Label21: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP4'
        end
        object SpeedButton171: TSpeedButton
          Tag = 2
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar20: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar17Change
        end
      end
      object Panel21: TPanel
        Tag = 5
        Left = 1064
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 45
        object SpeedButton141: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton113Click
        end
        object SpeedButton142: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'Limit'
          OnClick = SpeedButton114Click
        end
        object SpeedButton143: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = '5'
          OnClick = SpeedButton115Click
        end
        object SpeedButton144: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton116Click
        end
        object SpeedButton146: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton118Click
        end
        object Label22: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP5'
        end
        object SpeedButton172: TSpeedButton
          Tag = 2
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar21: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar17Change
        end
      end
      object Panel22: TPanel
        Tag = 6
        Left = 1112
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 46
        object SpeedButton148: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton113Click
        end
        object SpeedButton149: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'Limit'
          OnClick = SpeedButton114Click
        end
        object SpeedButton150: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = '6'
          OnClick = SpeedButton115Click
        end
        object SpeedButton151: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton116Click
        end
        object SpeedButton153: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton118Click
        end
        object Label23: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP6'
        end
        object SpeedButton173: TSpeedButton
          Tag = 2
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar22: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar17Change
        end
      end
      object Panel23: TPanel
        Tag = 7
        Left = 1160
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 47
        object SpeedButton155: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton113Click
        end
        object SpeedButton156: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'Limit'
          OnClick = SpeedButton114Click
        end
        object SpeedButton157: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = '7'
          OnClick = SpeedButton115Click
        end
        object SpeedButton158: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton116Click
        end
        object SpeedButton160: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton118Click
        end
        object Label24: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP7'
        end
        object SpeedButton174: TSpeedButton
          Tag = 2
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar23: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar17Change
        end
      end
      object Panel24: TPanel
        Tag = 8
        Left = 1208
        Top = 161
        Width = 51
        Height = 500
        BevelOuter = bvNone
        TabOrder = 48
        object SpeedButton162: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'EQ'
          OnClick = SpeedButton113Click
        end
        object SpeedButton163: TSpeedButton
          Tag = 1
          Left = 5
          Top = 73
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 2
          Caption = 'Limit'
          OnClick = SpeedButton114Click
        end
        object SpeedButton164: TSpeedButton
          Tag = 1
          Left = 5
          Top = 97
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 3
          Caption = '8'
          OnClick = SpeedButton115Click
        end
        object SpeedButton165: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton116Click
        end
        object SpeedButton167: TSpeedButton
          Tag = 1
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = SpeedButton118Click
        end
        object Label25: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'DSP8'
        end
        object SpeedButton175: TSpeedButton
          Tag = 2
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 8
          Caption = 'DSP'
          OnClick = SpeedButton77Click
        end
        object TrackBar24: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar17Change
        end
      end
      object Panel25: TPanel
        Tag = 17
        Left = 768
        Top = 161
        Width = 97
        Height = 500
        BevelOuter = bvNone
        TabOrder = 49
        object SpeedButton73: TSpeedButton
          Tag = 1
          Left = 5
          Top = 49
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 1
          Caption = 'F.SHIFT'
          OnClick = SpeedButton73Click
        end
        object SpeedButton80: TSpeedButton
          Tag = 1
          Left = 5
          Top = 145
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 5
          Caption = 'Invert'
          OnClick = SpeedButton80Click
        end
        object btnMixMute: TSpeedButton
          Tag = 3
          Left = 5
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = btnMixMuteClick
        end
        object Label26: TLabel
          Left = 1
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'MIX'
        end
        object btnMAXON: TSpeedButton
          Tag = 1
          Left = 5
          Top = 25
          Width = 41
          Height = 17
          GroupIndex = 8
          Down = True
          Caption = 'MAX ON'
          OnClick = btnMAXONClick
        end
        object SpeedButton108: TSpeedButton
          Tag = 1
          Left = 5
          Top = 121
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 4
          Caption = 'LAST ON'
          OnClick = SpeedButton108Click
        end
        object btnPRIORITY: TSpeedButton
          Left = 53
          Top = 25
          Width = 41
          Height = 17
          GroupIndex = 8
          Caption = 'PRIORITY'
          OnClick = btnMAXONClick
        end
        object Label27: TLabel
          Left = 49
          Top = 1
          Width = 49
          Height = 13
          Alignment = taCenter
          AutoSize = False
          Caption = 'MASTER'
        end
        object btnMasterMute: TSpeedButton
          Tag = 1
          Left = 53
          Top = 193
          Width = 41
          Height = 17
          AllowAllUp = True
          GroupIndex = 7
          Caption = 'Mute'
          OnClick = btnMasterMuteClick
        end
        object TrackBar25: TTrackBar
          Tag = 1
          Left = 9
          Top = 216
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar1Change
        end
        object TrackBar26: TTrackBar
          Tag = 1
          Left = 57
          Top = 219
          Width = 30
          Height = 137
          LineSize = 24
          Max = 168
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 1
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar26Change
        end
      end
      object pnlDspDetail: TPanel
        Left = 8
        Top = 208
        Width = 769
        Height = 457
        TabOrder = 50
        Visible = False
        DesignSize = (
          769
          457)
        object PaintBox1: TPaintBox
          Left = 96
          Top = 16
          Width = 657
          Height = 321
          Anchors = [akLeft, akTop, akRight, akBottom]
        end
        object lblDSPInfo: TLabel
          Left = 1
          Top = 1
          Width = 767
          Height = 13
          Align = alTop
          Caption = 'lblDSPInfo'
        end
        object SpeedButton84: TSpeedButton
          Tag = 1
          Left = 5
          Top = 393
          Width = 41
          Height = 17
          AllowAllUp = True
          Anchors = [akLeft, akBottom]
          GroupIndex = 1
          Caption = 'PHANTOM'
          OnClick = SpeedButton84Click
        end
        object SpeedButton98: TSpeedButton
          Tag = 1
          Left = 5
          Top = 417
          Width = 41
          Height = 17
          AllowAllUp = True
          Anchors = [akLeft, akBottom]
          GroupIndex = 2
          Caption = 'COMP'
          OnClick = SpeedButton2Click
        end
        object Label28: TLabel
          Left = 72
          Top = 379
          Width = 28
          Height = 13
          Anchors = [akLeft, akBottom]
          Caption = 'freq'
        end
        object Label29: TLabel
          Left = 72
          Top = 403
          Width = 28
          Height = 13
          Anchors = [akLeft, akBottom]
          Caption = 'Gain'
        end
        object Label30: TLabel
          Left = 93
          Top = 427
          Width = 7
          Height = 13
          Anchors = [akLeft, akBottom]
          Caption = 'Q'
        end
        object TrackBar27: TTrackBar
          Tag = 1
          Left = 9
          Top = 56
          Width = 30
          Height = 313
          Anchors = [akLeft, akTop, akBottom]
          LineSize = 24
          Max = 72
          Orientation = trVertical
          PageSize = 24
          Frequency = 24
          Position = 0
          SelEnd = 0
          SelStart = 0
          TabOrder = 0
          ThumbLength = 10
          TickMarks = tmBoth
          TickStyle = tsAuto
          OnChange = TrackBar27Change
        end
        object panelBandL: TPanel
          Tag = 1
          Left = 121
          Top = 352
          Width = 65
          Height = 92
          Alignment = taLeftJustify
          Anchors = [akLeft, akBottom]
          BevelOuter = bvNone
          Ctl3D = True
          ParentCtl3D = False
          TabOrder = 1
          object cbTypeL: TComboBox
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            Style = csDropDownList
            DropDownCount = 20
            ItemHeight = 13
            ItemIndex = 0
            TabOrder = 3
            Text = 'Peaking'
            Visible = False
            Items.Strings = (
              'Peaking'
              'BandPass'
              'HighShelving'
              'LowShelving'
              'Notch'
              'High Butterworth 2nd'
              'High Butterworth 4nd')
          end
          object edtFreqL: TEdit
            Tag = 101
            Left = 0
            Top = 23
            Width = 65
            Height = 21
            TabOrder = 0
            Text = '1000'
          end
          object edtGainL: TEdit
            Left = 0
            Top = 47
            Width = 65
            Height = 21
            TabOrder = 2
            Text = '0'
          end
          object edtQL: TEdit
            Tag = 102
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            TabOrder = 1
            Text = '4.09'
            Visible = False
          end
          object cbBypassL: TCheckBox
            Left = 2
            Top = 0
            Width = 47
            Height = 17
            Caption = 'LOW'
            TabOrder = 4
            OnClick = SpeedButton87Click
          end
        end
        object panelBand1: TPanel
          Tag = 2
          Left = 193
          Top = 352
          Width = 65
          Height = 92
          Alignment = taLeftJustify
          Anchors = [akLeft, akBottom]
          BevelOuter = bvNone
          Ctl3D = True
          ParentCtl3D = False
          TabOrder = 2
          object cbType1: TComboBox
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            Style = csDropDownList
            DropDownCount = 20
            ItemHeight = 13
            ItemIndex = 0
            TabOrder = 3
            Text = 'Peaking'
            Visible = False
            Items.Strings = (
              'Peaking'
              'BandPass'
              'HighShelving'
              'LowShelving'
              'Notch'
              'High Butterworth 2nd'
              'High Butterworth 4nd')
          end
          object edtFreq1: TEdit
            Tag = 101
            Left = 0
            Top = 23
            Width = 65
            Height = 21
            TabOrder = 0
            Text = '1000'
          end
          object edtGain1: TEdit
            Left = 0
            Top = 47
            Width = 65
            Height = 21
            TabOrder = 2
            Text = '0'
          end
          object edtQ1: TEdit
            Tag = 102
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            TabOrder = 1
            Text = '4.09'
          end
          object cbBypass1: TCheckBox
            Left = 2
            Top = 0
            Width = 47
            Height = 17
            Caption = 'EQ1'
            TabOrder = 4
            OnClick = SpeedButton105Click
          end
        end
        object panelBand2: TPanel
          Tag = 3
          Left = 265
          Top = 352
          Width = 65
          Height = 92
          Alignment = taLeftJustify
          Anchors = [akLeft, akBottom]
          BevelOuter = bvNone
          Ctl3D = True
          ParentCtl3D = False
          TabOrder = 3
          object cbType2: TComboBox
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            Style = csDropDownList
            DropDownCount = 20
            ItemHeight = 13
            ItemIndex = 0
            TabOrder = 3
            Text = 'Peaking'
            Visible = False
            Items.Strings = (
              'Peaking'
              'BandPass'
              'HighShelving'
              'LowShelving'
              'Notch'
              'High Butterworth 2nd'
              'High Butterworth 4nd')
          end
          object edtFreq2: TEdit
            Tag = 101
            Left = 0
            Top = 23
            Width = 65
            Height = 21
            TabOrder = 0
            Text = '1000'
          end
          object edtGain2: TEdit
            Left = 0
            Top = 47
            Width = 65
            Height = 21
            TabOrder = 2
            Text = '0'
          end
          object edtQ2: TEdit
            Tag = 102
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            TabOrder = 1
            Text = '4.09'
          end
          object cbBypass2: TCheckBox
            Left = 2
            Top = 0
            Width = 47
            Height = 17
            Caption = 'EQ2'
            TabOrder = 4
            OnClick = SpeedButton87Click
          end
        end
        object panelBand3: TPanel
          Tag = 4
          Left = 337
          Top = 352
          Width = 65
          Height = 92
          Alignment = taLeftJustify
          Anchors = [akLeft, akBottom]
          BevelOuter = bvNone
          Ctl3D = True
          ParentCtl3D = False
          TabOrder = 4
          object cbType3: TComboBox
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            Style = csDropDownList
            DropDownCount = 20
            ItemHeight = 13
            ItemIndex = 0
            TabOrder = 3
            Text = 'Peaking'
            Visible = False
            Items.Strings = (
              'Peaking'
              'BandPass'
              'HighShelving'
              'LowShelving'
              'Notch'
              'High Butterworth 2nd'
              'High Butterworth 4nd')
          end
          object edtFreq3: TEdit
            Tag = 101
            Left = 0
            Top = 23
            Width = 65
            Height = 21
            TabOrder = 0
            Text = '1000'
          end
          object edtGain3: TEdit
            Left = 0
            Top = 47
            Width = 65
            Height = 21
            TabOrder = 2
            Text = '0'
          end
          object edtQ3: TEdit
            Tag = 102
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            TabOrder = 1
            Text = '4.09'
          end
          object cbBypass3: TCheckBox
            Left = 2
            Top = 0
            Width = 47
            Height = 17
            Caption = 'EQ3'
            TabOrder = 4
            OnClick = SpeedButton87Click
          end
        end
        object panelBand4: TPanel
          Tag = 5
          Left = 409
          Top = 352
          Width = 65
          Height = 92
          Alignment = taLeftJustify
          Anchors = [akLeft, akBottom]
          BevelOuter = bvNone
          Ctl3D = True
          ParentCtl3D = False
          TabOrder = 5
          object cbType4: TComboBox
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            Style = csDropDownList
            DropDownCount = 20
            ItemHeight = 13
            ItemIndex = 0
            TabOrder = 3
            Text = 'Peaking'
            Visible = False
            Items.Strings = (
              'Peaking'
              'BandPass'
              'HighShelving'
              'LowShelving'
              'Notch'
              'High Butterworth 2nd'
              'High Butterworth 4nd')
          end
          object edtFreq4: TEdit
            Tag = 101
            Left = 0
            Top = 23
            Width = 65
            Height = 21
            TabOrder = 0
            Text = '1000'
          end
          object edtGain4: TEdit
            Left = 0
            Top = 47
            Width = 65
            Height = 21
            TabOrder = 2
            Text = '0'
          end
          object edtQ4: TEdit
            Tag = 102
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            TabOrder = 1
            Text = '4.09'
          end
          object cbBypass4: TCheckBox
            Left = 2
            Top = 0
            Width = 47
            Height = 17
            Caption = 'EQ4'
            TabOrder = 4
            OnClick = SpeedButton87Click
          end
        end
        object panelBand5: TPanel
          Tag = 6
          Left = 481
          Top = 352
          Width = 65
          Height = 92
          Alignment = taLeftJustify
          Anchors = [akLeft, akBottom]
          BevelOuter = bvNone
          Ctl3D = True
          ParentCtl3D = False
          TabOrder = 6
          object cbType5: TComboBox
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            Style = csDropDownList
            DropDownCount = 20
            ItemHeight = 13
            ItemIndex = 0
            TabOrder = 3
            Text = 'Peaking'
            Visible = False
            Items.Strings = (
              'Peaking'
              'BandPass'
              'HighShelving'
              'LowShelving'
              'Notch'
              'High Butterworth 2nd'
              'High Butterworth 4nd')
          end
          object edtFreq5: TEdit
            Tag = 101
            Left = 0
            Top = 23
            Width = 65
            Height = 21
            TabOrder = 0
            Text = '1000'
          end
          object edtGain5: TEdit
            Left = 0
            Top = 47
            Width = 65
            Height = 21
            TabOrder = 2
            Text = '0'
          end
          object edtQ5: TEdit
            Tag = 102
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            TabOrder = 1
            Text = '4.09'
          end
          object cbBypass5: TCheckBox
            Left = 2
            Top = 0
            Width = 47
            Height = 17
            Caption = 'EQ5'
            TabOrder = 4
            OnClick = SpeedButton87Click
          end
        end
        object panelBandH: TPanel
          Tag = 7
          Left = 553
          Top = 352
          Width = 65
          Height = 92
          Alignment = taLeftJustify
          Anchors = [akLeft, akBottom]
          BevelOuter = bvNone
          Ctl3D = True
          ParentCtl3D = False
          TabOrder = 7
          object cbTypeH: TComboBox
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            Style = csDropDownList
            DropDownCount = 20
            ItemHeight = 13
            ItemIndex = 0
            TabOrder = 3
            Text = 'Peaking'
            Visible = False
            Items.Strings = (
              'Peaking'
              'BandPass'
              'HighShelving'
              'LowShelving'
              'Notch'
              'High Butterworth 2nd'
              'High Butterworth 4nd')
          end
          object edtFreqH: TEdit
            Tag = 101
            Left = 0
            Top = 23
            Width = 65
            Height = 21
            TabOrder = 0
            Text = '1000'
          end
          object edtGainH: TEdit
            Left = 0
            Top = 47
            Width = 65
            Height = 21
            TabOrder = 2
            Text = '0'
          end
          object edtQH: TEdit
            Tag = 102
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            TabOrder = 1
            Text = '4.09'
            Visible = False
          end
          object cbBypassH: TCheckBox
            Left = 2
            Top = 0
            Width = 47
            Height = 17
            Caption = 'HIGH'
            TabOrder = 4
            OnClick = SpeedButton178Click
          end
        end
        object panelBand0: TPanel
          Left = 665
          Top = 352
          Width = 65
          Height = 92
          Alignment = taLeftJustify
          Anchors = [akLeft, akBottom]
          BevelOuter = bvNone
          Ctl3D = True
          ParentCtl3D = False
          TabOrder = 8
          object cbType0: TComboBox
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            Style = csDropDownList
            DropDownCount = 20
            ItemHeight = 13
            ItemIndex = 0
            TabOrder = 3
            Text = 'Peaking'
            Visible = False
            Items.Strings = (
              'Peaking'
              'BandPass'
              'HighShelving'
              'LowShelving'
              'Notch'
              'High Butterworth 2nd'
              'High Butterworth 4nd')
          end
          object edtFreq0: TEdit
            Tag = 101
            Left = 0
            Top = 23
            Width = 65
            Height = 21
            TabOrder = 0
            Text = '1000'
          end
          object edtGain0: TEdit
            Left = 0
            Top = 47
            Width = 65
            Height = 21
            TabOrder = 2
            Text = '0'
          end
          object edtQ0: TEdit
            Tag = 102
            Left = 0
            Top = 71
            Width = 65
            Height = 21
            TabOrder = 1
            Text = '4.09'
            Visible = False
          end
          object cbBypass0: TCheckBox
            Left = 2
            Top = 0
            Width = 47
            Height = 17
            Caption = 'HIDE'
            TabOrder = 4
            OnClick = SpeedButton87Click
          end
        end
      end
      object edtDebug: TEdit
        Left = 16
        Top = 0
        Width = 1041
        Height = 21
        ReadOnly = True
        TabOrder = 51
      end
      object edtPB1: TEdit
        Left = 16
        Top = 24
        Width = 121
        Height = 21
        TabOrder = 52
        Text = 'edtPB1'
      end
      object edtPB17: TEdit
        Left = 888
        Top = 24
        Width = 121
        Height = 21
        TabOrder = 53
        Text = 'edtPB1'
      end
    end
  end
  object udpSLP: TIdUDPServer
    BufferSize = 65535
    BroadcastEnabled = True
    Bindings = <>
    DefaultPort = 0
    OnUDPRead = udpSLPUDPRead
    Left = 448
    Top = 40
  end
  object tmSLP: TTimer
    Interval = 500
    OnTimer = tmSLPTimer
    Left = 480
    Top = 40
  end
  object udpControl: TIdUDPServer
    Active = True
    Bindings = <
      item
        IP = '127.0.0.1'
        Port = 9999
      end>
    DefaultPort = 0
    OnUDPRead = udpControlUDPRead
    Left = 512
    Top = 40
  end
  object tmWatch: TTimer
    OnTimer = tmWatchTimer
    Left = 416
    Top = 40
  end
end
