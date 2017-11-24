#include "main.h"

HHOOK keyboard_hook = 0;

VALUE Process_close(VALUE *this_, ARRAY *arguments) {
	return ValueNumber(CloseHandle(GetHandle(this_)));
}

VALUE Process_suspend(VALUE *this_, ARRAY *arguments) {
	SuspendProcess((DWORD)((VALUE *)ValueGetProperty(this_, "processID"))->number);
	return ValueNumber(0);
}

VALUE Process_resume(VALUE *this_, ARRAY *arguments) {
	ResumeProcess((DWORD)((VALUE *)ValueGetProperty(this_, "processID"))->number);
	return ValueNumber(0);
}

VALUE Process_exit(VALUE *this_, ARRAY *arguments) {
	return ValueNumber(TerminateProcess(GetHandle(this_), 0));
}

VALUE Process_findInt(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		HANDLE process = GetHandle(this_);
		int value = (int)((VALUE *)ArrayGet(arguments, 0))->number;

		VALUE ret = ValueArray();

		SINT base = 0;
		MEMORY_BASIC_INFORMATION mi = { 0 };
		for (VirtualQueryEx(process, (void *)base, &mi, sizeof(mi));;) {
			base += mi.RegionSize;
			VirtualQueryEx(process, (void *)base, &mi, sizeof(mi));
			if (base == (SINT)mi.BaseAddress + mi.RegionSize) break;
			if (!(mi.State & (MEM_COMMIT | MEM_RESERVE))) continue;

			char *buffer = (char *)malloc(mi.RegionSize);
			ReadProcessMemory(process, mi.BaseAddress, buffer, mi.RegionSize, 0);
			for (SINT i = 0; i < mi.RegionSize; i += 4) {
				if (*(int *)((SINT)buffer + i) == value) {
					ArrayPush(ret.array, &ValueNumber((double)((SINT)mi.BaseAddress + i)));
				}
			}

			free(buffer);
		}

		return ret;
	}

	return ValueNumber(0);
}

VALUE Process_findPattern(VALUE *this_, ARRAY *arguments) {
	if (arguments->length == 1) {

	} else if (arguments->length == 2) {
		VALUE *pattern = (VALUE *)ArrayGet(arguments, 0);
		if (pattern && pattern->type == VALUE_STRING) {
			VALUE *mask = (VALUE *)ArrayGet(arguments, 1);
			if (mask && mask->type == VALUE_STRING) {
				SINT base = (SINT)ValueGetProperty(this_, "base")->number;
				unsigned int size  = (unsigned int)ValueGetProperty(this_, "size")->number;
				return ValueNumber((double)((SINT)ProcessFindPattern(GetHandle(this_), (void *)base, size, pattern->string, mask->string)));
			}
		}
	}

	return ValueNumber(0);
}

VALUE Process_readPointer(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		HANDLE process = GetHandle(this_);
		BOOL x32 = false;
		IsWow64Process(process, &x32);
		if (x32) {
			DWORD base = 0;
			unsigned int i = 0;
			for (; i < arguments->length - 1; ++i) {
				ReadProcessMemory(process, (void *)((SINT)(base + (DWORD)(((VALUE *)ArrayGet(arguments, i)))->number)), &base, sizeof(base), 0);
			}
			base += (DWORD)(((VALUE *)ArrayGet(arguments, i))->number);
			return ValueNumber((double)base);
		} else {
			SINT base = 0;
			unsigned int i = 0;
			for (; i < arguments->length - 1; ++i) {
				ReadProcessMemory(process, (void *)(base + (SINT)(((VALUE *)ArrayGet(arguments, i))->number)), &base, sizeof(base), 0);
			}
			base += (SINT)(((VALUE *)ArrayGet(arguments, i))->number);
			return ValueNumber((double)base);
		}
	}

	return ValueNumber(0);
}

VALUE Process_readByte(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(ReadByte(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_readShort(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(ReadShort(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_readInt(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(ReadInt(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_readLong(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(ReadLong(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_readFloat(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(ReadFloat(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_readLongLong(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber((double)ReadLongLong(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_readDouble(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(ReadDouble(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_readBuffer(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		SINT address = (SINT)(((VALUE *)ArrayGet(arguments, 0))->number);
		VALUE *v = (VALUE *)ArrayGet(arguments, 1);
		if (v->type == VALUE_NUMBER) {
			byte *buffer = (byte *)malloc((DWORD)v->number);
			ReadBuffer(GetHandle(this_), (void *)address, buffer, (DWORD)v->number);

			VALUE r = ValueArray();

			for (DWORD i = 0; i < (DWORD)v->number; ++i) {
				ArrayPush(r.array, &ValueNumber((double)buffer[i]));
			}

			free(buffer);

			return r;
		}
	}

	return ValueNumber(0);
}

VALUE Process_writeBuffer(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		SINT address = (SINT)(((VALUE *)ArrayGet(arguments, 0))->number);
		VALUE *v = (VALUE *)ArrayGet(arguments, 1);
		if (v->type == VALUE_STRING) {
			return ValueNumber(WriteBuffer(GetHandle(this_), (void *)address, v->string, v->string_length));
		} else if (v->type == VALUE_ARRAY) {
			byte *buffer = (byte *)malloc(v->array->length);
			for (unsigned int i = 0; i < v->array->length; ++i) {
				buffer[i] = (byte)(((VALUE *)ArrayGet(v->array, i))->number);
			}
			bool ret = WriteBuffer(GetHandle(this_), (void *)address, buffer, v->array->length);
			free(buffer);
			return ValueNumber(ret);
		}
	}

	return ValueNumber(0);
}

VALUE Process_writeByte(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		return ValueNumber(WriteByte(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number), (byte)(((VALUE *)ArrayGet(arguments, 1))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_writeShort(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		return ValueNumber(WriteShort(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number), (short)(((VALUE *)ArrayGet(arguments, 1))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_writeInt(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		return ValueNumber(WriteInt(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number), (int)(((VALUE *)ArrayGet(arguments, 1))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_writeLong(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		return ValueNumber(WriteLong(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number), (long)(((VALUE *)ArrayGet(arguments, 1))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_writeFloat(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		return ValueNumber(WriteFloat(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number), (float)(((VALUE *)ArrayGet(arguments, 1))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_writeLongLong(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		return ValueNumber(WriteLongLong(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number), (long long)(((VALUE *)ArrayGet(arguments, 1))->number)));
	}

	return ValueNumber(0);
}

VALUE Process_writeDouble(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		return ValueNumber(WriteDouble(GetHandle(this_), (void *)(SINT)(((VALUE *)ArrayGet(arguments, 0))->number), (((VALUE *)ArrayGet(arguments, 1))->number)));
	}

	return ValueNumber(0);
}

VALUE ValueModule(MODULEENTRY32 info) {
	VALUE m = ValueNumber((double)((SINT)info.modBaseAddr));

	VALUE t = ValueNumber(m.number);
	ValueSetProperty(&m, "base", &t);

	t = ValueNumber((double)info.modBaseSize);
	ValueSetProperty(&m, "size", &t);
	
	char buffer[0xFF] = { 0 };
	WCharToChar(buffer, info.szModule);
	t = ValueRawString(buffer);
	ValueSetProperty(&m, "name", &t);

	WCharToChar(buffer, info.szExePath);
	t = ValueRawString(buffer);
	ValueSetProperty(&m, "exePath", &t);

	return m;
}

VALUE Process_modules(VALUE *this_, ARRAY *arguments) {
	MODULEENTRY32 entry = { 0 };
	entry.dwSize = sizeof(MODULEENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, (DWORD)((VALUE *)ValueGetProperty(this_, "processID"))->number);
	if (snapshot) {
		if (Module32First(snapshot, &entry)) {
			VALUE modules = ValueArray();
			do {
				ArrayPush(modules.array, &ValueModule(entry));
			} while (Module32Next(snapshot, &entry));
			CloseHandle(snapshot);
			return modules;
		}

		CloseHandle(snapshot);
	}

	return ValueNumber(0);
}

VALUE ValueProcess(HANDLE process, PROCESSENTRY32 info) {
	VALUE p = ValueNumber(info.th32ProcessID);

	VALUE t = ValueNumber(info.th32ProcessID);
	ValueSetProperty(&p, "processID", &t);

	t = ValueNumber(info.th32ParentProcessID);
	ValueSetProperty(&p, "parentProcessID", &t);

	char buffer[0xFF] = { 0 };
	WCharToChar(buffer, info.szExeFile);
	t = ValueRawString(buffer);
	ValueSetProperty(&p, "exeFile", &t);

	MODULEENTRY32 minfo = GetModuleInfoByName(info.th32ProcessID, info.szExeFile);
	t = ValueNumber((double)((SINT)minfo.modBaseAddr));
	ValueSetProperty(&p, "base", &t);

	t = ValueNumber((double)minfo.modBaseSize);
	ValueSetProperty(&p, "size", &t);

	t = ValueNumber((double)((SINT)process));
	ValueSetProperty(&p, "handle", &t);

	t = ValueCompiledFunction(&Process_suspend);
	ValueSetProperty(&p, "suspend", &t);

	t = ValueCompiledFunction(&Process_resume);
	ValueSetProperty(&p, "resume", &t);

	t = ValueCompiledFunction(&Process_exit);
	ValueSetProperty(&p, "exit", &t);

	t = ValueCompiledFunction(&Process_modules);
	ValueSetProperty(&p, "modules", &t);

	t = ValueCompiledFunction(&Process_close);
	ValueSetProperty(&p, "close", &t);

	t = ValueCompiledFunction(&Process_writeByte);
	ValueSetProperty(&p, "writeByte", &t);

	t = ValueCompiledFunction(&Process_writeShort);
	ValueSetProperty(&p, "writeShort", &t);

	t = ValueCompiledFunction(&Process_writeInt);
	ValueSetProperty(&p, "writeInt", &t);

	t = ValueCompiledFunction(&Process_writeFloat);
	ValueSetProperty(&p, "writeFloat", &t);

	t = ValueCompiledFunction(&Process_writeLong);
	ValueSetProperty(&p, "writeLong", &t);

	t = ValueCompiledFunction(&Process_writeLongLong);
	ValueSetProperty(&p, "writeLongLong", &t);

	t = ValueCompiledFunction(&Process_writeDouble);
	ValueSetProperty(&p, "writeDouble", &t);

	t = ValueCompiledFunction(&Process_writeBuffer);
	ValueSetProperty(&p, "writeBuffer", &t);

	t = ValueCompiledFunction(&Process_readPointer);
	ValueSetProperty(&p, "readPointer", &t);

	t = ValueCompiledFunction(&Process_readByte);
	ValueSetProperty(&p, "readByte", &t);

	t = ValueCompiledFunction(&Process_readShort);
	ValueSetProperty(&p, "readShort", &t);

	t = ValueCompiledFunction(&Process_readInt);
	ValueSetProperty(&p, "readInt", &t);

	t = ValueCompiledFunction(&Process_readFloat);
	ValueSetProperty(&p, "readFloat", &t);

	t = ValueCompiledFunction(&Process_readLong);
	ValueSetProperty(&p, "readLong", &t);

	t = ValueCompiledFunction(&Process_readLongLong);
	ValueSetProperty(&p, "readLongLong", &t);

	t = ValueCompiledFunction(&Process_readDouble);
	ValueSetProperty(&p, "readDouble", &t);

	t = ValueCompiledFunction(&Process_readBuffer);
	ValueSetProperty(&p, "readBuffer", &t);

	t = ValueCompiledFunction(&Process_findPattern);
	ValueSetProperty(&p, "findPattern", &t);

	t = ValueCompiledFunction(&Process_findInt);
	ValueSetProperty(&p, "findInt", &t);

	return p;
}

VALUE Process_open(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		if (v->type == VALUE_NUMBER) {
			HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, 0, (DWORD)v->number);
			if (process) {
				PROCESSENTRY32 info = GetProcessInfoById(GetProcessId(process));
				return ValueProcess(process, info);
			}
		} else if (v->type == VALUE_STRING) {
			wchar_t buffer[0xFF] = { 0 };
			CharToWChar(buffer, v->string);
			PROCESSENTRY32 info = GetProcessInfoByName(buffer);
			HANDLE process = 0;
			if (info.th32ProcessID && (process = OpenProcess(PROCESS_ALL_ACCESS, 0, info.th32ProcessID))) {
				return ValueProcess(process, info);
			}
		}
	}

	return ValueNumber(0);
}

VALUE Thread_sleep(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		if (v->type == VALUE_NUMBER) {
			Sleep((DWORD)v->number);
		}
	}
	return ValueNumber(0);
}

void CallUserFunction(THREAD_ARGUMENTS *ta) {
	VALUE arguments = ValueArray();
	ArrayFree(arguments.array);
	free(arguments.array);
	arguments.array = ta->arguments;
	StackPush(&ta->thread, "arguments", &arguments, 1);

	TREE *tree = ta->func->left;
	unsigned int i = 0;
	while (tree && i < ta->arguments->length) {
		StackPush(&ta->thread, tree->token->value, &ValueCopy((VALUE *)ArrayGet(ta->arguments, i)), 1);

		if (!tree->left) break;

		tree = tree->left;
		++i;
	}

	VALUE ret = { 0 };
	tree = ta->func;
	while (tree->right) {
		ret = Eval(&ta->thread, tree->right->left, 1);
		if (ret.return_) {
			ret.return_ = false;
			break;
		}
		ValueFree(&ret);
		ret = { 0 };
		tree = tree->right;
	}

	for (unsigned int i = 0; i < ta->thread.stack.length; ++i) {
		ValueFree((VALUE *)ArrayGet(&ta->thread.stack, i));
	}

	ValueFree(&ret);
}

void _Thread_create(THREAD_ARGUMENTS *ta) {
	if (ta) {
		CallUserFunction(ta);
		ArrayFree(&ta->thread.stack);
		FreeTree(ta->func);
		free(ta);
		CloseHandle(GetCurrentThread());
	}
}

VALUE Thread_create(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		if (v->type == VALUE_FUNCTION) {
			TREE *func = CopyTree(v->function);

			THREAD thread = { 0 };
			thread.stack = ArrayNew(sizeof(VALUE));
			ArrayPush(&thread.stack, &ValueStack());

			ARRAY args = ArrayNew(sizeof(VALUE));
			for (unsigned int i = 1; i < arguments->length; ++i) {
				VALUE v = ValueCopy((VALUE *)ArrayGet(arguments, i));
				v.stack_id = 1;
				ArrayPush(&args, &v);
			}

			THREAD_ARGUMENTS *ta = (THREAD_ARGUMENTS *)malloc(sizeof(THREAD_ARGUMENTS));
			ta->func = func;
			ta->thread = thread;
			ta->arguments = (ARRAY *)malloc(sizeof(ARRAY));
			*ta->arguments = args;

			CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_Thread_create, ta, 0, 0));
		}
	}

	return ValueNumber(0);
}

// Console
VALUE Console_print(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) PrintValue((VALUE *)ArrayGet(arguments, 0), 0);
	return ValueNumber(0);
}

VALUE Console_println(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) PrintValue((VALUE *)ArrayGet(arguments, 0), 0);
	putchar('\n');
	return ValueNumber(0);
}

VALUE Console_clear(VALUE *this_, ARRAY *arguments) {
	system("cls");
	return ValueNumber(0);
}

VALUE Console_readLine(VALUE *this_, ARRAY *arguments) {
	char *str = (char *)malloc(0xFFFF);

	fgets(str, 0xFFFF, stdin);
	
	VALUE r = ValueRawString(str);
	free(str);
	return r;
}

// Math
VALUE Math_abs(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(fabs(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_acos(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(acos(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_acosh(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(acosh(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_asin(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(asin(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_asinh(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(asinh(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_atan(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(atan(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_atanh(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(atanh(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_atan2(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		return ValueNumber(atan2(((VALUE *)ArrayGet(arguments, 0))->number, ((VALUE *)ArrayGet(arguments, 1))->number));
	}
	return ValueNumber(0);
}

VALUE Math_cbrt(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(cbrt(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_ceil(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(ceil(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_cos(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(cos(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_cosh(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(cosh(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_deg(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(((VALUE *)ArrayGet(arguments, 0))->number * (double)180 / (double)3.141592653589793);
	}
	return ValueNumber(0);
}

VALUE Math_exp(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(exp(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_expm1(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(expm1(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_floor(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(floor(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_log(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(log(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_log1p(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(log1p(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_log10(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(log10(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_log2(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(log2(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_pow(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 1) {
		return ValueNumber(pow(((VALUE *)ArrayGet(arguments, 0))->number, ((VALUE *)ArrayGet(arguments, 1))->number));
	}
	return ValueNumber(0);
}

VALUE Math_rad(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(((VALUE *)ArrayGet(arguments, 0))->number * (double)3.141592653589793 / (double)180);
	}
	return ValueNumber(0);
}

VALUE Math_random(VALUE *this_, ARRAY *arguments) {
	return ValueNumber(rand() / (float)RAND_MAX);
}

VALUE Math_round(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(round(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_sign(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		double v = ((VALUE *)ArrayGet(arguments, 0))->number;
		return ValueNumber(v == 0 ? 0 : v < 0 ? -1 : 1);
	}
	return ValueNumber(0);
}

VALUE Math_sin(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(sin(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_sinh(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(sinh(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_sqrt(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(sqrt(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_tan(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(tan(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_tanh(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(tanh(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

VALUE Math_trunc(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		return ValueNumber(trunc(((VALUE *)ArrayGet(arguments, 0))->number));
	}
	return ValueNumber(0);
}

// Date
VALUE Date_now(VALUE *this_, ARRAY *arguments) {
	return ValueNumber(timeGetTime());
}

VALUE Date_getMilliseconds(VALUE *this_, ARRAY *arguments) {
	SYSTEMTIME time;
	GetSystemTime(&time);
	return ValueNumber(time.wMilliseconds);
}

VALUE Date_getSeconds(VALUE *this_, ARRAY *arguments) {
	SYSTEMTIME time;
	GetSystemTime(&time);
	return ValueNumber(time.wSecond);
}

VALUE Date_getMinutes(VALUE *this_, ARRAY *arguments) {
	SYSTEMTIME time;
	GetSystemTime(&time);
	return ValueNumber(time.wMinute);
}

VALUE Date_getHours(VALUE *this_, ARRAY *arguments) {
	SYSTEMTIME time;
	GetSystemTime(&time);
	return ValueNumber(time.wHour);
}

VALUE Date_getDate(VALUE *this_, ARRAY *arguments) {
	SYSTEMTIME time;
	GetSystemTime(&time);
	return ValueNumber(time.wDay);
}

VALUE Date_getDay(VALUE *this_, ARRAY *arguments) {
	SYSTEMTIME time;
	GetSystemTime(&time);
	return ValueNumber(time.wDayOfWeek);
}

VALUE Date_getMonth(VALUE *this_, ARRAY *arguments) {
	SYSTEMTIME time;
	GetSystemTime(&time);
	return ValueNumber(time.wMonth);
}

VALUE Date_getYear(VALUE *this_, ARRAY *arguments) {
	SYSTEMTIME time;
	GetSystemTime(&time);
	return ValueNumber(time.wYear);
}

VALUE MessageBox_show(VALUE *this_, ARRAY *arguments) {
	switch (arguments->length) {
		case 1: {
			VALUE *v = (VALUE *)ArrayGet(arguments, 0);
			if (v->type == VALUE_STRING) {
				MessageBoxA(0, v->string, "", 0);
			}
			break;
		}
		case 2: {
			VALUE *v0 = (VALUE *)ArrayGet(arguments, 0);
			if (v0->type == VALUE_STRING) {
				VALUE *v1 = (VALUE *)ArrayGet(arguments, 1);
				if (v1->type == VALUE_STRING) {
					MessageBoxA(0, v1->string, v0->string, 0);
				}
			}
			break;
		}

		default:
			MessageBoxA(0, "", "", 0);
	}

	return ValueNumber(0);
}

// Input
VALUE Input_click(VALUE *this_, ARRAY *arguments) {
	return ValueNumber(0);
}

VALUE Input_keyDown(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		UINT keycode = (UINT)((VALUE *)ArrayGet(arguments, 0))->number;

		INPUT input = { 0 };
		input.type = INPUT_KEYBOARD;
		input.ki.wScan = MapVirtualKey(keycode, MAPVK_VK_TO_VSC);
		input.ki.wVk = keycode;

		SendInput(1, &input, sizeof(input));
	}

	return ValueNumber(0);
}

VALUE Input_keyUp(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		UINT keycode = (UINT)((VALUE *)ArrayGet(arguments, 0))->number;

		INPUT input = { 0 };
		input.type = INPUT_KEYBOARD;
		input.ki.wScan = MapVirtualKey(keycode, MAPVK_VK_TO_VSC);
		input.ki.wVk = keycode;
		input.ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(1, &input, sizeof(input));
	}
	
	return ValueNumber(0);
}

VALUE Input_keyPress(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		UINT keycode = (UINT)((VALUE *)ArrayGet(arguments, 0))->number;

		INPUT input = { 0 };
		input.type = INPUT_KEYBOARD;
		input.ki.wScan = MapVirtualKey(keycode, MAPVK_VK_TO_VSC);
		input.ki.wVk = keycode;
		
		SendInput(1, &input, sizeof(input));
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(input));
	}

	return ValueNumber(0);
}

VALUE Input_isKeyDown(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		return ValueNumber(GetAsyncKeyState((short)v->number) < 0);
	}

	return ValueNumber(0);
}

TREE *onKeyDown = 0;
TREE *onKeyUp = 0;
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {
		KBDLLHOOKSTRUCT data = *((KBDLLHOOKSTRUCT *)lParam);
		int keycode = data.vkCode;

		if (wParam == WM_KEYDOWN && onKeyDown) {
			THREAD thread = { 0 };
			thread.stack = ArrayNew(sizeof(VALUE));
			ArrayPush(&thread.stack, &ValueStack());

			ARRAY args = ArrayNew(sizeof(VALUE));
			VALUE a0 = ValueNumber((double)keycode);
			a0.stack_id = 1;
			ArrayPush(&args, &a0);

			THREAD_ARGUMENTS ta = { 0 };
			ta.func = onKeyDown;
			ta.thread = thread;
			ta.arguments = (ARRAY *)malloc(sizeof(ARRAY));
			*ta.arguments = args;

			CallUserFunction(&ta);

			ArrayFree(&ta.thread.stack);
		} else if (wParam == WM_KEYUP && onKeyUp) {
			THREAD thread = { 0 };
			thread.stack = ArrayNew(sizeof(VALUE));
			ArrayPush(&thread.stack, &ValueStack());

			ARRAY args = ArrayNew(sizeof(VALUE));
			VALUE a0 = ValueNumber((double)keycode);
			a0.stack_id = 1;
			ArrayPush(&args, &a0);

			THREAD_ARGUMENTS ta = { 0 };
			ta.func = onKeyUp;
			ta.thread = thread;
			ta.arguments = (ARRAY *)malloc(sizeof(ARRAY));
			*ta.arguments = args;

			CallUserFunction(&ta);

			ArrayFree(&ta.thread.stack);
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

VALUE Input_onKeyDown(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		if (v->type == VALUE_FUNCTION) {
			if (onKeyDown) {
				TREE *t = onKeyDown;
				onKeyDown = 0;
				FreeTree(t);
			}

			onKeyDown = CopyTree(v->function);
		}
	}

	return ValueNumber(0);
}

VALUE Input_onKeyUp(VALUE *this_, ARRAY *arguments) {
	if (arguments->length > 0) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		if (v->type == VALUE_FUNCTION) {
			if (onKeyUp) {
				TREE *t = onKeyUp;
				onKeyUp = 0;
				FreeTree(t);
			}

			onKeyUp = CopyTree(v->function);
		}
	}

	return ValueNumber(0);
}

in_addr GetAddrFromHost(char *host) {
	struct hostent *info = 0;

	if (info = gethostbyname(host)) {
		for (unsigned int i = 0; info->h_addr_list[i]; ++i) {
			return *(struct in_addr *)info->h_addr_list[i];
		}
	}

	return{ 0 };
}

VALUE HTTP_get(VALUE *this_, ARRAY *arguments) {
	VALUE ret = ValueNumber(0);

	if (arguments->length > 0) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		if (v->type == VALUE_STRING) {
			char *host = _strdup(v->string);
			char *path = _strdup("/");
			for (char *c = host; *c; ++c) {
				if (*c == '/') {
					free(path);
					path = _strdup(c);
					*c = 0;
					break;
				}
			}

			in_addr info = GetAddrFromHost(host);
			if (info.S_un.S_addr) {
				struct sockaddr_in service = { 0 };
				service.sin_family = AF_INET;
				service.sin_addr = info;
				service.sin_port = htons(80);

				SOCKET s;
				if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET) {
					if (connect(s, (SOCKADDR *)&service, sizeof(service)) != SOCKET_ERROR) {
						int length = snprintf(0, 0, \
							"GET %s HTTP/1.0\r\n"\
							"Host: %s\r\n"\
							"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n"\
							"Accept-Language: en-US,en;q=0.9\r\n"\
							"Accept-Encoding: gzip, deflate, br\r\n"\
							"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/62.0.3202.94 Safari/537.36\r\n\r\n", path, host);

						char *data = (char *)malloc(length + 1);
						sprintf(data,
							"GET %s HTTP/1.0\r\n"\
							"Host: %s\r\n"\
							"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n"\
							"Accept-Language: en-US,en;q=0.9\r\n"\
							"Accept-Encoding: gzip, deflate, br\r\n"\
							"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/62.0.3202.94 Safari/537.36\r\n\r\n", path, host);

						if (send(s, data, length, 0)) {
							int size = 0x1000;
							char *buffer = (char *)malloc(size);

							int total = 0;
							for (;;) {
								int r = recv(s, buffer + total, size - total, 0);
								if (r < 1) {
									buffer[total] = 0;
									break;
								}

								total += r;

								if (total >= size) {
									size *= 2;
									buffer = (char *)realloc(buffer, size);
								}
							}

							ret = ValueRawString(buffer);
							free(buffer);
						}

						free(data);
					} else {
						printf("\terror: unable to connect\n");
					}

					closesocket(s);
				} else {
					printf("\terror: unable to create a socket\n");
				}
			}

			free(host);
			free(path);
		}
	}

	return ret;
}

void MessageLoop() {
	keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, 0, 0);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void SetupLibraries(THREAD *thread) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		printf("\terror: WSAStartup failed. Sockets may be unavailable\n");
	}

	srand(GetTickCount());
	
	StackPush(thread, "true", &ValueNumber(1), 0);
	StackPush(thread, "false", &ValueNumber(0), 0);

	VALUE *Console = StackPush(thread, "Console", &ValueNumber(0), 0);
	ValueSetProperty(Console, "print", &ValueCompiledFunction(&Console_print));
	ValueSetProperty(Console, "println", &ValueCompiledFunction(&Console_println));
	ValueSetProperty(Console, "log", &ValueCompiledFunction(&Console_println));
	ValueSetProperty(Console, "clear", &ValueCompiledFunction(&Console_clear));
	ValueSetProperty(Console, "readLine", &ValueCompiledFunction(&Console_readLine));

	VALUE *Math = StackPush(thread, "Math", &ValueNumber(0), 0);
	ValueSetProperty(Math, "E", &ValueNumber(2.718281828459045));
	ValueSetProperty(Math, "LN2", &ValueNumber(0.6931471805599453));
	ValueSetProperty(Math, "LN10", &ValueNumber(2.302585092994046));
	ValueSetProperty(Math, "PI", &ValueNumber(3.141592653589793));
	ValueSetProperty(Math, "SQRT1_2", &ValueNumber(0.7071067811865476));
	ValueSetProperty(Math, "SQRT2", &ValueNumber(1.4142135623730951));
	ValueSetProperty(Math, "abs", &ValueCompiledFunction(&Math_abs));
	ValueSetProperty(Math, "acos", &ValueCompiledFunction(&Math_acos));
	ValueSetProperty(Math, "acosh", &ValueCompiledFunction(&Math_acosh));
	ValueSetProperty(Math, "asin", &ValueCompiledFunction(&Math_asin));
	ValueSetProperty(Math, "asinh", &ValueCompiledFunction(&Math_asinh));
	ValueSetProperty(Math, "atan", &ValueCompiledFunction(&Math_atan));
	ValueSetProperty(Math, "atanh", &ValueCompiledFunction(&Math_atanh));
	ValueSetProperty(Math, "atan2", &ValueCompiledFunction(&Math_atan2));
	ValueSetProperty(Math, "cbrt", &ValueCompiledFunction(&Math_cbrt));
	ValueSetProperty(Math, "ceil", &ValueCompiledFunction(&Math_ceil));
	ValueSetProperty(Math, "cos", &ValueCompiledFunction(&Math_cos));
	ValueSetProperty(Math, "cosh", &ValueCompiledFunction(&Math_cosh));
	ValueSetProperty(Math, "deg", &ValueCompiledFunction(&Math_deg));
	ValueSetProperty(Math, "exp", &ValueCompiledFunction(&Math_exp));
	ValueSetProperty(Math, "expm1", &ValueCompiledFunction(&Math_expm1));
	ValueSetProperty(Math, "floor", &ValueCompiledFunction(&Math_floor));
	ValueSetProperty(Math, "log", &ValueCompiledFunction(&Math_log));
	ValueSetProperty(Math, "log1p", &ValueCompiledFunction(&Math_log1p));
	ValueSetProperty(Math, "log10", &ValueCompiledFunction(&Math_log10));
	ValueSetProperty(Math, "pow", &ValueCompiledFunction(&Math_pow));
	ValueSetProperty(Math, "rad", &ValueCompiledFunction(&Math_rad));
	ValueSetProperty(Math, "random", &ValueCompiledFunction(&Math_random));
	ValueSetProperty(Math, "round", &ValueCompiledFunction(&Math_round));
	ValueSetProperty(Math, "sign", &ValueCompiledFunction(&Math_sign));
	ValueSetProperty(Math, "sin", &ValueCompiledFunction(&Math_sin));
	ValueSetProperty(Math, "sinh", &ValueCompiledFunction(&Math_sinh));
	ValueSetProperty(Math, "sqrt", &ValueCompiledFunction(&Math_sqrt));
	ValueSetProperty(Math, "tan", &ValueCompiledFunction(&Math_tan));
	ValueSetProperty(Math, "tanh", &ValueCompiledFunction(&Math_tanh));
	ValueSetProperty(Math, "trunc", &ValueCompiledFunction(&Math_trunc));

	VALUE *Date = StackPush(thread, "Date", &ValueNumber(0), 0);
	ValueSetProperty(Date, "now", &ValueCompiledFunction(&Date_now));
	ValueSetProperty(Date, "getMilliseconds", &ValueCompiledFunction(&Date_getMilliseconds));
	ValueSetProperty(Date, "getSeconds", &ValueCompiledFunction(&Date_getSeconds));
	ValueSetProperty(Date, "getMinutes", &ValueCompiledFunction(&Date_getMinutes));
	ValueSetProperty(Date, "getHours", &ValueCompiledFunction(&Date_getHours));
	ValueSetProperty(Date, "getDate", &ValueCompiledFunction(&Date_getDate));
	ValueSetProperty(Date, "getDay", &ValueCompiledFunction(&Date_getDay));
	ValueSetProperty(Date, "getMonth", &ValueCompiledFunction(&Date_getMonth));
	ValueSetProperty(Date, "getYear", &ValueCompiledFunction(&Date_getYear));

	VALUE *Thread = StackPush(thread, "Thread", &ValueNumber(0), 0);
	ValueSetProperty(Thread, "sleep", &ValueCompiledFunction(&Thread_sleep));
	ValueSetProperty(Thread, "create", &ValueCompiledFunction(&Thread_create));

	VALUE *Process = StackPush(thread, "Process", &ValueNumber(0), 0);
	ValueSetProperty(Process, "open", &ValueCompiledFunction(&Process_open));

	VALUE *MBox = StackPush(thread, "MessageBox", &ValueNumber(0), 0);
	ValueSetProperty(MBox, "show", &ValueCompiledFunction(&MessageBox_show));

	VALUE *Input = StackPush(thread, "Input", &ValueNumber(0), 0);
	ValueSetProperty(Input, "keyDown", &ValueCompiledFunction(&Input_keyDown));
	ValueSetProperty(Input, "keyUp", &ValueCompiledFunction(&Input_keyUp));
	ValueSetProperty(Input, "keyPress", &ValueCompiledFunction(&Input_keyPress));
	ValueSetProperty(Input, "isKeyDown", &ValueCompiledFunction(&Input_isKeyDown));
	ValueSetProperty(Input, "onKeyDown", &ValueCompiledFunction(&Input_onKeyDown));
	ValueSetProperty(Input, "onKeyUp", &ValueCompiledFunction(&Input_onKeyUp));

	VALUE *HTTP = StackPush(thread, "HTTP", &ValueNumber(0), 0);
	ValueSetProperty(HTTP, "get", &ValueCompiledFunction(&HTTP_get));

	VALUE *Key = StackPush(thread, "Key", &ValueNumber(0), 0);
	ValueSetProperty(Key, "LBUTTON", &ValueNumber(0x01));
	ValueSetProperty(Key, "RBUTTON", &ValueNumber(0x02));
	ValueSetProperty(Key, "CANCEL", &ValueNumber(0x03));
	ValueSetProperty(Key, "MBUTTON", &ValueNumber(0x04));
	ValueSetProperty(Key, "XBUTTON1", &ValueNumber(0x05));
	ValueSetProperty(Key, "XBUTTON2", &ValueNumber(0x06));
	ValueSetProperty(Key, "BACK", &ValueNumber(0x08));
	ValueSetProperty(Key, "BACKSPACE", &ValueNumber(0x08));
	ValueSetProperty(Key, "TAB", &ValueNumber(0x09));
	ValueSetProperty(Key, "CLEAR", &ValueNumber(0x0C));
	ValueSetProperty(Key, "RETURN", &ValueNumber(0x0D));
	ValueSetProperty(Key, "SHIFT", &ValueNumber(0x10));
	ValueSetProperty(Key, "CONTROL", &ValueNumber(0x11));
	ValueSetProperty(Key, "MENU", &ValueNumber(0x12));
	ValueSetProperty(Key, "PAUSE", &ValueNumber(0x13));
	ValueSetProperty(Key, "CAPITAL", &ValueNumber(0x14));
	ValueSetProperty(Key, "CAPSLOCK", &ValueNumber(0x14));
	ValueSetProperty(Key, "CAPS", &ValueNumber(0x14));
	ValueSetProperty(Key, "KANA", &ValueNumber(0x15));
	ValueSetProperty(Key, "HANGUEL", &ValueNumber(0x15));
	ValueSetProperty(Key, "HANGUL", &ValueNumber(0x15));
	ValueSetProperty(Key, "JUNJA", &ValueNumber(0x17));
	ValueSetProperty(Key, "FINAL", &ValueNumber(0x18));
	ValueSetProperty(Key, "HANJA", &ValueNumber(0x19));
	ValueSetProperty(Key, "KANJI", &ValueNumber(0x19));
	ValueSetProperty(Key, "ESCAPE", &ValueNumber(0x1B));
	ValueSetProperty(Key, "CONVERT", &ValueNumber(0x1C));
	ValueSetProperty(Key, "NONCONVERT", &ValueNumber(0x1D));
	ValueSetProperty(Key, "ACCEPT", &ValueNumber(0x1E));
	ValueSetProperty(Key, "MODECHANGE", &ValueNumber(0x1F));
	ValueSetProperty(Key, "SPACE", &ValueNumber(0x20));
	ValueSetProperty(Key, "PRIOR", &ValueNumber(0x21));
	ValueSetProperty(Key, "PAGEUP", &ValueNumber(0x21));
	ValueSetProperty(Key, "NEXT", &ValueNumber(0x22));
	ValueSetProperty(Key, "PAGEDOWN", &ValueNumber(0x22));
	ValueSetProperty(Key, "END", &ValueNumber(0x23));
	ValueSetProperty(Key, "HOME", &ValueNumber(0x24));
	ValueSetProperty(Key, "LEFT", &ValueNumber(0x25));
	ValueSetProperty(Key, "UP", &ValueNumber(0x26));
	ValueSetProperty(Key, "RIGHT", &ValueNumber(0x27));
	ValueSetProperty(Key, "DOWN", &ValueNumber(0x28));
	ValueSetProperty(Key, "SELECT", &ValueNumber(0x29));
	ValueSetProperty(Key, "PRINT", &ValueNumber(0x2A));
	ValueSetProperty(Key, "EXECUTE", &ValueNumber(0x2B));
	ValueSetProperty(Key, "SNAPSHOT", &ValueNumber(0x2C));
	ValueSetProperty(Key, "INSERT", &ValueNumber(0x2D));
	ValueSetProperty(Key, "DELETE", &ValueNumber(0x2E));
	ValueSetProperty(Key, "HELP", &ValueNumber(0x2F));
	ValueSetProperty(Key, "ZERO", &ValueNumber(0x30));
	ValueSetProperty(Key, "_0", &ValueNumber(0x30));
	ValueSetProperty(Key, "ONE", &ValueNumber(0x31));
	ValueSetProperty(Key, "_1", &ValueNumber(0x31));
	ValueSetProperty(Key, "TWO", &ValueNumber(0x32));
	ValueSetProperty(Key, "_2", &ValueNumber(0x32));
	ValueSetProperty(Key, "THREE", &ValueNumber(0x33));
	ValueSetProperty(Key, "_3", &ValueNumber(0x33));
	ValueSetProperty(Key, "FOUR", &ValueNumber(0x34));
	ValueSetProperty(Key, "_4", &ValueNumber(0x34));
	ValueSetProperty(Key, "FIVE", &ValueNumber(0x35));
	ValueSetProperty(Key, "_5", &ValueNumber(0x35));
	ValueSetProperty(Key, "SIX", &ValueNumber(0x36));
	ValueSetProperty(Key, "_6", &ValueNumber(0x36));
	ValueSetProperty(Key, "SEVEN", &ValueNumber(0x37));
	ValueSetProperty(Key, "_7", &ValueNumber(0x37));
	ValueSetProperty(Key, "EIGHT", &ValueNumber(0x38));
	ValueSetProperty(Key, "_8", &ValueNumber(0x38));
	ValueSetProperty(Key, "NINE", &ValueNumber(0x39));
	ValueSetProperty(Key, "_9", &ValueNumber(0x39));
	ValueSetProperty(Key, "A", &ValueNumber(0x41));
	ValueSetProperty(Key, "B", &ValueNumber(0x42));
	ValueSetProperty(Key, "C", &ValueNumber(0x43));
	ValueSetProperty(Key, "D", &ValueNumber(0x44));
	ValueSetProperty(Key, "E", &ValueNumber(0x45));
	ValueSetProperty(Key, "F", &ValueNumber(0x46));
	ValueSetProperty(Key, "G", &ValueNumber(0x47));
	ValueSetProperty(Key, "H", &ValueNumber(0x48));
	ValueSetProperty(Key, "I", &ValueNumber(0x49));
	ValueSetProperty(Key, "J", &ValueNumber(0x4A));
	ValueSetProperty(Key, "K", &ValueNumber(0x4B));
	ValueSetProperty(Key, "L", &ValueNumber(0x4C));
	ValueSetProperty(Key, "M", &ValueNumber(0x4D));
	ValueSetProperty(Key, "N", &ValueNumber(0x4E));
	ValueSetProperty(Key, "O", &ValueNumber(0x4F));
	ValueSetProperty(Key, "P", &ValueNumber(0x50));
	ValueSetProperty(Key, "Q", &ValueNumber(0x51));
	ValueSetProperty(Key, "R", &ValueNumber(0x52));
	ValueSetProperty(Key, "S", &ValueNumber(0x53));
	ValueSetProperty(Key, "T", &ValueNumber(0x54));
	ValueSetProperty(Key, "U", &ValueNumber(0x55));
	ValueSetProperty(Key, "V", &ValueNumber(0x56));
	ValueSetProperty(Key, "W", &ValueNumber(0x57));
	ValueSetProperty(Key, "X", &ValueNumber(0x58));
	ValueSetProperty(Key, "Y", &ValueNumber(0x59));
	ValueSetProperty(Key, "Z", &ValueNumber(0x5A));
	ValueSetProperty(Key, "LWIN", &ValueNumber(0x5B));
	ValueSetProperty(Key, "RWIN", &ValueNumber(0x5C));
	ValueSetProperty(Key, "APPS", &ValueNumber(0x5D));
	ValueSetProperty(Key, "SLEEP", &ValueNumber(0x5F));
	ValueSetProperty(Key, "NUMPAD0", &ValueNumber(0x60));
	ValueSetProperty(Key, "NUMPAD1", &ValueNumber(0x61));
	ValueSetProperty(Key, "NUMPAD2", &ValueNumber(0x62));
	ValueSetProperty(Key, "NUMPAD3", &ValueNumber(0x63));
	ValueSetProperty(Key, "NUMPAD4", &ValueNumber(0x64));
	ValueSetProperty(Key, "NUMPAD5", &ValueNumber(0x65));
	ValueSetProperty(Key, "NUMPAD6", &ValueNumber(0x66));
	ValueSetProperty(Key, "NUMPAD7", &ValueNumber(0x67));
	ValueSetProperty(Key, "NUMPAD8", &ValueNumber(0x68));
	ValueSetProperty(Key, "NUMPAD9", &ValueNumber(0x69));
	ValueSetProperty(Key, "MULTIPLY", &ValueNumber(0x6A));
	ValueSetProperty(Key, "ADD", &ValueNumber(0x6B));
	ValueSetProperty(Key, "SEPARATOR", &ValueNumber(0x6C));
	ValueSetProperty(Key, "SUBTRACT", &ValueNumber(0x6D));
	ValueSetProperty(Key, "DECIMAL", &ValueNumber(0x6E));
	ValueSetProperty(Key, "DIVIDE", &ValueNumber(0x6F));
	ValueSetProperty(Key, "F1", &ValueNumber(0x70));
	ValueSetProperty(Key, "F2", &ValueNumber(0x71));
	ValueSetProperty(Key, "F3", &ValueNumber(0x72));
	ValueSetProperty(Key, "F4", &ValueNumber(0x73));
	ValueSetProperty(Key, "F5", &ValueNumber(0x74));
	ValueSetProperty(Key, "F6", &ValueNumber(0x75));
	ValueSetProperty(Key, "F7", &ValueNumber(0x76));
	ValueSetProperty(Key, "F8", &ValueNumber(0x77));
	ValueSetProperty(Key, "F9", &ValueNumber(0x78));
	ValueSetProperty(Key, "F10", &ValueNumber(0x79));
	ValueSetProperty(Key, "F11", &ValueNumber(0x7A));
	ValueSetProperty(Key, "F12", &ValueNumber(0x7B));
	ValueSetProperty(Key, "F13", &ValueNumber(0x7C));
	ValueSetProperty(Key, "F14", &ValueNumber(0x7D));
	ValueSetProperty(Key, "F15", &ValueNumber(0x7E));
	ValueSetProperty(Key, "F16", &ValueNumber(0x7F));
	ValueSetProperty(Key, "F17", &ValueNumber(0x80));
	ValueSetProperty(Key, "F18", &ValueNumber(0x81));
	ValueSetProperty(Key, "F19", &ValueNumber(0x82));
	ValueSetProperty(Key, "F20", &ValueNumber(0x83));
	ValueSetProperty(Key, "F21", &ValueNumber(0x84));
	ValueSetProperty(Key, "F22", &ValueNumber(0x85));
	ValueSetProperty(Key, "F23", &ValueNumber(0x86));
	ValueSetProperty(Key, "F24", &ValueNumber(0x87));

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MessageLoop, 0, 0, 0);
}

VALUE Eval(THREAD *thread, TREE *tree, int stack_id) {
	if (!tree || !tree->token) {
		return{ 0 };
	}

	switch (tree->token->type) {
		case TOKEN_DECLARATION: {
			VALUE r = Eval(thread, tree->right, stack_id);
			if (StackContains(thread, tree->left->token->value, stack_id)) {
				StackSet(thread, tree->left, &ValueCopy(&r), stack_id);
			} else {
				VALUE n = { 0 };
				StackPush(thread, tree->left->token->value, &n, stack_id);
				StackSet(thread, tree->left, &ValueCopy(&r), stack_id);
			}

			return r;
		}
		case TOKEN_EQUAL: {
			VALUE r = Eval(thread, tree->right, stack_id);
			StackSet(thread, tree->left, &ValueCopy(&r), stack_id);
			return r;
		}
		case TOKEN_WORD:
			return ValueCopy(StackGetWithProperties(thread, tree, stack_id));
		case TOKEN_ACCESSOR: {
			VALUE l = Eval(thread, tree->left, stack_id);

			thread->access_stack = stack_id;
			stack_id = -1;

			StackPush(thread, 0, &ValueScope(thread), -1);
			StackPush(thread, "this", &l, -1);
			for (unsigned int i = 0; i < l.properties.length; ++i) {
				VALUE *v = (VALUE *)ArrayGet(&l.properties, i);
				StackPush(thread, v->name, &ValueCopy(v), -1);
			}
			
			VALUE r = Eval(thread, tree->right, -1);

			while (thread->stack.length > 0) {
				VALUE v;
				ArrayPop(&thread->stack, &v);
				if (v.type == VALUE_SCOPE) {
					ValueFree(&v);
					break;
				}
				ValueFree(&v);
			}

			return r;
		}
		case TOKEN_CALL: {
			bool in_access = stack_id == -1;
			VALUE *var = StackGetWithProperties(thread, tree->left, stack_id);
			if (!var || (var->type != VALUE_FUNCTION && var->type != VALUE_COMPILED_FUNCTION)) {
				break;
			}

			VALUE func = *var;
			if (func.type == VALUE_FUNCTION) {
				VALUE args = ValueArray();
				TREE *base = tree->left;
				TREE *this_ = 0;
				TREE *name = func.function->left;
				while (tree->right && tree->right->left) {
					VALUE v = Eval(thread, tree->right->left, stack_id);
					if (name) {
						v.name = _strdup(name->token->value);
						name = name->left;
					} else {
						v.name = 0;
					}
					tree = tree->right;
					ArrayPush(args.array, &v);
				}

				VALUE *vthis;
				if (in_access && StackContains(thread, "this", -1)) {
					vthis = StackGet(thread, "this", -1);
				} else {
					this_ = base;
					while (this_->left && this_->left->left) {
						this_ = this_->left;
					}

					TREE *copy = this_->left; this_->left = 0;
					vthis = StackGetWithProperties(thread, base, stack_id);
					this_->left = copy;
				}

				++stack_id;
				
				VALUE s = { 0 };

				s.type = VALUE_SCOPE;
				for (int i = thread->stack.length - 1; i > -1; --i) {
					VALUE *v = (VALUE *)ArrayGet(&thread->stack, i);
					if (v == var) {
						s.stack_last = i;
						break;
					}
				}

				StackPush(thread, 0, &s, -1);
				StackPush(thread, "arguments", &args, stack_id);
				StackPush(thread, "this", &ValueCopy(vthis), stack_id);

				for (unsigned int i = 0; i < args.array->length; ++i) {
					VALUE *v = (VALUE *)ArrayGet(args.array, i);
					if (!v->name) break;
					StackPush(thread, v->name, &ValueCopy(v), stack_id);
				}

				tree = func.function;
				VALUE ret = { 0 };
				while (tree->right) {
					ret = Eval(thread, tree->right->left, stack_id);
					if (ret.return_) {
						ret.return_ = false;
						break;
					}
					ValueFree(&ret); // used to cause a crash for: a=func(){b:=[1,2,3,4].pop();return b;}
					ret = { 0 };
					tree = tree->right;
				}

				while (thread->stack.length > 0) {
					VALUE v;
					ArrayPop(&thread->stack, &v);
					if (v.type == VALUE_SCOPE) {
						ValueFree(&v);
						break;
					}
					ValueFree(&v);
				}

				return ret;
			} else {
				VALUE args = ValueArray();
				TREE *base = tree->left;
				TREE *this_ = 0;
				
				while (tree->right && tree->right->left) {
					VALUE v = Eval(thread, tree->right->left, stack_id);
					tree = tree->right;
					ArrayPush(args.array, &v);
				}

				VALUE *vthis;
				if (in_access && StackContains(thread, "this", -1)) {
					vthis = StackGet(thread, "this", -1);
				} else {
					this_ = base;
					while (this_->left && this_->left->left) {
						this_ = this_->left;
					}

					TREE *copy = this_->left; this_->left = 0;
					vthis = StackGetWithProperties(thread, base, stack_id);
					this_->left = copy;
				}

				VALUE r = ((COMPILED_FUNC)func.compiled_function)(vthis, args.array);
				ValueFree(&args);
				return r;
			}

			break;
		}
		case TOKEN_IF: {
			VALUE r = Eval(thread, tree->left->left, ++stack_id);
			if (r.number) {
				tree = tree->left;
				while (tree->right) {
					VALUE v = Eval(thread, tree->right->left, stack_id);
					if (v.return_) {
						RETURN_OUT(v);
					}
					ValueFree(&v);
					tree = tree->right;
				}
			}
			ValueFree(&r);

			while (tree->right) {
				if (tree->right->token->type == TOKEN_ELSE_IF) {
					TREE *t = tree->right->left;
					r = Eval(thread, t->left, ++stack_id);
					if (r.number) {
						while (t->right) {
							VALUE v = Eval(thread, t->right->left, stack_id);
							if (v.return_) {
								RETURN_OUT(v);
							}
							ValueFree(&v);
							t = t->right;
						}
						ValueFree(&r);
					}
					ValueFree(&r);
				} else {
					++stack_id;
					TREE *t = tree->right;
					while (t->right) {
						VALUE v = Eval(thread, t->right->left, stack_id);
						if (v.return_) {
							RETURN_OUT(v);
						}
						ValueFree(&v);
						t = t->right;
					}
				}

				tree = tree->right;
			}

			break;
		}
		case TOKEN_WHILE: {
			TREE *t;
			++stack_id;
			char break_ = 0;
			VALUE v = { 0 };
			while (!break_ && (v = Eval(thread, tree->left, stack_id)).number) {
				ValueFree(&v);
				t = tree;
				while (t->right) {
					VALUE v = Eval(thread, t->right->left, stack_id);
					if (v.return_) {
						RETURN_OUT(v);
					} else if (v.type == VALUE_BREAK) {
						ValueFree(&v);
						break_ = 1;
						break;
					}
					ValueFree(&v);
					t = t->right;
				}
			}

			break;
		}
		case TOKEN_FOR: {
			++stack_id;
			VALUE v = { 0 };
			if (tree->left->left) {
				v = Eval(thread, tree->left->left, stack_id);
				ValueFree(&v);
				TREE *t = tree->left;
				while (t->right) {
					v = Eval(thread, t->right->left, stack_id);
					ValueFree(&v);
					t = t->right;
				}
			}

			char break_ = 0;
			while (!tree->right->left || (v = Eval(thread, tree->right->left, stack_id)).number) {
				ValueFree(&v);
				TREE *t = tree->right->right;
				while (t->right) {
					VALUE v = Eval(thread, t->right->left, stack_id);
					if (v.return_) {
						RETURN_OUT(v);
					} else if (v.type == VALUE_BREAK) {
						ValueFree(&v);
						break_ = 1;
						break;
					}
					ValueFree(&v);
					t = t->right;
				}

				if (break_) {
					break;
				}

				t = tree->right->right;
				if (t->left->left) {
					v = Eval(thread, t->left->left, stack_id);
					ValueFree(&v);
					t = tree->right->right->left;
					while (t->right) {
						v = Eval(thread, t->right->left, stack_id);
						ValueFree(&v);
						t = t->right;
					}
				}
			}

			break;
		}
		case TOKEN_QUESTION: {
			VALUE v = { 0 };
			if ((v = Eval(thread, tree->left, stack_id)).number) {
				ValueFree(&v);
				return Eval(thread, tree->right->left, stack_id);
			} else {
				ValueFree(&v);
				return Eval(thread, tree->right->right, stack_id);
			}

			break;
		}
		case TOKEN_PLUS_PLUS: {
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v && v->type == VALUE_NUMBER) {
				++(v->number);

				VALUE r = ValueNumber(v->number);
				if (*tree->token->value != '+') {
					--r.number;
				}
				return r;
			}

			break;
		}
		case TOKEN_MINUS_MINUS: {
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v && v->type == VALUE_NUMBER) {
				--(v->number);

				VALUE r = ValueNumber(v->number);
				if (*tree->token->value != '-') {
					++r.number;
				}
				return r;
			}

			break;
		}
		case TOKEN_PLUS_EQUAL: {
			tree->token->type = TOKEN_PLUS;
			VALUE n = Eval(thread, tree, stack_id);
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v) {
				StackSet(thread, tree->left, &ValueCopy(&n), stack_id);
				return n;
			}
			ValueFree(&n);
			break;
		}
		case TOKEN_MINUS_EQUAL: {
			tree->token->type = TOKEN_MINUS;
			VALUE n = Eval(thread, tree, stack_id);
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v) {
				StackSet(thread, tree->left, &ValueCopy(&n), stack_id);
				return n;
			}
			ValueFree(&n);
			break;
		}
		case TOKEN_MULTIPLY_EQUAL: {
			tree->token->type = TOKEN_MULTIPLY;
			VALUE n = Eval(thread, tree, stack_id);
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v) {
				StackSet(thread, tree->left, &ValueCopy(&n), stack_id);
				return n;
			}
			ValueFree(&n);
			break;
		}
		case TOKEN_DIVIDE_EQUAL: {
			tree->token->type = TOKEN_DIVIDE;
			VALUE n = Eval(thread, tree, stack_id);
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v) {
				StackSet(thread, tree->left, &ValueCopy(&n), stack_id);
				return n;
			}
			ValueFree(&n);
			break;
		}
		case TOKEN_MOD_EQUAL: {
			tree->token->type = TOKEN_MOD;
			VALUE n = Eval(thread, tree, stack_id);
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v) {
				StackSet(thread, tree->left, &ValueCopy(&n), stack_id);
				return n;
			}
			ValueFree(&n);
			break;
		}
		case TOKEN_AND_EQUAL: {
			tree->token->type = TOKEN_AND;
			VALUE n = Eval(thread, tree, stack_id);
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v) {
				StackSet(thread, tree->left, &ValueCopy(&n), stack_id);
				return n;
			}
			ValueFree(&n);
			break;
		}
		case TOKEN_OR_EQUAL: {
			tree->token->type = TOKEN_OR;
			VALUE n = Eval(thread, tree, stack_id);
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v) {
				StackSet(thread, tree->left, &ValueCopy(&n), stack_id);
				return n;
			}
			ValueFree(&n);
			break;
		}
		case TOKEN_XOR_EQUAL: {
			tree->token->type = TOKEN_XOR;
			VALUE n = Eval(thread, tree, stack_id);
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v) {
				StackSet(thread, tree->left, &ValueCopy(&n), stack_id);
				return n;
			}
			ValueFree(&n);
			break;
		}
		case TOKEN_SHIFT_LEFT_EQUAL: {
			tree->token->type = TOKEN_SHIFT_LEFT;
			VALUE n = Eval(thread, tree, stack_id);
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v) {
				StackSet(thread, tree->left, &ValueCopy(&n), stack_id);
				return n;
			}
			ValueFree(&n);
			break;
		}
		case TOKEN_SHIFT_RIGHT_EQUAL: {
			tree->token->type = TOKEN_SHIFT_RIGHT;
			VALUE n = Eval(thread, tree, stack_id);
			VALUE *v = StackGetWithProperties(thread, tree->left, stack_id);
			if (v) {
				StackSet(thread, tree->left, &ValueCopy(&n), stack_id);
				return n;
			}
			ValueFree(&n);
			break;
		}
		case TOKEN_PLUS: {
			VALUE l = Eval(thread, tree->left, stack_id);
			VALUE r = Eval(thread, tree->right, stack_id);

			VALUE v = { 0 };
			if (l.type == VALUE_NUMBER && r.type == VALUE_NUMBER) {
				v = ValueNumber(l.number + r.number);
			} else if (l.type == VALUE_STRING && r.type == VALUE_STRING) {
				char *s = (char *)malloc(l.string_length + r.string_length + 1);
				sprintf(s, "%s%s", l.string, r.string);
				v = ValueRawString(s);
				free(s);
			} else if (l.type == VALUE_STRING && r.type == VALUE_NUMBER) {
				char temp[0xFF] = { 0 };
				int nl = sprintf(temp, "%.17g", r.number);

				char *s = (char *)malloc(l.string_length + nl + 1);
				sprintf(s, "%s%s", l.string, temp);
				v = ValueRawString(s);
				free(s);
			} else if (l.type == VALUE_NUMBER && r.type == VALUE_STRING) {
				char temp[0xFF] = { 0 };
				int nl = sprintf(temp, "%.17g", l.number);

				char *s = (char *)malloc(r.string_length + nl + 1);
				sprintf(s, "%s%s", temp, r.string);
				v = ValueRawString(s);
				free(s);
			} else {
				v = ValueNumber(0);
			}

			ValueFree(&l);
			ValueFree(&r);
			return v;
		}
		case TOKEN_MINUS: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.type == VALUE_NUMBER && right.type == VALUE_NUMBER ? left.number - right.number : 0);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_MULTIPLY: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.type == VALUE_NUMBER && right.type == VALUE_NUMBER ? left.number * right.number : 0);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_DIVIDE: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.type == VALUE_NUMBER && right.type == VALUE_NUMBER ? left.number / right.number : 0);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_MOD: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.type == VALUE_NUMBER && right.type == VALUE_NUMBER ? (int)left.number % (int)right.number : 0);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_AND: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.type == VALUE_NUMBER && right.type == VALUE_NUMBER ? (int)left.number & (int)right.number : 0);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_OR: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.type == VALUE_NUMBER && right.type == VALUE_NUMBER ? (int)left.number | (int)right.number : 0);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_XOR: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.type == VALUE_NUMBER && right.type == VALUE_NUMBER ? (int)left.number ^ (int)right.number : 0);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_SHIFT_LEFT: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.type == VALUE_NUMBER && right.type == VALUE_NUMBER ? (int)left.number << (int)right.number : 0);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_SHIFT_RIGHT: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.type == VALUE_NUMBER && right.type == VALUE_NUMBER ? (int)left.number >> (int)right.number : 0);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_EQUAL_EQUAL: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = { 0 };
			if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
				v = ValueNumber(strcmp(left.string, right.string) == 0);
			} else {
				v = ValueNumber(left.number == right.number);
			}

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_NOT_EQUAL: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = { 0 };
			if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
				v = ValueNumber(strcmp(left.string, right.string) != 0);
			} else {
				v = ValueNumber(left.number == right.number);
			}

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_AND_AND: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.number && right.number);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_OR_OR: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.number || right.number);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_GREATER: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.number > right.number);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_GREATER_EQUAL: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.number >= right.number);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_LESS: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.number < right.number);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_LESS_EQUAL: {
			VALUE left = Eval(thread, tree->left, stack_id);
			VALUE right = Eval(thread, tree->right, stack_id);

			VALUE v = ValueNumber(left.number <= right.number);

			ValueFree(&left);
			ValueFree(&right);
			return v;
		}
		case TOKEN_NOT: {
			VALUE left = Eval(thread, tree->left, stack_id);

			VALUE v = ValueNumber(!left.number);

			ValueFree(&left);
			return v;
		}
		case TOKEN_DECIMAL_NUMBER:
			return ValueNumber(atof(tree->token->value));
		case TOKEN_HEX_NUMBER:
			return ValueNumber((double)strtoll(tree->token->value + 2, NULL, 16));
		case TOKEN_BINARY_NUMBER:
			return ValueNumber((double)strtoll(tree->token->value + 2, NULL, 2));
		case TOKEN_STRING: {
			VALUE v = ValueString(tree->token->value + 1);
			*(v.string + --v.string_length) = 0;
			return v;
		}
		case TOKEN_ARRAY: {
			VALUE v = ValueArray();

			TREE *e = tree;
			if (e->left) {
				ArrayPush(v.array, &Eval(thread, e->left, stack_id));

				while (e->right) {
					e = e->right;

					ArrayPush(v.array, &Eval(thread, e->left, stack_id));
				}
			}

			return v;
		}
		case TOKEN_FUNC:
			return ValueFunction(thread, tree);
		case TOKEN_BREAK: {
			VALUE v = { 0 };
			v.type = VALUE_BREAK;
			return v;
		}
		case TOKEN_RETURN: {
			VALUE v = Eval(thread, tree->left, stack_id);
			v.return_ = 1;
			return v;
		}
	}

	return{ 0 };
}

VALUE Number_toExponential(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_NUMBER) {
		char str[0xFF] = { 0 };
		sprintf(str, "%e", this_->number);
		return ValueRawString(str);
	}
	return ValueNumber(0);
}

VALUE Number_toString(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_NUMBER) {
		char str[0xFF] = { 0 };
		sprintf(str, "%.17g", this_->number);
		return ValueRawString(str);
	}
	return ValueNumber(0);
}

VALUE Number_toFixed(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_NUMBER) {
		char precision[0xFF] = "%.0";

		if (arguments->length > 0) {
			VALUE *v = (VALUE *)ArrayGet(arguments, 0);
			if (v->type == VALUE_NUMBER && v->number >= 0) {
				sprintf(precision + 2, "%.0f", v->number);
			}
		}

		strcat(precision, "f");

		char str[0xFF] = { 0 };
		sprintf(str, precision, this_->number);
		return ValueRawString(str);
	}
	return ValueNumber(0);
}

VALUE ValueNumber(double number) {
	VALUE v = { 0 };
	v.type = VALUE_NUMBER;
	v.number = number;

	ValueSetProperty(&v, "toExponential", &ValueCompiledFunction(&Number_toExponential));
	ValueSetProperty(&v, "toFixed", &ValueCompiledFunction(&Number_toFixed));
	ValueSetProperty(&v, "toString", &ValueCompiledFunction(&Number_toString));

	return v;
}

VALUE String_get(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_STRING && arguments->length > 0 && this_->string) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		if (v->type == VALUE_NUMBER && (int)v->number > -1 && (int)v->number < (int)this_->string_length) {
			char b[2];
			b[0] = this_->string[(int)v->number];
			b[1] = 0;
			return ValueRawString(b);
		}
	}

	return ValueNumber(0);
}

VALUE String_set(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_STRING && arguments->length > 1 && this_->string) {
		VALUE *v0 = (VALUE *)ArrayGet(arguments, 0);
		if (v0->type == VALUE_NUMBER && (int)v0->number > -1 && (int)v0->number < (int)this_->string_length) {
			VALUE *v1 = (VALUE *)ArrayGet(arguments, 1);
			if (v1->type == VALUE_STRING && v1->string && v1->string_length > 0) {
				this_->string[(int)v0->number] = *v1->string;
			}
		}
	}

	return ValueNumber(0);
}

VALUE String_charCodeAt(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_STRING && this_->string && arguments->length > 0) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		if (v->type == VALUE_NUMBER && (int)v->number > -1 && (int)v->number < (int)this_->string_length) {
			return ValueNumber(this_->string[(int)v->number]);
		}
	}

	return ValueNumber(0);
}

VALUE String_toUpperCase(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_STRING && this_->string) {
		char *str = _strdup(this_->string);

		for (char *c = str; *c; ++c) *c = toupper(*c);

		VALUE r = ValueRawString(str);
		free(str);
		return r;
	}

	return ValueNumber(0);
}

VALUE String_toLowerCase(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_STRING && this_->string) {
		char *str = _strdup(this_->string);

		for (char *c = str; *c; ++c) *c = tolower(*c);

		VALUE r = ValueRawString(str);
		free(str);
		return r;
	}

	return ValueNumber(0);
}

VALUE String_toNumber(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_STRING && this_->string) {
		return ValueNumber(atof(this_->string));
	}

	return ValueNumber(0);
}

VALUE String_indexOf(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_STRING && this_->string && arguments->length > 0) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		if (v->type == VALUE_STRING && v->string) {
			char *i = strstr(this_->string, v->string);
			if (i) {
				return ValueNumber((double)(i - this_->string));
			}
		}
	}
	
	return ValueNumber(-1);
}

VALUE String_length(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_STRING) {
		return ValueNumber(this_->string_length);
	}

	return ValueNumber(0);
}

VALUE ValueString(char *string_) {
	VALUE value = { 0 };

	value.type = VALUE_STRING;
	value.string_length = (unsigned int)strlen(string_);
	value.string = (char *)_strdup(string_);

	char *s = value.string;
	for (int l = value.string_length; *s && l > 1; ++s, --l) {
		if (memcmp(s, "\\'", 2) == 0) {
			--value.string_length;
			memmove(s, s + 1, l);
			*s = '\'';
		} else if (memcmp(s, "\\\"", 2) == 0) {
			--value.string_length;
			memmove(s, s + 1, l);
			*s = '\"';
		} else if (memcmp(s, "\\\\", 2) == 0) {
			--value.string_length;
			memmove(s, s + 1, l);
			*s = '\\';
		} else if (memcmp(s, "\\n", 2) == 0) {
			--value.string_length;
			memmove(s, s + 1, l);
			*s = '\n';
		} else if (memcmp(s, "\\r", 2) == 0) {
			--value.string_length;
			memmove(s, s + 1, l);
			*s = '\r';
		} else if (memcmp(s, "\\t", 2) == 0) {
			--value.string_length;
			memmove(s, s + 1, l);
			*s = '\t';
		} else if (memcmp(s, "\\b", 2) == 0) {
			--value.string_length;
			memmove(s, s + 1, l);
			*s = '\b';
		} else if (memcmp(s, "\\f", 2) == 0) {
			--value.string_length;
			memmove(s, s + 1, l);
			*s = '\f';
		} else if (memcmp(s, "\\v", 2) == 0) {
			--value.string_length;
			memmove(s, s + 1, l);
			*s = '\v';
		} else if (memcmp(s, "\\0", 2) == 0) {
			--value.string_length;
			memmove(s, s + 1, l);
			*s = '\0';
			value.string_length = (unsigned int)strlen(value.string) + 1;
			break;
		} else if (l > 3 && memcmp(s, "\\x", 2) == 0) {
			value.string_length -= 3;
			unsigned char v = (unsigned char)strtol(s + 2, NULL, 16);
			memmove(s, s + 3, l);
			*s = v;
		}
	}

	ValueSetProperty(&value, "length", &ValueCompiledFunction(&String_length));
	ValueSetProperty(&value, "get", &ValueCompiledFunction(&String_get));
	ValueSetProperty(&value, "set", &ValueCompiledFunction(&String_set));
	ValueSetProperty(&value, "charCodeAt", &ValueCompiledFunction(&String_charCodeAt));
	ValueSetProperty(&value, "toUpperCase", &ValueCompiledFunction(&String_toUpperCase));
	ValueSetProperty(&value, "toLowerCase", &ValueCompiledFunction(&String_toLowerCase));
	ValueSetProperty(&value, "toNumber", &ValueCompiledFunction(&String_toNumber));
	ValueSetProperty(&value, "indexOf", &ValueCompiledFunction(&String_indexOf));

	return value;
}

VALUE ValueRawString(char *string_) {
	VALUE value = { 0 };

	value.type = VALUE_STRING;
	value.string_length = (unsigned int)strlen(string_);
	value.string = (char *)_strdup(string_);

	ValueSetProperty(&value, "length", &ValueCompiledFunction(&String_length));
	ValueSetProperty(&value, "get", &ValueCompiledFunction(&String_get));
	ValueSetProperty(&value, "set", &ValueCompiledFunction(&String_set));
	ValueSetProperty(&value, "charCodeAt", &ValueCompiledFunction(&String_charCodeAt));
	ValueSetProperty(&value, "toUpperCase", &ValueCompiledFunction(&String_toUpperCase));
	ValueSetProperty(&value, "toLowerCase", &ValueCompiledFunction(&String_toLowerCase));
	ValueSetProperty(&value, "toNumber", &ValueCompiledFunction(&String_toNumber));
	ValueSetProperty(&value, "indexOf", &ValueCompiledFunction(&String_indexOf));

	return value;
}

VALUE Array_set(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_ARRAY && arguments->length > 1) {
		VALUE *v0 = (VALUE *)ArrayGet(arguments, 0);
		if (v0->type == VALUE_NUMBER) {
			int index = (int)v0->number;
			if (index > -1 && index < (int)this_->array->length) {
				ArraySet(this_->array, index, &ValueCopy((VALUE *)ArrayGet(arguments, 1)));
			}
		}
	}

	return ValueNumber(0);
}

VALUE Array_get(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_ARRAY && arguments->length > 0 && this_->array && this_->array->length > 0) {
		VALUE *v = (VALUE *)ArrayGet(arguments, 0);
		if (v->type == VALUE_NUMBER && (int)v->number > -1 && (int)v->number < (int)this_->array->length) {
			return ValueCopy((VALUE *)ArrayGet(this_->array, (int)v->number));
		}
	}

	return ValueNumber(0);
}

VALUE Array_push(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_ARRAY && arguments->length > 0) {
		for (unsigned int i = 0; i < arguments->length; ++i) {
			ArrayPush(this_->array, &ValueCopy((VALUE *)ArrayGet(arguments, i)));
		}
		
		return ValueNumber(this_->array->length - 1);
	}

	return ValueNumber(-1);
}

VALUE Array_pop(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_ARRAY && this_->array->length > 0) {
		VALUE out = { 0 };
		ArrayPop(this_->array, &out);
		return out;
	}

	return ValueNumber(0);
}

VALUE Array_length(VALUE *this_, ARRAY *arguments) {
	if (this_->type == VALUE_ARRAY) {
		return ValueNumber(this_->array->length);
	}

	return ValueNumber(0);
}

VALUE ValueArray() {
	VALUE value = { 0 };

	value.type = VALUE_ARRAY;
	value.array = (ARRAY *)malloc(sizeof(ARRAY));
	memcpy(value.array, &ArrayNew(sizeof(VALUE)), sizeof(ARRAY));

	ValueSetProperty(&value, "length", &ValueCompiledFunction(&Array_length));
	ValueSetProperty(&value, "set", &ValueCompiledFunction(&Array_set));
	ValueSetProperty(&value, "get", &ValueCompiledFunction(&Array_get));
	ValueSetProperty(&value, "push", &ValueCompiledFunction(&Array_push));
	ValueSetProperty(&value, "pop", &ValueCompiledFunction(&Array_pop));

	return value;
}

VALUE ValueFunction(THREAD *thread, TREE *function) {
	VALUE value = { 0 };

	value.type = VALUE_FUNCTION;
	value.function = CopyTree(function);
	value.stack_last = thread->stack.length;

	return value;
}

VALUE ValueScope(THREAD *thread) {
	VALUE value = { 0 };

	value.type = VALUE_SCOPE;
	for (int i = thread->stack.length - 1; i > -1; --i) {
		VALUE *v = (VALUE *)ArrayGet(&thread->stack, i);
		if (v->stack_id == 0) {
			value.stack_last = i;
			break;
		}
	}

	return value;
}

VALUE ValueCompiledFunction(void *function) {
	VALUE value = { 0 };

	value.type = VALUE_COMPILED_FUNCTION;
	value.compiled_function = function;

	return value;
}

VALUE ValueCopy(VALUE *value) {
	VALUE v = { 0 };
	if (value) {
		memcpy(&v, value, sizeof(VALUE));
		v.name = 0;
		if (value->type == VALUE_STRING) {
			if (value->string) {
				v.string = _strdup(value->string);
			} else {
				v.string = 0;
				v.string_length = 0;
			}
		} else if (value->type == VALUE_FUNCTION) {
			v.function = CopyTree(value->function);
		} else if (value->type == VALUE_ARRAY) {
			v.array = (ARRAY *)malloc(sizeof(ARRAY));
			memcpy(v.array, &ArrayNew(sizeof(VALUE)), sizeof(ARRAY));
			for (unsigned int i = 0; i < value->array->length; ++i) {
				ArrayPush(v.array, &ValueCopy((VALUE *)ArrayGet(value->array, i)));
			}
		}

		if (value->properties.buffer && value->properties.length > 0) {
			v.properties = ArrayNew(sizeof(VALUE));
			for (unsigned int i = 0; i < value->properties.length; ++i) {
				VALUE *p = (VALUE *)ArrayGet(&value->properties, i);
				if (p->name) {
					VALUE *n = (VALUE *)ArrayPush(&v.properties, &ValueCopy(p));
					n->name = _strdup(p->name);
				}
			}
		}
	}

	return v;
}

VALUE ValueStack() {
	THREAD *mt = GetMainThread();
	VALUE value = { 0 };
	value.type = VALUE_STACK;
	value.compiled_function = mt;

	for (int i = mt->stack.length - 1; i > -1; --i) {
		VALUE *v = (VALUE *)ArrayGet(&mt->stack, i);
		if (v->stack_id == 0) {
			value.stack_last = i;
			break;
		}
	}

	return value;
}

void PrintValue(VALUE *value, bool quotes) {
	if (value) {
		switch (value->type) {
			case VALUE_NUMBER:
				printf("%.17g", value->number);
				return;
			case VALUE_STRING:
				if (quotes) {
					printf("\"%s\"", value->string);
				} else {
					printf("%s", value->string);
				}
				return;
			case VALUE_ARRAY: {
				printf("[");
				for (unsigned int i = 0; i < value->array->length; ++i) {
					PrintValue((VALUE *)ArrayGet(value->array, i), 1);
					if (i + 1 < value->array->length) {
						printf(", ");
					}
				}
				printf("]");

				break;
			}
			default:
				#ifdef _WIN64
				printf("0x%016llx", (SINT)value->array);
				#else
				printf("0x%08x", (SINT)value->array);
				#endif
				break;
		}
	}
}

void ValueFree(VALUE *value) {
	switch (value->type) {
		case VALUE_STRING:
			if (value->string) {
				free(value->string);
			}
			break;
		case VALUE_ARRAY: {
			if (value->array) {
				for (unsigned int i = 0; i < value->array->length; ++i) {
					ValueFree((VALUE *)ArrayGet(value->array, i));
				}
				ArrayFree(value->array);
				free(value->array);
			}
			break;
		}
		case VALUE_FUNCTION:
			FreeTree(value->function);
			break;
	}

	if (value->name) {
		free(value->name);
	}

	if (value->properties.buffer) {
		for (unsigned int i = 0; i < value->properties.length; ++i) {
			ValueFree((VALUE *)ArrayGet(&value->properties, i));
		}
		ArrayFree(&value->properties);
		memset(&value->properties, 0, sizeof(ARRAY));
	}

	memset(value, 0, sizeof(VALUE));
}

void ValueFreeEx(VALUE *value) {
	switch (value->type) {
		case VALUE_STRING:
			if (value->string) {
				free(value->string);
			}
			break;
		case VALUE_ARRAY: {
			if (value->array) {
				for (unsigned int i = 0; i < value->array->length; ++i) {
					ValueFree((VALUE *)ArrayGet(value->array, i));
				}
				ArrayFree(value->array);
			}
			break;
		}
		case VALUE_FUNCTION:
			FreeTree(value->function);
			break;
	}
}

VALUE *ValueGetProperty(VALUE *value, char *name) {
	if (!value->properties.buffer) {
		value->properties = ArrayNew(sizeof(VALUE));
		VALUE p = { 0 };
		p.name = _strdup(name);
		return (VALUE *)ArrayPush(&value->properties, &p);
	}

	for (unsigned int i = 0; i < value->properties.length; ++i) {
		VALUE *v = (VALUE *)ArrayGet(&value->properties, i);
		if (v->name && strcmp(v->name, name) == 0) {
			return v;
		}
	}

	VALUE p = { 0 };
	p.name = _strdup(name);
	return (VALUE *)ArrayPush(&value->properties, &p);
}

VALUE *ValueSetProperty(VALUE *value, char *name, VALUE *property) {
	if (!value->properties.buffer) {
		value->properties = ArrayNew(sizeof(VALUE));
	} else {
		for (unsigned int i = 0; i < value->properties.length; ++i) {
			VALUE *v = (VALUE *)ArrayGet(&value->properties, i);
			if (v->name && strcmp(v->name, name) == 0) {
				ValueFree(v);
				property->name = _strdup(name);
				return (VALUE *)memcpy(v, property, sizeof(VALUE));
			}
		}
	}

	property->name = _strdup(name);
	return (VALUE *)ArrayPush(&value->properties, property);
}

bool StackContains(THREAD *thread, char *name, int stack_id) {
	for (int i = (int)thread->stack.length - 1; i > -1; --i) {
		VALUE *v = (VALUE *)ArrayGet(&thread->stack, i);
		if (v->stack_id == stack_id && v->name && strcmp(name, v->name) == 0) {
			return true;
		}
	}

	return false;
}

VALUE *StackPush(THREAD *thread, char *name, VALUE *value, int stack_id) {
	VALUE n = { 0 };
	memcpy(&n, value, sizeof(VALUE));
	n.name = name ? _strdup(name) : 0;
	n.stack_id = stack_id;
	return (VALUE *)ArrayPush(&thread->stack, &n);
}

VALUE *StackGet(THREAD *thread, char *name, int stack_id) {
	if (thread->access_stack != 0) {
		int t = thread->access_stack;
		thread->access_stack = 0;
		VALUE *ret = StackGet(thread, name, stack_id);
		stack_id = t;
		return ret;
	}

	for (int i = (int)thread->stack.length - 1; i > -1; --i) {
		VALUE *v = (VALUE *)ArrayGet(&thread->stack, i);
		if (v->type == VALUE_SCOPE) {
			i = v->stack_last + 1;
		} else if (v->type == VALUE_STACK) {
			thread = (THREAD *)v->compiled_function;
			i = v->stack_last + 1;
		} else if (v->stack_id <= stack_id && v->name && strcmp(name, v->name) == 0) {
			return v;
		}
	}

	return 0;
}

VALUE *StackGetWithProperties(THREAD *thread, TREE *var, int stack_id) {
	VALUE *v = StackGet(thread, var->token->value, stack_id);
	if (v) {
		while (var->left) {
			v = ValueGetProperty(v, var->left->token->value);
			var = var->left;
		}

		return v;
	}
	return 0;
}

void StackSet(THREAD *thread, TREE *var, VALUE *value, int stack_id) {
	VALUE *v = StackGetWithProperties(thread, var, stack_id);
	if (!v) {
		VALUE n = { 0 };
		StackPush(thread, var->token->value, &n, stack_id);
		return StackSet(thread, var, value, stack_id);
	}

	if (value->properties.buffer && value->properties.length > 0) {
		value->name = _strdup(v->name);
		ValueFree(v);
		memcpy(v, value, sizeof(VALUE));
		value->name = 0;
	} else {
		if (v->type == value->type && ((v->type == VALUE_ARRAY && v->array == value->array) || (v->type == VALUE_FUNCTION && v->function == value->function))) {
			return;
		}

		ValueFreeEx(v);
		v->type = value->type;

		switch (value->type) {
			case VALUE_NUMBER:
				v->number = value->number;
				break;
			case VALUE_STRING:
				v->string = value->string;
				v->string_length = value->string_length;
				break;
			case VALUE_ARRAY:
				v->array = value->array;
				break;
			case VALUE_FUNCTION:
				v->function = value->function;
				v->stack_last = value->stack_last;
				break;
			case VALUE_COMPILED_FUNCTION:
				v->compiled_function = value->compiled_function;
				break;
		}
	}
}