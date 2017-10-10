#ifndef CUT_GLOBALS_H
#define CUT_GLOBALS_H

#ifndef CUT_MAIN
#error "cannot be standalone"
#endif

CUT_PRIVATE struct cut_Arguments cut_arguments;
CUT_PRIVATE struct cut_UnitTestArray cut_unitTests = {0, 0, NULL};
CUT_PRIVATE FILE *cut_output = NULL;
CUT_PRIVATE int cut_spawnChild = 0;
CUT_PRIVATE int cut_pipeWrite = 0;
CUT_PRIVATE int cut_pipeRead = 0;
CUT_PRIVATE FILE *cut_stdout = NULL;
CUT_PRIVATE FILE *cut_stderr = NULL;
CUT_PRIVATE jmp_buf cut_executionPoint;
CUT_PRIVATE const char *cut_emergencyLog = "cut.log";

#endif // CUT_GLOBALS_H