Attribute VB_Name = "Registry"
' From the VBWorld website
' Detailed Registry access through win32 apis
'

Public Const HKEY_CLASSES_ROOT = &H80000000
Public Const HKEY_CURRENT_USER = &H80000001
Public Const HKEY_LOCAL_MACHINE = &H80000002
Public Const HKEY_USERS = &H80000003
Public Const HKEY_CURRENT_CONFIG = &H80000005
Public Const HKEY_DYN_DATA = &H80000006
Public Const REG_SZ = 1 'Unicode nul terminated string
Public Const REG_BINARY = 3 'Free form binary
Public Const REG_DWORD = 4 '32-bit number
Public Const ERROR_SUCCESS = 0&

Public Declare Function RegCloseKey Lib "advapi32.dll" _
(ByVal hKey As Long) As Long

Public Declare Function RegCreateKey Lib "advapi32.dll" _
Alias "RegCreateKeyA" (ByVal hKey As Long, ByVal lpSubKey _
As String, phkResult As Long) As Long

'Public Declare Function RegDeleteKey Lib "advapi32.dll" _
'Alias "RegDeleteKeyA" (ByVal hKey As Long, ByVal lpSubKey _
'As String) As Long

'Public Declare Function RegDeleteValue Lib "advapi32.dll" _
'Alias "RegDeleteValueA" (ByVal hKey As Long, ByVal _
'lpValueName As String) As Long

Public Declare Function RegOpenKey Lib "advapi32.dll" _
Alias "RegOpenKeyA" (ByVal hKey As Long, ByVal lpSubKey _
As String, phkResult As Long) As Long

Public Declare Function RegQueryValueEx Lib "advapi32.dll" _
Alias "RegQueryValueExA" (ByVal hKey As Long, ByVal lpValueName _
As String, ByVal lpReserved As Long, lpType As Long, lpData _
As Any, lpcbData As Long) As Long

Public Declare Function RegSetValueEx Lib "advapi32.dll" _
Alias "RegSetValueExA" (ByVal hKey As Long, ByVal _
lpValueName As String, ByVal Reserved As Long, ByVal _
dwType As Long, lpData As Any, ByVal cbData As Long) As Long


Public Function GetSettingString(hKey As Long, _
                                strPath As String, _
                                strValue As String, _
                                Optional Default As String) As String
    Dim hCurKey As Long
    Dim lResult As Long
    Dim lValueType As Long
    Dim strBuffer As String
    Dim lDataBufferSize As Long
    Dim intZeroPos As Integer
    Dim lRegResult As Long

    'Set up default value
    If Not IsEmpty(Default) Then
        GetSettingString = Default
    Else
        GetSettingString = ""
    End If

    lRegResult = RegOpenKey(hKey, strPath, hCurKey)
    lRegResult = RegQueryValueEx(hCurKey, strValue, 0&, _
                                lValueType, ByVal 0&, lDataBufferSize)

    If lRegResult = ERROR_SUCCESS Then
        If lValueType = REG_SZ Then
            strBuffer = String(lDataBufferSize, " ")
            lResult = RegQueryValueEx(hCurKey, strValue, 0&, 0&, _
                                      ByVal strBuffer, lDataBufferSize)
            intZeroPos = InStr(strBuffer, Chr$(0))
            If intZeroPos > 0 Then
                GetSettingString = Left$(strBuffer, intZeroPos - 1)
            Else
                GetSettingString = strBuffer
            End If
        End If
    Else
        Err.Raise 5013, "Registry", "Unable to find get key value"
    End If

    lRegResult = RegCloseKey(hCurKey)
End Function

Public Sub CreateKey(hKey As Long, strPath As String)
    
    Dim hCurKey As Long
    Dim lRegResult As Long

    lRegResult = RegCreateKey(hKey, strPath, hCurKey)
    If lRegResult <> ERROR_SUCCESS Then
        Err.Raise 5013, "Registry", "Unable to find create registry key"
    End If
    lRegResult = RegCloseKey(hCurKey)

End Sub

Public Sub SaveSettingString(hKey As Long, _
                            strPath As String, _
                            strValue As String, _
                            strData As String)
    Dim hCurKey As Long
    Dim lRegResult As Long

    lRegResult = RegCreateKey(hKey, strPath, hCurKey)
    lRegResult = RegSetValueEx(hCurKey, strValue, 0, REG_SZ, _
                                ByVal strData, Len(strData))

    If lRegResult <> ERROR_SUCCESS Then
            Err.Raise 5013, "Registry", "Unable to find save registry settings"
    End If

    lRegResult = RegCloseKey(hCurKey)
End Sub
