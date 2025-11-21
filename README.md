## How to Compile & Run

Open a terminal (Command Prompt or Git Bash) and run:

```bash
gcc -o billing.exe billing_form_win32_sqlite.c sqlite3.c -mwindows -lcomctl32 -lgdi32 -luser32 -lws2_32
./billing.exe
