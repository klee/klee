object frmSettings: TfrmSettings
  Left = 362
  Top = 229
  BorderIcons = []
  BorderStyle = bsSingle
  Caption = 'Graphviz Settings'
  ClientHeight = 465
  ClientWidth = 372
  Color = clBtnFace
  DefaultMonitor = dmPrimary
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poDesktopCenter
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 372
    Height = 465
    Align = alClient
    TabOrder = 0
    DesignSize = (
      372
      465)
    object Bevel2: TBevel
      Left = 4
      Top = 134
      Width = 364
      Height = 51
      Anchors = [akLeft, akTop, akRight]
    end
    object Bevel1: TBevel
      Left = 4
      Top = 2
      Width = 364
      Height = 129
      Anchors = [akLeft, akTop, akRight]
    end
    object Label1: TLabel
      Left = 16
      Top = 8
      Width = 68
      Height = 13
      Caption = 'Layout Engine'
    end
    object Label2: TLabel
      Left = 16
      Top = 35
      Width = 82
      Height = 13
      Caption = 'Output File Name'
    end
    object Label3: TLabel
      Left = 16
      Top = 58
      Width = 78
      Height = 13
      Caption = 'Output File Type'
    end
    object Label4: TLabel
      Left = 16
      Top = 83
      Width = 92
      Height = 13
      Caption = 'Graphviz Bin Folder'
    end
    object Label5: TLabel
      Left = 7
      Top = 139
      Width = 31
      Height = 13
      Caption = 'Scope'
    end
    object Label6: TLabel
      Left = 136
      Top = 139
      Width = 28
      Height = 13
      Caption = 'Name'
    end
    object Label7: TLabel
      Left = 8
      Top = 164
      Width = 27
      Height = 13
      Caption = 'Value'
    end
    object ComboBox1: TComboBox
      Left = 111
      Top = 4
      Width = 58
      Height = 21
      Style = csDropDownList
      Anchors = [akLeft, akTop, akRight]
      ItemHeight = 13
      TabOrder = 0
      Items.Strings = (
        'dot'
        'neato'
        'twopi'
        'circo'
        'fdp'
        'sfdp')
    end
    object Edit1: TEdit
      Left = 111
      Top = 31
      Width = 201
      Height = 21
      Anchors = [akLeft, akTop, akRight]
      TabOrder = 1
    end
    object ComboBox2: TComboBox
      Left = 111
      Top = 55
      Width = 201
      Height = 21
      Style = csDropDownList
      Anchors = [akLeft, akTop, akRight]
      ItemHeight = 13
      TabOrder = 2
      OnChange = ComboBox2Change
      Items.Strings = (
        'cmapx'
        'dot'
        'xdot'
        'fig'
        'gd'
        'gd2'
        'gif'
        'hpgl'
        'imap'
        'jpg'
        'mif'
        'mp'
        'pcl'
        'pic'
        'plain'
        'plain-ext'
        'png'
        'ps'
        'ps2'
        'svg'
        'svgz'
        'vrml'
        'vtx'
        'wbmp')
    end
    object Button1: TButton
      Left = 317
      Top = 31
      Width = 45
      Height = 21
      Anchors = [akTop, akRight]
      Caption = '...'
      TabOrder = 3
      OnClick = Button1Click
    end
    object CheckBox1: TCheckBox
      Left = 40
      Top = 107
      Width = 145
      Height = 17
      Caption = 'Preview Output File'
      Checked = True
      State = cbChecked
      TabOrder = 4
    end
    object Button3: TButton
      Left = 293
      Top = 438
      Width = 75
      Height = 25
      Anchors = [akRight, akBottom]
      Caption = 'OK'
      Default = True
      TabOrder = 5
      OnClick = Button3Click
    end
    object Button4: TButton
      Left = 213
      Top = 438
      Width = 75
      Height = 25
      Anchors = [akRight, akBottom]
      Caption = 'Cancel'
      TabOrder = 6
      OnClick = Button4Click
    end
    object Edit2: TEdit
      Left = 111
      Top = 79
      Width = 201
      Height = 21
      Anchors = [akLeft, akTop, akRight]
      TabOrder = 7
    end
    object ComboBox3: TComboBox
      Left = 43
      Top = 136
      Width = 85
      Height = 21
      Style = csDropDownList
      ItemHeight = 13
      ItemIndex = 0
      TabOrder = 8
      Text = 'Graph'
      OnChange = ComboBox3Change
      Items.Strings = (
        'Graph'
        'Node'
        'Edge')
    end
    object ComboBox4: TComboBox
      Left = 168
      Top = 136
      Width = 109
      Height = 21
      Style = csDropDownList
      Anchors = [akLeft, akTop, akRight]
      ItemHeight = 13
      ItemIndex = 0
      TabOrder = 9
      Text = 'URL'
      Items.Strings = (
        'URL'
        'bb'
        'bgColor')
    end
    object Memo1: TMemo
      Left = 4
      Top = 188
      Width = 364
      Height = 246
      Anchors = [akLeft, akTop, akRight, akBottom]
      TabOrder = 10
    end
    object Button2: TButton
      Left = 285
      Top = 137
      Width = 75
      Height = 22
      Anchors = [akTop, akRight]
      Caption = 'Add'
      TabOrder = 11
      OnClick = Button2Click
    end
    object Button6: TButton
      Left = 285
      Top = 161
      Width = 75
      Height = 22
      Anchors = [akTop, akRight]
      Caption = 'Help'
      TabOrder = 12
      OnClick = Button6Click
    end
    object Edit3: TEdit
      Left = 42
      Top = 160
      Width = 233
      Height = 21
      Anchors = [akLeft, akTop, akRight]
      TabOrder = 13
    end
    object BitBtn1: TBitBtn
      Left = 90
      Top = 438
      Width = 41
      Height = 25
      Hint = 'Save Attributes'
      Anchors = [akLeft, akBottom]
      ParentShowHint = False
      ShowHint = True
      TabOrder = 14
      OnClick = BitBtn1Click
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000120B0000120B00001000000000000000000000000000
        800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00333333333333
        333333FFFFFFFFFFFFF33000077777770033377777777777773F000007888888
        00037F3337F3FF37F37F00000780088800037F3337F77F37F37F000007800888
        00037F3337F77FF7F37F00000788888800037F3337777777337F000000000000
        00037F3FFFFFFFFFFF7F00000000000000037F77777777777F7F000FFFFFFFFF
        00037F7F333333337F7F000FFFFFFFFF00037F7F333333337F7F000FFFFFFFFF
        00037F7F333333337F7F000FFFFFFFFF00037F7F333333337F7F000FFFFFFFFF
        00037F7F333333337F7F000FFFFFFFFF07037F7F33333333777F000FFFFFFFFF
        0003737FFFFFFFFF7F7330099999999900333777777777777733}
      NumGlyphs = 2
    end
    object BitBtn2: TBitBtn
      Left = 6
      Top = 438
      Width = 41
      Height = 25
      Hint = 'Clear Attributes'
      Anchors = [akLeft, akBottom]
      ParentShowHint = False
      ShowHint = True
      TabOrder = 15
      OnClick = BitBtn2Click
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000130B0000130B00001000000000000000000000000000
        800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF0033333333B333
        333B33FF33337F3333F73BB3777BB7777BB3377FFFF77FFFF77333B000000000
        0B3333777777777777333330FFFFFFFF07333337F33333337F333330FFFFFFFF
        07333337F33333337F333330FFFFFFFF07333337F33333337F333330FFFFFFFF
        07333FF7F33333337FFFBBB0FFFFFFFF0BB37777F3333333777F3BB0FFFFFFFF
        0BBB3777F3333FFF77773330FFFF000003333337F333777773333330FFFF0FF0
        33333337F3337F37F3333330FFFF0F0B33333337F3337F77FF333330FFFF003B
        B3333337FFFF77377FF333B000000333BB33337777777F3377FF3BB3333BB333
        3BB33773333773333773B333333B3333333B7333333733333337}
      NumGlyphs = 2
    end
    object BitBtn3: TBitBtn
      Left = 48
      Top = 438
      Width = 41
      Height = 25
      Hint = 'Load from file'
      Anchors = [akLeft, akBottom]
      ParentShowHint = False
      ShowHint = True
      TabOrder = 16
      OnClick = BitBtn3Click
      Glyph.Data = {
        76010000424D7601000000000000760000002800000020000000100000000100
        04000000000000010000130B0000130B00001000000000000000000000000000
        800000800000008080008000000080008000808000007F7F7F00BFBFBF000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF0033333333B333
        333B33FF33337F3333F73BB3777BB7777BB3377FFFF77FFFF77333B000000000
        0B3333777777777777333330FFFFFFFF07333337F33333337F333330FFFFFFFF
        07333337F3FF3FFF7F333330F00F000F07333337F77377737F333330FFFFFFFF
        07333FF7F3FFFF3F7FFFBBB0F0000F0F0BB37777F7777373777F3BB0FFFFFFFF
        0BBB3777F3FF3FFF77773330F00F000003333337F773777773333330FFFF0FF0
        33333337F3FF7F37F3333330F08F0F0B33333337F7737F77FF333330FFFF003B
        B3333337FFFF77377FF333B000000333BB33337777777F3377FF3BB3333BB333
        3BB33773333773333773B333333B3333333B7333333733333337}
      NumGlyphs = 2
    end
    object CheckBox2: TCheckBox
      Left = 244
      Top = 107
      Width = 97
      Height = 17
      Caption = 'Apply cairo filter'
      Enabled = False
      TabOrder = 17
    end
    object Memo2: TMemo
      Left = 336
      Top = 72
      Width = 177
      Height = 57
      Lines.Strings = (
        'Memo2')
      TabOrder = 18
      Visible = False
      WordWrap = False
    end
  end
  object SD1: TSaveDialog
    Filter = 'JPG|*.jpg|JPEG|*.jpeg'
    Options = [ofOverwritePrompt, ofHideReadOnly, ofEnableSizing]
    OptionsEx = [ofExNoPlacesBar]
    Left = 232
    Top = 64
  end
  object SD2: TSaveDialog
    Left = 256
    Top = 64
  end
  object OD1: TOpenDialog
    Left = 184
    Top = 176
  end
end
