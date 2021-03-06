#include "jsi.h"
#include "jslex.h"
#include "utf.h"
#include <ZUI.h>
JS_NORETURN static void jsY_error(js_State *J, const wchar_t *fmt, ...) JS_PRINTFLIKE(2, 3);
static void jsY_error(js_State *J, const wchar_t *fmt, ...)
{
    va_list ap;
    wchar_t buf[512];
    wchar_t msgbuf[256];

    va_start(ap, fmt);
    _snwprintf(msgbuf, 256, fmt, ap);
    va_end(ap);

    _snwprintf(buf, 256, L"%ls:%d: ", J->filename, J->lexline);
    wcscat(buf, msgbuf);

    js_newsyntaxerror(J, buf);
    js_throw(J);
}

static const wchar_t *tokenstring[] = {
L"(end-of-file)",
L"\\x01", L"\\x02", L"\\x03", L"\\x04", L"\\x05", L"\\x06", L"\\x07",
L"\\x08", L"\\x09", L"\\x0A", L"\\x0B", L"\\x0C", L"\\x0D", L"\\x0E", L"\\x0F",
L"\\x10", L"\\x11", L"\\x12", L"\\x13", L"\\x14", L"\\x15", L"\\x16", L"\\x17",
L"\\x18", L"\\x19", L"\\x1A", L"\\x1B", L"\\x1C", L"\\x1D", L"\\x1E", L"\\x1F",
L" ", L"!", L"\"", L"#", L"$", L"%", L"&", L"\\'",
L"(", L")", L"*", L"+", L",", L"-", L".", L"/",
L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7",
L"8", L"9", L":", L";", L"<", L"=", L">", L"?",
L"@", L"A", L"B", L"C", L"D", L"E", L"F", L"G",
L"H", L"I", L"J", L"K", L"L", L"M", L"N", L"O",
L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W",
L"X", L"Y", L"Z", L"[", L"\\", L"]", L"^", L"_",
L"`", L"a", L"b", L"c", L"d", L"e", L"f", L"g",
L"h", L"i", L"j", L"k", L"l", L"m", L"n", L"o",
L"p", L"q", L"r", L"s", L"t", L"u", L"v", L"w",
L"x", L"y", L"z", L"{", L"|", L"}", L"~", L"\\x7F",

0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

L"(identifier)", L"(number)", L"(string)", L"(regexp)",

L"<=", L">=", L"==", L"!=", L"===", L"!==",
L"<<", L">>", L">>>", L"&&", L"||",
L"+=", L"-=", L"*=", L"/=", L"%=",
L"<<=", L">>=", L">>>=", L"&=", L"|=", L"^=",
L"++", L"--",

L"break", L"case", L"catch", L"continue", L"debugger",
L"default", L"delete", L"do", L"else", L"false", L"finally", L"for",
L"function", L"if", L"in", L"instanceof", L"new", L"null", L"return",
L"switch", L"this", L"throw", L"true", L"try", L"typeof", L"var",
L"void", L"while", L"with",
};

const wchar_t *jsY_tokenstring(int token)
{
    if (token >= 0 && token < (int)nelem(tokenstring))
        if (tokenstring[token])
            return tokenstring[token];
    return L"<unknown>";
}

static const wchar_t *keywords[] = {
L"break", L"case", L"catch", L"continue", L"debugger", L"default", L"delete",
L"do", L"else", L"false", L"finally", L"for", L"function", L"if", L"in",
L"instanceof", L"new", L"null", L"return", L"switch", L"this", L"throw",
L"true", L"try", L"typeof", L"var", L"void", L"while", L"with",
};
static const wchar_t *keywords_CN[] = {
L"break", L"case", L"catch", L"跳出循环", L"debugger", L"default", L"delete",
L"do", L"那么", L"假", L"finally", L"计次循环", L"函数", L"如果", L"in",
L"instanceof", L"new", L"空", L"返回", L"switch", L"this", L"throw",
L"真", L"try", L"typeof", L"变量", L"void", L"while", L"with",
};
int jsY_findword(const wchar_t *s, const wchar_t **list, int num)
{
    int l = 0;
    int r = num - 1;
    while (l <= r) {
        int m = (l + r) >> 1;
        int c = wcscmp(s, list[m]);
        if (c < 0)
            r = m - 1;
        else if (c > 0)
            l = m + 1;
        else
            return m;
    }
    return -1;
}
int jsY_findword_CN(const wchar_t *s, const wchar_t **list, int num)
{
    for (int i = 0; i < num; i++)
    {
        if (wcscmp(s, list[i]) == 0)
            return i;
    }
    return -1;
}
static int jsY_findkeyword(js_State *J, const wchar_t *s)
{
    int i = jsY_findword(s, keywords, nelem(keywords));
    if (i == -1)
        i = jsY_findword_CN(s, keywords_CN, nelem(keywords_CN));
    if (i >= 0) {
        J->text = keywords[i];
        return TK_BREAK + i; /* first keyword + i */
    }
    J->text = js_intern(J, s);
    return TK_IDENTIFIER;
}

int jsY_iswhite(int c)
{
    return c == 0x9 || c == 0xB || c == 0xC || c == 0x20 || c == 0xA0 || c == 0xFEFF;
}

int jsY_isnewline(int c)
{
    return c == 0xA || c == 0xD || c == 0x2028 || c == 0x2029;
}

#define isalpha(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define isdigit(c) (c >= '0' && c <= '9')
#define ishex(c) ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))

static int jsY_isidentifierstart(int c)
{
    return isalpha(c) || c == '$' || c == '_' || isalpharune(c);
}

static int jsY_isidentifierpart(int c)
{
    return isdigit(c) || isalpha(c) || c == '$' || c == '_' || isalpharune(c);
}

static int jsY_isdec(int c)
{
    return isdigit(c);
}

int jsY_ishex(int c)
{
    return isdigit(c) || ishex(c);
}

int jsY_tohex(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 0xA;
    if (c >= 'A' && c <= 'F') return c - 'A' + 0xA;
    return 0;
}

static void jsY_next(js_State *J)
{
    Rune c = *J->source;
    J->source++;
    /* consume CR LF as one unit */
    if (c == '\r' && *J->source == '\n')
        ++J->source;
    if (jsY_isnewline(c)) {
        J->line++;
        c = '\n';
    }
    J->lexchar = c;
}

#define jsY_accept(J, x) (J->lexchar == x ? (jsY_next(J), 1) : 0)

#define jsY_expect(J, x) if (!jsY_accept(J, x)) jsY_error(J, L"expected '%c'", x)

static void jsY_unescape(js_State *J)
{
    if (jsY_accept(J, '\\')) {
        if (jsY_accept(J, 'u')) {
            int x = 0;
            if (!jsY_ishex(J->lexchar)) goto error; x |= jsY_tohex(J->lexchar) << 12; jsY_next(J);
            if (!jsY_ishex(J->lexchar)) goto error; x |= jsY_tohex(J->lexchar) << 8; jsY_next(J);
            if (!jsY_ishex(J->lexchar)) goto error; x |= jsY_tohex(J->lexchar) << 4; jsY_next(J);
            if (!jsY_ishex(J->lexchar)) goto error; x |= jsY_tohex(J->lexchar);
            J->lexchar = x;
            return;
        }
    error:
        jsY_error(J, L"unexpected escape sequence");
    }
}

static void textinit(js_State *J)
{
    if (!J->lexbuf.text) {
        J->lexbuf.cap = 4096;
        J->lexbuf.text = ZuiMalloc(J->lexbuf.cap);
    }
    J->lexbuf.len = 0;
}

static void textpush(js_State *J, Rune c)
{
    if (J->lexbuf.len + 1 > J->lexbuf.cap) {
        J->lexbuf.cap = J->lexbuf.cap * 2;
        J->lexbuf.text = ZuiRealloc(J->lexbuf.text, J->lexbuf.cap);
    }
    *(J->lexbuf.text + J->lexbuf.len) = c;
    J->lexbuf.len++;
}

static wchar_t *textend(js_State *J)
{
    textpush(J, 0);
    return J->lexbuf.text;
}

static void lexlinecomment(js_State *J)
{
    while (J->lexchar && J->lexchar != '\n')
        jsY_next(J);
}

static int lexcomment(js_State *J)
{
    /* already consumed initial '/' '*' sequence */
    while (J->lexchar != 0) {
        if (jsY_accept(J, '*')) {
            while (J->lexchar == '*')
                jsY_next(J);
            if (jsY_accept(J, '/'))
                return 0;
        }
        else
            jsY_next(J);
    }
    return -1;
}

static double lexhex(js_State *J)
{
    double n = 0;
    if (!jsY_ishex(J->lexchar))
        jsY_error(J, L"malformed hexadecimal number");
    while (jsY_ishex(J->lexchar)) {
        n = n * 16 + jsY_tohex(J->lexchar);
        jsY_next(J);
    }
    return n;
}

static int lexnumber(js_State *J)
{
    const wchar_t *s = J->source - 1;

    if (jsY_accept(J, '0')) {
        if (jsY_accept(J, 'x') || jsY_accept(J, 'X')) {
            J->number = lexhex(J);
            return TK_NUMBER;
        }
        if (jsY_isdec(J->lexchar))
            jsY_error(J, L"number with leading zero");
        if (jsY_accept(J, '.')) {
            while (jsY_isdec(J->lexchar))
                jsY_next(J);
        }
    }
    else if (jsY_accept(J, '.')) {
        if (!jsY_isdec(J->lexchar))
            return '.';
        while (jsY_isdec(J->lexchar))
            jsY_next(J);
    }
    else {
        while (jsY_isdec(J->lexchar))
            jsY_next(J);
        if (jsY_accept(J, '.')) {
            while (jsY_isdec(J->lexchar))
                jsY_next(J);
        }
    }

    if (jsY_accept(J, 'e') || jsY_accept(J, 'E')) {
        if (J->lexchar == '-' || J->lexchar == '+')
            jsY_next(J);
        while (jsY_isdec(J->lexchar))
            jsY_next(J);
    }

    if (jsY_isidentifierstart(J->lexchar))
        jsY_error(J, L"number with letter suffix");

    J->number = js_strtod(s, NULL);
    return TK_NUMBER;

}


static int lexescape(js_State *J)
{
    int x = 0;

    /* already consumed '\' */

    if (jsY_accept(J, '\n'))
        return 0;

    switch (J->lexchar) {
    case 0: jsY_error(J, L"unterminated escape sequence");
    case 'u':
        jsY_next(J);
        if (!jsY_ishex(J->lexchar)) return 1; else { x |= jsY_tohex(J->lexchar) << 12; jsY_next(J); }
        if (!jsY_ishex(J->lexchar)) return 1; else { x |= jsY_tohex(J->lexchar) << 8; jsY_next(J); }
        if (!jsY_ishex(J->lexchar)) return 1; else { x |= jsY_tohex(J->lexchar) << 4; jsY_next(J); }
        if (!jsY_ishex(J->lexchar)) return 1; else { x |= jsY_tohex(J->lexchar); jsY_next(J); }
        textpush(J, x);
        break;
    case 'x':
        jsY_next(J);
        if (!jsY_ishex(J->lexchar)) return 1; else { x |= jsY_tohex(J->lexchar) << 4; jsY_next(J); }
        if (!jsY_ishex(J->lexchar)) return 1; else { x |= jsY_tohex(J->lexchar); jsY_next(J); }
        textpush(J, x);
        break;
    case '0': textpush(J, 0); jsY_next(J); break;
    case '\\': textpush(J, '\\'); jsY_next(J); break;
    case '\'': textpush(J, '\''); jsY_next(J); break;
    case '"': textpush(J, '"'); jsY_next(J); break;
    case 'b': textpush(J, '\b'); jsY_next(J); break;
    case 'f': textpush(J, '\f'); jsY_next(J); break;
    case 'n': textpush(J, '\n'); jsY_next(J); break;
    case 'r': textpush(J, '\r'); jsY_next(J); break;
    case 't': textpush(J, '\t'); jsY_next(J); break;
    case 'v': textpush(J, '\v'); jsY_next(J); break;
    default: textpush(J, J->lexchar); jsY_next(J); break;
    }
    return 0;
}

static int lexstring(js_State *J)
{
    const wchar_t *s;

    int q = J->lexchar;
    jsY_next(J);

    textinit(J);

    while (J->lexchar != q) {
        if (J->lexchar == 0 || J->lexchar == '\n')
            jsY_error(J, L"string not terminated");
        if (jsY_accept(J, '\\')) {
            if (lexescape(J))
                jsY_error(J, L"malformed escape sequence");
        }
        else {
            textpush(J, J->lexchar);
            jsY_next(J);
        }
    }
    jsY_expect(J, q);

    s = textend(J);

    J->text = js_intern(J, s);
    return TK_STRING;
}

/* the ugliest language wart ever... */
static int isregexpcontext(int last)
{
    switch (last) {
    case ']':
    case ')':
    case '}':
    case TK_IDENTIFIER:
    case TK_NUMBER:
    case TK_STRING:
    case TK_FALSE:
    case TK_NULL:
    case TK_THIS:
    case TK_TRUE:
        return 0;
    default:
        return 1;
    }
}

static int lexregexp(js_State *J)
{
    const wchar_t *s;
    int g, m, i;
    int inclass = 0;

    /* already consumed initial '/' */

    textinit(J);

    /* regexp body */
    while (J->lexchar != '/' || inclass) {
        if (J->lexchar == 0 || J->lexchar == '\n') {
            jsY_error(J, L"regular expression not terminated");
        }
        else if (jsY_accept(J, '\\')) {
            if (jsY_accept(J, '/')) {
                textpush(J, '/');
            }
            else {
                textpush(J, '\\');
                if (J->lexchar == 0 || J->lexchar == '\n')
                    jsY_error(J, L"regular expression not terminated");
                textpush(J, J->lexchar);
                jsY_next(J);
            }
        }
        else {
            if (J->lexchar == '[' && !inclass)
                inclass = 1;
            if (J->lexchar == ']' && inclass)
                inclass = 0;
            textpush(J, J->lexchar);
            jsY_next(J);
        }
    }
    jsY_expect(J, '/');

    s = textend(J);

    /* regexp flags */
    g = i = m = 0;

    while (jsY_isidentifierpart(J->lexchar)) {
        if (jsY_accept(J, 'g')) ++g;
        else if (jsY_accept(J, 'i')) ++i;
        else if (jsY_accept(J, 'm')) ++m;
        else jsY_error(J, L"illegal flag in regular expression: %c", J->lexchar);
    }

    if (g > 1 || i > 1 || m > 1)
        jsY_error(J, L"duplicated flag in regular expression");

    J->text = js_intern(J, s);
    J->number = 0;
    if (g) J->number += JS_REGEXP_G;
    if (i) J->number += JS_REGEXP_I;
    if (m) J->number += JS_REGEXP_M;
    return TK_REGEXP;
}

/* simple "return [no Line Terminator here] ..." contexts */
static int isnlthcontext(int last)
{
    switch (last) {
    case TK_BREAK:
    case TK_CONTINUE:
    case TK_RETURN:
    case TK_THROW:
        return 1;
    default:
        return 0;
    }
}

static int jsY_lexx(js_State *J)
{
    J->newline = 0;

    while (1) {
        J->lexline = J->line; /* save location of beginning of token */

        while (jsY_iswhite(J->lexchar))
            jsY_next(J);

        if (jsY_accept(J, '\n')) {
            J->newline = 1;
            if (isnlthcontext(J->lasttoken))
                return ';';
            continue;
        }

        if (jsY_accept(J, '/')) {
            if (jsY_accept(J, '/')) {
                lexlinecomment(J);
                continue;
            }
            else if (jsY_accept(J, '*')) {
                if (lexcomment(J))
                    jsY_error(J, L"multi-line comment not terminated");
                continue;
            }
            else if (isregexpcontext(J->lasttoken)) {
                return lexregexp(J);
            }
            else if (jsY_accept(J, '=')) {
                return TK_DIV_ASS;
            }
            else {
                return '/';
            }
        }

        if (J->lexchar >= '0' && J->lexchar <= '9') {
            return lexnumber(J);
        }

        switch (J->lexchar) {
        case '(': jsY_next(J); return '(';
        case ')': jsY_next(J); return ')';
        case ',': jsY_next(J); return ',';
        case ':': jsY_next(J); return ':';
        case ';': jsY_next(J); return ';';
        case '?': jsY_next(J); return '?';
        case '[': jsY_next(J); return '[';
        case ']': jsY_next(J); return ']';
        case '{': jsY_next(J); return '{';
        case '}': jsY_next(J); return '}';
        case '~': jsY_next(J); return '~';

        case '\'':
        case '"':
            return lexstring(J);

        case '.':
            return lexnumber(J);

        case '<':
            jsY_next(J);
            if (jsY_accept(J, '<')) {
                if (jsY_accept(J, '='))
                    return TK_SHL_ASS;
                return TK_SHL;
            }
            if (jsY_accept(J, '='))
                return TK_LE;
            return '<';

        case '>':
            jsY_next(J);
            if (jsY_accept(J, '>')) {
                if (jsY_accept(J, '>')) {
                    if (jsY_accept(J, '='))
                        return TK_USHR_ASS;
                    return TK_USHR;
                }
                if (jsY_accept(J, '='))
                    return TK_SHR_ASS;
                return TK_SHR;
            }
            if (jsY_accept(J, '='))
                return TK_GE;
            return '>';

        case '=':
            jsY_next(J);
            if (jsY_accept(J, '=')) {
                if (jsY_accept(J, '='))
                    return TK_STRICTEQ;
                return TK_EQ;
            }
            return '=';

        case '!':
            jsY_next(J);
            if (jsY_accept(J, '=')) {
                if (jsY_accept(J, '='))
                    return TK_STRICTNE;
                return TK_NE;
            }
            return '!';

        case '+':
            jsY_next(J);
            if (jsY_accept(J, '+'))
                return TK_INC;
            if (jsY_accept(J, '='))
                return TK_ADD_ASS;
            return '+';

        case '-':
            jsY_next(J);
            if (jsY_accept(J, '-'))
                return TK_DEC;
            if (jsY_accept(J, '='))
                return TK_SUB_ASS;
            return '-';

        case '*':
            jsY_next(J);
            if (jsY_accept(J, '='))
                return TK_MUL_ASS;
            return '*';

        case '%':
            jsY_next(J);
            if (jsY_accept(J, '='))
                return TK_MOD_ASS;
            return '%';

        case '&':
            jsY_next(J);
            if (jsY_accept(J, '&'))
                return TK_AND;
            if (jsY_accept(J, '='))
                return TK_AND_ASS;
            return '&';

        case '|':
            jsY_next(J);
            if (jsY_accept(J, '|'))
                return TK_OR;
            if (jsY_accept(J, '='))
                return TK_OR_ASS;
            return '|';

        case '^':
            jsY_next(J);
            if (jsY_accept(J, '='))
                return TK_XOR_ASS;
            return '^';

        case 0:
            return 0; /* EOF */
        }

        /* Handle \uXXXX escapes in identifiers */
        jsY_unescape(J);
        if (jsY_isidentifierstart(J->lexchar)) {
            textinit(J);
            textpush(J, J->lexchar);

            jsY_next(J);
            jsY_unescape(J);
            while (jsY_isidentifierpart(J->lexchar)) {
                textpush(J, J->lexchar);
                jsY_next(J);
                jsY_unescape(J);
            }

            textend(J);

            return jsY_findkeyword(J, J->lexbuf.text);
        }

        if (J->lexchar >= 0x20 && J->lexchar <= 0x7E)
            jsY_error(J, L"unexpected character: '%c'", J->lexchar);
        jsY_error(J, L"unexpected character: \\u%04X", J->lexchar);
    }
}

void jsY_initlex(js_State *J, const wchar_t *filename, const wchar_t *source)
{
    J->filename = filename;
    J->source = source;
    J->line = 1;
    J->lasttoken = 0;
    jsY_next(J); /* load first lookahead character */
}

int jsY_lex(js_State *J)
{
    return J->lasttoken = jsY_lexx(J);
}

int jsY_lexjson(js_State *J)
{
    while (1) {
        J->lexline = J->line; /* save location of beginning of token */

        while (jsY_iswhite(J->lexchar) || J->lexchar == '\n')
            jsY_next(J);

        if (J->lexchar >= '0' && J->lexchar <= '9') {
            return lexnumber(J);
        }

        switch (J->lexchar) {
        case ',': jsY_next(J); return ',';
        case ':': jsY_next(J); return ':';
        case '[': jsY_next(J); return '[';
        case ']': jsY_next(J); return ']';
        case '{': jsY_next(J); return '{';
        case '}': jsY_next(J); return '}';

        case '"':
            return lexstring(J);

        case '.':
            return lexnumber(J);

        case 'f':
            jsY_next(J); jsY_expect(J, 'a'); jsY_expect(J, 'l'); jsY_expect(J, 's'); jsY_expect(J, 'e');
            return TK_FALSE;

        case 'n':
            jsY_next(J); jsY_expect(J, 'u'); jsY_expect(J, 'l'); jsY_expect(J, 'l');
            return TK_NULL;

        case 't':
            jsY_next(J); jsY_expect(J, 'r'); jsY_expect(J, 'u'); jsY_expect(J, 'e');
            return TK_TRUE;

        case 0:
            return 0; /* EOF */
        }

        if (J->lexchar >= 0x20 && J->lexchar <= 0x7E)
            jsY_error(J, L"unexpected character: '%c'", J->lexchar);
        jsY_error(J, L"unexpected character: \\u%04X", J->lexchar);
    }
}
