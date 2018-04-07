#ifndef CUT_DECLARATIONS_H
#define CUT_DECLARATIONS_H

#ifndef CUT_MAIN
#error "cannot be standalone"
#endif

// common functions
CUT_NORETURN int cut_FatalExit(const char *reason);
CUT_NORETURN int cut_ErrorExit(const char *reason, ...);
void cut_Register(cut_Instance instance, const char *name);
CUT_PRIVATE int cut_Help();
CUT_PRIVATE int cut_SendMessage(const struct cut_Fragment *message);
CUT_PRIVATE int cut_ReadMessage(struct cut_Fragment *message);
CUT_PRIVATE void cut_ResetLocalMessage();
CUT_PRIVATE int cut_SendLocalMessage(struct cut_Fragment *message);
CUT_PRIVATE int cut_ReadLocalMessage(struct cut_Fragment *message);
CUT_PRIVATE void cut_SendOK(int counter);
void cut_DebugMessage(const char *file, size_t line, const char *fmt, ...);
CUT_NORETURN void cut_Stop(const char *text, const char *file, size_t line);
void cut_Check(const char *text, const char *file, size_t line);
#  ifdef __cplusplus
CUT_PRIVATE int cut_StopException(const char *type, const char *text);
#  endif
CUT_PRIVATE void cut_ExceptionBypass(int testId, int subtest);
CUT_PRIVATE void cut_Timeouted();
void cut_Subtest(int number, const char *name);
CUT_PRIVATE void *cut_PipeReader(struct cut_UnitResult *result);
CUT_PRIVATE int cut_SetSubtestName(struct cut_UnitResult *result, int number, const char *name);
CUT_PRIVATE int cut_AddInfo(struct cut_Info **info,
    size_t line, const char *file, const char *text);
CUT_PRIVATE int cut_SetFailResult(struct cut_UnitResult *result,
    size_t line, const char *file, const char *text);
CUT_PRIVATE int cut_SetExceptionResult(struct cut_UnitResult *result,
    const char *type, const char *text);
CUT_PRIVATE void cut_ParseArguments(int argc, char **argv);
CUT_PRIVATE int cut_SkipUnit(int testId);
CUT_PRIVATE const char *cut_GetStatus(const struct cut_UnitResult *result, int *length);
CUT_PRIVATE const char *cut_ShortPath(const char *path);
CUT_PRIVATE void cut_PrintResult(int base, int subtest, const struct cut_UnitResult *result);
CUT_PRIVATE void cut_CleanInfo(struct cut_Info *info);
CUT_PRIVATE void cut_CleanMemory(struct cut_UnitResult *result);
CUT_PRIVATE int cut_Runner(int argc, char **argv);
CUT_PRIVATE void cut_RunUnitForkless(int testId, int subtest, struct cut_UnitResult *result);

// platform specific functions
CUT_PRIVATE ssize_t cut_Read(int fd, char *destination, size_t bytes);
CUT_PRIVATE ssize_t cut_Write(int fd, const char *source, size_t bytes);
CUT_PRIVATE void cut_RedirectIO();
CUT_PRIVATE void cut_ResumeIO();
CUT_PRIVATE int cut_PreRun();
CUT_PRIVATE void cut_RunUnit(int testId, int subtest, struct cut_UnitResult *result);
int cut_File(FILE *file, const char *content);

#endif // CUT_DECLARATIONS_H
