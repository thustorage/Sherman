/*** Debug source. ***/

/** Version 1 + Functional Model Modification **/

/** Included files. **/
#include "Debug.h"

/** Implemented functions. **/
/* Print debug title string.
   @param   str     String of debug title. */
void Debug::debugTitle(const char *str) {
    if (TITLE == true)                          /* If debug option is set. */
        printf("\033[0;45;1m%s\033[0m\n", str); /* Print debug title string. */
}

/* Print debug item string. Can be used in a formatted style like a printf().
   @param   format  Format of debug item. Same as printf().
                    POTENTIALPROBLEM: the length of format can not exceed
   MAX_FORMAT_LEN - 1, but there is no check.
   @param   ...     Other argument variables to print. Same as printf(). */
void Debug::debugItem(const char *format, ...) {
    char newFormat[MAX_FORMAT_LEN];

    va_list args;
    va_start(args, format); /* Start of variable arguments. */

    if (DEBUG_ON == true) /* If debug option is set. */
    {
        sprintf(newFormat, "\033[0;42;1m%s\033[0m\n",
                format);          /* Wrap format in a style. */
        vprintf(newFormat, args); /* Print string of debug item. */
    }

    va_end(args); /* End of variable arguments. */
}

void Debug::debugCur(const char *format, ...) {
    char newFormat[MAX_FORMAT_LEN];

    va_list args;
    va_start(args, format); /* Start of variable arguments. */

    if (CUR == true) /* If debug option is set. */
    {
        sprintf(newFormat, "%s\n", format); /* Wrap format in a style. */
        vprintf(newFormat, args);           /* Print string of debug item. */
    }

    va_end(args); /* End of variable arguments. */
}
/* Print necessary information at start period. Can be used in a formatted style
   like a printf().
   @param   format  Format of debug item. Same as printf().
                    POTENTIALPROBLEM: the length of format can not exceed
   MAX_FORMAT_LEN - 1, but there is no check.
   @param   ...     Other argument variables to print. Same as printf(). */
void Debug::notifyInfo(const char *format, ...) {
    char newFormat[MAX_FORMAT_LEN];

    va_list args;
    va_start(args, format); /* Start of variable arguments. */
    sprintf(newFormat, "\033[4m%s\033[0m\n",
            format);          /* Wrap format in a style. */
    vprintf(newFormat, args); /* Print string of notify information. */
    va_end(args);             /* End of variable arguments. */
}

/* Print error information at start period. Can be used in a formatted style
   like a printf().
   @param   format  Format of debug item. Same as printf().
                    POTENTIALPROBLEM: the length of format can not exceed
   MAX_FORMAT_LEN - 1, but there is no check.
   @param   ...     Other argument variables to print. Same as printf(). */
void Debug::notifyError(const char *format, ...) {
    char newFormat[MAX_FORMAT_LEN];

    va_list args;
    va_start(args, format); /* Start of variable arguments. */
    sprintf(newFormat, "\033[0;31m%s\033[0m\n",
            format);          /* Wrap format in a style. */
    vprintf(newFormat, args); /* Print string of notify information. */
    va_end(args);             /* End of variable arguments. */
}

