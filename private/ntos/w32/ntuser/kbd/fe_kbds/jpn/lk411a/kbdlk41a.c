/*
* Module Name: kbdlk41a.c

* DEC LK411-AJ keyboard layout driver

* History:
* 20-SEP-1996: Nakazato(v-mnakaz, DEC-J) created
*/

#include <windows.h>
#include <ime.h>
#include "vkoem.h"
#include "kbdjpn.h"
#include "kbdlk41a.h"

#if defined(_M_IA64)
#pragma section(".data")
#define ALLOC_SECTION_LDATA __declspec(allocate(".data"))
#else
#pragma data_seg(".data")
#define ALLOC_SECTION_LDATA
#endif

/*
* ausVK[] - Virtual Scan Code to Virtual Key conversion table for LK411-AJ
*/

static ALLOC_SECTION_LDATA USHORT ausVK[] = {
    T00, T01, T02, T03, T04, T05, T06, T07,
    T08, T09, T0A, T0B, T0C, T0D, T0E, T0F,
    T10, T11, T12, T13, T14, T15, T16, T17,
    T18, T19, T1A, T1B, T1C, T1D, T1E, T1F,
    T20, T21, T22, T23, T24, T25, T26, T27,
    T28,

    // LK411-AJ uses "<>" key as SBCS/DBCS (NLS key)
    T29 | KBDSPECIAL,

    T2A, T2B, T2C, T2D, T2E, T2F,
    T30, T31, T32, T33, T34, T35,

     // Right-hand Shift key must have KBDEXT bit set.
    T36 | KBDEXT,

     // numpad_* + Shift/Alt -> SnapShot
    T37 | KBDMULTIVK,

    T38, T39,

     // Lock key(CapsLock) must have KBDSPECIAL bit set (NLS key)
    T3A | KBDSPECIAL,

    T3B, T3C, T3D, T3E, T3F,
    T40, T41, T42, T43, T44,

     // NumLock Key:
     //     KBDEXT     - VK_NUMLOCK is an Extended key
     //     KBDMULTIVK - VK_NUMLOCK or VK_PAUSE (without or with CTRL)
    T45 | KBDEXT | KBDMULTIVK,

    T46 | KBDMULTIVK,

     // Number Pad keys:
     //    KBDNUMPAD  - digits 0-9 and decimal point.
     //    KBDSPECIAL - require special processing by Windows
    T47 | KBDNUMPAD | KBDSPECIAL,   // Numpad 7 (Home)
    T48 | KBDNUMPAD | KBDSPECIAL,   // Numpad 8 (Up),
    T49 | KBDNUMPAD | KBDSPECIAL,   // Numpad 9 (PgUp),
    T4A,
    T4B | KBDNUMPAD | KBDSPECIAL,   // Numpad 4 (Left),
    T4C | KBDNUMPAD | KBDSPECIAL,   // Numpad 5 (Clear),
    T4D | KBDNUMPAD | KBDSPECIAL,   // Numpad 6 (Right),
    T4E,
    T4F | KBDNUMPAD | KBDSPECIAL,   // Numpad 1 (End),
    T50 | KBDNUMPAD | KBDSPECIAL,   // Numpad 2 (Down),
    T51 | KBDNUMPAD | KBDSPECIAL,   // Numpad 3 (PgDn),
    T52 | KBDNUMPAD | KBDSPECIAL,   // Numpad 0 (Ins),
    T53 | KBDNUMPAD | KBDSPECIAL,   // Numpad . (Del),

    T54, T55, T56, T57, T58, T59, T5A, T5B,
    T5C, T5D, T5E, T5F, T60, T61, T62, T63,
    T64, T65, T66, T67, T68, T69, T6A, T6B,
    T6C, T6D, T6E, T6F,

     // Hiragana/Katakana/Roman key must have KBDSPECIAL bit set (NLS key)
    T70 | KBDSPECIAL,

    T71, T72, T73,
    T74, T75, T76, T77, T78,

     // Conversion key must have KBDSPECIAL bit set (NLS key)
    T79 | KBDSPECIAL,

    T7A,

     // Non-Conversion key must have KBDSPECIAL bit set (NLS key)
    T7B | KBDSPECIAL,

    T7C, T7D, T7E, T7F
};


static ALLOC_SECTION_LDATA VSC_VK aE0VscToVk[] = {
        { 0x0F, X0F | KBDEXT | KBDSPECIAL | KBDMULTIVK },       // LK411 added (kana key)
        { 0x1C, X1C | KBDEXT              },  // Numpad Enter
        { 0x1D, X1D | KBDEXT              },  // RControl
        { 0x35, X35 | KBDEXT              },  // Numpad Divide
        { 0x37, X37 | KBDEXT              },  // Snapshot
        { 0x38, X38 | KBDEXT              },  // RMenu
        { 0x3D, X3D | KBDEXT              },  // F13            // LK411 added
        { 0x3E, X3E | KBDEXT              },  // F14            // LK411 added
        { 0x3F, X3F | KBDEXT              },  // F15 (Help)     // LK411 added
        { 0x40, X40 | KBDEXT              },  // F16 (Do)       // LK411 added
        { 0x41, X41 | KBDEXT              },  // F17            // LK411 added
        { 0x46, X46 | KBDEXT              },  // Break (Ctrl + Pause)   // LK411 removed (@@ check)
        { 0x47, X47 | KBDEXT              },  // Home
        { 0x48, X48 | KBDEXT              },  // Up
        { 0x49, X49 | KBDEXT              },  // Prior
        { 0x4B, X4B | KBDEXT              },  // Left
        { 0x4D, X4D | KBDEXT              },  // Right
        { 0x4E, X4E | KBDEXT              },  // Keypad Minus       // LK411
        { 0x4F, X4F | KBDEXT              },  // End
        { 0x50, X50 | KBDEXT              },  // Down
        { 0x51, X51 | KBDEXT              },  // Next
        { 0x52, X52 | KBDEXT              },  // Insert
        { 0x53, X53 | KBDEXT              },  // Delete
//      { 0x5B, X5B | KBDEXT              },  // Left Win       // LK411 remove
//      { 0x5C, X5C | KBDEXT              },  // Right Win      // LK411 remove
//      { 0x5D, X5D | KBDEXT              },  // Application        // LK411 remove
        { 0,      0                       }
};


static ALLOC_SECTION_LDATA VSC_VK aE1VscToVk[] = {
        { 0x1D, Y1D                       },  // Pause
        { 0   ,   0                       }
};



/*
* aVkToBits[]  - map Virtual Keys to Modifier Bits

* See kbd.h for a full description.

* US Keyboard has only three shifter keys:
*     SHIFT (L & R) affects alphabnumeric keys,
*     CTRL  (L & R) is used to generate control characters
*     ALT   (L & R) used for generating characters by number with numpad
*/

static ALLOC_SECTION_LDATA VK_TO_BIT aVkToBits[] = {
    { VK_SHIFT,   KBDSHIFT },   // 1st bit (0001)
    { VK_CONTROL, KBDCTRL  },   // 2nd bit (0010)
    { VK_MENU,    KBDALT   },   // 4th bit (0100)
    { VK_KANA,    KBDKANA  },   // 8th bit (1000)
    { 0,          0        }
};



/*
* aModification[]  - map character modifier bits to modification number

* See kbd.h for a full description.

*/

static ALLOC_SECTION_LDATA MODIFIERS CharModifiers = {
    &aVkToBits[0],
    11,
    {
    //  Modification# // bit    Keys Pressed  : Explanation
    //  ============= // ====  ============== : =============================
        0,            // 0000                 : unshifted characters
        1,            // 0001           SHIFT : capitals, ~!@#$%^&*()_+{}:"<>? etc.
        4,            // 0010      CTRL       : control characters
        6,            // 0011      CTRL SHIFT :
        SHFT_INVALID, // 0100  ALT            : invalid
        SHFT_INVALID, // 0101  ALT      SHIFT : invalid
        SHFT_INVALID, // 0110  ALT CTRL       : invalid
        SHFT_INVALID, // 0111  ALT CTRL SHIFT : invalid
        2,            // 1000  KANA
        3,            // 1001  KANA      SHIFT
        5,            // 1010  KANA CTRL
        7             // 1011  KANA CTRL SHIFT
    }
};



/*

* aVkToWch4[]  - Virtual Key to WCHAR translation for 4 shift states
* aVkToWch6[]  - Virtual Key to WCHAR translation for 6 shift states
* aVkToWch8[]  - Virtual Key to WCHAR translation for 8 shift states
* aVkToWch1[]  - Virtual Key to WCHAR translation for NUMPAD

* Table attributes: Unordered Scan, null-terminated

* Search this table for an entry with a matching Virtual Key to find the
* corresponding unshifted and shifted WCHAR characters.

* Reserved VirtualKey values (first column)
*     -1            - this line contains dead characters (diacritic)
*     0             - terminator

* Reserved Attribute values (second column)
*     CAPLOK        - CapsLock affects this key like Shift
*     KANALOK       - The KANA-LOCK key affects this key like KANA

* Reserved character values (third through last column)
*     WCH_NONE      - No character
*     WCH_DEAD      - Dead character (diacritic) value is in next line

*/

static ALLOC_SECTION_LDATA VK_TO_WCHARS4 aVkToWch4[] = {
    //                               |          |   SHIFT  |  KANA  | K+SHFT |
    //                               |          |==========|========|========|
    {'0'          ,          KANALOK ,'0'       ,')'       ,WCH_WA  ,WCH_WO  },
    {'1'          ,          KANALOK ,'1'       ,'!'       ,WCH_NU  ,WCH_NU  },
    {'3'          ,          KANALOK ,'3'       ,'#'       ,WCH_A   ,WCH_AA  },
    {'4'          ,          KANALOK ,'4'       ,'$'       ,WCH_U   ,WCH_UU  },
    {'5'          ,          KANALOK ,'5'       ,'%'       ,WCH_E   ,WCH_EE  },
    {'7'          ,          KANALOK ,'7'       ,'&'       ,WCH_YA  ,WCH_YAA },
    {'8'          ,          KANALOK ,'8'       ,'*'       ,WCH_YU  ,WCH_YUU },
    {'9'          ,          KANALOK ,'9'       ,'('       ,WCH_YO  ,WCH_YOO },
    {'A'          , CAPLOK | KANALOK ,'a'       ,'A'       ,WCH_TI  ,WCH_TI  },
    {'B'          , CAPLOK | KANALOK ,'b'       ,'B'       ,WCH_KO  ,WCH_KO  },
    {'C'          , CAPLOK | KANALOK ,'c'       ,'C'       ,WCH_SO  ,WCH_SO  },
    {'D'          , CAPLOK | KANALOK ,'d'       ,'D'       ,WCH_SI  ,WCH_SI  },
    {'E'          , CAPLOK | KANALOK ,'e'       ,'E'       ,WCH_I   ,WCH_II  },
    {'F'          , CAPLOK | KANALOK ,'f'       ,'F'       ,WCH_HA  ,WCH_HA  },
    {'G'          , CAPLOK | KANALOK ,'g'       ,'G'       ,WCH_KI  ,WCH_KI  },
    {'H'          , CAPLOK | KANALOK ,'h'       ,'H'       ,WCH_KU  ,WCH_KU  },
    {'I'          , CAPLOK | KANALOK ,'i'       ,'I'       ,WCH_NI  ,WCH_NI  },
    {'J'          , CAPLOK | KANALOK ,'j'       ,'J'       ,WCH_MA  ,WCH_MA  },
    {'K'          , CAPLOK | KANALOK ,'k'       ,'K'       ,WCH_NO  ,WCH_NO  },
    {'L'          , CAPLOK | KANALOK ,'l'       ,'L'       ,WCH_RI  ,WCH_RI  },
    {'M'          , CAPLOK | KANALOK ,'m'       ,'M'       ,WCH_MO  ,WCH_MO  },
    {'N'          , CAPLOK | KANALOK ,'n'       ,'N'       ,WCH_MI  ,WCH_MI  },
    {'O'          , CAPLOK | KANALOK ,'o'       ,'O'       ,WCH_RA  ,WCH_RA  },
    {'P'          , CAPLOK | KANALOK ,'p'       ,'P'       ,WCH_SE  ,WCH_SE  },
    {'Q'          , CAPLOK | KANALOK ,'q'       ,'Q'       ,WCH_TA  ,WCH_TA  },
    {'R'          , CAPLOK | KANALOK ,'r'       ,'R'       ,WCH_SU  ,WCH_SU  },
    {'S'          , CAPLOK | KANALOK ,'s'       ,'S'       ,WCH_TO  ,WCH_TO  },
    {'T'          , CAPLOK | KANALOK ,'t'       ,'T'       ,WCH_KA  ,WCH_KA  },
    {'U'          , CAPLOK | KANALOK ,'u'       ,'U'       ,WCH_NA  ,WCH_NA  },
    {'V'          , CAPLOK | KANALOK ,'v'       ,'V'       ,WCH_HI  ,WCH_HI  },
    {'W'          , CAPLOK | KANALOK ,'w'       ,'W'       ,WCH_TE  ,WCH_TE  },
    {'X'          , CAPLOK | KANALOK ,'x'       ,'X'       ,WCH_SA  ,WCH_SA  },
    {'Y'          , CAPLOK | KANALOK ,'y'       ,'Y'       ,WCH_NN  ,WCH_NN  },
    {'Z'          , CAPLOK | KANALOK ,'z'       ,'Z'       ,WCH_TU  ,WCH_TUU },
    {VK_OEM_1     ,          KANALOK ,';'       ,':'       ,WCH_RE  ,WCH_RE  },
    {VK_OEM_2     ,          KANALOK ,'/'       ,'?'       ,WCH_ME  ,WCH_MD  },
    {VK_OEM_7     ,          KANALOK ,0x27      ,'"'       ,WCH_KE  ,WCH_KE  },
//  {VK_OEM_8     , 0                ,','       ,','       ,','     ,','     },     //LK411 removed
    {VK_OEM_COMMA ,          KANALOK ,','       ,'<'       ,WCH_NE  ,WCH_IC  },
    {VK_OEM_PERIOD,          KANALOK ,'.'       ,'>'       ,WCH_RU  ,WCH_IP  },
    {VK_OEM_PLUS  ,          KANALOK ,'='       ,'+'       ,WCH_HE  ,WCH_HE  },
    {VK_DBE_SBCSCHAR, 0              ,'`'       ,'~'       ,'`'     ,'~'     },     //LK411 added
    {VK_TAB       , 0                ,'\t'      ,'\t'      ,'\t'    ,'\t'    },
    {VK_ADD       , 0                ,'+'       ,'+'       ,'+'     ,'+'     },
    {VK_DECIMAL   , 0                ,'.'       ,'.'       ,'.'     ,'.'     },
    {VK_DIVIDE    , 0                ,'/'       ,'/'       ,'/'     ,'/'     },
    {VK_MULTIPLY  , 0                ,'*'       ,'*'       ,'*'     ,'*'     },
    {VK_SUBTRACT  , 0                ,'-'       ,'-'       ,'-'     ,'-'     },
    {0            , 0                ,0         ,0         ,0       ,0       }
};


static ALLOC_SECTION_LDATA VK_TO_WCHARS6 aVkToWch6[] = {
    //                      |          |   SHIFT  |  KANA  | K+SHFT |  CONTROL  |  K+CTRL   |
    //                      |          |==========|========|========|===========|===========|
    {VK_BACK      , 0       ,'\b'      ,'\b'      ,'\b'    ,'\b'    , 0x7f      , 0x7f      },
    {VK_CANCEL    , 0       ,0x03      ,0x03      ,0x03    ,0x03    , 0x03      , 0x03      },
    {VK_ESCAPE    , 0       ,0x1b      ,0x1b      ,0x1b    ,0x1b    , 0x1b      , 0x1b      },
    {VK_OEM_4     , KANALOK ,'['       ,'{'       ,WCH_RO  ,WCH_VS  , 0x1b      , 0x1b      },  // LK411 modified
    {VK_OEM_5     , KANALOK ,'\\'      ,'|'       ,WCH_OB  ,WCH_CB  , 0x1c      , 0x1c      },  // LK411 modified
    {VK_OEM_6     , KANALOK ,']'       ,'}'       ,WCH_MU  ,WCH_SVS , 0x1d      , 0x1d      },  // LK411 modified
    {VK_RETURN    , 0       ,'\r'      ,'\r'      ,'\r'    ,'\r'    , '\n'      , '\n'      },
    {VK_SPACE     , 0       ,' '       ,' '       ,' '     ,' '     , 0x20      , 0x20      },
    {0            , 0       ,0         ,0         ,0       ,0       , 0         , 0         }
};


static ALLOC_SECTION_LDATA VK_TO_WCHARS8 aVkToWch8[] = {
    //                      |          |   SHIFT  |  KANA  | K+SHFT |  CONTROL  |  K+CTRL   | SHFT+CTRL |K+SHFT+CTRL|
    //                      |          |==========|========|========|===========|===========|===========|===========|
    {'2'          , KANALOK ,'2'       ,'@'       ,WCH_HU  ,WCH_HU  , WCH_NONE  , WCH_NONE  , 0x00      , 0x00      },
    {'6'          , KANALOK ,'6'       ,'^'       ,WCH_O   ,WCH_OO  , WCH_NONE  , WCH_NONE  , 0x1e      , 0x1e      },
    {VK_OEM_MINUS , KANALOK ,'-'       ,'_'       ,WCH_HO  ,WCH_HO  , WCH_NONE  , WCH_NONE  , 0x1f      , 0x1f      },  // LK411
    {0            , 0       ,0         ,0         ,0       ,0       , 0         , 0         , 0         , 0         }
};


// Put this last so that VkKeyScan interprets number characters
// as coming from the main section of the kbd (aVkToWch2 and
// aVkToWch4) before considering the numpad (aVkToWch1).

// LK411 doesn't use this table. (Because it doesn't use NUMPAD VK)

static ALLOC_SECTION_LDATA VK_TO_WCHARS4 aVkToWch1[] = {
    //                     |          |   SHIFT  |  KANA  | K+SHFT |
    //                     |          |==========|========|========|
    { VK_NUMPAD0   , 0      ,  '0'    , WCH_NONE ,   '0'  ,WCH_NONE},
    { VK_NUMPAD1   , 0      ,  '1'    , WCH_NONE ,   '1'  ,WCH_NONE},
    { VK_NUMPAD2   , 0      ,  '2'    , WCH_NONE ,   '2'  ,WCH_NONE},
    { VK_NUMPAD3   , 0      ,  '3'    , WCH_NONE ,   '3'  ,WCH_NONE},
    { VK_NUMPAD4   , 0      ,  '4'    , WCH_NONE ,   '4'  ,WCH_NONE},
    { VK_NUMPAD5   , 0      ,  '5'    , WCH_NONE ,   '5'  ,WCH_NONE},
    { VK_NUMPAD6   , 0      ,  '6'    , WCH_NONE ,   '6'  ,WCH_NONE},
    { VK_NUMPAD7   , 0      ,  '7'    , WCH_NONE ,   '7'  ,WCH_NONE},
    { VK_NUMPAD8   , 0      ,  '8'    , WCH_NONE ,   '8'  ,WCH_NONE},
    { VK_NUMPAD9   , 0      ,  '9'    , WCH_NONE ,   '9'  ,WCH_NONE},
    { 0            , 0      ,  '\0'   , 0        ,   0    ,0       }   //null terminator
};


/*
* aVkToWcharTable: table of pointers to Character Tables

* Describes the character tables and the order they should be searched.

* Note: the order determines the behavior of VkKeyScan() : this function
*       takes a character and attempts to find a Virtual Key and character-
*       modifier key combination that produces that character.  The table
*       containing the numeric keypad (aVkToWch1) must appear last so that
*       VkKeyScan('0') will be interpreted as one of keys from the main
*       section, not the numpad.  etc.
*/

static ALLOC_SECTION_LDATA VK_TO_WCHAR_TABLE aVkToWcharTable[] = {
    {  (PVK_TO_WCHARS1)aVkToWch6, 6, sizeof(aVkToWch6[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch8, 8, sizeof(aVkToWch8[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch4, 4, sizeof(aVkToWch4[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch1, 4, sizeof(aVkToWch1[0]) },  // must come last
    {                       NULL, 0, 0                    }
};



/*
* aKeyNames[], aKeyNamesExt[]  - Scan Code -> Key Name tables

* For the GetKeyNameText() API function

* Tables for non-extended and extended (KBDEXT) keys.
* (Keys producing printable characters are named by the character itself)
*/

static ALLOC_SECTION_LDATA VSC_LPWSTR aKeyNames[] = {
    0x01,    L"Esc",
    0x0e,    L"Backspace",
    0x0f,    L"Tab",
    0x1c,    L"Enter",
    0x1d,    L"Ctrl",
    0x29,    L"<>",
    0x2a,    L"Shift",
    0x36,    L"Right Shift",
    0x37,    L"Num *",
    0x38,    L"Alt",
    0x39,    L"Space",
    0x3a,    L"Caps Lock",
    0x3b,    L"F1",
    0x3c,    L"F2",
    0x3d,    L"F3",
    0x3e,    L"F4",
    0x3f,    L"F5",
    0x40,    L"F6",
    0x41,    L"F7",
    0x42,    L"F8",
    0x43,    L"F9",
    0x44,    L"F10",
    0x45,    L"Num Lock",
    0x46,    L"Scroll Lock",
    0x47,    L"Num 7",
    0x48,    L"Num 8",
    0x49,    L"Num 9",
    0x4a,    L"Num -",
    0x4b,    L"Num 4",
    0x4c,    L"Num 5",
    0x4d,    L"Num 6",
    0x4e,    L"Num +",
    0x4f,    L"Num 1",
    0x50,    L"Num 2",
    0x51,    L"Num 3",
    0x52,    L"Num 0",
    0x53,    L"Num Del",
    0x54,    L"Sys Req",
    0x57,    L"F11",
    0x58,    L"F12",
    0x70,    (LPWSTR)SZ_KEY_NAME_HIRAGANA,
    0x79,    (LPWSTR)SZ_KEY_NAME_HENKAN,
    0x7b,    (LPWSTR)SZ_KEY_NAME_MUHENKAN,
    0   ,    NULL
};


static ALLOC_SECTION_LDATA VSC_LPWSTR aKeyNamesExt[] = {
    0x0f,    (LPWSTR)SZ_KEY_NAME_KANA,  //LK411 added, (@@ check), this macro defined by NEC
    0x1c,    L"Num Enter",
    0x1d,    L"Right Control",
    0x35,    L"Num /",
    0x37,    L"Prnt Scrn",
    0x38,    L"Right Alt",
    0x3d,    L"F13",            // LK411 added
    0x3e,    L"F14",            // LK411 added
    0x3f,    L"F15",            // LK411 added (Help)
    0x40,    L"F16",            // LK411 added (Do)
    0x41,    L"F17",            // LK411 added
//  0x45,    L"Num Lock",       // LK411 removed
    0x46,    L"Break",
    0x47,    L"Home",
    0x48,    L"Up",
    0x49,    L"Page Up",
    0x4b,    L"Left",
    0x4d,    L"Right",
    0x4e,    L"Num +",          // LK411 modified(same KP,)
    0x4f,    L"End",
    0x50,    L"Down",
    0x51,    L"Page Down",
    0x52,    L"Insert",
    0x53,    L"Delete",
//  0x54,    L"<00>",           // LK411 removed
//  0x56,    L"Help",           // LK411 removed
//  0x5B,    L"Left Windows",       // LK411 removed
//  0x5C,    L"Right Windows",      // LK411 removed
//  0x5D,    L"Application",        // LK411 removed
    0   ,    NULL
};



static ALLOC_SECTION_LDATA KBDTABLES KbdTables = {
    /*
     * Modifier keys
     */
    &CharModifiers,

    /*
     * Characters tables
     */
    aVkToWcharTable,

    /*
     * Diacritics  (none for US English)
     */
    NULL,

    /*
     * Names of Keys  (no dead keys)
     */
    aKeyNames,
    aKeyNamesExt,
    NULL,

    /*
     * Scan codes to Virtual Keys
     */
    ausVK,
    sizeof(ausVK) / sizeof(ausVK[0]),
    aE0VscToVk,
    aE1VscToVk,

    /*
     * No Locale-specific special processing
     */
    0
};


PKBDTABLES KbdLayerDescriptor(VOID)
{
    return &KbdTables;
}



/*
* VkToFuncTable_LK411[]

*/

static ALLOC_SECTION_LDATA VK_F VkToFuncTable_LK411[] = {
    /* LOCK key */
    {
        VK_CAPITAL,           // Base Vk
        KBDNLS_TYPE_TOGGLE,   // NLSFEProcType
        KBDNLS_INDEX_NORMAL,  // NLSFEProcCurrent
        0x32, /* 0011 0010 */ // NLSFEProcSwitch
        {                     // NLSFEProc
            {KBDNLS_SEND_BASE_VK,0},         // Base
            {KBDNLS_ALPHANUM,0},             // Shift
            {KBDNLS_NOEVENT,0},              // Control
            {KBDNLS_NOEVENT,0},              // Shift+Control
            {KBDNLS_ALPHANUM,0},             // Alt
            {KBDNLS_ALPHANUM,0},             // Shift+Alt
            {KBDNLS_CODEINPUT,0},            // Control+Alt
            {KBDNLS_CODEINPUT,0}             // Shift+Control+Alt
        },
        {                     // NLSFEProcAlt
            {KBDNLS_ALPHANUM,0},             // Base
            {KBDNLS_ALPHANUM,0},             // Shift
            {KBDNLS_NOEVENT,0},              // Control
            {KBDNLS_NOEVENT,0},              // Shift+Control
            {KBDNLS_ALPHANUM,0},             // Alt
            {KBDNLS_ALPHANUM,0},             // Shift+Alt
            {KBDNLS_NOEVENT,0},              // Control+Alt
            {KBDNLS_NOEVENT,0}               // Shift+Control+Alt
        }
    },

    /* KANA key */
    {
        VK_KANA,              // Base Vk
        KBDNLS_TYPE_TOGGLE,   // NLSFEProcType
        KBDNLS_INDEX_NORMAL,  // NLSFEProcCurrent
        0x08, /* 0000 1000 */ // NLSFEProcSwitch
        {                     // NLSFEProc
            {KBDNLS_SEND_BASE_VK,0},         // Base
            {KBDNLS_NOEVENT,0},              // Shift
            {KBDNLS_NOEVENT,0},              // Control
            {KBDNLS_SEND_PARAM_VK,VK_KANA},  // Shift+Control
            {KBDNLS_NOEVENT,0},              // Alt
            {KBDNLS_NOEVENT,0},              // Shift+Alt
            {KBDNLS_NOEVENT,0},              // Control+Alt
            {KBDNLS_NOEVENT,0}               // Shift+Control+Alt
        },
        {                    // NLSFEProcIndexAlt
            {KBDNLS_SEND_PARAM_VK,VK_KANA},  // Base
            {KBDNLS_SEND_PARAM_VK,VK_KANA},  // Shift
            {KBDNLS_SEND_PARAM_VK,VK_KANA},  // Control
            {KBDNLS_SEND_PARAM_VK,VK_KANA},  // Shift+Control
            {KBDNLS_NOEVENT,0},              // Alt
            {KBDNLS_NOEVENT,0},              // Shift+Alt
            {KBDNLS_NOEVENT,0},              // Control+Alt
            {KBDNLS_NOEVENT,0}               // Shift+Control+Alt
        }
    },

    /* Hiragana/Katakana key */
    {
        VK_DBE_HIRAGANA,      // Base Vk
        KBDNLS_TYPE_TOGGLE,   // NLSFEProcType
        KBDNLS_INDEX_NORMAL,  // NLSFEProcCurrent
        0x08, /* 0000 1000 */ // NLSFEProcSwitch
        {                     // NLSFEProc
            {KBDNLS_HIRAGANA,0},             // Base
            {KBDNLS_KATAKANA,0},             // Shift
            {KBDNLS_SBCSDBCS,0},             // Control
            {KBDNLS_SEND_PARAM_VK,VK_KANA},  // Shift+Control
            {KBDNLS_ROMAN,0},                // Alt
            {KBDNLS_ROMAN,0},                // Shift+Alt
            {KBDNLS_ROMAN,0},                // Control+Alt
            {KBDNLS_NOEVENT,0}               // Shift+Control+Alt
        },
        {                    // NLSFEProcIndexAlt
            {KBDNLS_SEND_PARAM_VK,VK_KANA},  // Base
            {KBDNLS_SEND_PARAM_VK,VK_KANA},  // Shift
            {KBDNLS_SEND_PARAM_VK,VK_KANA},  // Control
            {KBDNLS_SEND_PARAM_VK,VK_KANA},  // Shift+Control
            {KBDNLS_NOEVENT,0},              // Alt
            {KBDNLS_NOEVENT,0},              // Shift+Alt
            {KBDNLS_NOEVENT,0},              // Control+Alt
            {KBDNLS_NOEVENT,0}               // Shift+Control+Alt
        }
    },

    /* SBCS/DBCS key */
    {
        VK_DBE_SBCSCHAR,      // Base Vk
        KBDNLS_TYPE_TOGGLE,   // NLSFEProcType
        KBDNLS_INDEX_NORMAL,  // NLSFEProcCurrent
        0x30, /* 0011 0000 */ // NLSFEProcSwitch
        {                     // NLSFEProc
            {KBDNLS_SEND_BASE_VK,0},         // Base  ("<")
            {KBDNLS_SEND_BASE_VK,0},         // Shift (">")
            {KBDNLS_SBCSDBCS,0},             // Control
            {KBDNLS_SBCSDBCS,0},             // Shift+Control
            {KBDNLS_SEND_PARAM_VK,VK_KANJI}, // Alt
            {KBDNLS_SEND_PARAM_VK,VK_KANJI}, // Shift+Alt
            {KBDNLS_SEND_PARAM_VK,VK_DBE_ENTERIMECONFIGMODE}, // Control+Alt
            {KBDNLS_SEND_PARAM_VK,VK_DBE_ENTERIMECONFIGMODE}  // Shift+Control+Alt
        },
        {                    // NLSFEProcIndexAlt
            {KBDNLS_SEND_PARAM_VK,VK_KANJI},  // Base
            {KBDNLS_SEND_PARAM_VK,VK_KANJI},  // Shift
            {KBDNLS_NOEVENT,0},              // Control
            {KBDNLS_NOEVENT,0},              // Shift+Control
            {KBDNLS_SEND_PARAM_VK,VK_KANJI}, // Alt
            {KBDNLS_SEND_PARAM_VK,VK_KANJI}, // Shift+Alt
            {KBDNLS_NOEVENT,0},              // Control+Alt
            {KBDNLS_NOEVENT,0}               // Shift+Control+Alt
        }
    },

    /* CONVERT key */
    {
        VK_CONVERT,          // Base Vk
        KBDNLS_TYPE_NORMAL,  // NLSFEProcType
        KBDNLS_INDEX_NORMAL, // NLSFEProcCurrent
        0x0,                 // NLSFEProcSwitch
        {                    // NLSFEProc
            {KBDNLS_SEND_BASE_VK,0},         // Base
            {KBDNLS_SEND_BASE_VK,0},         // Shift
            {KBDNLS_NOEVENT,0},              // Control
            {KBDNLS_NOEVENT,0},              // Shift+Control
            {KBDNLS_SEND_PARAM_VK,VK_KANJI}, // Alt
            {KBDNLS_SEND_PARAM_VK,VK_DBE_ENTERIMECONFIGMODE}, // Shift+Alt (LK411: Shift+Alt+NoConv -> IME config)
            {KBDNLS_SEND_PARAM_VK,VK_DBE_ENTERWORDREGISTERMODE}, // Control+Alt
            {KBDNLS_SEND_PARAM_VK,VK_DBE_ENTERWORDREGISTERMODE}  // Shift+Control+Alt
        },
        {                         // NLSFEProcIndexAlt
            {KBDNLS_NULL,0},  // Base
            {KBDNLS_NULL,0},  // Shift
            {KBDNLS_NULL,0},  // Control
            {KBDNLS_NULL,0},  // Shift+Control
            {KBDNLS_NULL,0},  // Alt
            {KBDNLS_NULL,0},  // Shift+Alt
            {KBDNLS_NULL,0},  // Control+Alt
            {KBDNLS_NULL,0}   // Shift+Control+Alt
        }
    },

    /* NONE-CONVERT key */
    {
        VK_NONCONVERT,       // Base Vk
        KBDNLS_TYPE_NORMAL,  // NLSFEProcType
        KBDNLS_INDEX_NORMAL, // NLSFEProcCurrent
        0x0,                 // NLSFEProcSwitch
        {                    // NLSFEProc
            {KBDNLS_SEND_BASE_VK,0},         // Base
            {KBDNLS_NOEVENT,0},              // Shift
            {KBDNLS_NOEVENT,0},              // Control
            {KBDNLS_NOEVENT,0},              // Shift+Control
            {KBDNLS_SBCSDBCS,0},             // Alt  (LK411: Alt+NoConv -> SBCS/DBCS)
            {KBDNLS_SEND_PARAM_VK,VK_DBE_ENTERIMECONFIGMODE}, // Shift+Alt (LK411: Shift+Alt+NoConv -> IME config)
            {KBDNLS_SEND_PARAM_VK,VK_DBE_ENTERWORDREGISTERMODE}, // Control+Alt
            {KBDNLS_SEND_PARAM_VK,VK_DBE_ENTERWORDREGISTERMODE}  // Shift+Control+Alt
        },
        {                         // NLSFEProcIndexAlt
            {KBDNLS_NULL,0},  // Base
            {KBDNLS_NULL,0},  // Shift
            {KBDNLS_NULL,0},  // Control
            {KBDNLS_NULL,0},  // Shift+Control
            {KBDNLS_NULL,0},  // Alt
            {KBDNLS_NULL,0},  // Shift+Alt
            {KBDNLS_NULL,0},  // Control+Alt
            {KBDNLS_NULL,0}   // Shift+Control+Alt
        }
    }
};


/*
* KbdNlsTables

*/

static ALLOC_SECTION_LDATA KBDNLSTABLES KbdNlsTables = {
    NLSKBD_OEM_DEC,                      // OEM ID
    NLSKBD_INFO_EMURATE_101_KEYBOARD,    // Information
    6,                                   // Number of VK_F entry
    VkToFuncTable_LK411,                 // Pointer to VK_F array
    0,                                   // Pointer to MouseVk entry
    NULL                                 // Pointer to MouseVk array
};


PKBDNLSTABLES KbdNlsLayerDescriptor(VOID)
{
    return &KbdNlsTables;
}

/* EOF */
