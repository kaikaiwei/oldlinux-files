.\"#! troff -Q -ms $1
.\" This is the "Xterm Control Sequences" document, originally written by
.\" Edward Moy, University of California, Berkeley, for the X.V10R4 xterm.
.\" Some minor edits have been made to begin to reconcile this document with
.\" the current sources, but it still has a long way to go:
.\"
.\"	1) I don't guarantee that this is 100% correct.  I tried most of
.\"	   the things that seemed to be different, and this document
.\"	   reflects what I saw.  Stuff that appears to be in the X10R4
.\"	   document and missing from this document is so because it
.\"	   appears not to be present in the X11 version of "xterm" (e.g.,
.\"	   the "Alternate Character ROM" character sets and the Sun TTY
.\"	   emulation).
.\"
.\"	2) It is definitely not 100% complete; some escape sequences
.\"	   that do something that's either unobservable - at least in
.\"	   the current state of the code - or not useful aren't
.\"	   documented.  An example of the former are the Locking Shift
.\"	   sequences that modify the interpretation of the GR
.\"	   characters; at present, it sets a state variable that's
.\"	   unused, but perhaps some future version will use it (perhaps
.\"	   for displaying characters in the range 0200-0377?).  An
.\"	   example of the latter is the sequence to set DECANM mode
.\"	   (ANSI/VT52 mode): it doesn't do what it does on a VT100,
.\"	   namely get the terminal to emulate a VT100, it only seems to
.\"	   fiddle the current character set a bit.
.\"
.\"	3) It doesn't document any of the mouse-related stuff, such as
.\"	   what the "Track Mouse" escape sequence does, or what the
.\"	   different mouse modes (MIT, VT220, "VT220 Hilite") do; I
.\"	   leave that to somebody familiar with that part of the code....
.\"
.\" Run this file through troff and use the -ms macro package.
.ND
.de St
.sp
.nr PD 0
.nr PI 1.5i
.nr VS 16
..
.de Ed
.nr PD .3v
.nr VS 12
..
.rm CH
.ds LH Xterm Control Sequences
.nr s 6*\n(PS/10
.ds L \s\nsBEL\s0
.ds E \s\nsESC\s0
.ds T \s\nsTAB\s0
.ds X \s\nsETX\s0
.ds N \s\nsENQ\s0
.ds e \s\nsETB\s0
.ds C \s\nsCAN\s0
.ds S \s\nsSUB\s0
.nr [W \w'\*L'u
.nr w \w'\*E'u
.if \nw>\n([W .nr [W \nw
.nr w \w'\*T'u
.if \nw>\n([W .nr [W \nw
.nr w \w'\*X'u
.if \nw>\n([W .nr [W \nw
.nr w \w'\*N'u
.if \nw>\n([W .nr [W \nw
.nr w \w'\*e'u
.if \nw>\n([W .nr [W \nw
.nr w \w'\*C'u
.if \nw>\n([W .nr [W \nw
.nr w \w'\*S'u
.if \nw>\n([W .nr [W \nw
.nr [W +\w'\|\|'u
.de []
.nr w \w'\\$2'
.nr H \\n([Wu-\\nwu
.nr h \\nHu/2u
.ds \\$1 \(br\v'-1p'\(br\v'1p'\h'\\nhu'\\$2\h'\\nHu-\\nhu'\(br\l'-\\n([Wu\(ul'\v'-1p'\(br\l'-\\n([Wu\(rn'\v'1p'\|
..
.[] Et \v'-1p'\*X\v'1p'
.[] En \v'-1p'\*N\v'1p'
.[] Be \v'-1p'\*L\v'1p'
.[] Bs \v'-1p'\s\nsBS\s0\v'1p'
.[] Ta \v'-1p'\*T\v'1p'
.[] Lf \v'-1p'\s\nsLF\s0\v'1p'
.[] Vt \v'-1p'\s\nsVT\s0\v'1p'
.[] Ff \v'-1p'\s\nsFF\s0\v'1p'
.[] Cr \v'-1p'\s\nsCR\s0\v'1p'
.[] So \v'-1p'\s\nsSO\s0\v'1p'
.[] Si \v'-1p'\s\nsSI\s0\v'1p'
.[] Eb \v'-1p'\*e\v'1p'
.[] Ca \v'-1p'\*C\v'1p'
.[] Su \v'-1p'\*S\v'1p'
.[] Es \v'-1p'\*E\v'1p'
.[] Fs \v'-1p'\s\nsFS\s0\v'1p'
.[] Gs \v'-1p'\s\nsGS\s0\v'1p'
.[] Rs \v'-1p'\s\nsRS\s0\v'1p'
.[] Us \v'-1p'\s\nsUS\s0\v'1p'
.[] # #
.[] (( (
.[] ) )
.[] * *
.[] + +
.[] 0 0
.[] 1 1
.[] 2 2
.[] 3 3
.[] 4 4
.[] 5 5
.[] 6 6
.[] 7 7
.[] 8 8
.[] 9 9
.[] : :
.[] ; ;
.[] = =
.[] > >
.[] ? ?
.[] @ @
.[] A A
.[] cB B
.[] C C
.[] D D
.[] E E
.[] F F
.[] H H
.[] J J
.[] K K
.[] L L
.[] M M
.[] N N
.[] O O
.[] P P
.[] R R
.[] S S
.[] T T
.[] Z Z
.[] [[ [
.[] ] ]
.[] ` \`
.[] a a
.[] b b
.[] c c
.[] d d
.[] f f
.[] g g
.[] h h
.[] i i
.[] j j
.[] k k
.[] l l
.[] m m
.[] n n
.[] o o
.[] p p
.[] q q
.[] r r
.[] s s
.[] t t
.[] x x
.[] | |
.[] } }
.[] c~ ~
.ds Cc \fIC\fP
.ds Ps \fIP\v'.3m'\h'-.2m'\s-2s\s0\v'-.3m'\fP
.ds Pm \fIP\v'.3m'\h'-.2m'\s-2m\s0\v'-.3m'\fP
.ds Pt \fIP\v'.3m'\h'-.2m'\s-2t\s0\v'-.3m'\fP
.ds Ix \fIx\fP
.ds Iy \fIy\fP
.ds Iw \fIw\fP
.ds Ih \fIh\fP
.ds Ir \fIr\fP
.ds Ic \fIc\fP
.nr LL 6.5i
.TL
Xterm Control Sequences
.am NP
.ds CF %
..
.SH
Definitions
.IP \*(Cc
A single (required) character.
.IP \*(Ps
A single (usually optional) numeric parameter, composed of one of more digits.
.IP \*(Pm
A multiple numeric parameter composed of any number of single numeric
parameters, separated by \*; character(s).
.IP \*(Pt
A text parameter composed of printable characters.
.SH
VT102 Mode
.ds RH VT102 Mode
.LP
Most of these control sequences are standard VT102 control sequences.
There are, however, additional ones to provide control of
.I xterm
dependent functions, like the scrollbar or window size.
.St
.IP \\*(Be
Bell (Ctrl-G)
.IP \\*(Bs
Backspace (Ctrl-H)
.IP \\*(Ta
Horizontal Tab (Ctrl-I)
.IP \\*(Lf
Line Feed or New Line (Ctrl-J)
.IP \\*(Vt
Vertical Tab (Ctrl-K)
.IP \\*(Ff
Form Feed or New Page (Ctrl-L)
.IP \\*(Cr
Carriage Return (Ctrl-M)
.IP \\*(So
Shift Out (Ctrl-N) \(-> Switch to Alternate Character Set
.IP \\*(Si
Shift In (Ctrl-O) \(-> Switch to Standard Character Set
.IP \\*(Es\\*#\\*8
DEC Screen Alignment Test (DECALN)
.IP \\*(Es\\*(((\\*(Cc
Select G0 Character Set (SCS)
  \*(Cc = \*0 \(-> Special Character and Line Drawing Set
  \*(Cc = \*A \(-> United Kingdom (UK)
  \*(Cc = \*(cB \(-> United States (USASCII)
.IP \\*(Es\\*)\\*(Cc
Select G1 Character Set (SCS)
  \*(Cc = \*0 \(-> Special Character and Line Drawing Set
  \*(Cc = \*A \(-> United Kingdom (UK)
  \*(Cc = \*(cB \(-> United States (USASCII)
.IP \\*(Es\\**\\*(Cc
Select G2 Character Set (SCS)
  \*(Cc = \*0 \(-> Special Character and Line Drawing Set
  \*(Cc = \*A \(-> United Kingdom (UK)
  \*(Cc = \*(cB \(-> United States (USASCII)
.IP \\*(Es\\*+\\*(Cc
Select G3 Character Set (SCS)
  \*(Cc = \*0 \(-> Special Character and Line Drawing Set
  \*(Cc = \*A \(-> United Kingdom (UK)
  \*(Cc = \*(cB \(-> United States (USASCII)
.IP \\*(Es\\*7
Save Cursor (DECSC)
.IP \\*(Es\\*8
Restore Cursor (DECRC)
.IP \\*(Es\\*=
Application Keypad (DECPAM)
.IP \\*(Es\\*>
Normal Keypad (DECPNM)
.IP \\*(Es\\*D
Index (IND)
.IP \\*(Es\\*E
Next Line (NEL)
.IP \\*(Es\\*H
Tab Set (HTS)
.IP \\*(Es\\*M
Reverse Index (RI)
.IP \\*(Es\\*N
Single Shift Select of G2 Character Set (SS2)
.IP \\*(Es\\*O
Single Shift Select of G3 Character Set (SS3)
.IP \\*(Es\\*Z
Return Terminal ID (DECID)
.IP \\*(Es\\*([[\\*(Ps\|\\*@
Insert \*(Ps (Blank) Character(s) (default = 1) (ICH)
.IP \\*(Es\\*([[\\*(Ps\|\\*A
Cursor Up \*(Ps Times (default = 1) (CUU)
.IP \\*(Es\\*([[\\*(Ps\|\\*(cB
Cursor Down \*(Ps Times (default = 1) (CUD)
.IP \\*(Es\\*([[\\*(Ps\|\\*C
Cursor Forward \*(Ps Times (default = 1) (CUF)
.IP \\*(Es\\*([[\\*(Ps\|\\*D
Cursor Backward \*(Ps Times (default = 1) (CUB)
.IP \\*(Es\\*([[\\*(Ps\|\\*;\\*(Ps\|\\*H
Cursor Position [row;column] (default = [1,1]) (CUP)
.IP \\*(Es\\*([[\\*(Ps\|\\*J
Erase in Display (ED)
  \*(Ps = \*0 \(-> Clear Below (default)
  \*(Ps = \*1 \(-> Clear Above
  \*(Ps = \*2 \(-> Clear All
.IP \\*(Es\\*([[\\*(Ps\|\\*K
Erase in Line (EL)
  \*(Ps = \*0 \(-> Clear to Right (default)
  \*(Ps = \*1 \(-> Clear to Left
  \*(Ps = \*2 \(-> Clear All
.IP \\*(Es\\*([[\\*(Ps\|\\*L
Insert \*(Ps Line(s) (default = 1) (IL)
.IP \\*(Es\\*([[\\*(Ps\|\\*M
Delete \*(Ps Line(s) (default = 1) (DL)
.IP \\*(Es\\*([[\\*(Ps\|\\*P
Delete \*(Ps Character(s) (default = 1) (DCH)
.IP \\*(Es\\*([[\\*(Ps\|\\*;\\*(Ps\|\\*;\\*(Ps\|\\*;\\*(Ps\|\\*;\\*(Ps\|\\*T
Track Mouse [func;startcol;startrow;firstrow;lastrow] XXX - describe
.IP \\*(Es\\*([[\\*(Ps\|\\*c
Device Attributes (default 0) (DA)
.IP \\*(Es\\*([[\\*(Ps\|\\*;\\*(Ps\|\\*f
Horizontal and Vertical Position [row;column] (default = [1,1]) (HVP)
.IP \\*(Es\\*([[\\*(Ps\|\\*g
Tab Clear
  \*(Ps = \*0 \(-> Clear Current Column (default)
  \*(Ps = \*3 \(-> Clear All
.IP \\*(Es\\*([[\\*(Ps\|\\*h
Mode Set (SET)
  \*(Ps = \*4 \(-> Insert Mode (IRM)
  \*(Ps = \*2\*0 \(-> Automatic Linefeed (LNM)
.IP \\*(Es\\*([[\\*(Ps\|\\*l
Mode Reset (RST)
  \*(Ps = \*4 \(-> Insert Mode (IRM)
  \*(Ps = \*2\*0 \(-> Automatic Linefeed (LNM)
.IP \\*(Es\\*([[\\*(Pm\|\\*m
Character Attributes (SGR)
  \*(Ps = \*0 \(-> Normal (default)
  \*(Ps = \*1 \(-> Blink (appears as Bold)
  \*(Ps = \*4 \(-> Underscore
  \*(Ps = \*5 \(-> Bold
  \*(Ps = \*7 \(-> Inverse
.IP \\*(Es\\*([[\\*(Ps\|\\*n
Device Status Report (DSR)
  \*(Ps = 5 \(-> Status Report \*(Es\*([[\*0\*n \(-> OK
  \*(Ps = 6 \(-> Report Cursor Position (CPR) [row;column] as
\*(Es\*([[\*(Ir\|\*;\*(Ic\|\*R
.IP \\*(Es\\*([[\\*(Ps\|\\*;\\*(Ps\|\\*r
Set Scrolling Region [top;bottom] (default = full size of window) (DECSTBM)
.IP \\*(Es\\*([[\\*(Ps\|\\*x
Request Terminal Parameters (DECREQTPARM)
.IP \\*(Es\\*([[\\*?\\*(Ps\|\\*h
DEC Private Mode Set (DECSET)
  \*(Ps = \*1 \(-> Application Cursor Keys (DECCKM)
  \*(Ps = \*3 \(-> 132 Column Mode (DECCOLM)
  \*(Ps = \*4 \(-> Smooth (Slow) Scroll (DECSCLM)
  \*(Ps = \*5 \(-> Reverse Video (DECSCNM)
  \*(Ps = \*6 \(-> Origin Mode (DECOM)
  \*(Ps = \*7 \(-> Wraparound Mode (DECAWM)
  \*(Ps = \*8 \(-> Auto-repeat Keys (DECARM)
  \*(Ps = \*9 \(-> Send MIT Mouse Row & Column on Button Press
  \*(Ps = \*3\*8 \(-> Enter Tektronix Mode (DECTEK)
  \*(Ps = \*4\*0 \(-> Allow 80 \z\(<-\(-> 132 Mode
  \*(Ps = \*4\*1 \(-> \fIcurses\fP(5) fix
  \*(Ps = \*4\*4 \(-> Turn On Margin Bell
  \*(Ps = \*4\*5 \(-> Reverse-wraparound Mode
  \*(Ps = \*4\*6 \(-> Start Logging
  \*(Ps = \*4\*7 \(-> Use Alternate Screen Buffer
  \*(Ps = \*1\*0\*0\*0 \(-> Send VT200 Mouse Row & Column on Button Press
  \*(Ps = \*1\*0\*0\*3 \(-> Send VT200 Hilite Mouse Row & Column on Button Press
.IP \\*(Es\\*([[\\*?\\*(Ps\|\\*l
DEC Private Mode Reset (DECRST)
  \*(Ps = \*1 \(-> Normal Cursor Keys (DECCKM)
  \*(Ps = \*3 \(-> 80 Column Mode (DECCOLM)
  \*(Ps = \*4 \(-> Jump (Fast) Scroll (DECSCLM)
  \*(Ps = \*5 \(-> Normal Video (DECSCNM)
  \*(Ps = \*6 \(-> Normal Cursor Mode (DECOM)
  \*(Ps = \*7 \(-> No Wraparound Mode (DECAWM)
  \*(Ps = \*8 \(-> No Auto-repeat Keys (DECARM)
  \*(Ps = \*9 \(-> Don't Send Mouse Row & Column on Button Press
  \*(Ps = \*4\*0 \(-> Disallow 80 \z\(<-\(-> 132 Mode
  \*(Ps = \*4\*1 \(-> No \fIcurses\fP(5) fix
  \*(Ps = \*4\*4 \(-> Turn Off Margin Bell
  \*(Ps = \*4\*5 \(-> No Reverse-wraparound Mode
  \*(Ps = \*4\*6 \(-> Stop Logging
  \*(Ps = \*4\*7 \(-> Use Normal Screen Buffer
  \*(Ps = \*1\*0\*0\*0 \(-> Don't Send Mouse Row & Column on Button Press
  \*(Ps = \*1\*0\*0\*3 \(-> Don't Send Mouse Row & Column on Button Press
.IP \\*(Es\\*([[\\*?\\*(Ps\|\\*r
Restore DEC Private Mode
  \*(Ps = \*1 \(-> Normal/Application Cursor Keys (DECCKM)
  \*(Ps = \*3 \(-> 80/132 Column Mode (DECCOLM)
  \*(Ps = \*4 \(-> Jump (Fast)/Smooth (Slow) Scroll (DECSCLM)
  \*(Ps = \*5 \(-> Normal/Reverse Video (DECSCNM)
  \*(Ps = \*6 \(-> Normal/Origin Cursor Mode (DECOM)
  \*(Ps = \*7 \(-> No Wraparound/Wraparound Mode (DECAWM)
  \*(Ps = \*8 \(-> Auto-repeat/No Auto-repeat Keys (DECARM)
  \*(Ps = \*9 \(-> Don't Send/Send MIT Mouse Row & Column on Button Press
  \*(Ps = \*4\*0 \(-> Disallow/Allow 80 \z\(<-\(-> 132 Mode
  \*(Ps = \*4\*1 \(-> Off/On \fIcurses\fP(5) fix
  \*(Ps = \*4\*4 \(-> Turn Off/On Margin Bell
  \*(Ps = \*4\*5 \(-> No Reverse-wraparound/Reverse-wraparound Mode
  \*(Ps = \*4\*6 \(-> Stop/Start Logging
  \*(Ps = \*4\*7 \(-> Use Normal/Alternate Screen Buffer
  \*(Ps = \*1\*0\*0\*0 \(-> Don't Send/Send VT220 Mouse Row & Column on Button Press
  \*(Ps = \*1\*0\*0\*3 \(-> Don't Send/Send VT220 Hilite Mouse Row & Column on Button Press
.IP \\*(Es\\*([[\\*?\\*(Ps\|\\*s
Save DEC Private Mode
  \*(Ps = \*1 \(-> Normal/Application Cursor Keys (DECCKM)
  \*(Ps = \*3 \(-> 80/132 Column Mode (DECCOLM)
  \*(Ps = \*4 \(-> Jump (Fast)/Smooth (Slow) Scroll (DECSCLM)
  \*(Ps = \*5 \(-> Normal/Reverse Video (DECSCNM)
  \*(Ps = \*6 \(-> Normal/Origin Cursor Mode (DECOM)
  \*(Ps = \*7 \(-> No Wraparound/Wraparound Mode (DECAWM)
  \*(Ps = \*8 \(-> Auto-repeat/No Auto-repeat Keys (DECARM)
  \*(Ps = \*9 \(-> Don't Send/Send MIT Mouse Row & Column on Button Press
  \*(Ps = \*4\*0 \(-> Disallow/Allow 80 \z\(<-\(-> 132 Mode
  \*(Ps = \*4\*1 \(-> Off/On \fIcurses\fP(5) fix
  \*(Ps = \*4\*4 \(-> Turn Off/On Margin Bell
  \*(Ps = \*4\*5 \(-> No Reverse-wraparound/Reverse-wraparound Mode
  \*(Ps = \*4\*6 \(-> Stop/Start Logging
  \*(Ps = \*4\*7 \(-> Use Normal/Alternate Screen Buffer
  \*(Ps = \*1\*0\*0\*0 \(-> Don't Send/Send VT220 Mouse Row & Column on Button Press
  \*(Ps = \*1\*0\*0\*3 \(-> Don't Send/Send VT220 Hilite Mouse Row & Column on Button Press
.IP \\*(Es\\*]\\*(Ps\|\\*;\\*(Pt\|\\*(Be
Set Text Parameters
  \*(Ps = \*0 \(-> Change Icon Name and Window Title to \*(Pt
  \*(Ps = \*1 \(-> Change Icon Name to \*(Pt
  \*(Ps = \*2 \(-> Change Window Title to \*(Pt
  \*(Ps = \*4\*6 \(-> Change Log File to \*(Pt
.IP \\*(Es\\*c
Full Reset (RIS)
.IP \\*(Es\\*n
Locking Shift Select of G2 Character Set (LS2)
.IP \\*(Es\\*o
Locking Shift Select of G3 Character Set (LS3)
.Ed
.SH
Tektronix 4015 Mode
.ds RH Tektronix 4015 Mode
.LP
Most of these sequences are standard Tektronix 4015 control sequences.
The major features missing are the alternate (APL) character set and
the write-thru and defocused modes.
.St
.IP \\*(Be
Bell (Ctrl-G)
.IP \\*(Bs
Backspace (Ctrl-H)
.IP \\*(Ta
Horizontal Tab (Ctrl-I)
.IP \\*(Lf
Line Feed or New Line (Ctrl-J)
.IP \\*(Vt
Vertical Tab (Ctrl-K)
.IP \\*(Ff
Form Feed or New Page (Ctrl-L)
.IP \\*(Cr
Carriage Return (Ctrl-M)
.IP \\*(Es\\*(Et
Switch to VT102 Mode
.IP \\*(Es\\*(En
Return Terminal Status
.IP \\*(Es\\*(Lf
PAGE (Clear Screen)
.IP \\*(Es\\*(Eb
COPY (Save Tektronix Codes to File)
.IP \\*(Es\\*(Ca
Bypass Condition
.IP \\*(Es\\*(Su
GIN mode
.IP \\*(Es\\*(Fs
Special Point Plot Mode
.IP \\*(Es\\*(Gs
Graph Mode (same as \*(Gs)
.IP \\*(Es\\*(Rs
Incremental Plot Mode (same as \*(Rs)
.IP \\*(Es\\*(Us
Alpha Mode (same as \*(Us)
.IP \\*(Es\\*8
Select Large Character Set
.IP \\*(Es\\*9
Select #2 Character Set
.IP \\*(Es\\*:
Select #3 Character Set
.IP \\*(Es\\*;
Select Small Character Set
.IP \\*(Es\\*]\\*(Ps\|\\*;\\*(Pt\|\\*(Be
Set Text Parameters
  \*(Ps = \*0 \(-> Change Icon Name and Window Title to \*(Pt
  \*(Ps = \*1 \(-> Change Icon Name to \*(Pt
  \*(Ps = \*2 \(-> Change Window Title to \*(Pt
  \*(Ps = \*4\*6 \(-> Change Log File to \*(Pt
.IP \\*(Es\\*`
Normal Z Axis and Normal (solid) Vectors
.IP \\*(Es\\*a
Normal Z Axis and Dotted Line Vectors
.IP \\*(Es\\*b
Normal Z Axis and Dot-Dashed Vectors
.IP \\*(Es\\*c
Normal Z Axis and Short-Dashed Vectors
.IP \\*(Es\\*d
Normal Z Axis and Long-Dashed Vectors
.IP \\*(Es\\*h
Defocused Z Axis and Normal (solid) Vectors
.IP \\*(Es\\*i
Defocused Z Axis and Dotted Line Vectors
.IP \\*(Es\\*j
Defocused Z Axis and Dot-Dashed Vectors
.IP \\*(Es\\*k
Defocused Z Axis and Short-Dashed Vectors
.IP \\*(Es\\*l
Defocused Z Axis and Long-Dashed Vectors
.IP \\*(Es\\*p
Write-Thru Mode and Normal (solid) Vectors
.IP \\*(Es\\*q
Write-Thru Mode and Dotted Line Vectors
.IP \\*(Es\\*r
Write-Thru Mode and Dot-Dashed Vectors
.IP \\*(Es\\*s
Write-Thru Mode and Short-Dashed Vectors
.IP \\*(Es\\*t
Write-Thru Mode and Long-Dashed Vectors
.IP \\*(Fs
Point Plot Mode
.IP \\*(Gs
Graph Mode
.IP \\*(Rs
Incremental Plot Mode
.IP \\*(Us
Alpha Mode
.Ed
