#include "console.h"
#include "config.h"
#include "unit.h"
#include "bsp.h"

void *console_port = NULL;
xTaskHandle consoleHandle;

//HalUsartComPortType *HalShell = NULL;
consoleBufferType        console_data;

/* The queue used by both tasks. */
//static QueueHandle_t xConsoleQueue = NULL;


cmd_tbl_t  *__u_boot_cmd_start =__section_begin("u_boot_cmd");
cmd_tbl_t  *__u_boot_cmd_end =__section_end("u_boot_cmd");


/******************************************************
** shell receive data byte
*/
static void console_getchar( uint8_t c)
{
    switch(c)
    {
        case '\r': /* Enter */
        case '\n':
        {
            static BaseType_t xHigherPriorityTaskWoken =  pdFALSE;;
            
            console_data.buff[console_data.cnt++] = '\0';

            //xQueueSendFromISR( xConsoleQueue, ( void* )&console_data, NULL );  
            
            //memset(&console_data , 0x00 , sizeof(consoleBufferType));

            xSemaphoreGiveFromISR( xConsoleSemaphore, &xHigherPriorityTaskWoken );
            portEND_SWITCHING_ISR(xHigherPriorityTaskWoken );
            
            printf("\r\n");
            
        }break;
        case '\0': // nul 
                break;
        case 0x08: /* ^H - backspace */
        case 0x7F: /* DEL - backspace */ //É¾³ý²Ù×÷
        {
            if( console_data.cnt > 0)
            {
                console_data.cnt--;
                console_data.buff[console_data.cnt] = 0x00;
                printf( "\b \b");
            }
        }break;
        default:
        {
            console_data.buff[console_data.cnt++] = c;
            if( console_data.cnt > (CFG_CBSIZE - 1) )
            {
                log_err("console receive is too length\n");
                memset(&console_data , 0x00 , sizeof(consoleBufferType) );
            }
            putchar( c ); 
        }
    }
}


void serial_console_init( void )
{
    console_port = serial.open("serial1");
    
    if( console_port == NULL)
    {
        //beep.write(BEEP_ALARM);
        return ;
    }
    serial.init(console_port  , 921600 , console_getchar);
}


/***************************************************************************
 * find command table entry for a command
 */
cmd_tbl_t *find_cmd (const char *cmd)
{
	cmd_tbl_t *cmdtp;
	cmd_tbl_t *cmdtp_temp = __u_boot_cmd_start;	/*Init value */
	const char *p;
	int len;
	int n_found = 0;

	/*
	 * Some commands allow length modifiers (like "cp.b");
	 * compare command name only until first dot.
	 */
	len = ((p = strchr(cmd, '.')) == NULL) ? strlen (cmd) : (p - cmd);

	for (cmdtp = __u_boot_cmd_start;
	     cmdtp != __u_boot_cmd_end;
	     cmdtp++) {
		if (strncmp (cmd, cmdtp->name, len) == 0) {
			if (len == strlen (cmdtp->name))
				return cmdtp;	/* full match */

			cmdtp_temp = cmdtp;	/* abbreviated command ? */
			n_found++;
		}
	}
	if (n_found == 1) {			/* exactly one match */
		return cmdtp_temp;
	}

	return NULL;	/* not found or ambiguous command */
}

static int parse_line (char *line, char *argv[])
{
	int nargs = 0;

	while (nargs < CFG_MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return (nargs);
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return (nargs);
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	log (ERR,"** Too many args (max. %d) **\n", CFG_MAXARGS);
	return (nargs);
}

/****************************************************************************/

static void process_macros (const char *input, char *output)
{
	char c, prev;

	int inputcnt = strlen (input);
	int outputcnt = CFG_CBSIZE;
	int state = 0;		/* 0 = waiting for '$'  */

	/* 1 = waiting for '(' or '{' */
	/* 2 = waiting for ')' or '}' */
	/* 3 = waiting for '''  */

	prev = '\0';		/* previous character   */

	while (inputcnt && outputcnt) {
		c = *input++;
		inputcnt--;

		if (state != 3) {
			/* remove one level of escape characters */
			if ((c == '\\') && (prev != '\\')) {
				if (inputcnt-- == 0)
					break;
				prev = c;
				c = *input++;
			}
		}

		switch (state) {
		case 0:	/* Waiting for (unescaped) $    */
			if ((c == '\'') && (prev != '\\')) {
				state = 3;
				break;
			}
			if ((c == '$') && (prev != '\\')) {
				state++;
			} else {
				*(output++) = c;
				outputcnt--;
			}
			break;
		case 1:	/* Waiting for (        */
			if (c == '(' || c == '{') {
				state++;
			} else {
				state = 0;
				*(output++) = '$';
				outputcnt--;

				if (outputcnt) {
					*(output++) = c;
					outputcnt--;
				}
			}
			break;
		case 2:	/* Waiting for )        */
			if (c == ')' || c == '}') {

				char *envval;

				envval = NULL;

				/* Copy into the line if it exists */
				if (envval != NULL)
					while ((*envval) && outputcnt) {
						*(output++) = *(envval++);
						outputcnt--;
					}
				/* Look for another '$' */
				state = 0;
			}
			break;
		case 3:	/* Waiting for '        */
			if ((c == '\'') && (prev != '\\')) {
				state = 0;
			} else {
				*(output++) = c;
				outputcnt--;
			}
			break;
		}
		prev = c;
	}

	if (outputcnt)
		*output = 0;
}

/****************************************************************************
 * returns:
 *	1  - command executed, repeatable
 *	0  - command executed but not repeatable, interrupted commands are
 *	     always considered not repeatable
 *	-1 - not executed (unrecognized, bootd recursion or too many args)
 *           (If cmd is NULL or "" or longer than CFG_CBSIZE-1 it is
 *           considered unrecognized)
 *
 * WARNING:
 *
 * We must create a temporary copy of the command since the command we get
 * may be the result from getenv(), which returns a pointer directly to
 * the environment data, which may change magicly when the command we run
 * creates or modifies environment variables (like "bootp" does).
 */

static int console_command (const char *cmd, int flag)
{
	cmd_tbl_t *cmdtp;
	char cmdbuf[CFG_CBSIZE];	/* working copy of cmd		*/
	char *token;			/* start of token in cmdbuf	*/
	char *sep;			/* end of token (separator) in cmdbuf */
	char finaltoken[CFG_CBSIZE];
	char *str = cmdbuf;
	char *argv[CFG_MAXARGS + 1];	/* NULL terminated	*/
	int argc, inquotes;
	int repeatable = 1;
	int rc = 0;


	if (!cmd || !*cmd) {
		return -1;	/* empty command */
	}

	if (strlen(cmd) >= CFG_CBSIZE) {
		log_err("%s" ,"## Command too long!\n");
		return -1;
	}

	strcpy (cmdbuf, cmd);

	/* Process separators and check for invalid
	 * repeatable commands
	 */
	while (*str) {

		/*
		 * Find separator, or string end
		 * Allow simple escape of ';' by writing "\;"
		 */
		for (inquotes = 0, sep = str; *sep; sep++) {
			if ((*sep=='\'') &&
			    (*(sep-1) != '\\'))
				inquotes=!inquotes;

			if (!inquotes &&
			    (*sep == ';') &&	/* separator		*/
			    ( sep != str) &&	/* past string start	*/
			    (*(sep-1) != '\\'))	/* and NOT escaped	*/
				break;
		}

		/*
		 * Limit the token to data between separators
		 */
		token = str;
		if (*sep) {
			str = sep + 1;	/* start of command for next pass */
			*sep = '\0';
		}
		else
			str = sep;	/* no more commands for next pass */

		/* find macros in this token and replace them */
		process_macros (token, finaltoken);

		/* Extract arguments */
		if ((argc = parse_line (finaltoken, argv)) == 0) {
			rc = -1;	/* no command at all */
			continue;
		}

		/* Look up command in command table */
		if ((cmdtp = find_cmd(argv[0])) == NULL) {
			printf ("Unknown command '%s' - try 'help'\n", argv[0]);
			rc = -1;	/* give up after bad command */
			continue;
		}
        
		/* found - check max args */
		if (argc > cmdtp->maxargs) {
			log (ERR,"Too many parameters to allow at most %d\n %s", cmdtp->maxargs , cmdtp->help);
			rc = -1;
			continue;
		}

		/* OK - call function to do the command */
		if ((cmdtp->cmd) (cmdtp, flag, argc, argv) != 0) {
			rc = -1;
		}

		repeatable &= cmdtp->repeatable;

	}

	return rc ? rc : repeatable;
}


static void console_run( void const *pvParameters )
{
    //consoleBufferType console_d;

    configASSERT( ( ( unsigned long ) pvParameters ) == 0 );
     
    printf("console > ");
    
    while(1)
    {
        
        //if( xQueueReceive( xConsoleQueue, &console_d,  1000) == pdTRUE)
        if( xSemaphoreTake( xConsoleSemaphore, 1000 ) == pdTRUE )
        {
            //console_command ((char const *)console_d.buff, 0); 

            //memset(&console_d , 0x00 , sizeof(consoleBufferType) );
            console_command ((char const *)console_data.buff, 0); 

            memset(&console_data , 0x00 , sizeof(consoleBufferType) );
            
            printf("console > ");
        }

        //read_task_stack(__func__,consoleHandle);
        task_keep_alive(TASK_CONSOLE_BIT);   
    }       
}

void creat_console_task( void )
{
    osThreadDef( console, console_run , osPriorityNormal, 0, configMINIMAL_STACK_SIZE*4);
    consoleHandle = osThreadCreate(osThread(console), NULL);
    configASSERT(consoleHandle)
}



int fputc(int ch, FILE *f)
{
    if(ch == '\n')
    {
        serial.putc(console_port,'\r');
    }
    
	serial.putc(console_port, ch);
    
	return ch;
}


