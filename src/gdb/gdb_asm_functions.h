#ifndef gdb_asm_functions_h_
#define gdb_asm_functions_h_ 1

class GdbCpu;
extern "C" void restoreRegisters(GdbCpu*);

/**
 * I have not imported all of exception handlers
 * from the sample gdbstub here, as the code
 * seemed to use only interrupt 1 & 3.
 */
extern "C" void catchException0x00();
extern "C" void catchException0x01();
extern "C" void catchException0x02();
extern "C" void catchException0x03();
extern "C" void catchException0x04();
extern "C" void catchException0x05();
extern "C" void catchException0x06();
extern "C" void catchException0x07();
extern "C" void catchException0x09();
extern "C" void catchExceptionFault0x0c();
extern "C" void catchExceptionFault0x0d();
extern "C" void catchExceptionFault0x0e();
extern "C" void catchException0x10();

extern "C" int get_char(char* addr);
extern "C" void set_char(char* addr, int val);

#endif // gdb_asm_functions_h_
