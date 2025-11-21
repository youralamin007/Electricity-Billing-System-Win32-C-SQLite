#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>

#define UNIT_RATE 5  // প্রতি unit এর দাম

enum {
    IDC_NAME = 101,
    IDC_METER,
    IDC_UNITS,
    IDC_TAKA,
    IDC_SAVE,
    IDC_VIEW,
    IDC_DELETE,
    IDC_DELETE_ALL,
    IDC_STATUS
};

sqlite3 *db = NULL;
HWND hListBox = NULL; // ListBox handle

// Save to DB
void save_to_db(const wchar_t *name, const wchar_t *meter, double units, double amount) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO bills(name,meter,units,amount) VALUES(?1,?2,?3,?4);";
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK){
        MessageBox(NULL, L"DB Prepare Failed!", L"Error", MB_ICONERROR);
        return;
    }
    sqlite3_bind_text16(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 2, meter, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, units);
    sqlite3_bind_double(stmt, 4, amount);
    if(sqlite3_step(stmt) != SQLITE_DONE){
        MessageBox(NULL, L"DB Insert Failed!", L"Error", MB_ICONERROR);
    } else {
        MessageBox(NULL, L"Saved Successfully!", L"Success", MB_OK);
    }
    sqlite3_finalize(stmt);
}

// Save button
void do_save(HWND hwnd){
    wchar_t name[50], meter[50], units_str[20], taka_str[20];
    GetWindowText(GetDlgItem(hwnd, IDC_NAME), name, 50);
    GetWindowText(GetDlgItem(hwnd, IDC_METER), meter, 50);
    GetWindowText(GetDlgItem(hwnd, IDC_UNITS), units_str, 20);
    GetWindowText(GetDlgItem(hwnd, IDC_TAKA), taka_str, 20);

    if(wcslen(name) == 0 || wcslen(meter) == 0 || wcslen(units_str) == 0){
        SetWindowText(GetDlgItem(hwnd, IDC_STATUS), L"Fill all fields!");
        return;
    }

    double units = wcstod(units_str, NULL);
    double taka  = wcstod(taka_str, NULL);
    save_to_db(name, meter, units, taka);
    SetWindowText(GetDlgItem(hwnd, IDC_STATUS), L"Data saved!");
}

// Auto-calculate Taka
void update_taka(HWND hwnd){
    wchar_t units_str[20];
    GetWindowText(GetDlgItem(hwnd, IDC_UNITS), units_str, 20);
    if(wcslen(units_str) > 0){
        double units = wcstod(units_str, NULL);
        double taka = units * UNIT_RATE;
        wchar_t taka_str[20];
        swprintf(taka_str, 20, L"%.2f", taka);   // FIXED
        SetWindowText(GetDlgItem(hwnd, IDC_TAKA), taka_str);
    } else {
        SetWindowText(GetDlgItem(hwnd, IDC_TAKA), L"");
    }
}

// View Records
void do_view(HWND hwnd){
    if(hListBox == NULL){
        hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
            10, 180, 500, 250, hwnd, NULL, NULL, NULL);

        HFONT hFont = CreateFont(
            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH|FF_MODERN, L"Courier New"
        );
        SendMessage(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    const char *sql = "SELECT meter,name,units,amount,date FROM bills ORDER BY id;";
    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK){
        MessageBox(hwnd, L"Failed to fetch records!", L"Error", MB_ICONERROR);
        return;
    }

    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"Meter       Name                   Units    Taka      Date");

    while(sqlite3_step(stmt) == SQLITE_ROW){
        const wchar_t *meter = (const wchar_t*)sqlite3_column_text16(stmt, 0);
        const wchar_t *name  = (const wchar_t*)sqlite3_column_text16(stmt, 1);
        double units = sqlite3_column_double(stmt, 2);
        double amount = sqlite3_column_double(stmt, 3);
        const wchar_t *date = (const wchar_t*)sqlite3_column_text16(stmt, 4);

        wchar_t buffer[256];
        swprintf(buffer, 256, L"%-10ls  %-20ls %-7.2f %-8.2f %ls",
                 meter, name, units, amount, date);   // FIXED
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)buffer);
    }
    sqlite3_finalize(stmt);
}

// Delete selected row
void do_delete(HWND hwnd){
    if(hListBox == NULL) return;

    int sel = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
    if(sel == LB_ERR){
        MessageBox(hwnd, L"Select a record to delete!", L"Info", MB_OK);
        return;
    }

    wchar_t buffer[256];
    SendMessage(hListBox, LB_GETTEXT, sel, (LPARAM)buffer);

    wchar_t meter[50];
    wcsncpy(meter, buffer, 10);
    meter[10] = L'\0';
    for(int i=9;i>=0;i--){
        if(meter[i] == L' ') meter[i] = L'\0';
        else break;
    }

    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM bills WHERE meter=?1;";
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return;
    sqlite3_bind_text16(stmt, 1, meter, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    SendMessage(hListBox, LB_DELETESTRING, sel, 0);
    MessageBox(hwnd, L"Deleted!", L"Success", MB_OK);
}

// Delete All
void do_delete_all(HWND hwnd){
    if(MessageBox(hwnd, L"Delete ALL records?", L"Confirm",
       MB_YESNO | MB_ICONWARNING) != IDYES) return;

    sqlite3_exec(db, "DELETE FROM bills;", 0, 0, 0);

    if(hListBox)
        SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    MessageBox(hwnd, L"All records deleted!", L"Success", MB_OK);
}

// UI
void create_ui(HWND hwnd){
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    CreateWindow(L"STATIC", L"Name:", WS_VISIBLE|WS_CHILD, 10,10,80,20, hwnd, NULL, NULL, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE, 100,10,200,22, hwnd, (HMENU)IDC_NAME, NULL, NULL);

    CreateWindow(L"STATIC", L"Meter No:", WS_VISIBLE|WS_CHILD, 10,40,80,20, hwnd, NULL, NULL, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE, 100,40,200,22, hwnd, (HMENU)IDC_METER, NULL, NULL);

    CreateWindow(L"STATIC", L"Units:", WS_VISIBLE|WS_CHILD, 10,70,80,20, hwnd, NULL, NULL, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE, 100,70,200,22, hwnd, (HMENU)IDC_UNITS, NULL, NULL);

    CreateWindow(L"STATIC", L"Taka:", WS_VISIBLE|WS_CHILD, 10,100,80,20, hwnd, NULL, NULL, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_READONLY, 100,100,200,22, hwnd, (HMENU)IDC_TAKA, NULL, NULL);

    CreateWindow(L"BUTTON", L"Save", WS_CHILD|WS_VISIBLE, 10,140,80,30, hwnd, (HMENU)IDC_SAVE, NULL, NULL);
    CreateWindow(L"BUTTON", L"View Records", WS_CHILD|WS_VISIBLE, 100,140,120,30, hwnd, (HMENU)IDC_VIEW, NULL, NULL);
    CreateWindow(L"BUTTON", L"Delete by Meter", WS_CHILD|WS_VISIBLE, 230,140,120,30, hwnd, (HMENU)IDC_DELETE, NULL, NULL);
    CreateWindow(L"BUTTON", L"Delete All", WS_CHILD|WS_VISIBLE, 10,440,120,30, hwnd, (HMENU)IDC_DELETE_ALL, NULL, NULL);

    CreateWindow(L"STATIC", L"", WS_CHILD|WS_VISIBLE, 10,480,500,22, hwnd, (HMENU)IDC_STATUS, NULL, NULL);

    for(int id=IDC_NAME; id<=IDC_STATUS; id++){
        HWND h = GetDlgItem(hwnd,id);
        if(h) SendMessage(h, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_CREATE: create_ui(hwnd); break;

        case WM_COMMAND:
            switch(LOWORD(wParam)){
                case IDC_SAVE: do_save(hwnd); break;
                case IDC_VIEW: do_view(hwnd); break;
                case IDC_DELETE: do_delete(hwnd); break;
                case IDC_DELETE_ALL: do_delete_all(hwnd); break;
            }

            if(HIWORD(wParam)==EN_CHANGE && LOWORD(wParam)==IDC_UNITS){
                update_taka(hwnd);
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Main
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nShow){
    if(sqlite3_open("billing.db", &db) != SQLITE_OK){
        MessageBox(NULL, L"Cannot open database!", L"Error", MB_ICONERROR);
        return 1;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS bills("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT,"
        "meter TEXT,"
        "units REAL,"
        "amount REAL,"
        "date TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";

    char *err;
    sqlite3_exec(db, sql, 0, 0, &err);
    if(err) sqlite3_free(err);

    const wchar_t CLASS_NAME[] = L"BillingApp";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Electricity Billing Management",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 540,
        NULL, NULL, hInst, NULL
    );

    if(!hwnd) return 0;

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    sqlite3_close(db);
    return 0;
}
