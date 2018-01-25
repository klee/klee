Attribute VB_Name = "Process"
' From Microsoft Knowledge Base Article - Q129796 and 173085
'
'

Public Type ProcessOutput
    ret As Long             'return value
    out As String           'whatever was printed to stdout
    err As String           'whatever was printed to stderr
End Type

Public Type ProcessContext
    outHnd As Long          'handler for the redirected output
    outName As String       'name of the file to which output is redirected
End Type

Private Type STARTUPINFO
    cb As Long
    lpReserved As String
    lpDesktop As String
    lpTitle As String
    dwX As Long
    dwY As Long
    dwXSize As Long
    dwYSize As Long
    dwXCountChars As Long
    dwYCountChars As Long
    dwFillAttribute As Long
    dwFlags As Long
    wShowWindow As Integer
    cbReserved2 As Integer
    lpReserved2 As Long
    hStdInput As Long
    hStdOutput As Long
    hStdError As Long
End Type

Private Type PROCESS_INFORMATION
    hProcess As Long
    hThread As Long
    dwProcessID As Long
    dwThreadID As Long
End Type

Private Type SECURITY_ATTRIBUTES
          nLength As Long
          lpSecurityDescriptor As Long
          bInheritHandle As Long
End Type

Public Type OVERLAPPED
        Internal As Long
        InternalHigh As Long
        offset As Long
        OffsetHigh As Long
        hEvent As Long
End Type
    
Private Declare Function WaitForSingleObject Lib "kernel32" (ByVal _
                    hHandle As Long, ByVal dwMilliseconds As Long) As Long

Private Declare Function CreateProcessA Lib "kernel32" (ByVal _
                    lpApplicationName As String, ByVal lpCommandLine As String, ByVal _
                    lpProcessAttributes As Long, ByVal lpThreadAttributes As Long, _
                    ByVal bInheritHandles As Long, ByVal dwCreationFlags As Long, _
                    ByVal lpEnvironment As Long, ByVal lpCurrentDirectory As String, _
                    lpStartupInfo As STARTUPINFO, lpProcessInformation As _
                    PROCESS_INFORMATION) As Long

Private Declare Function CloseHandle Lib "kernel32" _
                  (ByVal hObject As Long) As Long

Private Declare Function GetExitCodeProcess Lib "kernel32" _
                    (ByVal hProcess As Long, lpExitCode As Long) As Long

Private Declare Function CreatePipe Lib "kernel32" _
                    (phReadPipe As Long, phWritePipe As Long, _
                     lpPipeAttributes As Any, ByVal nSize As Long) As Long

Public Declare Function CreateFile Lib "kernel32" Alias "CreateFileA" _
                    (ByVal lpFileName As String, ByVal dwDesiredAccess As Long, _
                     ByVal dwShareMode As Long, lpSecurityAttributes As SECURITY_ATTRIBUTES, _
                     ByVal dwCreationDisposition As Long, ByVal dwFlagsAndAttributes As Long, _
                     ByVal hTemplateFile As Long) As Long
                     
Public Declare Function DeleteFile Lib "kernel32" Alias "DeleteFileA" _
                    (ByVal lpFileName As String) As Long

Private Declare Function ReadFile Lib "kernel32" _
                    (ByVal hFile As Long, ByVal lpBuffer As String, _
                     ByVal nNumberOfBytesToRead As Long, lpNumberOfBytesRead As Long, _
                     ByVal lpOverlapped As Any) As Long

Public Declare Function GetTempFileName Lib "kernel32" Alias "GetTempFileNameA" _
                    (ByVal lpszPath As String, ByVal lpPrefixString As String, _
                     ByVal wUnique As Long, ByVal lpTempFileName As String) As Long

Public Declare Function GetFileSize Lib "kernel32" _
                    (ByVal hFile As Long, lpFileSizeHigh As Long) As Long
                    
Public Declare Function SetFilePointer Lib "kernel32" _
                    (ByVal hFile As Long, ByVal lDistanceToMove As Long, _
                     lpDistanceToMoveHigh As Long, ByVal dwMoveMethod As Long) As Long
                    


Private Const NORMAL_PRIORITY_CLASS = &H20&
Private Const STARTF_USESTDHANDLES = &H100&
Private Const INFINITE = -1&

Public Const GENERIC_WRITE = &H40000000
Public Const GENERIC_READ = &H80000000

Public Const FILE_SHARE_READ = &H1
Public Const FILE_SHARE_WRITE = &H2
Public Const FILE_SHARE_DELETE = &H4

Public Const CREATE_ALWAYS = 2

Public Const INVALID_HANDLE_VALUE = -1

Public Const FILE_FLAG_DELETE_ON_CLOSE = &H4000000
Public Const FILE_FLAG_OVERLAPPED = &H40000000

Public Const FILE_ATTRIBUTE_NORMAL = &H80
Public Const FILE_ATTRIBUTE_TEMPORARY = &H100

Public Const FILE_BEGIN = 0

Public Const MAX_PATH = 260

'my own constants
Private Const STDOUT_PIPE = 1
Private Const STDOUT_IN_PIPE = 2
Private Const STDERR_PIPE = 3
Private Const STDERR_IN_PIPE = 4

Private Const READ_BUFF_SIZE = 256


'
' Pending issue here: waiting for the called process is infinite
' The output is intercepted only if doWait and doOutput are set to true
'
Public Function ExecCmd(cmdline$, doWait As Boolean, doOutput As Boolean) As ProcessOutput
    Dim ret As Long
    Dim res As ProcessOutput
    Dim ctx As ProcessContext
    '
    Dim proc As PROCESS_INFORMATION
    Dim start As STARTUPINFO
    '
    Dim doPipes As Boolean
    '
    Dim cnt As Long, atpt As Long
    Dim buff As String * READ_BUFF_SIZE
     
    doPipes = (doWait = True) And (doOutput = True)
    ctx.outName = String(MAX_PATH, Chr$(0))
    
    ' Initialize the STARTUPINFO structure:
    start.cb = Len(start)
    
    If doPipes Then
        OpenContext ctx, True
        start.dwFlags = STARTF_USESTDHANDLES
        start.hStdError = ctx.outHnd
    End If
    
    ' Start the shelled application:
    ret& = CreateProcessA(vbNullString, cmdline$, 0&, 0&, 1&, _
         NORMAL_PRIORITY_CLASS, 0&, vbNullString, start, proc)

    If ret <> 1 Then
        CloseContext ctx, False
        err.Raise 12001, , "CreateProcess failed. Error: " & err.LastDllError
    End If
        
    ' Wait for the shelled application to finish:
    If doWait = True Then
        ret& = WaitForSingleObject(proc.hProcess, INFINITE)
        Call GetExitCodeProcess(proc.hProcess, res.ret&)
        
        Call CloseHandle(proc.hThread)
        Call CloseHandle(proc.hProcess)
        
        'read the dump of the stderr, non-blocking
        If doPipes Then
            ret& = GetFileSize(ctx.outHnd, 0)
            'we consider that not being able to read the stderr log is not a fatal error
            If ret <> -1 Then
                ret = SetFilePointer(ctx.outHnd, 0, 0, FILE_BEGIN)
                If ret = 0 Then
                    Do
                        ret = ReadFile(ctx.outHnd, buff, READ_BUFF_SIZE, cnt, 0&)
                        res.err = res.err & Left$(buff, cnt)
                    Loop While ret <> 0 And cnt <> 0
                Else
                    MsgBox "SetFilePointer failed. Error: " & err.LastDllError & " " & err.Description
                End If
            Else
                MsgBox "GetFileSize failed. Error: " & err.LastDllError & " " & err.Description
            End If
            
            CloseContext ctx, False
        End If
                
    End If
    
    ExecCmd = res
End Function

Private Sub OpenContext(ByRef Context As ProcessContext, doError As Boolean)

    Dim sa As SECURITY_ATTRIBUTES
    Dim msg As String
    
    ' Initialize the SECURITY_ATRIBUTES info
    sa.nLength = Len(sa)
    sa.bInheritHandle = 1&
    sa.lpSecurityDescriptor = 0&
   
    ret& = GetTempFileName(".", "err", 0, Context.outName)
    If ret = 0 Then
        msg = "GetTempFileName failed for stderr. Error: " & err.LastDllError
        If doError = True Then
            err.Raise 12000, , msg
        Else
            MsgBox msg
        End If
    End If
        
    Context.outHnd = CreateFile(Context.outName, _
                                GENERIC_WRITE Or GENERIC_READ, _
                                FILE_SHARE_WRITE Or FILE_SHARE_READ Or FILE_SHARE_DELETE, _
                                sa, CREATE_ALWAYS, _
                                FILE_ATTRIBUTE_TEMPORARY, _
                                FILE_FLAG_DELETE_ON_CLOSE)
                                
    If Context.outHnd = INVALID_HANDLE_VALUE Then
        msg = "CreateFile failed for stderr. Error: " & err.LastDllError
        If doError = True Then
            err.Raise 12000, , msg
        Else
            MsgBox msg
        End If
    End If
    
End Sub

Private Sub CloseContext(ByRef Context As ProcessContext, doError As Boolean)
    Dim msg As String
    If Context.outHnd <> 0 Then
        CloseHandle Context.outHnd
    End If
    If Not IsNull(Context.outName) Then
        'delete the file
        ret = DeleteFile(Context.outName)
        If ret = 0 Then
            msg = "DeleteFile failed. Error: " & err.LastDllError & Chr(13) & _
                        "You have a temporary file at " & pathBuff
            If doError = True Then
                err.Raise 12000, , msg
            Else
                MsgBox msg
            End If
        End If
    End If
End Sub

Private Sub ClosePipes(ByRef Pipes() As Long, Size As Integer)
    Dim i As Integer
    For i = 0 To Size
        ClosePipe Pipes, i
    Next i
End Sub

Private Sub ClosePipe(ByRef Pipes() As Long, Pos As Integer)
    
    If Pipes(Pos) <> 0 Then
        CloseHandle Pipes(Pos)
        Pipes(Pos) = 0
    End If
End Sub

