VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "LayoutPropertySet"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Collection" ,"LayoutProperty"
Attribute VB_Ext_KEY = "Member0" ,"LayoutProperty"
Attribute VB_Ext_KEY = "Top_Level" ,"Yes"
'local variable to hold collection
Private mCol As Collection

'
' Avoids duplicates. If only an update is done than is returns 'Nothing'; otherwise
' returns the newly created LayoutProperty instance
'
Public Function Add(Scope As String, Value As String, PName As String) As LayoutProperty
    
    Dim objNewMember As LayoutProperty
    
    Set objNewMember = Item(PName, Scope)
    If Not objNewMember Is Nothing Then
        objNewMember.Value = Value
        Set objNewMember = Nothing
    Else
        Set objNewMember = New LayoutProperty
        
        'set the properties passed into the method
        objNewMember.Scope = Scope
        objNewMember.Value = Value
        objNewMember.PName = PName

        mCol.Add objNewMember, objNewMember.Id
    End If
    
    'return the object
    Set Add = objNewMember
    Set objNewMember = Nothing
    
End Function

Public Property Get ItemByPos(Pos As Integer) As LayoutProperty
    Set ItemByPos = mCol.Item(Pos)
End Property

Public Property Get Item(thePName As String, theScope As String) As LayoutProperty
Attribute Item.VB_UserMemId = 0
    Dim p As LayoutProperty
    Dim i As Integer
    
    i = Pos(thePName, theScope)
    If i = 0 Then
        Set Item = Nothing
    Else
        Set Item = mCol(i)
    End If
    
End Property
'finds the position of a given property, returns 0 if not found
Public Function Pos(thePName As String, theScope As String) As Integer
    Dim i As Integer
    Dim p As LayoutProperty
    
    For i = 1 To mCol.Count
        Set p = mCol(i)
        If p.PName = thePName And p.Scope = theScope Then
            Pos = i
            Exit Function
        End If
    Next i
    
    Pos = 0
End Function

Public Property Get Count() As Long
    'used when retrieving the number of elements in the
    'collection. Syntax: Debug.Print x.Count
    Count = mCol.Count
End Property

Public Function Remove(PName As String, Scope As String) As LayoutProperty
    Set Remove = Item(PName, Scope)
    If Not Remove Is Nothing Then
        mCol.Remove Remove.Id
    End If
End Function

Public Function RemoveByPos(Pos As Integer) As LayoutProperty
    Set RemoveByPos = mCol.Item(Pos)
    If Not RemoveByPos Is Nothing Then
        mCol.Remove RemoveByPos.Id
    End If
End Function

Public Property Get NewEnum() As IUnknown
Attribute NewEnum.VB_UserMemId = -4
Attribute NewEnum.VB_MemberFlags = "40"
    'this property allows you to enumerate
    'this collection with the For...Each syntax
    Set NewEnum = mCol.[_NewEnum]
End Property


Private Sub Class_Initialize()
    'creates the collection when this class is created
    Set mCol = New Collection
End Sub


Private Sub Class_Terminate()
    'destroys collection when this class is terminated
    Set mCol = Nothing
End Sub

Public Function ToString() As String
    Dim p As LayoutProperty
    For Each p In mCol
        ToString = ToString & " " & p.ToString
    Next p
End Function

Public Sub Clear()
    While mCol.Count <> 0
        mCol.Remove 1
    Wend
End Sub

