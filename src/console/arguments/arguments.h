#ifndef _CONSOLE_ARGUMENTS_ARGUMENTS_H
#define _CONSOLE_ARGUMENTS_ARGUMENTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>


struct console_args_subcommand {
 // struct console_args_command **commands;
  struct console_args_command **commands;
  size_t size, count;

  struct console_args_command *parent;

  bool defined;
};

struct console_args_command {
  const char *name, /* *type, */ *flags;
  bool sensitive, required;

  long required_values;

  const char *help, *description;
  bool defined, initialized;

  char **values;
  size_t values_count;

  struct console_args_subcommand subcommands;
};


extern void console_args_command_addsubcommands(struct console_args_command *, struct console_args_command *, size_t);
extern void console_args_command_addsubcommand(struct console_args_command *, struct console_args_command *);

extern struct console_args_command *console_args_command_subcommand(struct console_args_command *, const char *);

extern int console_args_command_subcommands_new(struct console_args_command *, struct console_args_command **, size_t);


typedef struct console_args_parser console_args_parser_t;


// extern void console_args_parser_addsubcommands(console_args_parser_t *, struct console_args_command *, struct console_args_command *, size_t);
// extern void console_args_parser_addsubcommand(console_args_parser_t *, struct console_args_command *, struct console_args_command *);

extern void console_args_parser_addcommands(console_args_parser_t *, struct console_args_command *, size_t);
extern void console_args_parser_addcommand(console_args_parser_t *, struct console_args_command *);

extern size_t console_args_parser_defined(console_args_parser_t *);

extern bool console_args_parser_parse(console_args_parser_t *);

extern console_args_parser_t *console_args_parser_new(int, char **);
extern void console_args_parser_free(console_args_parser_t *);

#ifdef __cplusplus
}
#endif

#endif
