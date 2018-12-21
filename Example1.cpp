/*********************************************************************************************************
**
** 创   建   人: CSZQ
**
** 描        述: ASMJIT 使用静态库的例子
**
*********************************************************************************************************/
/*********************************************************************************************************
	头文件
*********************************************************************************************************/
#include <iostream>

#include "../ASMJITLIB/asmjit/asmjit.h"

/*********************************************************************************************************
	前置声明
*********************************************************************************************************/
using namespace asmjit;

typedef UINT(*ADDFUNC)(UINT arg0, UINT arg1);

/*********************************************************************************************************
	函数
*********************************************************************************************************/
/*********************************************************************************************************
	说明：
		产生汇编
	参数：
		assemblerX86  汇编器
	返回值：
		无
*********************************************************************************************************/
VOID generateShellCode(X86Assembler &as) {
	/*
	 * Lable 测试，相对跳转到 Lable 处
	 */
	Label lbNext = as.newLabel();
	as.jmp(lbNext);														// 长跳转
	as.long_().jmp(lbNext);												// 长跳转 -2G - 2G
	as.short_().jmp(lbNext);											// 短跳转 -127 - 127
	as.nop();
	as.bind(lbNext);
	as.nop();
	as.nop();

	/*
	 * X86Mem 测试，从 Lable 处取值并执行绝对地址跳转
	 */
	Label lbRva = as.newLabel();
	X86Mem rvaMem = x86::ptr(lbRva);
	as.jmp(rvaMem);														// 从指定地址取值的绝对跳转
	as.bind(lbRva);
	as.dq(0x1122334455667788);
	as.nop();
	as.nop();
	as.nop();
	as.nop();
	as.nop();
	as.nop();
	as.nop();
	as.nop();

	/*
	 * X86Mem 测试，从 rax + 1 处取值并执行绝对地址跳转
	 */
	X86Mem gpMem = x86::ptr(x86::rax);
	gpMem.addOffset(1);
	as.jmp(gpMem);														// jmp qword ptr ds:[rax+1]
	as.nop();
	as.nop();
	as.nop();

	/*
	 * X86Mem 测试，从多个偏移处取值并执行绝对地址跳转
	 */
	X86Mem gpRax = x86::ptr(x86::rax);
	as.jmp(gpRax.adjusted(0x10));										// jmp qword ptr ds:[rax+0x10]
	as.jmp(gpRax.adjusted(0x20));
	as.nop();
	as.nop();
	as.nop();

	/*
	 * X86Gp 寄存器变量测试
	 */
	X86Gp var0 = x86::rax;
	X86Gp var1 = x86::eax;
	as.add(var0, 1);
	as.add(var1, 1);
	as.nop();
	as.nop();
	as.nop();

	/*
	 * Patch code 测试，setOffset() 设置当前指针位置，不能大于当前最大 offset 
	 */
	size_t patchOffset = as.getOffset();
	as.xor_(x86::rax, x86::rax);
	as.nop();
	as.nop();
	as.nop();
	as.setOffset(patchOffset);
	as.ret();
	as.nop();
	as.nop();
	as.nop();

	/*
	 * Prefix 测试
	 */
	as.add(x86::esp, 16);												// 没有前缀
	as.rex().add(x86::esp, 16);											// 强制前缀
	as.add(x86::rsp, 16);												// 自带前缀无论是否强制

}

/*********************************************************************************************************
	说明：
		产生 add 函数
	参数：
		assemblerX86  汇编器
	返回值：
		无
*********************************************************************************************************/
VOID generateAddFunc(X86Assembler &as) {
	X86Gp arg0 = x86::ecx;
	X86Gp arg1 = x86::edx;
	X86Gp res = x86::eax;

	as.xor_(res, res);
	as.add(res, arg0);
	as.add(res, arg1);
	as.ret();
}
/*********************************************************************************************************
	说明：
		Main
	参数：
		无
	返回值：
		无
*********************************************************************************************************/
int main()
{
	std::cout << "Start" << std::endl;

	/*
	 * 基本信息初始化
	 */
	CodeHolder codeHdForShellCode;
	CodeInfo codeIf(ArchInfo::kTypeX64);
	codeIf.setBaseAddress(0x12345678);									// 设置相对基地址，也可以不管
	codeHdForShellCode.init(codeIf);
	X86Assembler assemblerX86(&codeHdForShellCode);

	/*
	 * 代码生成
	 */
	generateShellCode(assemblerX86);

	/*
	 * 获取 shellcode 长度
	 */
	UINT32 uLen = codeHdForShellCode.getCodeSize();
	
	/*
	 * 代码获取，在数据段，无法直接运行，想要运行请参考下面的重定位
	 */
	codeHdForShellCode.sync();														// 最好同步下，不分配空间的指令不会自动同步生成
	CodeBuffer codeBf = codeHdForShellCode.getSectionEntry(0)->getBuffer();
	PVOID pvCode = codeBf.getData();

	std::cout << pvCode << std::endl;
	std::cout << "Len = " << uLen << std::endl;

	/*
	 * 自动暂停
	 */
	_CrtDbgBreak();

	/*
	 * 重用 assemblerX86 
	 */
	codeHdForShellCode.detach(assemblerX86.asEmitter());
	CodeHolder codeHdForAddFunc;
	codeHdForAddFunc.init(CodeInfo(ArchInfo::kTypeX64));
	codeHdForAddFunc.attach(assemblerX86.asEmitter());

	/*
	 * 产生 add function
	 */
	generateAddFunc(assemblerX86);

	/*
	 * 代码重定位，使得可以在本程序直接运行
	 */
	VMemMgr vm;
	PVOID pvBuf = vm.alloc(codeHdForAddFunc.getCodeSize());
	codeHdForAddFunc.relocate(pvBuf);
	UINT iRet = ptr_as_func<ADDFUNC>(pvBuf)(1, 2);						// asmjit 中的直接指针转函数
	std::cout << "iRet = " << iRet << std::endl;

	/* 
	 * 自动暂停
	 */
	_CrtDbgBreak();

	std::cout << "Endl" << std::endl;

	return 0;
}