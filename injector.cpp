#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>

static void PrintUsage( LPCSTR lpszExeName )
{
	printf( "Usage: %s <.exe> <.dll>\n", lpszExeName );
}

int main( int argc, char** argv )
{
	char szDLLName[ MAX_PATH ];
	size_t nDLLNameSize;
	SIZE_T nBytesWritten;
	HANDLE hProcess;
	DWORD th32ProcessId;
	HANDLE hSnapshot;
	HMODULE hKernel32;
	PROCESSENTRY32 ProcessEntry;
	LPVOID lpvLoadLibrary;
	LPVOID lpvRemotePath;

	if ( argc < 3 )
	{
		PrintUsage( argv[ 0 ] );

		return 1;
	}

	GetFullPathName( argv[ 2 ], sizeof( szDLLName ), szDLLName, NULL );

	if ( GetFileAttributes( szDLLName ) == -1 )
	{
		printf( "The specified file '%s' does not exist.\n", argv[ 2 ] );

		return 1;
	}

	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

	if ( hSnapshot == INVALID_HANDLE_VALUE )
	{
		printf( "Could not capture process snapshot.\n" );

		return 1;
	}

	th32ProcessId = 0;

	if ( Process32First( hSnapshot, &ProcessEntry ) )
	{
		do
		{
			if ( !strcmp( ProcessEntry.szExeFile, argv[ 1 ] ) )
			{
				printf( "Located %s process.\n", argv[ 1 ] );
				th32ProcessId = ProcessEntry.th32ProcessID;

				break;
			}
		} while ( Process32Next( hSnapshot, &ProcessEntry ) );
	}
	else
	{
		printf( "Could not capture process snapshot. 2\n" );

		return 1;
	}

	CloseHandle( hSnapshot );

	if ( !th32ProcessId )
	{
		printf( "Could not find '%s' process.\n", argv[ 1 ] );

		return 1;
	}

	hKernel32 = GetModuleHandle( "kernel32.dll" );

	if ( hKernel32 == INVALID_HANDLE_VALUE || !hKernel32 )
	{
		printf( "Could not get the handle to kernel32.\n" );

		return 1;
	}

	lpvLoadLibrary = GetProcAddress( hKernel32, "LoadLibraryA" );

	if ( !lpvLoadLibrary )
	{
		printf( "Could not get the address of LoadLibraryA from kernel32.\n" );

		return 1;
	}

	printf( "Injecting %s into %s...\n", argv[ 2 ], argv[ 1 ] );

	hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, th32ProcessId );

	if ( hProcess == INVALID_HANDLE_VALUE )
	{
		printf( "Could not open handle to process.\n" );

		return 1;
	}

	nDLLNameSize = strlen( szDLLName ) + 1;
	lpvRemotePath = VirtualAllocEx( hProcess, NULL, nDLLNameSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );

	if ( !lpvRemotePath )
	{
		printf( "Could not allocate string of size %d to process.\n", nDLLNameSize );

		CloseHandle( hProcess );

		return 1;
	}

	if ( !WriteProcessMemory( hProcess, lpvRemotePath, szDLLName, nDLLNameSize, &nBytesWritten ) )
	{
		printf( "Could not write to memory.\n" );

		CloseHandle( hProcess );

		return 1;
	}

	CreateRemoteThread( hProcess, NULL, 0, ( LPTHREAD_START_ROUTINE )lpvLoadLibrary, lpvRemotePath, 0, NULL );
	CloseHandle( hProcess );

	printf( "Done.\n" );
}
