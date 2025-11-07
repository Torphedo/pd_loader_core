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
    LOG_MSG(info, "This is a test command. Hello, world!\n");
    for (u32 i = 0; i < argc; i++) {
        LOG_MSG(debug, "arg %d: '%s'\n", i, argv[i]);
    }

    return 0;
}

bool command_sys_init() {
    commands = hb_create(32, 32, sizeof(command_func));

    plugin_register_command(command_hello, "pd_loader_core", "hello");

    return commands.buckets != NULL;
}

void command_sys_deinit() {
    hb_destroy(&commands);
}

void plugin_register_command(command_func command, const char* module_name, const char* command_name) {
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
        LOG_MSG(error, "The command '%s' was empty.\n", txt);
        return false;
    }

    *argc_out = args.end_idx;
    *argv_out = list_get_element(args, 0);
    return true;
}

int exec_command(const char* command_txt) {
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
