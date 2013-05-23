    1 COUT     EQU   $FDED
    2 WAIT     EQU   $FCA8
    3 KEYBD    EQU   $C000
    4 KBCLR    EQU   $C010
    5 PTR      EQU   $06
    6 PTRL     EQU   $06
    7 PTRH     EQU   $07
    8 MOUROM   EQU   $300
    9 SSCROM   EQU   $301
   10 MOUSLOT  EQU   $302
   11 SSCSLOT  EQU   $303
   12 MOUINDEX EQU   $304
   13          ORG   $1000
   14 ENTRY    LDA   #$C1
   15          STA   PTRH
   16          LDA   #$00
   17          STA   PTRL
   18          STA   MOUROM
   19          STA   SSCROM
   20          STA   MOUSLOT
   21          STA   SSCSLOT
   22          STA   MOUINDEX
   23 *
   24 * SEARCH SLOTS FOR MOUSE AND SUPERSERIAL CARDS
   25 *
   26 IDCARD   LDY   #$05
   27          LDA   (PTR),Y
   28          CMP   #$38       ; PASCAL ID BYTE
   29          BNE   NEXTCARD
   30          LDY   #$07
   31          LDA   (PTR),Y
   32          CMP   #$18
   33          BNE   NEXTCARD   ; PASCAL ID BYTE
   34 ISMOU    LDY   #$0C
   35          LDA   (PTR),Y
   36          CMP   #$20
   37          BNE   ISSSC
   38          LDY   #$FB
   39          LDA   (PTR),Y
   40          CMP   #$D6
   41          BNE   ISSSC
   42          LDA   PTRH
   43          STA   MOUROM
   44          AND   #$07
   45          STA   MOUINDEX
   46          ASL
   47          ASL
   48          ASL
   49          ASL
   50          STA   MOUSLOT
   51          BNE   NEXTCARD
   52 ISSSC    LDY   #$0C
   53          LDA   (PTR),Y
   54          CMP   #$31
   55          BNE   NEXTCARD
   56          LDA   PTRH
   57          STA   SSCROM
   58          ASL
   59          ASL
   60          ASL
   61          ASL
   62          STA   SSCSLOT
   63 NEXTCARD INC   PTRH
   64          LDA   PTRH
   65          CMP   #$C8
   66          BNE   IDCARD
   67 *
   68 * INIT MOUSE
   69 *
   70 INITMOU  LDY   MOUINDEX
   71          BEQ   INITSSC
   72          LDY   #$19       ; INIT MOUSE
   73          JSR   CALLMOU
   74          LDY   #$12       ; SET MOUSE
   75          LDA   #$01       ; TRANSPARENT MODE
   76          JSR   CALLMOU
   77          LDA   #$00
   78          STA   $478
   79          STA   $4F8
   80          LDA   #$C0
   81          STA   $578
   82          LDA   #$3F
   83          STA   $5F8
   84          LDY   #$17       ; CLAMP MOUSE
   85          LDA   #$00       ; X AXIS
   86          JSR   CALLMOU
   87          LDY   #$17       ; CLAMP MOUSE
   88          LDA   #$01       ; YAXIS
   89          JSR   CALLMOU
   90 *
   91 * INIT SSC CARD FOR 115K BAUD
   92 *
   93 INITSSC  LDY   SSCSLOT
   94          BNE   SETSSC
   95          JMP   EXIT
   96 SETSSC   LDA   #$0B
   97          STA   $C08A,Y
   98          LDA   #$10
   99          STA   $C08B,Y    ; 115K
  100 *
  101 * SYNCHRONIZE WITH PI
  102 *
  103 SYNC     LDA   #$80
  104          STA   $C088,Y
  105          ORA   #'.'
  106          JSR   COUT
  107          LDA   #$FF
  108          JSR   WAIT
  109          LDA   KEYBD
  110          BMI   EXIT
  111          LDY   SSCSLOT
  112          LDA   $C089,Y
  113          AND   #$08
  114          BEQ   SYNC
  115          LDA   $C088,Y
  116          CMP   #$81
  117          BNE   SYNC
  118 *
  119 * CHECK INPUT STATE FOR CHANGE
  120 *
  121 EVENTLP  LDX   KEYBD
  122          BMI   SENDKEY
  123          DEC   WAITMOU
  124          BNE   EVENTLP
  125 CHKMOU   LDY   #$14       ; READ MOUSE
  126          JSR   CALLMOU
  127          BCS   EVENTLP
  128          LDY   MOUINDEX
  129          LDA   $478,Y
  130          STA   MOUXSTAT
  131          LDA   $4F8,Y
  132          STA   MOUYSTAT
  133          LDY   #$15       ; CLEAR MOUSE
  134          JSR   CALLMOU
  135          LDA   MOUXSTAT
  136          ORA   MOUYSTAT
  137          BNE   SENDMOU
  138 CHKBTTN  LDY   MOUINDEX
  139          LDA   $778,Y
  140          AND   #$80
  141          CMP   BTTNSTAT
  142          BEQ   EVENTLP
  143          JMP   SENDBTTN
  144 SENDKEY  STX   KBCLR
  145          LDY   SSCSLOT
  146          LDA   #$82       ; KBEVENT CODE
  147          JSR   SENDACC
  148          JSR   SENDMOD
  149          TXA
  150          JSR   SENDACC
  151          CPX   #$9B       ; ESC KEY
  152          BNE   EVENTLP
  153          LDA   $C062
  154          BPL   EVENTLP
  155 EXIT     LDA   KBCLR
  156          LDY   #$12       ; SET MOUSE
  157          LDA   #$00       ; DISABLE MOUSE
  158          JMP   CALLMOU
  159 SENDMOU  LDY   SSCSLOT
  160          LDA   #$84
  161          JSR   SENDACC
  162          LDA   MOUXSTAT
  163          JSR   SENDACC
  164          LDA   MOUYSTAT
  165          JSR   SENDACC
  166          JMP   CHKBTTN
  167 SENDBTTN STA   BTTNSTAT
  168          LDY   SSCSLOT
  169          LDA   #$86
  170          JSR   SENDACC
  171          JSR   SENDMOD
  172          LDA   BTTNSTAT
  173          JSR   SENDACC
  174          JMP   EVENTLP
  175 SENDMOD  LDA   $C062
  176          ASL
  177          LDA   $C061
  178          ROR
  179          AND   #$C0
  180 SENDACC  STA   $C088,Y
  181 SENDWT   LDA   #$10
  182          AND   $C089,Y
  183          BEQ   SENDWT
  184          RTS
  185 CALLMOU  LDX   MOUROM
  186          BEQ   EXITMOU
  187          STX   PTRH
  188          PHA
  189          LDA   #$00
  190          STA   PTRL
  191          LDA   (PTR),Y
  192          STA   PTRL
  193          LDX   MOUROM
  194          LDY   MOUSLOT
  195          PLA
  196          PHP
  197          SEI
  198          JSR   IJMP
  199          PLP
  200          CLC
  201          RTS
  202 EXITMOU  SEC
  203          RTS
  204 IJMP     JMP   (PTR)
  205 MOUXSTAT DB    0
  206 MOUYSTAT DB    0
  207 BTTNSTAT DB    0
  208 WAITMOU  DB    0
