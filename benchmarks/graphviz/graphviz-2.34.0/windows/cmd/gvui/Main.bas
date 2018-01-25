Attribute VB_Name = "Main"
'
' Constants used here and there
'
Public Const GRAPH As Integer = &H1
Public Const NODE As Integer = &H2
Public Const EDGE As Integer = &H4
Public Const SUBGRAPH As Integer = &H8
Public Const CLUSTER As Integer = &H10
Public Const ANY_ELEMENT As Integer = GRAPH Or NODE Or EDGE Or SUBGRAPH Or CLUSTER

Public Const DOT As Integer = &H1
Public Const NEATO As Integer = &H2
Public Const TWOPI As Integer = &H4
Public Const CIRCO As Integer = &H8
Public Const FDP As Integer = &H16
Public Const ALL_ENGINES As Integer = DOT Or NEATO Or TWOPI or CIRCO or FDP
