Attribute VB_Name = "Other"
'
' BrowseForFolder comes from codeguru
' GetExecutable comes from VBWorld
' TrimAll comes from freevbcode

'
' Suport for the browse directory dialog
'
Public Enum eBIF
    BIF_RETURNONLYFSDIRS = &H1
    BIF_DONTGOBELOWDOMAIN = &H2
    BIF_STATUSTEXT = &H4
    BIF_RETURNFSANCESTORS = &H8
    BIF_BROWSEFORCOMPUTER = &H1000
    BIF_BROWSEFORPRINTER = &H2000
End Enum

Private Type BROWSEINFO
    hwndOwner       As Long
    pidlRoot        As Long
    pszDisplayName  As String
    lpszTitle       As String
    ulFlags         As Long
    lpfnCallback    As Long
    lParam          As Long
    iImage          As Long
End Type

Private Declare Function SHBrowseForFolder Lib "shell32.dll" (lpbi As BROWSEINFO) As Long
Private Declare Function SHGetPathFromIDList Lib "shell32.dll" Alias "SHGetPathFromIDListA" (ByVal pidl As Long, ByVal pszPath As String) As Long
Private Declare Sub CoTaskMemFree Lib "ole32.dll" (ByVal hMem As Long)

'
' Support for the find executable
'

Private Declare Function FindExecutable Lib "shell32.dll" Alias "FindExecutableA" _
                                (ByVal lpFile As String, ByVal lpDirectory _
                                As String, ByVal lpResult As String) As Long

Private Declare Function GetTempFileName Lib "kernel32" Alias "GetTempFileNameA" (ByVal _
                                lpszPath As String, ByVal lpPrefixString _
                                As String, ByVal wUnique As Long, ByVal _
                                lpTempFileName As String) As Long

Private Declare Function GetTempPath Lib "kernel32" Alias "GetTempPathA" (ByVal _
                                nBufferLength As Long, ByVal lpBuffer As _
                                String) As Long

'
' high level interfaces
'

Public Function BrowseForFolder(ByVal hwndOwner As Long, _
                                ByVal sPrompt As String, _
                                Optional ByVal lFlags As eBIF = BIF_RETURNONLYFSDIRS) As String
    '
    Dim iNull As Integer
    Dim lpIDList As Long
    Dim lResult As Long
    Dim sPath As String
    Dim udtBI As BROWSEINFO

    With udtBI
        .hwndOwner = hwndOwner
        .lpszTitle = sPrompt & ""
        .ulFlags = BIF_RETURNONLYFSDIRS
    End With

    lpIDList = SHBrowseForFolder(udtBI)
    If lpIDList Then
        sPath = String$(260, 0)
        lResult = SHGetPathFromIDList(lpIDList, sPath)
        Call CoTaskMemFree(lpIDList)
        iNull = InStr(sPath, vbNullChar)
        If iNull Then
            sPath = Left$(sPath, iNull - 1)
        End If
    Else
        'Cancel is clicked
        sPath = ""
    End If

    BrowseForFolder = sPath
End Function


Public Function GetExecutable(ByVal Extension As String) As String

    Dim Path As String
    Dim FileName As String
    Dim nRet As Long
    Const MAX_PATH As Long = 260
    
    'Create a tempfile
    Path = String$(MAX_PATH, 0)
    
    If GetTempPath(MAX_PATH, Path) Then
        FileName = String$(MAX_PATH, 0)
    
        If GetTempFileName(Path, "~", 0, FileName) Then
            FileName = Left$(FileName, _
                InStr(FileName, vbNullChar) - 1)
        
            'Rename it to use supplied extension
            Name FileName As Left$(FileName, _
                InStr(FileName, ".")) & Extension
                FileName = Left$(FileName, _
                InStr(FileName, ".")) & Extension
        
            'Get name of associated EXE
            Path = String$(MAX_PATH, 0)
        
            Call FindExecutable(FileName, vbNullString, Path)
            GetExecutable = Left$(Path, InStr(Path, vbNullChar) - 1)
        
            'Clean up
            Kill FileName
        End If
    End If

End Function

Public Function GetFileExtension(ByVal FilePath As String) As String
    Dim Pos As Integer
    Pos = InStrRev(FilePath, "\")
    If Pos <> 0 Then
        FilePath = Right$(FilePath, Len(FilePath) - Pos)
    End If
    Pos = InStrRev(FilePath, ".")
    If Pos <> 0 Then
        GetFileExtension = Right$(FilePath, Len(FilePath) - Pos)
    Else
        GetFileExtension = ""
    End If
End Function

Public Function GetFileName(ByVal FilePath As String) As String
    Dim Pos As Integer
    Dim apos As Integer
    
    Pos = InStrRev(FilePath, "\")
    apos = InStrRev(FilePath, ".")
    
    GetFileName = Mid(FilePath, _
                      IIf(Pos = 0, 1, Pos + 1), _
                     IIf(apos = 0, Len(FilePath), apos - Pos - 1))
End Function

Public Function TrimAll(ByVal TextIN As String, Optional NonPrints As Boolean) As String

    TrimAll = Trim(TextIN)

    If NonPrints Then
        Dim x As Long
        ' remove all non-printable characters
        While InStr(TrimAll, vbCrLf) > 0
            TrimAll = Replace(TrimAll, vbCrLf, " ")
        Wend

        While InStr(TrimAll, vbTab) > 0
            TrimAll = Replace(TrimAll, vbTab, " ")
        Wend

        For x = 0 To 31
            While InStr(TrimAll, Chr(x)) > 0
                TrimAll = Replace(TrimAll, Chr(x), " ")
            Wend
        Next x

        For x = 127 To 255
            While InStr(TrimAll, Chr(x)) > 0
                TrimAll = Replace(TrimAll, Chr(x), " ")
            Wend
        Next x
    End If

    While InStr(TrimAll, String(2, " ")) > 0
        TrimAll = Replace(TrimAll, String(2, " "), " ")
    Wend

End Function

