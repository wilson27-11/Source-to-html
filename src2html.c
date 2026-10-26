/*
 * src2html.c - turn a C source file into a colored HTML page
 *
 * Usage : ./src2html input.c [output.html] 
 *         default output name is <input>.html  (e.g. sample.c -> sample.c.html)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Keyword tables used for token classification
// Control-flow keywords
static const char *control_kw[] = {
    "if", "else", "for", "while", "do", "switch", "case", "default",
    "break", "continue", "return", "goto", NULL
};

// Storage class, type qualifiers, and related keywords
static const char *storage_kw[] = {
    "auto", "const", "enum", "extern", "inline", "register", "restrict",
    "sizeof", "static", "struct", "typedef", "union", "volatile", NULL
};

// Built-in and commonly used C data types
static const char *type_kw[] = {
    "void", "char", "short", "int", "long", "float", "double", "signed",
    "unsigned", "_Bool", "size_t", "FILE", "uint8_t", "uint16_t",
    "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t", NULL
};

// Predefined constants and literals
static const char *const_kw[] = { "NULL", "EOF", "true", "false", NULL };

// Check whether a word exists in the specified keyword list
static int in_list(const char *w, size_t len, const char **list)
{
    for (int i = 0; list[i]; i++)
        if (strlen(list[i]) == len && strncmp(list[i], w, len) == 0)
            return 1;
    return 0;
}

static FILE *out;
static int line_no = 0;

// for line number
static void start_line(void)
{
    fprintf(out, "<div class=\"l\"><span class=\"n\">%d</span>", ++line_no);
}

// end line
static void end_line(void)
{
    fputs("</div>\n", out);
}

// Output a character with HTML escaping for special symbols
static void put_esc(char c)
{
    switch (c)
    {
    case '<':  fputs("&lt;", out);   break;
    case '>':  fputs("&gt;", out);   break;
    case '&':  fputs("&amp;", out);  break;
    case '"':  fputs("&quot;", out); break;
    case '\'': fputs("&#39;", out);  break;
    default:   fputc(c, out);
    }
}

/* Emit text with an optional CSS class. Spans never cross a line <div>,
 * so multi-line comments are closed and reopened at every newline. */
static void emit(const char *s, size_t n, const char *cls)
{
    if (cls) fprintf(out, "<span class=\"%s\">", cls);

    for (size_t i = 0; i < n; i++)
    {
        if (s[i] == '\n') {
            if (cls) fputs("</span>", out);
            end_line();
            start_line();

            if (cls)
                fprintf(out, "<span class=\"%s\">", cls);
        }
        else if (s[i] == '\r') {
            continue;
        }
        else {
            put_esc(s[i]);
        }
    }
    if (cls) fputs("</span>", out);
}

// header and color code for html
static void write_head(const char *title)
{
    fputs("<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
          "<title>", out);
    for (const char *p = title; *p; p++) put_esc(*p);
    fputs(
        "</title>\n<style>\n"
        "body{margin:0;background:#1e1e1e;}\n"
        ".code{font-family:Consolas,'Courier New',monospace;font-size:14px;"
        "line-height:1.5;color:#d4d4d4;background:#1e1e1e;padding:16px 0;"
        "overflow-x:auto;}\n"
        ".l{white-space:pre;min-height:1.5em;}\n"
        ".n{display:inline-block;width:3.5em;padding-right:1.5em;text-align:right;"
        "color:#858585;user-select:none;}\n"
        ".kw{color:#569cd6;}\n"                    // storage keywords
        ".ct{color:#c586c0;}\n"                    // control flow
        ".ty{color:#4ec9b0;}\n"                    // types
        ".fn{color:#dcdcaa;}\n"                    // function names
        ".str{color:#ce9178;}\n"                   // strings / chars
        ".num{color:#b5cea8;}\n"                   // numbers
        ".pp{color:#c586c0;}\n"                    // preprocessor
        ".cn{color:#4fc1ff;}\n"                    // NULL, EOF, ...
        ".op{color:#d4d4d4;}\n"                    // operators
        ".cm{color:#6a9955;font-style:italic;}\n"  // comments
        "</style>\n</head>\n<body>\n<div class=\"code\">\n",
        out);
}

// tail of html
static void write_tail(void)
{
    fputs("</div>\n</body>\n</html>\n", out);
}

// tokenizer
static void highlight(const char *src, size_t n)
{
    size_t i = 0;
    int at_line_start = 1;

    start_line();
    while (i < n)
    {
        char c = src[i];
        char next = (i + 1 < n) ? src[i + 1] : '\0';

        // whitespace
        if (c == '\n')
        {
            emit("\n", 1, NULL);
            at_line_start = 1;
            i++;
            continue;
        }
        if (isspace((unsigned char)c))
        {
            emit(&src[i], 1, NULL);
            i++;
            continue;
        }

        // comment line
        if (c == '/' && next == '/')
        {
            size_t j = i;

            while (j < n && src[j] != '\n') j++;

            emit(src + i, j - i, "cm");
            i = j; at_line_start = 0;
            continue;
        }

        // block comment
        if (c == '/' && next == '*')
        {
            size_t j = i + 2;
            while (j + 1 < n && !(src[j] == '*' && src[j + 1] == '/')) j++;

            j = (j + 1 < n) ? j + 2 : n;

            emit(src + i, j - i, "cm");
            i = j; at_line_start = 0;
            continue;
        }

        // preprocessor directive
        if (c == '#' && at_line_start)
        {
            size_t j = i + 1;

            while (j < n && (src[j] == ' ' || src[j] == '\t')) j++;

            while (j < n && isalpha((unsigned char)src[j])) j++;

            emit(src + i, j - i, "pp");
            i = j; at_line_start = 0;

            // <header.h> after #include
            size_t k = i;
            while (k < n && (src[k] == ' ' || src[k] == '\t')) k++;

            if (k < n && src[k] == '<')
            {
                size_t e = k;
                while (e < n && src[e] != '>' && src[e] != '\n') e++;

                if (e < n && src[e] == '>')
                {
                    emit(src + i, k - i, NULL);
                    emit(src + k, e - k + 1, "str");
                    i = e + 1;
                }
            }
            continue;
        }
        at_line_start = 0;

        // string / char literal
        if (c == '"' || c == '\'')
        {
            size_t j = i + 1;
            while (j < n && src[j] != c && src[j] != '\n')
            {
                if (src[j] == '\\' && j + 1 < n) j++;
                j++;
            }

            if (j < n && src[j] == c) j++;
            emit(src + i, j - i, "str");
            i = j;
            continue;
        }

        /* number */
        if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)next)))
        {
            size_t j = i;
            while (j < n && (isalnum((unsigned char)src[j]) || src[j] == '.' ||
                   ((src[j] == '+' || src[j] == '-') &&
                    (src[j - 1] == 'e' || src[j - 1] == 'E'))))
                j++;

            emit(src + i, j - i, "num");
            i = j; continue;
        }

        // identifier / keyword / function name
        if (isalpha((unsigned char)c) || c == '_')
        {
            size_t j = i;

            while (j < n && (isalnum((unsigned char)src[j]) || src[j] == '_')) j++;

            size_t len = j - i;
            const char *cls = NULL;

            if      (in_list(src + i, len, control_kw)) cls = "ct";
            else if (in_list(src + i, len, storage_kw)) cls = "kw";
            else if (in_list(src + i, len, type_kw))    cls = "ty";
            else if (in_list(src + i, len, const_kw))   cls = "cn";
            else
            {
                size_t k = j;
                while (k < n && (src[k] == ' ' || src[k] == '\t')) k++;
                if (k < n && src[k] == '(') cls = "fn";
            }
            emit(src + i, len, cls);
            i = j;
            continue;
        }

        // operators
        if (strchr("+-*/%=<>!&|^~?:.", c)) {
            emit(&src[i], 1, "op");
            i++; continue;
        }

        // everything else: ( ) [ ] { } ; ,
        emit(&src[i], 1, NULL);
        i++;
    }
    end_line();
}

// read .c file and store it in buffer
static char *read_file(const char *path, size_t *len)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    // empty file
    if (size < 0) { fclose(fp); return NULL; }

    // allocate memory
    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(fp); return NULL; }

    // read and store
    *len = fread(buf, 1, (size_t)size, fp);
    buf[*len] = '\0';

    fclose(fp);
    return buf;
}

// main
int main(int argc, char *argv[])
{
    const char *in_path = NULL, *out_path = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (!in_path)          in_path = argv[i];
        else if (!out_path)    out_path = argv[i];
    }
    if (!in_path)
    {
        fprintf(stderr, "Usage: %s input.c [output.html]\n", argv[0]);
        return 1;
    }

    char default_out[1024];
    if (!out_path)
    {
        snprintf(default_out, sizeof default_out, "%s.html", in_path);
        out_path = default_out;
    }

    size_t size;
    char *src = read_file(in_path, &size);

    if (!src) { perror(in_path); return 1; }

    out = fopen(out_path, "w");
    if (!out) { perror(out_path); free(src); return 1; }

    write_head(in_path);
    highlight(src, size);
    write_tail();

    fclose(out);
    free(src);
    printf("%s html file was created\n", out_path);

    return 0;
}
