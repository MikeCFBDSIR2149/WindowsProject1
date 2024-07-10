#include "framework.h"
#include "WindowsProject1.h"
#include <stdlib.h>
#include <string.h>
#include <windowsx.h>

#define MAX_LOADSTRING 100
#define IDT_TIMER1 1
#define PLAYER_SPEED 10
#define BULLET_SPEED 20
// #define ENEMY_SPEED 8
#define ENEMY_COUNT 6
#define MAX_BULLETS 100
#define MAX_ENEMIES 80
#define MAX_ENEMY_BULLETS 500
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// 全局变量:
HINSTANCE hInst;                                // 当前实例
HWND hButtonRestart;                            // 重新开始按钮句柄
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

HBITMAP hbmPlayer, hbmEnemy, hbmBullet, hbmEnemyBullet;
RECT clientRect;
int wndWidth = 0;	int wndHeight = 0;  // 窗口尺寸

int score = 0;
int gameOver = 0;

typedef struct {
    int x, y;
    int width, height;
    int speed;
    int fireTimer;
} GameObject;

GameObject player;
GameObject bullets[MAX_BULLETS];
GameObject enemies[MAX_ENEMIES];
int bullet_count = 0;
int enemy_count = 0;

typedef struct {
    int x, y;
    int width, height;
    int speed;
} Bullet;

Bullet enemyBullets[MAX_ENEMY_BULLETS];
int enemyBullet_count = 0;

// 前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void UpdateGame();
void DrawGame(HDC hdc);
void FireBullet();
void SpawnEnemies();
void FireEnemyBullet(GameObject enemy);

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
        CW_USEDEFAULT, 0, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, hInstance, nullptr);

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

    hbmEnemyBullet = (HBITMAP)LoadImage(hInstance, L"bulletenemy.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hbmEnemyBullet) {
        MessageBox(hWnd, L"Failed to load bulletenemy.bmp", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    GetClientRect(hWnd, &clientRect);
    player.x = clientRect.right / 2 - 20;
    player.y = clientRect.bottom - 50;
    player.width = 40;
    player.height = 40;

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
        case 2: // 重新开始按钮标识符
            // 重置游戏
            gameOver = 0;
            score = 0;
            bullet_count = 0;
            enemy_count = 0;
            player.x = clientRect.right / 2 - 20; // 重置玩家位置
            player.y = clientRect.bottom - 50;
            SpawnEnemies();
            InvalidateRect(hWnd, NULL, TRUE);
            break;
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
    case WM_ERASEBKGND:		// 不擦除背景,避免闪烁
    {
        break;
	}
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 以下的步骤是为了避免产生屏幕闪烁,将画面绘制到内存中,一次性拷贝到屏幕上
        HDC memHDC = CreateCompatibleDC(hdc);
        RECT rectClient;
        GetClientRect(hWnd, &rectClient);
        HBITMAP bmpBuff = CreateCompatibleBitmap(hdc, wndWidth, wndHeight);
        HBITMAP pOldBMP = (HBITMAP)SelectObject(memHDC, bmpBuff);
        DrawGame(memHDC);

        //拷贝内存HDC内容到实际HDC
        BOOL tt = BitBlt(hdc, rectClient.left, rectClient.top, wndWidth,
            wndHeight, memHDC, rectClient.left, rectClient.top, SRCCOPY);

        //内存回收
        SelectObject(memHDC, pOldBMP);
        DeleteObject(bmpBuff);
        DeleteDC(memHDC);

        EndPaint(hWnd, &ps);
    }
    break;
    case WM_KEYDOWN:
        switch (wParam)
        {
		/*
        case VK_LEFT:
            player.x -= PLAYER_SPEED;
            if (player.x < 0) player.x = 0;
            break;
        case VK_RIGHT:
            player.x += PLAYER_SPEED;
            if (player.x + player.width > clientRect.right) player.x = clientRect.right - player.width;
            break;
        */
        case VK_SPACE:
            FireBullet();
            break;
        }
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    case WM_MOUSEMOVE:
    {
        int xPos = GET_X_LPARAM(lParam); // 获取鼠标的x坐标
        player.x = xPos - player.width / 2; // 更新玩家的x坐标
        if (player.x < 0) player.x = 0; // 限制玩家的x坐标不超出左边界
        if (player.x + player.width > clientRect.right) player.x = clientRect.right - player.width; // 限制玩家的x坐标不超出右边界
        InvalidateRect(hWnd, NULL, TRUE); // 重新绘制窗口
    }
    break;
    case WM_TIMER:
        if (wParam == IDT_TIMER1 && !gameOver) {
            UpdateGame();
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    case WM_SIZE:
	    {
            wndWidth = LOWORD(lParam);
            wndHeight = HIWORD(lParam);
            break;
	    }
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
    int i, j;
    for (i = 0; i < bullet_count; ++i) {
        bullets[i].y -= BULLET_SPEED;
    }

    // 移除离开屏幕的子弹
    for (i = 0; i < bullet_count; ) {
        if (bullets[i].y < 0) {
            bullets[i] = bullets[--bullet_count];
        }
        else {
            ++i;
        }
    }

    for (i = 0; i < enemy_count; ++i) {
        enemies[i].y += enemies[i].speed;
    }

    // 移除离开屏幕的敌人
    for (i = 0; i < enemy_count; ) {
        if (enemies[i].y > clientRect.bottom) {
            enemies[i] = enemies[--enemy_count];
        }
        else {
            ++i;
        }
    }

    // 检查子弹和敌人碰撞
    for (i = 0; i < bullet_count; ) {
        int hit = 0;
        for (j = 0; j < enemy_count; ) {
            if (bullets[i].x < enemies[j].x + enemies[j].width && bullets[i].x + bullets[i].width > enemies[j].x &&
                bullets[i].y < enemies[j].y + enemies[j].height && bullets[i].y + bullets[i].height > enemies[j].y) {
                enemies[j] = enemies[--enemy_count];
                hit = 1;
                score += 1; // 增加分数
                break;
            }
            else {
                ++j;
            }
        }
        if (hit) {
            bullets[i] = bullets[--bullet_count];
        }
        else {
            ++i;
        }
    }

    // 检查敌人是否碰到玩家
    for (i = 0; i < enemy_count; ++i) {
        if (enemies[i].x < player.x + player.width && enemies[i].x + enemies[i].width > player.x &&
            enemies[i].y < player.y + player.height && enemies[i].y + enemies[i].height > player.y) {
            gameOver = 1;
            return;
        }
    }

    if (enemy_count < ENEMY_COUNT) {
        SpawnEnemies();
    }

    // 更新敌人子弹位置
    for (int i = 0; i < enemyBullet_count; ++i) {
        enemyBullets[i].y += enemyBullets[i].speed;
    }

    // 移除离开屏幕的敌人子弹
    for (int i = 0; i < enemyBullet_count; ) {
        if (enemyBullets[i].y > clientRect.bottom) {
            enemyBullets[i] = enemyBullets[--enemyBullet_count];
        }
        else {
            ++i;
        }
    }

    // 检查敌人子弹和玩家碰撞
    for (int i = 0; i < enemyBullet_count; ) {
        if (enemyBullets[i].x < player.x + player.width && enemyBullets[i].x + enemyBullets[i].width > player.x &&
            enemyBullets[i].y < player.y + player.height && enemyBullets[i].y + enemyBullets[i].height > player.y) {
            gameOver = 1;
            return;
        }
        else {
            ++i;
        }
    }

    // 敌人发射子弹逻辑
    for (i = 0; i < enemy_count; ++i) {
        enemies[i].fireTimer--;
        if (enemies[i].fireTimer <= 0) {
            FireEnemyBullet(enemies[i]);
            enemies[i].fireTimer = rand() % 20 + 10; // 重置计时器
        }
    }

    if (enemy_count < ENEMY_COUNT) {
        SpawnEnemies();
    }
}

void DrawGame(HDC hdc)
{
    if (gameOver) {
        // 绘制游戏结束画面
        const wchar_t* gameOverText = L"游戏结束";
        SetTextColor(hdc, RGB(255, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        RECT rect;
        GetClientRect(GetForegroundWindow(), &rect);
        rect.top -= 50;
        DrawText(hdc, gameOverText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    	// 绘制得分
        wchar_t scoreText[50];
        wsprintf(scoreText, L"本局分数: %d", score);
        rect.top += 50;
        DrawText(hdc, scoreText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 显示重新开始按钮
        if (!hButtonRestart) {
            hButtonRestart = CreateWindow(
                L"BUTTON",
                L"重新开始",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                rect.right / 2 - 60, // x 坐标
                rect.bottom / 2 + 50, // y 坐标
                120,        // 宽度
                30,         // 高度
                GetForegroundWindow(),
                (HMENU)2,
                hInst,
                NULL);
        }

    	return;
    }

    // 如果游戏没有结束且按钮存在，销毁按钮
    if (hButtonRestart) {
        DestroyWindow(hButtonRestart);
        hButtonRestart = NULL;
    }

    HDC hdcMem = CreateCompatibleDC(hdc);
    HGDIOBJ oldBitmap;

    oldBitmap = SelectObject(hdcMem, hbmPlayer);
    BitBlt(hdc, player.x, player.y, player.width, player.height, hdcMem, 0, 0, SRCCOPY);

    int i;
    for (i = 0; i < bullet_count; ++i) {
        SelectObject(hdcMem, hbmBullet);
        BitBlt(hdc, bullets[i].x, bullets[i].y, bullets[i].width, bullets[i].height, hdcMem, 0, 0, SRCCOPY);
    }

    for (i = 0; i < enemy_count; ++i) {
        SelectObject(hdcMem, hbmEnemy);
        BitBlt(hdc, enemies[i].x, enemies[i].y, enemies[i].width, enemies[i].height, hdcMem, 0, 0, SRCCOPY);
    }

    for (i = 0; i < enemyBullet_count; ++i) {
        SelectObject(hdcMem, hbmEnemyBullet);
        BitBlt(hdc, enemyBullets[i].x, enemyBullets[i].y, enemyBullets[i].width, enemyBullets[i].height, hdcMem, 0, 0, SRCCOPY);
    }

    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);

    // 绘制分数
    wchar_t scoreText[20];
    wsprintf(scoreText, L"分数: %d", score);
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, 10, 10, scoreText, wcslen(scoreText));
}

void FireBullet()
{
    if (bullet_count < MAX_BULLETS) {
        GameObject bullet = { player.x + player.width / 2 - 5, player.y - 10, 10, 20 };
        bullets[bullet_count++] = bullet;
    }
}

void SpawnEnemies()
{
    while (enemy_count < ENEMY_COUNT) {
        GameObject enemy = { rand() % (clientRect.right - 40), -50, 40, 40, rand() % 7 + 6, rand() % 20 }; // 随机初始化计时器
        enemies[enemy_count++] = enemy;

        // 随机决定敌人是否发射子弹
        if (rand() % 2 == 0) { // 50% 概率发射子弹
            FireEnemyBullet(enemy);
        }
    }
}

void FireEnemyBullet(GameObject enemy)
{
    if (enemyBullet_count < MAX_ENEMY_BULLETS) {
        Bullet bullet = { enemy.x + enemy.width / 2 - 5, enemy.y + enemy.height, 10, 20, 16 + enemy.speed };
        enemyBullets[enemyBullet_count++] = bullet;
    }
}
