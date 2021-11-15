#include "components_ins.h"

/*
 * Use puts() instead of printf() to avoid printf buffer overflow
 * for long help messages
 */
__root int do_help (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]) 
{
	int i;
	int rcode = 0;

	if (argc == 1) {	/*show list of commands */

		int  cmd_items = __u_boot_cmd_end - __u_boot_cmd_start;	/* pointer arith! */
		cmd_tbl_t *cmd_array[50];
		int i, j, swaps;

        if( cmd_items > 100 )
        {
            log_err("Too many command, you must increase the cache\n");
        }
        
		/* Make array of commands from .uboot_cmd section */
		cmdtp = __u_boot_cmd_start;
		for (i = 0; i < cmd_items; i++) {
			cmd_array[i] = cmdtp++;
		}

		/* Sort command list (trivial bubble sort) */
		for (i = cmd_items - 1; i > 0; --i) {
			swaps = 0;
			for (j = 0; j < i; ++j) {
				if (strcmp (cmd_array[j]->name,
					    cmd_array[j + 1]->name) > 0) {
					cmd_tbl_t *tmp;
					tmp = cmd_array[j];
					cmd_array[j] = cmd_array[j + 1];
					cmd_array[j + 1] = tmp;
					++swaps;
				}
			}
			if (!swaps)
				break;
		}

		/* print short help (usage) */
		for (i = 0; i < cmd_items; i++) {
			const char *usage = cmd_array[i]->usage;

			if (usage == NULL)
				continue;
			printf("%s" ,usage);
		}
		return 0;
	}
	/*
	 * command help (long version)
	 */
	for (i = 1; i < argc; ++i) {
		if ((cmdtp = find_cmd (argv[i])) != NULL) {
			/* found - print (long) help info */
			printf("%s" ,cmdtp->name);
			putchar (' ');
			if (cmdtp->help) {
				printf("%s" ,cmdtp->help);
			} else {
				printf("%s" ,"- No help available.\n");
				rcode = 1;
			}
			putchar ('\n');
		} else {
			printf ("Unknown command '%s' - try 'help'"
				" without arguments for list of all"
				" known commands\n\n", argv[i]
					);
			rcode = 1;
		}
	}
	return rcode;
}




U_BOOT_CMD(
	help,	CFG_MAXARGS,	1,	do_help,
 	"help    - print online help\n",
 	"[command ...]\n"
 	"    - show help information (for 'command')\n"
 	"'help' prints online help for the monitor commands.\n\n"
 	"Without arguments, it prints a short usage message for all commands.\n\n"
 	"To get detailed help information for specific commands you can type\n"
  "'help' with one or more command names as arguments.\n"
);
