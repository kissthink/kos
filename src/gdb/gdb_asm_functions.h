#ifndef Gdb_Asm_Functions_h_
#define Gdb_Asm_Functions_h_ 1

/**
 * I have not imported all of exception handlers
 * from the sample gdbstub here, as the code
 * seemed to use only interrupt 1 & 3.
 */
extern "C" void catchException0();
extern "C" void catchException1();
extern "C" void catchException2();
extern "C" void catchException3();
extern "C" void catchException4();
extern "C" void catchException5();
extern "C" void catchException6();
extern "C" void catchException7();
extern "C" void catchException9();
extern "C" void catchException16();

#endif // Gdb_Asm_Functions_h_
