#ifndef CUT_GLOBALS_H
#define CUT_GLOBALS_H

#ifndef CUT_MAIN
#error "cannot be standalone"
#endif

#define CUT_MAX_LOCAL_MESSAGE_LENGTH 4096

CUT_PRIVATE struct cut_Arguments cut_arguments;
CUT_PRIVATE struct cut_UnitTestArray cut_unitTests = {0, 0, NULL};
CUT_PRIVATE FILE *cut_output = NULL;
CUT_PRIVATE int cut_outputsRedirected = 0;
CUT_PRIVATE int cut_pipeWrite = 0;
CUT_PRIVATE int cut_pipeRead = 0;
CUT_PRIVATE int cut_originalStdOut = 0;
CUT_PRIVATE int cut_originalStdErr = 0;
CUT_PRIVATE FILE *cut_stdout = NULL;
CUT_PRIVATE FILE *cut_stderr = NULL;
CUT_PRIVATE jmp_buf cut_executionPoint;
CUT_PRIVATE const char *cut_emergencyLog = "cut.log";
CUT_PRIVATE int cut_localMessageSize = 0;
CUT_PRIVATE char cut_localMessage[CUT_MAX_LOCAL_MESSAGE_LENGTH];
CUT_PRIVATE char *cut_localMessageCursor = NULL;
CUT_PRIVATE cut_GlobalTear cut_globalTearUp = NULL;
CUT_PRIVATE cut_GlobalTear cut_globalTearDown = NULL;

#endif // CUT_GLOBALS_H