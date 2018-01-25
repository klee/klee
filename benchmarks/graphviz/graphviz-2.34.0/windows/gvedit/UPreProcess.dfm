object frmPre: TfrmPre
  Left = 399
  Top = 288
  BorderStyle = bsSingle
  Caption = 'Preprocessor Settings'
  ClientHeight = 103
  ClientWidth = 383
  Color = clBtnFace
  DefaultMonitor = dmPrimary
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnShow = FormShow
  DesignSize = (
    383
    103)
  PixelsPerInch = 96
  TextHeight = 13
  object Panel2: TPanel
    Left = -3
    Top = 0
    Width = 384
    Height = 72
    BevelInner = bvRaised
    BevelOuter = bvLowered
    TabOrder = 0
    DesignSize = (
      384
      72)
    object Label9: TLabel
      Left = 17
      Top = 23
      Width = 22
      Height = 13
      Caption = 'Path'
    end
    object Label8: TLabel
      Left = 16
      Top = 44
      Width = 70
      Height = 13
      Caption = 'Command Line'
    end
    object Label10: TLabel
      Left = 6
      Top = 4
      Width = 66
      Height = 13
      Caption = 'Pre-Processor'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = [fsUnderline]
      ParentFont = False
    end
    object PreprocEdit: TEdit
      Left = 111
      Top = 42
      Width = 266
      Height = 21
      Anchors = [akLeft, akTop, akRight]
      TabOrder = 0
    end
    object PreprocPathEdit: TEdit
      Left = 111
      Top = 19
      Width = 266
      Height = 21
      Anchors = [akLeft, akTop, akRight]
      TabOrder = 1
    end
  end
  object Button4: TButton
    Left = 229
    Top = 74
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Caption = 'Cancel'
    TabOrder = 1
    OnClick = Button4Click
  end
  object Button3: TButton
    Left = 305
    Top = 74
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Caption = 'OK'
    Default = True
    TabOrder = 2
    OnClick = Button3Click
  end
end
