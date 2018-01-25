VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "LayoutPropertyInfo"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"Yes"
'local variable(s) to hold property value(s)
Private mvarPName As String '
Private mvarDefaultValue As String '
Private mvarScope As Integer    'a mask indicating the types of graph elems
                                '(graph, node, edge) to which the prop is applicable
Private mvarEngine  As Integer 'a mask indicating to which layout engine the
                                'prop is applicable

Public Property Let Engine(ByVal vData As Integer)
    mvarEngine = vData
End Property

Public Property Get Engine() As Integer
    Engine = mvarEngine
End Property

Public Function hasEngine(Engine As Integer) As Boolean
    hasEngine = (mvarEngine And Engine) <> 0
End Function

Public Function isDot() As Boolean
    isDot = (mvarEngine And DOT) <> 0
End Function

Public Function isNeato() As Boolean
    isNeato = (mvarEngine And NEATO) <> 0
End Function

Public Function isTwopi() As Boolean
    isTwopi = (mvarEngine And TWOPI) <> 0
End Function

Public Function isCirco() As Boolean
    isCirco = (mvarEngine And CIRCO) <> 0
End Function

Public Function isFdp() As Boolean
    isFdp = (mvarEngine And FDP) <> 0
End Function

Public Property Let Scope(ByVal vData As Integer)
    mvarScope = vData
End Property

Public Property Get Scope() As Integer
    Scope = mvarScope
End Property

Public Function hasScope(Scope As Integer) As Boolean
    hasScope = (mvarScope And Scope) <> 0
End Function

Public Function isEdge() As Boolean
    isEdge = (mvarScope And EDGE) <> 0
End Function

Public Function isNode() As Boolean
    isNode = (mvarScope And NODE) <> 0
End Function

Public Function isGraph() As Boolean
    isGraph = (mvarScope And GRAPH) <> 0
End Function

Public Property Let DefaultValue(ByVal vData As String)
    mvarDefaultValue = vData
End Property

Public Property Get DefaultValue() As String
    DefaultValue = mvarDefaultValue
End Property

Public Property Let PName(ByVal vData As String)
    mvarPName = vData
End Property

Public Property Get PName() As String
    PName = mvarPName
End Property



