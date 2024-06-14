#ifndef LOGGER_H_
#define LOGGER_H_

void LOG_Info(const char* format, ...);
void LOG_Warn(const char* format, ...);
void LOG_Debug(const char* format, ...);
void LOG_Error(const char* format, ...);

#endif