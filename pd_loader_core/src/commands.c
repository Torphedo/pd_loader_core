#include "commands.h"
#include <string.h>
#include <stdio.h>

#include <windows.h>

#include <common/hashmap.h>
#include <common/list.h>
#include <common/parsing.h>
#include <common/logging.h>

hashbuckets_desc commands = {0};

int command_hello(int argc, char** argv) {
    LOG_MSG(info, "Hello, world!\n");
    for (u32 i = 0; i < argc; i++) {
        LOG_MSG(debug, "arg %d: '%s'\n", i, argv[i]);
    }

    // This matches --value and -v
    const char* val = command_getoption(argc, argv, "value", "v");
    if (val) {
        LOG_MSG(info, "You found the secret flag! The value is '%s'\n", val);
    }

    return 0;
}

bool command_sys_init() {
    commands = hb_create(32, 32, sizeof(command_func));

    command_register(command_hello, "core", "hello");

    return commands.buckets != NULL;
}

void command_sys_deinit() {
    hb_destroy(&commands);
}

void command_register(command_func command, const char* module_name, const char* command_name) {
    // If your command name is longer than this, you have bigger problems.
    char cmd_name[128] = {0};
    snprintf(cmd_name, ARRAY_SIZE(cmd_name), "%s!%s", module_name, command_name);

    const void* ptr = command;
    hb_add_obj(&commands, cmd_name, &ptr);

    LOG_MSG(info, "Registered command '%s'\n", cmd_name);
}

command_func plugin_get_command_func(const char* command_name) {
    command_func* func = hb_find_obj(&commands, command_name);
    if (func) {
        return *func;
    }

    return NULL;
}

bool parse_args(const char* txt, int* argc_out, char*** argv_out) {
    queue tokens = shatter_str(txt, strlen(txt), "\"", NULL, 0);
    list filtered_tokens = list_create(10, sizeof(substr_t));
    if (!tokens.buf.data || !filtered_tokens.buf.data) {
        LOG_MSG(error, "Allocation failure while parsing input string: '%s'\n", txt);
        queue_destroy(&tokens);
        list_destroy(&filtered_tokens);
        return false;
    }

    bool in_quotes = false;
    u32 num_quoted_tokens = 0;
    while (!queue_empty(tokens)) {
        substr_t tok = {0};
        queue_get(&tokens, &tok);

        const char* str = txt + tok.offset;
        if (str[0] == '"') {
            in_quotes = !in_quotes;
            if (!in_quotes) {
                num_quoted_tokens = 0; // We're leaving the quotes, reset this
            }
            continue; // Cut out the quote tokens
        }

        if (in_quotes) {
            substr_t* prev_tok = list_get_element(filtered_tokens, filtered_tokens.end_idx - 1);
            num_quoted_tokens++;
            if (prev_tok && num_quoted_tokens > 1) {
                // Extend the last token to include this one
                prev_tok->length = (tok.offset + tok.length) - prev_tok->offset;
                continue;
            }
        }

        list_add(&filtered_tokens, &tok);
    }

    // Clone substrings to be separate buffers as a C main() expects
    list args = list_create(10, sizeof(char*));
    for (u32 i = 0; i < filtered_tokens.end_idx; i++) {
        substr_t* tok = list_get_element(filtered_tokens, i);
        char* arg = calloc(1, tok->length + 2);
        if (arg) {
            strncpy(arg, (txt + tok->offset), tok->length);
            list_add(&args, &arg);
        }
    }
    queue_destroy(&tokens);
    list_destroy(&filtered_tokens);

    if (args.end_idx < 1) {
        return false;
    }

    *argc_out = args.end_idx;
    *argv_out = list_get_element(args, 0);
    return true;
}

int command_exec(const char* command_txt) {
    int argc = 0;
    char** argv = NULL;
    if (!parse_args(command_txt, &argc, &argv)) {
        return -1;
    }

    command_func cmd = plugin_get_command_func(argv[0]);
    if (!cmd) {
        LOG_MSG(error, "Command '%s' not found.\n", argv[0]);
        return -1;
    }

    const int result = (cmd)(argc, argv);
    for (u32 i = 0; i < argc; i++) {
        free(argv[i]);
    }

    return result;
}
char* command_getoption(int argc, char** argv, const char* option, const char* shorthand) {
    // Make our pointers always non-NULL
    option = (option) ? option : "";
    shorthand = (shorthand) ? shorthand : "";

    char* result = NULL;
    bool hit = false;
    for (u32 i = 0; i < argc; i++) {
        char* arg = argv[i];
        if (hit) {
            // Last arg was the flag, this must be the value
            result = arg;
            break;
        }

        // Skip up to 2 dashes to handle long and short hand flags
        for (u32 j = 0; j < 2; j++) {
            if (*arg == '-') {
                arg++;
            }
        }
        hit = (strcmp(arg, option) == 0) || (strcmp(arg, shorthand) == 0);
    }

    return result;
}

bool command_getflag(int argc, char** argv, const char* option, const char* shorthand) {
    // Make our pointers always non-NULL
    option = (option) ? option : "";
    shorthand = (shorthand) ? shorthand : "";

    char* result = NULL;
    for (u32 i = 0; i < argc; i++) {
        char* arg = argv[i];

        // Skip up to 2 dashes to handle long and short hand flags
        for (u32 j = 0; j < 2; j++) {
            if (*arg == '-') {
                arg++;
            }
        }
        bool hit = (strcmp(arg, option) == 0) || (strcmp(arg, shorthand) == 0);
        if (hit) {
            return true;
        }
    }

    return false;
}
