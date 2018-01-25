object frmEditor: TfrmEditor
  Left = 566
  Top = 254
  Width = 734
  Height = 509
  Caption = 'frmEditor'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsMDIChild
  OldCreateOrder = False
  Position = poDefault
  Visible = True
  OnActivate = FormActivate
  OnClose = FormClose
  OnCreate = FormCreate
  OnDeactivate = FormDeactivate
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 726
    Height = 456
    Align = alClient
    Caption = 'Panel1'
    TabOrder = 2
    object ScrollBox1: TScrollBox
      Left = 1
      Top = 34
      Width = 724
      Height = 421
      Align = alClient
      BevelInner = bvNone
      BevelOuter = bvNone
      TabOrder = 0
      object Image1: TImage
        Left = 0
        Top = 0
        Width = 720
        Height = 417
        Align = alClient
        Proportional = True
      end
    end
    object Panel3: TPanel
      Left = 1
      Top = 1
      Width = 724
      Height = 33
      Align = alTop
      BevelOuter = bvSpace
      BorderStyle = bsSingle
      TabOrder = 1
      DesignSize = (
        720
        29)
      object SB1: TSpeedButton
        Left = 545
        Top = 3
        Width = 81
        Height = 22
        Anchors = [akTop, akRight]
        GroupIndex = 1
        Caption = 'Real Size'
        OnClick = SB1Click
      end
      object SB2: TSpeedButton
        Left = 631
        Top = 3
        Width = 81
        Height = 22
        Anchors = [akTop, akRight]
        GroupIndex = 1
        Down = True
        Caption = 'Fit To Screen'
        OnClick = SB2Click
      end
      object BitBtn2: TBitBtn
        Left = 4
        Top = 2
        Width = 65
        Height = 25
        Caption = 'Save'
        TabOrder = 0
        OnClick = BitBtn2Click
        Glyph.Data = {
          76010000424D7601000000000000760000002800000020000000100000000100
          04000000000000010000130B0000130B00001000000000000000000000000000
          800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
          FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333330070
          7700333333337777777733333333008088003333333377F73377333333330088
          88003333333377FFFF7733333333000000003FFFFFFF77777777000000000000
          000077777777777777770FFFFFFF0FFFFFF07F3333337F3333370FFFFFFF0FFF
          FFF07F3FF3FF7FFFFFF70F00F0080CCC9CC07F773773777777770FFFFFFFF039
          99337F3FFFF3F7F777F30F0000F0F09999937F7777373777777F0FFFFFFFF999
          99997F3FF3FFF77777770F00F000003999337F773777773777F30FFFF0FF0339
          99337F3FF7F3733777F30F08F0F0337999337F7737F73F7777330FFFF0039999
          93337FFFF7737777733300000033333333337777773333333333}
        NumGlyphs = 2
      end
    end
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 456
    Width = 726
    Height = 19
    Panels = <
      item
        Text = '1:1'
        Width = 100
      end
      item
        Width = 50
      end>
    SimplePanel = False
  end
  object Panel2: TPanel
    Left = 0
    Top = 0
    Width = 726
    Height = 456
    Align = alClient
    Caption = 'Panel2'
    TabOrder = 0
    object R: TRichEdit
      Left = 1
      Top = 1
      Width = 724
      Height = 454
      Align = alClient
      Font.Charset = ANSI_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'Courier New'
      Font.Style = []
      HideSelection = False
      ParentFont = False
      PlainText = True
      ScrollBars = ssBoth
      TabOrder = 0
      WantTabs = True
      WordWrap = False
      OnChange = RChange
      OnSelectionChange = RSelectionChange
    end
  end
  object SDB: TSaveDialog
    Filter = 'Bitmap File|*.bmp'
    Left = 120
  end
  object FindDialog1: TFindDialog
    OnFind = FindDialog1Find
    Left = 176
    Top = 128
  end
  object ReplaceDialog1: TReplaceDialog
    Options = [frDown, frHideUpDown, frReplace, frReplaceAll]
    OnFind = ReplaceDialog1Find
    Left = 224
    Top = 128
  end
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 160
    Top = 296
  end
  object SDB2: TSaveDialog
    Filter = 'Dot File|*.dot'
    Left = 64
    Top = 104
  end
end
