#include <windows.h>
#include <winevt.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#define CLASS_NAME	L"MySystemLogMonitor"

#pragma comment(lib, "wevtapi.lib")

void PrintEvent(EVT_HANDLE);
wchar_t *GetDescriptionMessage(const wchar_t *ProviderName, EVT_HANDLE hEvent, EVT_FORMAT_MESSAGE_FLAGS Format);
// void PrintEvtVariantType(EVT_VARIANT variant, const wchar_t *PropertyName);

DWORD WINAPI SystemCallback(EVT_SUBSCRIBE_NOTIFY_ACTION Action, void* UserContext, void* hEvent);
DWORD WINAPI ApplicationCallback(EVT_SUBSCRIBE_NOTIFY_ACTION Action, void* UserContext, void* hEvent);
DWORD WINAPI SecurityCallback(EVT_SUBSCRIBE_NOTIFY_ACTION Action, void* UserContext, void* hEvent);

int wmain(){
	//HANDLE hSystem		= CreateEvent(NULL, FALSE, FALSE, NULL),
	//	   hApplication	= CreateEvent(NULL, FALSE, FALSE, NULL),
	//	   hSecurity	= CreateEvent(NULL, FALSE, FALSE, NULL);
	_wsetlocale(LC_ALL, L"");
	SetConsoleOutputCP(CP_UTF8);
	void	*SystemArguments		= NULL,
			*ApplicationArguments	= NULL,
			*SecurityArguments		= NULL;

	const wchar_t *lpszQuery = L"*";
	EVT_HANDLE hSubsSystem = EvtSubscribe(NULL, NULL, L"System", lpszQuery, NULL, SystemArguments, SystemCallback, EvtSubscribeToFutureEvents);

	if(hSubsSystem == NULL){
		wprintf(L"hSubsSystem is null: %d\n", GetLastError());
		return -1;
	}

	EVT_HANDLE hSubsApplication = EvtSubscribe(NULL, NULL, L"Application", lpszQuery, NULL, ApplicationArguments, ApplicationCallback, EvtSubscribeToFutureEvents);
	if(hSubsApplication == NULL){
		wprintf(L"hSubsApplication is null: %d\n", GetLastError());
		EvtClose(hSubsSystem);
		return -1;
	}

	EVT_HANDLE hSubsSecurity = EvtSubscribe(NULL, NULL, L"Security", lpszQuery, NULL, SecurityArguments, SecurityCallback, EvtSubscribeToFutureEvents);
	if(hSubsSecurity == NULL){
		EvtClose(hSubsSystem);
		EvtClose(hSubsApplication);
		return -1;
	}

	while(1){
		Sleep(1000);
	}

	if(hSubsSecurity)		{ EvtClose(hSubsSecurity); }
	if(hSubsApplication)	{ EvtClose(hSubsApplication); }
	if(hSubsSystem)			{ EvtClose(hSubsApplication); }
	// if(hSecurity)		{ CloseHandle(hSecurity);}
	// if(hApplication)		{ CloseHandle(hApplication);}
	// if(hSystem)			{ CloseHandle(hSystem);}
	return 0;
}

void PrintEvent(EVT_HANDLE hEvent){
	DWORD dwBufferUsed = 0,
		  dwPropertyCount = 0,
		  dwBufferSize = 0;
	PEVT_VARIANT pValues = NULL;

	EVT_HANDLE hContext = EvtCreateRenderContext(0, NULL, EvtRenderContextSystem);
	if(hContext == NULL) {
		wprintf(L"EvtCreateRenderContext failed: %d\n", GetLastError());
		return;
	}

	if(!EvtRender(hContext, hEvent, EvtRenderEventValues, dwBufferSize, pValues, &dwBufferUsed, &dwPropertyCount)){
		if(GetLastError() != ERROR_INSUFFICIENT_BUFFER){
			wprintf(L"EvtRender failed(Resize): %d\n", GetLastError());
			EvtClose(hEvent);
			EvtClose(hContext);
			return;
		}
	}

	dwBufferSize = dwBufferUsed;
	pValues = (PEVT_VARIANT)malloc(dwBufferUsed);
	if(pValues == NULL){
		wprintf(L"Allocation failed: %d\n", GetLastError());
		EvtClose(hEvent);
		EvtClose(hContext);
		return;
	}

	if(!EvtRender(hContext, hEvent, EvtRenderEventValues, dwBufferSize, pValues, &dwBufferUsed, &dwPropertyCount)){
		if(GetLastError() != ERROR_SUCCESS){
			wprintf(L"EvtRender failed(Rendering): %d\n", GetLastError());
			free(pValues);
			EvtClose(hEvent);
			EvtClose(hContext);
			return;
		}
	}

	wprintf(L"------------------------------------\n");
	if(pValues[EvtSystemTimeCreated].FileTimeVal){
		ULONGLONG TimeOffset = pValues[EvtSystemTimeCreated].FileTimeVal + (60.0 * 60.0 * 9.0 * 10000000ULL) + 10000000ULL;

		SYSTEMTIME	st;
		FILETIME	ft;

		ft.dwLowDateTime = (DWORD)TimeOffset;
		ft.dwHighDateTime = (DWORD)(TimeOffset >> 32);
		FileTimeToSystemTime(&ft, &st);

		wprintf(L"TimeCreated:	%04d-%02d-%02d %02d:%02d:%02d\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	}else{
		wprintf(L"TimeCreated: (null)\n");
	}

	wchar_t *pBuffer[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
	wprintf(L"EventRecordID: %I64u\n", pValues[EvtSystemEventRecordId].UInt64Val);
	wprintf(L"Provider Name: %ls\n", pValues[EvtSystemProviderName].StringVal);
	wprintf(L"Channel: %ls\n", (EvtVarTypeNull == pValues[EvtSystemChannel].Type) ? L"" : pValues[EvtSystemChannel].StringVal);
	wprintf(L"Version: %u\n", (EvtVarTypeNull == pValues[EvtSystemVersion].Type) ? 0 : pValues[EvtSystemVersion].ByteVal);
	if(pValues[EvtSystemOpcode].Type != EvtVarTypeNull){
		pBuffer[0] = GetDescriptionMessage(pValues[EvtSystemProviderName].StringVal, hEvent, EvtFormatMessageOpcode);
	}
	if(pValues[EvtSystemLevel].Type != EvtVarTypeNull){
		pBuffer[1] = GetDescriptionMessage(pValues[EvtSystemProviderName].StringVal, hEvent, EvtFormatMessageLevel);
	}
	if(pValues[EvtSystemEventID].Type != EvtVarTypeNull){
		pBuffer[2] = GetDescriptionMessage(pValues[EvtSystemProviderName].StringVal, hEvent, EvtFormatMessageEvent);
	}
	if(pValues[EvtSystemTask].Type != EvtVarTypeNull){
		pBuffer[3] = GetDescriptionMessage(pValues[EvtSystemProviderName].StringVal, hEvent, EvtFormatMessageTask);
	}
	if(pValues[EvtSystemKeywords].Type != EvtVarTypeNull){
		pBuffer[4] = GetDescriptionMessage(pValues[EvtSystemProviderName].StringVal, hEvent, EvtFormatMessageKeyword);
	}
	wprintf(L"Opcode: %ls\n", pBuffer[0]);
	wprintf(L"Level: %ls\n", pBuffer[1]);
	wprintf(L"EventID: %ls\n", pBuffer[2]);
	wprintf(L"Task: %ls\n", pBuffer[3]);
	wprintf(L"Keywords: %ls\n", pBuffer[4]);
	// wprintf(L"Opcode:				%u\n", (EvtVarTypeNull == pValues[EvtSystemOpcode].Type) ? 0 : pValues[EvtSystemOpcode].ByteVal);
	// wprintf(L"Level:				%u\n", (EvtVarTypeNull == pValues[EvtSystemLevel].Type) ? 0 : pValues[EvtSystemLevel].ByteVal);
	// wprintf(L"EventID:				%lu\n", pValues[EvtSystemEventID].UInt16Val);
	// wprintf(L"Task:					%hu\n", (EvtVarTypeNull == pValues[EvtSystemTask].Type) ? 0 : pValues[EvtSystemTask].UInt16Val);
	// wprintf(L"Keywords:				0x%I64x\n", pValues[EvtSystemKeywords].UInt64Val);
	wprintf(L"Execution ThreadID: %lu\n", pValues[EvtSystemThreadID].UInt32Val);
	wprintf(L"Execution ProcessID: %lu\n", pValues[EvtSystemProcessID].UInt32Val);
	wprintf(L"------------------------------------\n");

	for(int i=0; i<sizeof(pBuffer)/sizeof(pBuffer[0]); i++){
		if(pBuffer[i] != NULL){
			free(pBuffer[i]);
		}
	}
	if(pValues)		{ free(pValues); }
	if(hEvent)		{ EvtClose(hEvent); }
	if(hContext)	{ EvtClose(hContext); }
}

wchar_t *GetDescriptionMessage(const wchar_t *ProviderName, EVT_HANDLE hEvent, EVT_FORMAT_MESSAGE_FLAGS Format){
	DWORD dwBufferSize = 0,
		  dwBufferUsed = 0;
	wchar_t* pBuffer = NULL;

	EVT_HANDLE hMetaData = EvtOpenPublisherMetadata(NULL, ProviderName, NULL, 0, 0);
	if(hMetaData == NULL){ return pBuffer; }

	EvtFormatMessage(hMetaData, hEvent, 0, 0, NULL, Format, 0, NULL, &dwBufferSize);
	if(dwBufferSize == 0){ return pBuffer; }

	pBuffer = (wchar_t*)malloc(sizeof(wchar_t) * dwBufferSize);
	if(pBuffer == NULL){ return pBuffer; }

	if(!EvtFormatMessage(hMetaData, hEvent, 0, 0, NULL, Format, dwBufferSize, pBuffer, &dwBufferUsed)) {
		free(pBuffer);
		return pBuffer;
	}

	if(Format == EvtFormatMessageEvent){
		wchar_t *firstLine = wcstok(pBuffer, L"\r\n");
		wcscpy(pBuffer, firstLine);
	}

	if(hMetaData) { EvtClose(hMetaData); }
	return pBuffer;
}

/*
void PrintEvtVariantType(EVT_VARIANT variant, const wchar_t *PropertyName){
	wprintf(L"%ls: ", PropertyName);

	switch (variant.Type) {
		case EvtVarTypeNull:
			wprintf(L"Null\n");
			break;
		case EvtVarTypeString:
			wprintf(L"String (값: %ls)\n", variant.StringVal);
			break;
		case EvtVarTypeAnsiString:
			wprintf(L"Ansi String (값: %S)\n", variant.AnsiStringVal);
			break;
		case EvtVarTypeSByte:
			wprintf(L"SByte (값: %d)\n", variant.SByteVal);
			break;
		case EvtVarTypeByte:
			wprintf(L"Byte (값: %u)\n", variant.ByteVal);
			break;
		case EvtVarTypeInt16:
			wprintf(L"Int16 (값: %d)\n", variant.Int16Val);
			break;
		case EvtVarTypeUInt16:
			wprintf(L"UInt16 (값: %u)\n", variant.UInt16Val);
			break;
		case EvtVarTypeInt32:
			wprintf(L"Int32 (값: %d)\n", variant.Int32Val);
			break;
		case EvtVarTypeUInt32:
			wprintf(L"UInt32 (값: %u)\n", variant.UInt32Val);
			break;
		case EvtVarTypeInt64:
			wprintf(L"Int64 (값: %lld)\n", variant.Int64Val);
			break;
		case EvtVarTypeUInt64:
			wprintf(L"UInt64 (값: %llu)\n", variant.UInt64Val);
			break;
		case EvtVarTypeBoolean:
			wprintf(L"Boolean (값: %s)\n", variant.BooleanVal ? L"TRUE" : L"FALSE");
			break;
		case EvtVarTypeSingle:
			wprintf(L"Single (값: %f)\n", variant.SingleVal);
			break;
		case EvtVarTypeDouble:
			wprintf(L"Double (값: %lf)\n", variant.DoubleVal);
			break;
		case EvtVarTypeBinary:
			wprintf(L"Binary\n");
			break;
		case EvtVarTypeSizeT:
			wprintf(L"SizeT (값: %zu)\n", variant.SizeTVal);
			break;
		case EvtVarTypeFileTime:
			wprintf(L"FileTime\n");
			break;
		case EvtVarTypeSysTime:
			wprintf(L"SystemTime\n");
			break;
		case EvtVarTypeSid:
			wprintf(L"SID\n");
			break;
		case EvtVarTypeHexInt32:
			wprintf(L"HexInt32 (값: 0x%X)\n", variant.UInt32Val);
			break;
		case EvtVarTypeHexInt64:
			wprintf(L"HexInt64 (값: 0x%llX)\n", variant.UInt64Val);
			break;
		default:
			wprintf(L"알 수 없는 타입 (%d)\n", variant.Type);
			break;
	}
}
*/

DWORD WINAPI SystemCallback(EVT_SUBSCRIBE_NOTIFY_ACTION Action, void* UserContext, void* hEvent){
	switch(Action){
		case EvtSubscribeActionDeliver:
			PrintEvent(hEvent);
			break;

		case EvtSubscribeActionError:
			wprintf(L"Subscribe error: %lu\n", (ULONG)(ULONG_PTR)UserContext);
			break;

		default:
			break;
	}

	return ERROR_SUCCESS;
}

DWORD WINAPI ApplicationCallback(EVT_SUBSCRIBE_NOTIFY_ACTION Action, void* UserContext, void* hEvent){
	switch(Action){
		case EvtSubscribeActionDeliver:
			PrintEvent(hEvent);
			break;

		case EvtSubscribeActionError:
			wprintf(L"Subscribe error: %lu\n", (ULONG)(ULONG_PTR)UserContext);
			break;

		default:
			break;
	}

	return ERROR_SUCCESS;
}

DWORD WINAPI SecurityCallback(EVT_SUBSCRIBE_NOTIFY_ACTION Action, void* UserContext, void* hEvent){
	switch(Action){
		case EvtSubscribeActionDeliver:
			PrintEvent(hEvent);
			break;

		case EvtSubscribeActionError:
			wprintf(L"Subscribe error: %lu\n", (ULONG)(ULONG_PTR)UserContext);
			break;

		default:
			break;
	}

	return ERROR_SUCCESS;
}

