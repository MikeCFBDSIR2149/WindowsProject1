#include "framework.h"
#include "WindowsProject1.h"
#include <vector>
#include <algorithm>
#include <ctime>

#define MAX_LOADSTRING 100
#define IDT_TIMER1 1
#define PLAYER_SPEED 5
#define BULLET_SPEED 10
#define ENEMY_SPEED 2
#define ENEMY_COUNT 5

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

HBITMAP hbmPlayer, hbmEnemy, hbmBullet;
RECT clientRect;

int score = 0;
bool gameOver = false;

struct GameObject {
    int x, y;
    int width, height;
};

GameObject player;
std::vector<GameObject> bullets;
std::vector<GameObject> enemies;

// 前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void UpdateGame();
void DrawGame(HDC hdc);
void FireBullet();
void SpawnEnemies();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

    MSG msg;
    HWND hWnd = FindWindow(szWindowClass, szTitle); // 获取窗口句柄
    if (!hWnd)
    {
        return FALSE;
    }

    SpawnEnemies();
    SetTimer(hWnd, IDT_TIMER1, 50, (TIMERPROC)nullptr); // 使用窗口句柄设置定时器

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    KillTimer(hWnd, IDT_TIMER1); // 使用窗口句柄关闭定时器
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    hbmPlayer = (HBITMAP)LoadImage(hInstance, L"player.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hbmPlayer) {
        MessageBox(hWnd, L"Failed to load player.bmp", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    hbmEnemy = (HBITMAP)LoadImage(hInstance, L"enemy.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hbmEnemy) {
        MessageBox(hWnd, L"Failed to load enemy.bmp", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    hbmBullet = (HBITMAP)LoadImage(hInstance, L"bullet.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hbmBullet) {
        MessageBox(hWnd, L"Failed to load bullet.bmp", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    GetClientRect(hWnd, &clientRect);
    player = { clientRect.right / 2 - 20, clientRect.bottom - 50, 40, 40 };

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        DrawGame(hdc);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_LEFT:
            player.x -= PLAYER_SPEED;
            if (player.x < 0) player.x = 0;
            break;
        case VK_RIGHT:
            player.x += PLAYER_SPEED;
            if (player.x + player.width > clientRect.right) player.x = clientRect.right - player.width;
            break;
        case VK_SPACE:
            FireBullet();
            break;
        }
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    case WM_TIMER:
        if (wParam == IDT_TIMER1) {
            UpdateGame();
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void UpdateGame()
{
    for (auto& bullet : bullets) {
        bullet.y -= BULLET_SPEED;
    }
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](GameObject& b) { return b.y < 0; }), bullets.end());

    for (auto& enemy : enemies) {
        enemy.y += ENEMY_SPEED;
    }
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](GameObject& e) { return e.y > clientRect.bottom; }), enemies.end());

    // Check for collisions
    for (auto it = bullets.begin(); it != bullets.end(); ) {
        bool hit = false;
        for (auto et = enemies.begin(); et != enemies.end(); ) {
            if (it->x < et->x + et->width && it->x + it->width > et->x &&
                it->y < et->y + et->height && it->y + it->height > et->y) {
                et = enemies.erase(et);
                hit = true;
                score += 1; // 增加分数
                break;
            }
            else {
                ++et;
            }
        }
        if (hit) {
            it = bullets.erase(it);
        }
        else {
            ++it;
        }
    }

    // 检查敌人是否碰到玩家
    for (auto& enemy : enemies) {
        if (enemy.x < player.x + player.width && enemy.x + enemy.width > player.x &&
            enemy.y < player.y + player.height && enemy.y + enemy.height > player.y) {
            gameOver = true;
            return;
        }
    }

    if (enemies.size() < ENEMY_COUNT) {
        SpawnEnemies();
    }
}


void DrawGame(HDC hdc)
{
    if (gameOver) {
        // 绘制游戏结束画面
        const wchar_t* gameOverText = L"Game Over";
        SetTextColor(hdc, RGB(255, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        RECT rect;
        GetClientRect(GetForegroundWindow(), &rect);
        DrawText(hdc, gameOverText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    HDC hdcMem = CreateCompatibleDC(hdc);
    HGDIOBJ oldBitmap;

    oldBitmap = SelectObject(hdcMem, hbmPlayer);
    BitBlt(hdc, player.x, player.y, player.width, player.height, hdcMem, 0, 0, SRCCOPY);

    for (auto& bullet : bullets) {
        SelectObject(hdcMem, hbmBullet);
        BitBlt(hdc, bullet.x, bullet.y, bullet.width, bullet.height, hdcMem, 0, 0, SRCCOPY);
    }

    for (auto& enemy : enemies) {
        SelectObject(hdcMem, hbmEnemy);
        BitBlt(hdc, enemy.x, enemy.y, enemy.width, enemy.height, hdcMem, 0, 0, SRCCOPY);
    }

    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);

    // 绘制分数
    wchar_t scoreText[20];
    wsprintf(scoreText, L"Score: %d", score);
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, 10, 10, scoreText, wcslen(scoreText));
}


void FireBullet()
{
    GameObject bullet = { player.x + player.width / 2 - 5, player.y - 10, 10, 20 };
    bullets.push_back(bullet);
}

void SpawnEnemies()
{
    while (enemies.size() < ENEMY_COUNT) {
        GameObject enemy = { rand() % (clientRect.right - 40), -50, 40, 40 };
        enemies.push_back(enemy);
    }
}
