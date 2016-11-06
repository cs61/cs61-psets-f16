#include "sh61.h"
#include <ctype.h>

typedef struct buildstring {
    char* s;
    int length;
    int capacity;
} buildstring;


// buildstring_append(bstr, ch)
//    Add `ch` to the end of the dynamically-allocated string `bstr->s`.

void buildstring_append(buildstring* bstr, int ch) {
    if (bstr->length == bstr->capacity) {
        int new_capacity = bstr->capacity ? bstr->capacity * 2 : 32;
        bstr->s = (char*) realloc(bstr->s, new_capacity);
        bstr->capacity = new_capacity;
    }
    bstr->s[bstr->length] = ch;
    ++bstr->length;
}


// isshellspecial(ch)
//    Test if `ch` is a command that's special to the shell (that ends
//    a command word).

static inline int isshellspecial(int ch) {
    return ch == '<' || ch == '>' || ch == '&' || ch == '|' || ch == ';'
        || ch == '(' || ch == ')' || ch == '#';
}


// parse_shell_token(str, type, token)
//    Parse the next token from the shell command `str`. Stores the type of
//    the token in `*type`; this is one of the TOKEN_ constants. Stores the
//    token itself in `*token`; this string should be freed eventually with
//    `free`. Advances `str` to the next token and returns that pointer.
//
//    At the end of the string, returns NULL, sets `*type` to TOKEN_SEQUENCE,
//    and sets `*token` to NULL.

const char* parse_shell_token(const char* str, int* type, char** token) {
    buildstring buildtoken = { NULL, 0, 0 };

    // skip spaces; return NULL and token ";" at end of line
    while (str && isspace((unsigned char) *str))
        ++str;
    if (!str || !*str || *str == '#') {
        *type = TOKEN_SEQUENCE;
        *token = NULL;
        return NULL;
    }

    // check for a redirection or special token
    for (; isdigit((unsigned char) *str); ++str)
        buildstring_append(&buildtoken, *str);
    if (*str == '<' || *str == '>') {
        *type = TOKEN_REDIRECTION;
        buildstring_append(&buildtoken, *str);
        if (str[1] == '>') {
            buildstring_append(&buildtoken, str[1]);
            ++str;
        } else if (str[1] == '&' && isdigit((unsigned char) str[2])) {
            buildstring_append(&buildtoken, str[1]);
            for (str += 2; isdigit((unsigned char) *str); ++str)
                buildstring_append(&buildtoken, *str);
        }
        ++str;
    } else if (buildtoken.length == 0
               && (*str == '&' || *str == '|')
               && str[1] == *str) {
        *type = (*str == '&' ? TOKEN_AND : TOKEN_OR);
        buildstring_append(&buildtoken, *str);
        buildstring_append(&buildtoken, str[1]);
        str += 2;
    } else if (buildtoken.length == 0
               && isshellspecial((unsigned char) *str)) {
        switch (*str) {
        case ';': *type = TOKEN_SEQUENCE;   break;
        case '&': *type = TOKEN_BACKGROUND; break;
        case '|': *type = TOKEN_PIPE;       break;
        case '(': *type = TOKEN_LPAREN;     break;
        case ')': *type = TOKEN_RPAREN;     break;
        default:  *type = TOKEN_OTHER;      break;
        }
        buildstring_append(&buildtoken, *str);
        ++str;
    } else {
        // it's a normal token
        *type = TOKEN_NORMAL;
        int quoted = 0;
        // Read characters up to the end of the token.
        while ((*str && quoted)
               || (*str && !isspace((unsigned char) *str)
                   && !isshellspecial((unsigned char) *str))) {
            if ((*str == '\"' || *str == '\'') && !quoted)
                quoted = *str;
            else if (*str == quoted)
                quoted = 0;
            else if (*str == '\\' && str[1] != '\0' && quoted != '\'') {
                buildstring_append(&buildtoken, str[1]);
                ++str;
            } else
                buildstring_append(&buildtoken, *str);
            ++str;
        }
    }

    // store new token and return the location of the next token
    buildstring_append(&buildtoken, '\0'); // terminating NUL character
    *token = buildtoken.s;
    return str;
}


// set_foreground(pgid)
//    Mark `pgid` as the current foreground process group for this terminal.
//    This uses some ugly Unix warts, so we provide it for you.
int set_foreground(pid_t pgid) {
    // YOU DO NOT NEED TO UNDERSTAND THIS.

    // Initialize state first time we're called.
    static int ttyfd = -1;
    static int shell_owns_foreground = 0;
    static pid_t shell_pgid = -1;
    if (ttyfd < 0) {
        // We need a fd for the current terminal, so open /dev/tty.
        int fd = open("/dev/tty", O_RDWR);
        assert(fd >= 0);
        // Re-open to a large file descriptor (>=10) so that pipes and such
        // use the expected small file descriptors.
        ttyfd = fcntl(fd, F_DUPFD, 10);
        assert(ttyfd >= 0);
        close(fd);
        // The /dev/tty file descriptor should be closed in child processes.
        fcntl(ttyfd, F_SETFD, FD_CLOEXEC);
        // Only mess with /dev/tty's controlling process group if the shell
        // is in /dev/tty's controlling process group.
        shell_pgid = getpgrp();
        shell_owns_foreground = (shell_pgid == tcgetpgrp(ttyfd));
    }

    // Set the terminal's controlling process group to `p` (so processes in
    // group `p` can output to the screen, read from the keyboard, etc.).
    if (shell_owns_foreground && pgid)
        return tcsetpgrp(ttyfd, pgid);
    else if (shell_owns_foreground)
        return tcsetpgrp(ttyfd, shell_pgid);
    else
        return 0;
}
