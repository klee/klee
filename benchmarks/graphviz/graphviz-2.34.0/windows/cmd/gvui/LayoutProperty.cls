VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "LayoutProperty"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"Yes"
'local variable(s) to hold property value(s)
Private mvarScope As String 'local copy
Private mvarValue As String 'local copy
Private mvarPName As String 'local copy

Public Property Let PName(ByVal vData As String)
'used when assigning a value to the property, on the left side of an assignment.
'Syntax: X.PName = 5
    mvarPName = vData
End Property

Public Property Get PName() As String
'used when retrieving value of a property, on the right side of an assignment.
'Syntax: Debug.Print X.PName
    PName = mvarPName
End Property



Public Property Let Value(ByVal vData As String)
'used when assigning a value to the property, on the left side of an assignment.
'Syntax: X.Value = 5
    mvarValue = vData
End Property


Public Property Get Value() As String
'used when retrieving value of a property, on the right side of an assignment.
'Syntax: Debug.Print X.Value
    Value = mvarValue
End Property



Public Property Let Scope(ByVal vData As String)
'used when assigning a value to the property, on the left side of an assignment.
'Syntax: X.Scope = 5
    mvarScope = vData
End Property


Public Property Get Scope() As String
'used when retrieving value of a property, on the right side of an assignment.
'Syntax: Debug.Print X.Scope
    Scope = mvarScope
End Property

Public Property Get Id() As String
    Id = mvarPName & ":" & mvarScope
End Property

Public Function ToString() As String

    If mvarScope = "Graph" Then
        ToString = "-G"
    ElseIf mvarScope = "Node" Then
        ToString = "-N"
    ElseIf mvarScope = "Edge" Then
        ToString = "-E"
    ElseIf mvarScope = "Subgraph" Then
        ToString = "-S"
    ElseIf mvarScope = "Cluster" Then
        ToString = "-C"
    End If
    
    ToString = ToString & mvarPName & "=" & _
                IIf(Mid(mvarValue, 1, 1) = Chr(34), "", Chr(34)) & _
                mvarValue & _
                IIf(Mid(mvarValue, 1, 1) = Chr(34), "", Chr(34))
End Function

