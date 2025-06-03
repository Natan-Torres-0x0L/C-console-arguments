#include "arguments.h"

#include <collections/list/list.h>

#include <strings/strings.h>

#include <stdlib.h>
#include <stdio.h>


struct console_args_parser_arguments {
  char **vector;
  size_t count;
};

struct console_args_parser {
  struct console_args_parser_arguments arguments;

  list_t *commands;
  size_t defined;
};


void
console_args_command_addsubcommands(struct console_args_command *command, struct console_args_command *subcommands, size_t count) {
  size_t x11;

  for (x11 = 0; command->subcommands.count < command->subcommands.size && x11 < count; x11++)
    console_args_command_addsubcommand(command, &subcommands[x11]);
}

void
console_args_command_addsubcommand(struct console_args_command *command, struct console_args_command *subcommand) {
  size_t x11 = command->subcommands.count;

  if (command->subcommands.count >= command->subcommands.size)
    return;

  subcommand->subcommands.parent = command;

  command->subcommands.commands[x11++] = subcommand;
  command->subcommands.count = x11;
}


static struct console_args_command console_args_subcommand_unknown;


struct console_args_command *
console_args_command_subcommand(struct console_args_command *command, const char *name) {
  size_t x11;
  
  for (x11 = 0; x11 < command->subcommands.count; x11++)
    if (string_equals(command->subcommands.commands[x11]->name, name, true))
      return command->subcommands.commands[x11];

  return &console_args_subcommand_unknown;
}

static bool
console_args_command_flags_parse(struct console_args_command *command, const char *argument) {
  size_t argument_length = string_length(argument);

  char *flags = (char *)command->flags;
  long separator;

  for (;;) {
    if ((separator = string_find(flags, "|", true)) == -1)
      separator = (long)string_length(flags);

    if (argument_length == separator && string_match(flags, argument, separator, command->sensitive))
      return true;

    if (!*(flags+separator))
      break;

    flags += separator+1;
  }

  return false;
}

#if impl
static void *
console_args_command_values_parse(struct console_args_command *command, char **arguments) {
  char **values = NULL;
  size_t x11;

  if (!(values = calloc((size_t)command->values_count, sizeof(char *))))
    return NULL;

  for (x11 = 0; x11 < command->values_count; x11++, arguments++)
    values[x11] = *arguments;

/*
  if (string_equals(command->type, "string", false)) {
    if (!(values = calloc((size_t)command->values_count, sizeof(char *))))
      return NULL;

    for (x11 = 0; x11 < command->values_count; x11++, arguments++)
      ((char **)values)[x11] = *arguments;
  } else if (string_equals(command->type, "bool", false)) {
    if (!(values = calloc((size_t)command->values_count, sizeof(bool))))
      return NULL;

    for (x11 = 0; x11 < command->values_count; x11++, arguments++)
      if (string_equals(*arguments, "true", true) || string_equals(*arguments, "1", true))
        ((bool *)values)[x11] = true;
      else if (string_equals(*arguments, "false", true) || string_equals(*arguments, "0", true))
        ((bool *)values)[x11] = false;
  } else if (string_equals(command->type, "int", false)) {
    if (!(values = calloc((size_t)command->values_count, sizeof(int))))
      return NULL;

    for (x11 = 0; x11 < command->values_count; x11++, arguments++)
      sscanf(*arguments, "%i", &((int *)values)[x11]);
  } else if (string_equals(command->type, "double", true)) {
    if (!(values = calloc((size_t)command->values_count, sizeof(double))))
      return NULL;

    for (x11 = 0; x11 < command->values_count; x11++, arguments++)
      sscanf(*arguments, "%lf", &((double *)values)[x11]);
  } else if (string_equals(command->type, "float", true)) {
    if (!(values = calloc((size_t)command->values_count, sizeof(float))))
      return NULL;

    for (x11 = 0; x11 < command->values_count; x11++, arguments++)
      sscanf(*arguments, "%f", &((float *)values)[x11]);
  } else if (string_equals(command->type, "char", true)) {
    if (!(values = calloc((size_t)command->values_count, sizeof(char))))
      return NULL;

    for (x11 = 0; x11 < command->values_count; x11++, arguments++)
      sscanf(*arguments, "%c", &((char *)values)[x11]);
  }
*/

  return values;
}

#endif

int
console_args_command_subcommands_new(struct console_args_command *command, struct console_args_command **subcommands, size_t size) {
  command->subcommands.commands = subcommands;
  command->subcommands.size = size;
  command->subcommands.count = 0;

  return 1;
}

static inline void
console_args_command_values_free(struct console_args_command *command) {
  if (command->values) {
    free(command->values);
    command->values = NULL;
  }
}

static bool
console_args_parser_reserved_command(console_args_parser_t *parser, const char *argument) {
  struct console_args_command *command = NULL;
  list_iterator_t command_iter = NULL;

  for (command_iter = list_begin(parser->commands); command_iter; command_iter = list_next(command_iter)) {
    command = (struct console_args_command *)list_value(command_iter);

    if (console_args_command_flags_parse(command, argument))
      return true;
  }

  return false;
}

static bool
console_args_command_reserved_subcommand(struct console_args_command *command, const char *argument) {
  struct console_args_command *parent = command->subcommands.parent;
  size_t x11;

  for (x11 = 0; parent && x11 < parent->subcommands.count; x11++)
    if (console_args_command_flags_parse(parent->subcommands.commands[x11], argument))
      return true;

  return false;
}

static long
console_args_parser_command_values_count(console_args_parser_t *parser, struct console_args_command *command, size_t x11) {
  long values_count = 0;

  for (; x11 < parser->arguments.count; x11++) {
    if (console_args_parser_reserved_command(parser, parser->arguments.vector[x11]))
      break;

    if (console_args_command_reserved_subcommand(command, parser->arguments.vector[x11]))
      break;

    values_count++;
  }

  return values_count;
}

static bool
console_args_parser_command_parse(console_args_parser_t *parser, struct console_args_command *command) {
  char **arguments = NULL;
  size_t arguments_count;

  long values_count;

  size_t x11, x22;

  for (x11 = 0; x11 < parser->arguments.count; x11++) {
    if (console_args_command_flags_parse(command, parser->arguments.vector[x11])) {
      values_count = console_args_parser_command_values_count(parser, command, x11+1);

      if (command->required_values < 0)
        command->required_values = values_count;

      if (command->required_values && command->subcommands.commands) {
        arguments = parser->arguments.vector; // -(x11+1);
        arguments_count = parser->arguments.count;

        parser->arguments.vector = &parser->arguments.vector[x11+1];
        parser->arguments.count = (size_t)command->required_values;

        for (x22 = 0; x22 < command->subcommands.count; x22++)
          console_args_parser_command_parse(parser, command->subcommands.commands[x22]);

        parser->arguments.vector = arguments;
        parser->arguments.count = arguments_count;
      }

      if (command->required_values && x11+1 < parser->arguments.count) {
        command->values_count = (size_t)command->required_values;
        command->values = &parser->arguments.vector[x11+1]; // console_args_command_values_parse(command, &parser->arguments.vector[x11+1]);

        command->initialized = true;
      }

      if (!command->subcommands.parent)
        parser->defined++;

      if (command->subcommands.parent)
        command->subcommands.parent->subcommands.defined = true;

      if (!command->subcommands.parent) {
     // parser->arguments.vector = parser->arguments.vector+1+command->values_count;
     // parser->arguments.count -= 1+command->values_count;
      }

      command->defined = true;
      break;
    }
  }

  return true;
}

static inline void
console_args_command_initialize(struct console_args_command *command) {
  command->defined = false;

  command->values = NULL;
  command->values_count = 0;
}

static void
console_args_command_free(struct console_args_command *command) {
  if (command) {
    if (command->subcommands.commands) {
   // do {
   //   console_args_command_free(command->subcommands.commands[--command->subcommands.count]);
   //   command->subcommands.commands[command->subcommands.count] = NULL;
   // } while (command->subcommands.count);

   // free(command->subcommands.commands);
   // command->subcommands.commands = NULL;

   // command->subcommands.count = 0;

   // command->subcommands.defined = false;
    }

 // console_args_command_values_free(command);
  }
}

static inline int
console_args_parser_add_command(list_t *commands, struct console_args_command *command) {
  return list_pushfront(commands, list_rvalue(command, sizeof(struct console_args_command *)));
}

void
console_args_parser_addcommands(console_args_parser_t *parser, struct console_args_command *commands, size_t count) {
  size_t x11;

  for (x11 = 0; x11 < count; x11++)
    console_args_parser_addcommand(parser, &commands[x11]);
}

void
console_args_parser_addcommand(console_args_parser_t *parser, struct console_args_command *command) {
  console_args_command_initialize(command);
  console_args_parser_add_command(parser->commands, command);
}

size_t
console_args_parser_defined(console_args_parser_t *parser) {
  return parser->defined;
}

bool
console_args_parser_parse(console_args_parser_t *parser) {
  struct console_args_command *command = NULL;
  list_iterator_t command_iter = NULL;

  for (command_iter = list_begin(parser->commands); command_iter; command_iter = list_next(command_iter)) {
    command = (struct console_args_command *)list_value(command_iter);

    if (!command->defined && !console_args_parser_command_parse(parser, command))
      return false;

    if (command->required && (!command->defined || (command->required_values > 0 && !command->initialized)))
      return false;
  }

  return true;
}

console_args_parser_t *
console_args_parser_new(int argc, char **argv) {
  console_args_parser_t *parser = NULL;

  if (!(parser = (console_args_parser_t *)calloc(1, sizeof(console_args_parser_t))))
    goto _return;

  if (!(parser->commands = list_new(free)))
    goto _return;

  parser->arguments.vector = argv;
  parser->arguments.count = (size_t)argc;

  return parser;

_return:
  console_args_parser_free(parser);
  return NULL;
}

void
console_args_parser_free(console_args_parser_t *parser) {
  if (parser) {
    if (parser->commands) {
      struct console_args_command *command = NULL;
      list_iterator_t command_iter = NULL;

      for (command_iter = list_begin(parser->commands); command_iter; command_iter = list_next(command_iter)) {
        command = (struct console_args_command *)list_value(command_iter);
        console_args_command_free(command);
      }

      list_free(parser->commands);
    }

    free(parser);
  }
}
