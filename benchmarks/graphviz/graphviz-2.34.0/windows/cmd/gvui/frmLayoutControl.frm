VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form frmLayoutControl 
   Caption         =   "Graphviz"
   ClientHeight    =   8070
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7230
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   8070
   ScaleWidth      =   7230
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame fraControl 
      Height          =   7935
      Left            =   60
      TabIndex        =   17
      Top             =   60
      Width           =   7095
      Begin VB.Frame fraOutput 
         Caption         =   "Output"
         Height          =   1995
         Left            =   180
         TabIndex        =   27
         Top             =   5820
         Width           =   6735
         Begin VB.CommandButton cmdOutputClear 
            Caption         =   "Clear"
            Height          =   375
            Left            =   5520
            TabIndex        =   29
            Top             =   1500
            Width           =   1095
         End
         Begin VB.TextBox txtOutput 
            Height          =   1575
            Left            =   180
            Locked          =   -1  'True
            MultiLine       =   -1  'True
            ScrollBars      =   3  'Both
            TabIndex        =   28
            Top             =   300
            Width           =   5235
         End
      End
      Begin VB.CommandButton cmdClear 
         Caption         =   "Clear"
         Height          =   375
         Left            =   4800
         TabIndex        =   15
         Top             =   5280
         Width           =   1035
      End
      Begin VB.Frame fraProperties 
         Caption         =   "Properties"
         Height          =   2415
         Left            =   180
         TabIndex        =   23
         Top             =   2700
         Width           =   6735
         Begin VB.ComboBox optPropName 
            Height          =   315
            Left            =   1560
            TabIndex        =   7
            Top             =   600
            Width           =   1335
         End
         Begin VB.ListBox lstProperties 
            Columns         =   2
            Height          =   1230
            Left            =   180
            TabIndex        =   10
            Top             =   1020
            Width           =   5175
         End
         Begin VB.CommandButton cmdPropClear 
            Caption         =   "Clear"
            Height          =   375
            Left            =   5520
            TabIndex        =   12
            Top             =   1560
            Width           =   975
         End
         Begin VB.CommandButton cmdPropDelete 
            Caption         =   "Delete"
            Height          =   375
            Left            =   5520
            TabIndex        =   11
            Top             =   1080
            Width           =   975
         End
         Begin VB.CommandButton cmdPropSet 
            Caption         =   "Set"
            Height          =   375
            Left            =   5520
            TabIndex        =   9
            Top             =   600
            Width           =   975
         End
         Begin VB.ComboBox optPropScope 
            Height          =   315
            Left            =   180
            Style           =   2  'Dropdown List
            TabIndex        =   6
            Top             =   600
            Width           =   1275
         End
         Begin VB.TextBox txtPropValue 
            Height          =   315
            Left            =   3000
            TabIndex        =   8
            Top             =   600
            Width           =   2355
         End
         Begin VB.Label lblPropScope 
            Caption         =   "Scope"
            Height          =   255
            Left            =   180
            TabIndex        =   26
            Top             =   300
            Width           =   855
         End
         Begin VB.Label lblPropValue 
            Caption         =   "Value"
            Height          =   255
            Left            =   3000
            TabIndex        =   25
            Top             =   300
            Width           =   1095
         End
         Begin VB.Label lblPropName 
            Caption         =   "Name"
            Height          =   255
            Left            =   1560
            TabIndex        =   24
            Top             =   300
            Width           =   855
         End
      End
      Begin VB.CommandButton cmdViewOutput 
         Caption         =   "View Output"
         Height          =   375
         Left            =   3360
         TabIndex        =   14
         Top             =   5280
         Width           =   1335
      End
      Begin VB.ComboBox optLayoutEngine 
         CausesValidation=   0   'False
         Height          =   315
         ItemData        =   "frmLayoutControl.frx":0000
         Left            =   1380
         List            =   "frmLayoutControl.frx":0002
         Style           =   2  'Dropdown List
         TabIndex        =   0
         Top             =   360
         Width           =   2535
      End
      Begin VB.TextBox txtInputFile 
         Height          =   345
         Left            =   1380
         TabIndex        =   1
         Top             =   1260
         Width           =   4335
      End
      Begin VB.TextBox txtOutputFile 
         Height          =   345
         Left            =   1380
         TabIndex        =   3
         Top             =   1740
         Width           =   4335
      End
      Begin VB.CommandButton cmdLayout 
         Caption         =   "Do layout"
         Height          =   375
         Left            =   2040
         TabIndex        =   13
         Top             =   5280
         Width           =   1215
      End
      Begin VB.ComboBox optOutputType 
         Height          =   315
         ItemData        =   "frmLayoutControl.frx":0004
         Left            =   1380
         List            =   "frmLayoutControl.frx":0006
         Sorted          =   -1  'True
         Style           =   2  'Dropdown List
         TabIndex        =   5
         Top             =   2220
         Width           =   2535
      End
      Begin VB.CommandButton cmdClose 
         Caption         =   "Close"
         Height          =   375
         Left            =   5940
         TabIndex        =   16
         Top             =   5280
         Width           =   975
      End
      Begin VB.CommandButton cmdInputFile 
         Caption         =   "Browse .."
         Height          =   375
         Left            =   5820
         TabIndex        =   2
         Top             =   1260
         Width           =   1095
      End
      Begin VB.CommandButton cmdOuptutLocation 
         Caption         =   "Browse .."
         Height          =   375
         Left            =   5820
         TabIndex        =   4
         Top             =   1740
         Width           =   1095
      End
      Begin MSComDlg.CommonDialog dlgFileSelection 
         Left            =   1200
         Top             =   5220
         _ExtentX        =   847
         _ExtentY        =   847
         _Version        =   393216
      End
      Begin VB.Label lblLayoutEngine 
         Caption         =   "Layout engine"
         Height          =   255
         Left            =   180
         TabIndex        =   22
         Top             =   360
         Width           =   1335
      End
      Begin VB.Label lblEngineHelp 
         Caption         =   "No selection"
         Height          =   255
         Left            =   1380
         TabIndex        =   21
         Top             =   840
         Width           =   3735
      End
      Begin VB.Label lblInputFile 
         Caption         =   "Input file"
         Height          =   255
         Left            =   180
         TabIndex        =   20
         Top             =   1320
         Width           =   975
      End
      Begin VB.Label lblOutputLocation 
         Caption         =   "Output file"
         Height          =   255
         Left            =   180
         TabIndex        =   19
         Top             =   1800
         Width           =   1695
      End
      Begin VB.Label lblOutputType 
         Caption         =   "Output file type"
         Height          =   255
         Left            =   180
         TabIndex        =   18
         Top             =   2280
         Width           =   1695
      End
   End
End
Attribute VB_Name = "frmLayoutControl"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Private engineHelp As Collection
Private installPath As String

Private props As New LayoutPropertySet
Private propsInfo As New LayoutPropertyInfoSet

Private manualOutputType As Boolean
Private manualOutputFile As Boolean

Private Const installPathRegLoc = "SOFTWARE\ATT\Graphviz"


Private Sub cmdClear_Click()
    optLayoutEngine.ListIndex = -1
    optOutputType.ListIndex = -1
    txtInputFile.Text = ""
    txtOutputFile.Text = ""
    manualOutputType = False
    cmdPropClear_Click
End Sub

Private Sub cmdClose_Click()
    ExitGVUI
End Sub

Private Sub cmdInputFile_Click()
    dlgFileSelection.InitDir = installPath & "\graphs"
    dlgFileSelection.DialogTitle = "Select input graph"
    dlgFileSelection.ShowOpen

    txtInputFile.Text = dlgFileSelection.FileName
End Sub

Private Sub cmdLayout_Click()
    
    If Not assertArgs Then
        Exit Sub
    End If
    
    Dim res As ProcessOutput
    Dim apps As Integer
    Dim cmd As String
    
    manualOutputType = False
    manualOutputFile = False
    
    'ensure the file extension
    If Not manualOutputFile And GetFileExtension(txtOutputFile.Text) = "" Then
        txtOutputFile.Text = txtOutputFile.Text + "." + optOutputType.Text
    End If
    
    cmd = Chr(34) & installPath & "\bin\" & optLayoutEngine.Text & ".exe" & Chr(34) & _
            props.ToString & " " & _
            " -T" & optOutputType.Text & _
            " -o " & Chr(34) & txtOutputFile.Text & Chr(34) & _
            " " & Chr(34) & txtInputFile.Text & Chr(34)
    Debug.Print cmd
    
    Me.MousePointer = MousePointerConstants.vbHourglass
    On Error GoTo LayoutFail
    res = ExecCmd(cmd, True, True)
    Me.MousePointer = MousePointerConstants.vbDefault
        
    txtOutput.Text = txtOutput.Text + optLayoutEngine.Text + " said: "
    If res.ret = 0 Then
        txtOutput.Text = txtOutput.Text + "Layout ended succesfully." + Chr(13) + Chr(10)
    Else
        txtOutput.Text = txtOutput.Text + "Layout ended with errors." + Chr(13) + Chr(10)
    End If
    
    If Not IsNull(res.err) Then
        txtOutput.Text = txtOutput.Text + TrimAll(res.err, True)
    End If
    
    txtOutput.Text = txtOutput.Text + Chr(13) + Chr(10)
    txtOutput.SelStart = Len(txtOutput.Text)
    
    Exit Sub
    
LayoutFail:
    Me.MousePointer = MousePointerConstants.vbDefault
    MsgBox err.Description & "(" & err.Number & ")" & Chr(13) & "Command: " & cmd
    err.Clear
    
End Sub

Private Sub cmdOuptutLocation_Click()
    If txtInputFile.Text <> "" Then
        dlgFileSelection.FileName = GetFileName(txtInputFile.Text)
    End If
    dlgFileSelection.DialogTitle = "Select output file"
    dlgFileSelection.ShowSave

    txtOutputFile.Text = dlgFileSelection.FileName
End Sub

Private Sub cmdOutputClear_Click()
    txtOutput.Text = ""
End Sub

Private Sub cmdPropClear_Click()
    props.Clear
    lstProperties.Clear
    ClearPropInput
End Sub

Private Sub cmdPropDelete_Click()
    Dim p As LayoutProperty
    
    If lstProperties.ListIndex <> -1 Then
        Set p = props.RemoveByPos(lstProperties.ListIndex + 1)
        lstProperties.RemoveItem lstProperties.ListIndex
    Else
        MsgBox "Cannot find property to delete"
    End If

End Sub

Private Sub cmdPropSet_Click()
    
    If optPropScope.Text = "" Or _
        txtPropValue.Text = "" Or _
        optPropName.Text = "" Then
        MsgBox "You need to specify a name, a value and a scope for the property"
        Exit Sub
    End If

    Dim p As LayoutProperty
    
    Set p = props.Add(optPropScope.Text, txtPropValue.Text, optPropName.Text)
    lstProperties.Clear
    For Each p In props
        lstProperties.AddItem p.ToString
    Next p
'    If Not p Is Nothing Then
'        lstProperties.AddItem (p.ToString)
'    Else
'        'update the text: I can only do that through re-insert!
'        Dim i As Integer
'        i = props.Pos(optPropName.Text, optPropScope.Text)
'        lstProperties.ListIndex = i - 1
'        lstProperties = props.ItemByPos(i).ToString
'    End If
    
    ClearPropInput
End Sub

'
'
'
Private Sub cmdViewOutput_Click()
       
    If txtOutputFile.Text = "" Then
        MsgBox "No output file is specified"
        Exit Sub
    End If
       
    Dim exeName As String
    exeName = GetExecutable(optOutputType.Text)
    If exeName = "" Then
        MsgBox "Cannot find associated program for extension " & optOutputType.Text
        Exit Sub
    End If
    
    Me.MousePointer = MousePointerConstants.vbHourglass
    On Error Resume Next
    Call ExecCmd(Chr(34) & exeName & Chr(34) & " " & Chr(34) & txtOutputFile.Text & Chr(34), False, False)
    Me.MousePointer = MousePointerConstants.vbDefault
  
End Sub

Private Sub optLayoutEngine_Click()
    If optLayoutEngine.ListIndex <> -1 Then
        lblEngineHelp.Caption = engineHelp.Item(optLayoutEngine.Text)
    Else
        lblEngineHelp.Caption = "No selection"
    End If
End Sub

Private Sub optOutputType_Click()
    manualOutputType = True
End Sub

Private Sub optOutputType_GotFocus()
    If Not manualOutputType Then
        optOutputType.ListIndex = GetIndex(optOutputType, GetFileExtension(txtOutputFile.Text))
    End If
End Sub
'ugly code
Private Sub optPropScope_LostFocus()
    Dim pi As LayoutPropertyInfo
    
    Dim e As Integer
    Dim s As Integer
    
    optPropName.Clear
    
    e = Switch(optLayoutEngine.Text = "dot", DOT, _
                optLayoutEngine.Text = "neato", NEATO, _
                optLayoutEngine.Text = "twopi", TWOPI, _
                optLayoutEngine.Text = "circo", CIRCO, _
                optLayoutEngine.Text = "fdp", FDP, _
                True, ALL_ENGINES)
        
    s = Switch(optPropScope.Text = "Graph", GRAPH, _
                optPropScope.Text = "Edge", EDGE, _
                optPropScope.Text = "Node", NODE, _
                optPropScope.Text = "Subgraph", SUBGRAPH, _
                optPropScope.Text = "Cluster", CLUSTER, _
                True, ANY_ELEMENT)
    
    For Each pi In propsInfo
        If pi.hasEngine(e) And pi.hasScope(s) Then
            optPropName.AddItem pi.PName
        End If
    Next pi
End Sub

Private Sub txtOutputFile_KeyPress(KeyAscii As Integer)
    manualOutputFile = True
End Sub

Private Function assertArgs() As Boolean
    assertArgs = False
    If optLayoutEngine.ListIndex = -1 Then
        MsgBox "Missing engine specification", , "Error"
        Exit Function
    ElseIf optOutputType.ListIndex = -1 Then
        MsgBox "Missing output type specification", , "Error"
        Exit Function
    ElseIf txtInputFile.Text = "" Then
        MsgBox "Missing input file specification", , "Error"
        Exit Function
    ElseIf txtOutputFile.Text = "" Then
        MsgBox "Missing output file specification", , "Error"
        Exit Function
    End If
    
    assertArgs = True
End Function

Private Sub ExitGVUI()
    Me.Visible = False
    Unload Me
End Sub

Private Sub ClearPropInput()
    optPropName.Text = ""
    txtPropValue.Text = ""
    optPropScope.ListIndex = -1
End Sub

Private Function GetIndex(Combo As ComboBox, Text As String) As Integer
    Dim i As Integer
    GetIndex = -1
    For i = 0 To Combo.ListCount - 1
        Debug.Print Combo.List(i)
        If Combo.List(i) = Text Then
            GetIndex = i
            Exit For
        End If
    Next i
End Function

Private Sub Form_Load()
    manualOutputType = False

    Set engineHelp = New Collection
    engineHelp.Add "Hierarchical drawing of directed graphs", "dot"
    engineHelp.Add "Force layout of undirected graphs", "neato"
    engineHelp.Add "Radial layout", "twopi"
    engineHelp.Add "Circular layout", "circo"
    engineHelp.Add "Force layout of undirected graphs", "fdp"
    
    optPropScope.AddItem "Graph"
    optPropScope.AddItem "Node"
    optPropScope.AddItem "Edge"
    'optPropScope.AddItem "Subgraph"
    'optPropScope.AddItem "Cluster"
    
    optLayoutEngine.AddItem "dot"
    optLayoutEngine.AddItem "neato"
    optLayoutEngine.AddItem "twopi"
    optLayoutEngine.AddItem "circo"
    optLayoutEngine.AddItem "fdp"
    
    optOutputType.AddItem "ps"
    optOutputType.AddItem "ps2"
    optOutputType.AddItem "hpgl"
    optOutputType.AddItem "pcl"
    optOutputType.AddItem "mif"
    optOutputType.AddItem "pic"
    optOutputType.AddItem "gd"
    optOutputType.AddItem "gd2"
    optOutputType.AddItem "gif"
    optOutputType.AddItem "jpg"
    optOutputType.AddItem "jpeg"
    optOutputType.AddItem "png"
    optOutputType.AddItem "wbmp"
    optOutputType.AddItem "ismap"
    optOutputType.AddItem "imap"
    optOutputType.AddItem "cmap"
    optOutputType.AddItem "vrml"
    optOutputType.AddItem "vtx"
    optOutputType.AddItem "mp"
    optOutputType.AddItem "fig"
    optOutputType.AddItem "svg"
    optOutputType.AddItem "svgz"
    optOutputType.AddItem "dot"
    optOutputType.AddItem "canon"
    optOutputType.AddItem "plain"
    optOutputType.AddItem "plain-ext"
    
    propsInfo.Add "Damping", "0.99", GRAPH, NEATO
    propsInfo.Add "Epsilon", "", GRAPH, NEATO
    propsInfo.Add "URL", "", ANY_ELEMENT, ALL_ENGINES
    propsInfo.Add "arrowhead", "normal", EDGE, ALL_ENGINES
    propsInfo.Add "arrowsize", "1.0", EDGE, ALL_ENGINES
    propsInfo.Add "arrowtail", "normal", EDGE, ALL_ENGINES
    propsInfo.Add "bb", "", GRAPH, ALL_ENGINES
    propsInfo.Add "bgcolor", "", GRAPH Or CLUSTER, ALL_ENGINES
    propsInfo.Add "bottomlabel", "", NODE, ALL_ENGINES
    propsInfo.Add "center", "false", GRAPH, ALL_ENGINES
    propsInfo.Add "clusterrank", "local", GRAPH, DOT
    propsInfo.Add "color", "black", EDGE Or NODE Or CLUSTER, ALL_ENGINES
    propsInfo.Add "comment", "", EDGE Or NODE Or GRAPH, ALL_ENGINES
    propsInfo.Add "compound", "false", GRAPH, DOT
    propsInfo.Add "concentrate", "false", GRAPH, DOT
    propsInfo.Add "constraint", "true", EDGE, DOT
    propsInfo.Add "decorate", "false", EDGE, ALL_ENGINES
    propsInfo.Add "dir", "forward", EDGE, ALL_ENGINES
    propsInfo.Add "distortion", "0.0", NODE, ALL_ENGINES
    propsInfo.Add "fillcolor", "lightgrey", NODE Or CLUSTER, ALL_ENGINES
    propsInfo.Add "fixedsize", "false", NODE, ALL_ENGINES
    propsInfo.Add "fontcolor", "black", EDGE Or NODE Or GRAPH Or CLUSTER, ALL_ENGINES
    propsInfo.Add "fontname", "Times-Roman", EDGE Or NODE Or GRAPH Or CLUSTER, ALL_ENGINES
    propsInfo.Add "fontpath", "", GRAPH, ALL_ENGINES
    propsInfo.Add "fontsize", "14.0", EDGE Or NODE Or GRAPH Or CLUSTER, ALL_ENGINES
    propsInfo.Add "group", "", NODE, DOT
    propsInfo.Add "headURL", "", EDGE, ALL_ENGINES
    propsInfo.Add "headlabel", "", EDGE, ALL_ENGINES
    propsInfo.Add "headport", "center", EDGE, ALL_ENGINES
    propsInfo.Add "height", "0.5", NODE, ALL_ENGINES
    propsInfo.Add "label", "", EDGE Or NODE Or GRAPH Or CLUSTER, ALL_ENGINES
    propsInfo.Add "labelangle", "-25.0", EDGE, ALL_ENGINES
    propsInfo.Add "labeldistance", "1.0", EDGE, ALL_ENGINES
    propsInfo.Add "labelfloat", "false", EDGE, ALL_ENGINES
    propsInfo.Add "labelfontcolor", "black", EDGE, ALL_ENGINES
    propsInfo.Add "labelfontname", "Times-Roman", EDGE, ALL_ENGINES
    propsInfo.Add "labelfontsize", "11.0", EDGE, ALL_ENGINES
    propsInfo.Add "labeljust", "", ANY_ELEMENT, DOT
    propsInfo.Add "labelloc", "t", GRAPH Or CLUSTER, DOT
    propsInfo.Add "layer", "", EDGE Or NODE, ALL_ENGINES
    propsInfo.Add "layers", "", GRAPH, ALL_ENGINES
    propsInfo.Add "len", "1.0", EDGE, NEATO
    propsInfo.Add "lhead", "", EDGE, DOT
    propsInfo.Add "lp", "", EDGE Or GRAPH Or CLUSTER, ALL_ENGINES
    propsInfo.Add "ltail", "", EDGE, DOT
    propsInfo.Add "margin", "", GRAPH, ALL_ENGINES
    propsInfo.Add "maxiter", "", GRAPH, NEATO
    propsInfo.Add "mclimit", "1.0", GRAPH, DOT
    propsInfo.Add "minlen", "1", EDGE, DOT
    propsInfo.Add "model", "", GRAPH, NEATO
    propsInfo.Add "nodesep", "0.25", GRAPH, DOT
    propsInfo.Add "normalize", "false", GRAPH, NEATO
    propsInfo.Add "nslimit", "", GRAPH, DOT
    propsInfo.Add "ordering", "", GRAPH, DOT
    propsInfo.Add "orientation", "0.0", NODE, ALL_ENGINES
    propsInfo.Add "orientation", "", GRAPH, ALL_ENGINES
    propsInfo.Add "overlap", "", GRAPH, NEATO
    propsInfo.Add "pack", "false", GRAPH, NEATO
    propsInfo.Add "page", "", GRAPH, ALL_ENGINES
    propsInfo.Add "pagedir", "", GRAPH, ALL_ENGINES
    propsInfo.Add "pencolor", "black", CLUSTER, ALL_ENGINES
    propsInfo.Add "peripheries", "0", NODE, ALL_ENGINES
    propsInfo.Add "pin", "false", NODE, NEATO
    propsInfo.Add "pos", "", EDGE Or NODE, ALL_ENGINES
    propsInfo.Add "quantum", "0.0", GRAPH, ALL_ENGINES
    propsInfo.Add "rank", "", SUBGRAPH, DOT
    propsInfo.Add "rankdir", "", GRAPH, DOT
    propsInfo.Add "ranksep", "", GRAPH, ALL_ENGINES
    propsInfo.Add "ratio", "", GRAPH, ALL_ENGINES
    propsInfo.Add "rects", "", NODE, ALL_ENGINES
    propsInfo.Add "regular", "false", NODE, ALL_ENGINES
    propsInfo.Add "remincross", "false", GRAPH, DOT
    propsInfo.Add "rotate", "0", GRAPH, ALL_ENGINES
    propsInfo.Add "samehead", "", EDGE, DOT
    propsInfo.Add "sametail", "", EDGE, DOT
    propsInfo.Add "samplepoints", "8", GRAPH, ALL_ENGINES
    propsInfo.Add "searchsize", "30", GRAPH, DOT
    propsInfo.Add "sep", "0.01", GRAPH, NEATO
    propsInfo.Add "shape", "ellipse", NODE, ALL_ENGINES
    propsInfo.Add "shapefile", "", NODE, ALL_ENGINES
    propsInfo.Add "showboxes", "0", EDGE Or NODE Or GRAPH, DOT
    propsInfo.Add "sides", "4", NODE, ALL_ENGINES
    propsInfo.Add "size", "", GRAPH, ALL_ENGINES
    propsInfo.Add "skew", "0.0", NODE, ALL_ENGINES
    propsInfo.Add "splines", "false", GRAPH, NEATO
    propsInfo.Add "start", "", GRAPH, ALL_ENGINES
    propsInfo.Add "style", "", EDGE Or NODE, ALL_ENGINES
    propsInfo.Add "stylesheet", "", GRAPH, ALL_ENGINES
    propsInfo.Add "tailURL", "", EDGE, ALL_ENGINES
    propsInfo.Add "taillabel", "", EDGE, ALL_ENGINES
    propsInfo.Add "tailport", "center", EDGE, ALL_ENGINES
    propsInfo.Add "toplabel", "", NODE, ALL_ENGINES
    propsInfo.Add "vertices", "", NODE, ALL_ENGINES
    propsInfo.Add "voro_margin", "0.05", GRAPH, NEATO
    propsInfo.Add "weight", "", EDGE, DOT Or NEATO
    propsInfo.Add "width", "0.75", NODE, ALL_ENGINES
    propsInfo.Add "z", "0.0", NODE, ALL_ENGINES
    
    optLayoutEngine.ListIndex = GetIndex(optLayoutEngine, Command)

    On Error GoTo nopath
    installPath = GetSettingString(HKEY_LOCAL_MACHINE, _
                                    installPathRegLoc, _
                                    "InstallPath")
    Exit Sub
    
nopath:
    MsgBox "Missing graphviz installation location. Please specify one."
    installPath = BrowseForFolder(Me.hWnd, "Graphviz install directory")
    If installPath = "" Then
        MsgBox "Missing graphviz location. Cannot continue"
        ExitGVUI
    Else
       On Error GoTo noreg
       SaveSettingString HKEY_LOCAL_MACHINE, installPathRegLoc, "InstallPath", installPath
    End If
    Exit Sub
    
noreg:
    MsgBox "Unable to save registry info. Your setting will only be available " & vbCrLf & _
            "for this session"
    ExitGVUI
End Sub
