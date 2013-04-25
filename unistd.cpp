#ifdef _WIN32
#include <Windows.h>
#include <string>

#include <cstdlib>

using namespace std;

int rmdir(const char *dir) {
  if (RemoveDirectory(dir) == TRUE) return 0;
  else return -1;
}

int mkdir(const char *dir, unsigned short mode) {
  if (CreateDirectory(dir, NULL)) return 0;
  else return -1;
}

int unlinkW(const char *file) {
  if (DeleteFile(file) == TRUE) return 0;
  else return -1;
}

char *strdupW(const char *str) {
  int sz = strlen(str);
  char *toReturn = new char[sz + 1];
  toReturn[sz] = 0;
  for (int i = 0; i < sz; i++) toReturn[i] = str[i];
  return toReturn;
}

int execvW(const char *file, const char *const *argv) {

  // STARTUPINFO si;
  // PROCESS_INFORMATION pi;

  // ZeroMemory( &si, sizeof(si) );
  // si.cb = sizeof(si);
  // ZeroMemory( &pi, sizeof(pi) );

  // string args = "";
  // for (int at = 1; argv[at] != 0; at++) {
  //   if (at != 1) args += " ";
  //   args += argv[at];
  // }

  // char *cmdline = new char[args.size() + 1];
  // strcpy(cmdline, args.c_str());


  // // Start the child process. 
  // if( !CreateProcess( argv[0],   // No module name (use command line)
  // 		      cmdline,        // Command line
  // 		      NULL,           // Process handle not inheritable
  // 		      NULL,           // Thread handle not inheritable
  // 		      FALSE,          // Set handle inheritance to FALSE
  // 		      0,              // No creation flags
  // 		      NULL,           // Use parent's environment block
  // 		      NULL,           // Use parent's starting directory 
  // 		      &si,            // Pointer to STARTUPINFO structure
  // 		      &pi )           // Pointer to PROCESS_INFORMATION structure
  //     ) 
  //   {
  //     return -1;
  //   }

  // for (int at = 0; argv[at] != 0; at++) {
  //   delete []argv[at];
  // }
  // delete []argv;

  // // Wait until child process exits.
  // WaitForSingleObject( pi.hProcess, INFINITE );

  // // Close process and thread handles. 
  // CloseHandle( pi.hProcess );
  // CloseHandle( pi.hThread );

  // delete []cmdline;

  string args = "";
  for (int at = 0; argv[at] != 0; at++) {
    if (at != 0) args += " ";
    args += argv[at];
  }

  system(("\"" + args + "\"").c_str());

  return 0;
}

#endif
