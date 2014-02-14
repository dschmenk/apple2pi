
int keycode[256] = {
    /*
     * normal scancode
     */
    MOD_CTRL | MOD_SHIFT | KEY_2,           // CTRL-@ code 00
    MOD_CTRL |             KEY_A,           // CTRL-A code 01
    MOD_CTRL |             KEY_B,           // CTRL-B code 02
    MOD_CTRL |             KEY_C,           // CTRL-C code 03
    MOD_CTRL |             KEY_D,           // CTRL-D code 04
    MOD_CTRL |             KEY_E,           // CTRL-E code 05
    MOD_CTRL |             KEY_F,           // CTRL-F code 06
    MOD_CTRL |             KEY_G,           // CTRL-G code 07
   			   KEY_LEFT,        // CTRL-H code 08
                           KEY_TAB,         // CTRL-I code 09
                           KEY_DOWN,        // CTRL-J code 0A
                           KEY_UP,          // CTRL-K code 0B
    MOD_CTRL |             KEY_L,           // CTRL-L code 0C
                           KEY_ENTER,       // CTRL-M code 0D
    MOD_CTRL |             KEY_N,           // CTRL-N code 0E
    MOD_CTRL |             KEY_O,           // CTRL-O code 0F
    MOD_CTRL |             KEY_P,           // CTRL-P code 10
    MOD_CTRL |             KEY_Q,           // CTRL-Q code 11
    MOD_CTRL |             KEY_R,           // CTRL-R code 12
    MOD_CTRL |             KEY_S,           // CTRL-S code 13
    MOD_CTRL |             KEY_T,           // CTRL-T code 14
                           KEY_RIGHT,       // CTRL-U code 15
    MOD_CTRL |             KEY_V,           // CTRL-V code 16
    MOD_CTRL |             KEY_W,           // CTRL-W code 17
    MOD_CTRL |             KEY_X,           // CTRL-X code 18
    MOD_CTRL |             KEY_Y,           // CTRL-Y code 19
    MOD_CTRL |             KEY_Z,           // CTRL-Z code 1A
                           KEY_ESC,         // ESCAPE code 1B
    MOD_CTRL |             KEY_BACKSLASH,   // CTRL-\ code 1C
    MOD_CTRL |             KEY_RIGHTBRACE,  // CTRL-] code 1D
    MOD_CTRL |             KEY_6,           // CTRL-6 code 1E
    MOD_CTRL |             KEY_MINUS,       // CTRL-- code 1F
                           KEY_SPACE,       // ' '    code 20
               MOD_SHIFT | KEY_1,           // !      code 21
               MOD_SHIFT | KEY_APOSTROPHE,  // "      code 22
               MOD_SHIFT | KEY_3,           // #      code 23
               MOD_SHIFT | KEY_4,           // $      code 24
               MOD_SHIFT | KEY_5,           // %      code 25
               MOD_SHIFT | KEY_7,           // &      code 26
                           KEY_APOSTROPHE,  // '      code 27
               MOD_SHIFT | KEY_9,           // (      code 28
               MOD_SHIFT | KEY_0,           // )      code 29
               MOD_SHIFT | KEY_8,           // *      code 2A
               MOD_SHIFT | KEY_EQUAL,       // +      code 2B
                           KEY_COMMA,       // ,      code 2C
                           KEY_MINUS,       // -      code 2D
                           KEY_DOT,         // .      code 2E
                           KEY_SLASH,       // /      code 2F
                           KEY_0,           // 0      code 30
                           KEY_1,           // 1      code 31
                           KEY_2,           // 2      code 32
                           KEY_3,           // 3      code 33
                           KEY_4,           // 4      code 34
                           KEY_5,           // 5      code 35
                           KEY_6,           // 6      code 36
                           KEY_7,           // 7      code 37
                           KEY_8,           // 8      code 38
                           KEY_9,           // 9      code 39
               MOD_SHIFT | KEY_SEMICOLON,   // :      code 3A
                           KEY_SEMICOLON,   // ;      code 3B
               MOD_SHIFT | KEY_COMMA,       // <      code 3C
                           KEY_EQUAL,       // =      code 3D
               MOD_SHIFT | KEY_DOT,         // >      code 3E
               MOD_SHIFT | KEY_SLASH,       // ?      code 3F
               MOD_SHIFT | KEY_2,           // @      code 40
               MOD_SHIFT | KEY_A,           // A      code 41
               MOD_SHIFT | KEY_B,           // B      code 42
               MOD_SHIFT | KEY_C,           // C      code 43
               MOD_SHIFT | KEY_D,           // D      code 44
               MOD_SHIFT | KEY_E,           // E      code 45
               MOD_SHIFT | KEY_F,           // F      code 46
               MOD_SHIFT | KEY_G,           // G      code 47
               MOD_SHIFT | KEY_H,           // H      code 48
               MOD_SHIFT | KEY_I,           // I      code 49
               MOD_SHIFT | KEY_J,           // J      code 4A
               MOD_SHIFT | KEY_K,           // K      code 4B
               MOD_SHIFT | KEY_L,           // L      code 4C
               MOD_SHIFT | KEY_M,           // M      code 4D
               MOD_SHIFT | KEY_N,           // N      code 4E
               MOD_SHIFT | KEY_O,           // O      code 4F
               MOD_SHIFT | KEY_P,           // P      code 50
               MOD_SHIFT | KEY_Q,           // Q      code 51
               MOD_SHIFT | KEY_R,           // R      code 52
               MOD_SHIFT | KEY_S,           // S      code 53
               MOD_SHIFT | KEY_T,           // T      code 54
               MOD_SHIFT | KEY_U,           // U      code 55
               MOD_SHIFT | KEY_V,           // V      code 56
               MOD_SHIFT | KEY_W,           // W      code 57
               MOD_SHIFT | KEY_X,           // X      code 58
               MOD_SHIFT | KEY_Y,           // Y      code 59
               MOD_SHIFT | KEY_Z,           // Z      code 5A
                           KEY_LEFTBRACE,   // [      code 5B
                           KEY_BACKSLASH,   // \      code 5C
                           KEY_RIGHTBRACE,  // ]      code 5D
               MOD_SHIFT | KEY_6,           // ^      code 5E
               MOD_SHIFT | KEY_MINUS,       // _      code 5F
                           KEY_GRAVE,       // `      code 60
                           KEY_A,           // a      code 61
                           KEY_B,           // b      code 62
                           KEY_C,           // c      code 63
                           KEY_D,           // d      code 64
                           KEY_E,           // e      code 65
                           KEY_F,           // f      code 66
                           KEY_G,           // g      code 67
                           KEY_H,           // h      code 68
                           KEY_I,           // i      code 69
                           KEY_J,           // j      code 6A
                           KEY_K,           // k      code 6B
                           KEY_L,           // l      code 6C
                           KEY_M,           // m      code 6D
                           KEY_N,           // n      code 6E
                           KEY_O,           // o      code 6F
                           KEY_P,           // p      code 70
                           KEY_Q,           // q      code 71
                           KEY_R,           // r      code 72
                           KEY_S,           // s      code 73
                           KEY_T,           // t      code 74
                           KEY_U,           // u      code 75
                           KEY_V,           // v      code 76
                           KEY_W,           // w      code 77
                           KEY_X,           // x      code 78
                           KEY_Y,           // y      code 79
                           KEY_Z,           // z      code 7A
               MOD_SHIFT | KEY_LEFTBRACE,   // {      code 7B
               MOD_SHIFT | KEY_BACKSLASH,   // |      code 7C
               MOD_SHIFT | KEY_RIGHTBRACE,  // }      code 7D
               MOD_SHIFT | KEY_GRAVE,       // ~      code 7E
                           KEY_BACKSPACE,   // BS     code 7F
    /*
     * w/ solid apple scancodes
     */
    MOD_CTRL | MOD_SHIFT | KEY_2,           // CTRL-@ code 00
    MOD_CTRL |             KEY_A,           // CTRL-A code 01
    MOD_CTRL |             KEY_B,           // CTRL-B code 02
    MOD_CTRL |             KEY_C,           // CTRL-C code 03
    MOD_CTRL |             KEY_D,           // CTRL-D code 04
    MOD_CTRL |             KEY_E,           // CTRL-E code 05
    MOD_CTRL |             KEY_F,           // CTRL-F code 06
    MOD_CTRL |             KEY_G,           // CTRL-G code 07
                           KEY_HOME,        // CTRL-H code 08
                           KEY_INSERT,      // CTRL-I code 09
                           KEY_PAGEDOWN,    // CTRL-J code 0A
                           KEY_PAGEUP,      // CTRL-K code 0B
    MOD_CTRL |             KEY_L,           // CTRL-L code 0C
                           KEY_LINEFEED,    // CTRL-M code 0D
    MOD_CTRL |             KEY_N,           // CTRL-N code 0E
    MOD_CTRL |             KEY_O,           // CTRL-O code 0F
    MOD_CTRL |             KEY_P,           // CTRL-P code 10
    MOD_CTRL |             KEY_Q,           // CTRL-Q code 11
    MOD_CTRL |             KEY_R,           // CTRL-R code 12
    MOD_CTRL |             KEY_S,           // CTRL-S code 13
    MOD_CTRL |             KEY_T,           // CTRL-T code 14
                           KEY_END,         // CTRL-U code 15
    MOD_CTRL |             KEY_V,           // CTRL-V code 16
    MOD_CTRL |             KEY_W,           // CTRL-W code 17
    MOD_CTRL |             KEY_X,           // CTRL-X code 18
    MOD_CTRL |             KEY_Y,           // CTRL-Y code 19
    MOD_CTRL |             KEY_Z,           // CTRL-Z code 1A
    MOD_CTRL |             KEY_ESC,         // ESCAPE code 1B
    MOD_CTRL |             KEY_BACKSLASH,   // CTRL-\ code 1C
    MOD_CTRL |             KEY_RIGHTBRACE,  // CTRL-] code 1D
    MOD_CTRL |             KEY_6,           // CTRL-6 code 1E
    MOD_CTRL |             KEY_MINUS,       // CTRL-- code 1F
                           KEY_SPACE,       // ' '    code 20
               MOD_SHIFT | KEY_F1,          // !      code 21
               MOD_SHIFT | KEY_APOSTROPHE,  // "      code 22
               MOD_SHIFT | KEY_F3,          // #      code 23
               MOD_SHIFT | KEY_F4,          // $      code 24
               MOD_SHIFT | KEY_F5,          // %      code 25
               MOD_SHIFT | KEY_F7,          // &      code 26
                           KEY_APOSTROPHE,  // '      code 27
               MOD_SHIFT | KEY_F9,          // (      code 28
               MOD_SHIFT | KEY_F10,         // )      code 29
               MOD_SHIFT | KEY_F8,          // *      code 2A
               MOD_SHIFT | KEY_F12,         // +      code 2B
                           KEY_COMMA,       // ,      code 2C
                           KEY_F11,         // -      code 2D
                           KEY_DOT,         // .      code 2E
                           KEY_SLASH,       // /      code 2F
                           KEY_F10,         // 0      code 30
                           KEY_F1,          // 1      code 31
                           KEY_F2,          // 2      code 32
                           KEY_F3,          // 3      code 33
                           KEY_F4,          // 4      code 34
                           KEY_F5,          // 5      code 35
                           KEY_F6,          // 6      code 36
                           KEY_F7,          // 7      code 37
                           KEY_F8,          // 8      code 38
                           KEY_F9,          // 9      code 39
               MOD_SHIFT | KEY_SEMICOLON,   // :      code 3A
                           KEY_SEMICOLON,   // ;      code 3B
               MOD_SHIFT | KEY_COMMA,       // <      code 3C
                           KEY_F12,         // =      code 3D
               MOD_SHIFT | KEY_DOT,         // >      code 3E
               MOD_SHIFT | KEY_SLASH,       // ?      code 3F
               MOD_SHIFT | KEY_F2,          // @      code 40
               MOD_SHIFT | KEY_A,           // A      code 41
               MOD_SHIFT | KEY_B,           // B      code 42
               MOD_SHIFT | KEY_C,           // C      code 43
               MOD_SHIFT | KEY_D,           // D      code 44
               MOD_SHIFT | KEY_E,           // E      code 45
               MOD_SHIFT | KEY_F,           // F      code 46
               MOD_SHIFT | KEY_G,           // G      code 47
               MOD_SHIFT | KEY_H,           // H      code 48
               MOD_SHIFT | KEY_I,           // I      code 49
               MOD_SHIFT | KEY_J,           // J      code 4A
               MOD_SHIFT | KEY_K,           // K      code 4B
               MOD_SHIFT | KEY_L,           // L      code 4C
               MOD_SHIFT | KEY_M,           // M      code 4D
               MOD_SHIFT | KEY_N,           // N      code 4E
               MOD_SHIFT | KEY_O,           // O      code 4F
               MOD_SHIFT | KEY_P,           // P      code 50
               MOD_SHIFT | KEY_Q,           // Q      code 51
               MOD_SHIFT | KEY_R,           // R      code 52
               MOD_SHIFT | KEY_S,           // S      code 53
               MOD_SHIFT | KEY_T,           // T      code 54
               MOD_SHIFT | KEY_U,           // U      code 55
               MOD_SHIFT | KEY_V,           // V      code 56
               MOD_SHIFT | KEY_W,           // W      code 57
               MOD_SHIFT | KEY_X,           // X      code 58
               MOD_SHIFT | KEY_Y,           // Y      code 59
               MOD_SHIFT | KEY_Z,           // Z      code 5A
                           KEY_LEFTBRACE,   // [      code 5B
                           KEY_BACKSLASH,   // \      code 5C
                           KEY_RIGHTBRACE,  // ]      code 5D
               MOD_SHIFT | KEY_F6,          // ^      code 5E
               MOD_SHIFT | KEY_F11,         // _      code 5F
                           KEY_GRAVE,       // `      code 60
                           KEY_A,           // a      code 61
                           KEY_B,           // b      code 62
                           KEY_C,           // c      code 63
                           KEY_D,           // d      code 64
                           KEY_E,           // e      code 65
                           KEY_F,           // f      code 66
                           KEY_G,           // g      code 67
                           KEY_H,           // h      code 68
                           KEY_I,           // i      code 69
                           KEY_J,           // j      code 6A
                           KEY_K,           // k      code 6B
                           KEY_L,           // l      code 6C
                           KEY_M,           // m      code 6D
                           KEY_N,           // n      code 6E
                           KEY_O,           // o      code 6F
                           KEY_P,           // p      code 70
                           KEY_Q,           // q      code 71
                           KEY_R,           // r      code 72
                           KEY_S,           // s      code 73
                           KEY_T,           // t      code 74
                           KEY_U,           // u      code 75
                           KEY_V,           // v      code 76
                           KEY_W,           // w      code 77
                           KEY_X,           // x      code 78
                           KEY_Y,           // y      code 79
                           KEY_Z,           // z      code 7A
               MOD_SHIFT | KEY_LEFTBRACE,   // {      code 7B
               MOD_SHIFT | KEY_BACKSLASH,   // |      code 7C
               MOD_SHIFT | KEY_RIGHTBRACE,  // }      code 7D
               MOD_SHIFT | KEY_GRAVE,       // ~      code 7E
                           KEY_DELETE       // DELETE code 7F
};
