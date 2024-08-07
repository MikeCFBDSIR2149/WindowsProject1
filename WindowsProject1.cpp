﻿#include "framework.h"
#include "WindowsProject1.h"

#include <cmath>
#include <cstdio>
#include <ctime>
#include <stdlib.h>
#include <windowsx.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Msimg32.lib")

#define MAX_LOADSTRING 100
#define IDT_TIMER1 1
#define PLAYER_SPEED 10
#define BULLET_SPEED 20
// #define ENEMY_SPEED 8
// #define ENEMY_COUNT 5
#define MAX_BULLETS 100
#define MAX_ENEMIES 80
#define MAX_ENEMY_BULLETS 500
#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 800
#define _CRT_SECURE_NO_WARNINGS

// 全局变量:
HINSTANCE hInst;                                // 当前实例
HWND hButtonRestart;                            // 重新开始按钮句柄
HWND hButtonClearHighScore;
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HMODULE hMod;

HBITMAP hbmPlayer, hbmEnemy, hbmBullet, hbmEnemyBullet, hbmBackground, hbmEnemyDamaged, hbmEnemy2, hbmEnemy3;

RECT clientRect;
int wndWidth = 0;	int wndHeight = 0;  // 窗口尺寸

int score = 0;
int gameOver = 0;
int highScore = 0; // 最高记录

int playerLives = 3;
int bulletCooldown = 0;
const int bulletCooldownTime = 3; // 冷却时间

int invincibleTime = 0; // 无敌时间倒计时
const int manualInvincibleTime = 100; // 手动无敌时间
const int passiveInvincibleTime = 20; // 被动无敌时间
int invincibleSkills = 0; // 无敌技能数量

int recoveryTime = 0;
const int recoveryStartTime = 30;
int recoverySkills = 0;

int powerShotTime = 0; // 强力技能时间倒计时
const int powerShotDuration = 150; // 强力技能持续时间
int powerShotSkills = 0; // 强力技能数量

int clearScreenTime = 0; // 清空技能时间倒计时
const int clearScreenDuration = 30; // 清空技能持续时间
int clearScreenSkills = 0; // 清空技能数量

int currentEnemyCount = 2; // 当前敌机上限
const int maxEnemyCount = 10; // 最大敌机数量

int showIntro = 1;
HBITMAP hbmIntro;

typedef struct {
    int x, y;
    int width, height;
    int speed;
    int fireTimer;
    int health;
    int type;
} GameObject;

GameObject player;
GameObject bullets[MAX_BULLETS];
GameObject enemies[MAX_ENEMIES];
int bullet_count = 0;
int enemy_count = 0;

typedef struct {
    int x, y;
    int width, height;
    int speedX, speedY;
} Bullet;

Bullet enemyBullets[MAX_ENEMY_BULLETS];
int enemyBullet_count = 0;

typedef MCIERROR(WINAPI* LPFN_MCISENDSTRING)(LPCWSTR, LPWSTR, UINT, HANDLE);
LPFN_MCISENDSTRING lpfn_mciSendString;

// 前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void UpdateGame(HWND hWnd);
void DrawGame(HDC hdc);
void FireBullet();
void SpawnEnemies();
void FireEnemyBullet(GameObject enemy);

void LoadHighScore() {
    FILE* file = fopen("highscore.txt", "r");
    if (file) {
        fscanf(file, "%d", &highScore);
        fclose(file);
    }
}

void SaveHighScore() {
    FILE* file = fopen("highscore.txt", "w");
    if (file) {
        fprintf(file, "%d", highScore);
        fclose(file);
    }
}

void ShakeWindow(HWND hWnd, int duration, int magnitude) {
    RECT rect;
    GetWindowRect(hWnd, &rect);
    int originalX = rect.left;
    int originalY = rect.top;

    for (int i = 0; i < duration; ++i) {
        int offsetX = (rand() % (2 * magnitude)) - magnitude;
        int offsetY = (rand() % (2 * magnitude)) - magnitude;
        SetWindowPos(hWnd, nullptr, originalX + offsetX, originalY + offsetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        Sleep(10);
    }

    // 还原到原始位置
    SetWindowPos(hWnd, nullptr, originalX, originalY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

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

    srand((unsigned)time(NULL));

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

    MSG msg;
    HWND hWnd = FindWindow(szWindowClass, szTitle); // 获取窗口句柄
    if (!hWnd)
    {
        return FALSE;
    }

    LoadHighScore(); // 加载最高记录

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

    // 获取屏幕的宽度和高度
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 计算窗口的初始位置
    int xPos = (screenWidth - WINDOW_WIDTH) / 2;
    int yPos = (screenHeight - WINDOW_HEIGHT) / 2;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        xPos, yPos, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, hInstance, nullptr);

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

    hbmEnemyDamaged = (HBITMAP)LoadImage(hInstance, L"enemydamaged.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);  // 新增加载受损敌人的图片
    if (!hbmEnemyDamaged) {
        MessageBox(hWnd, L"Failed to load enemydamaged.bmp", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    hbmEnemy2 = (HBITMAP)LoadImage(hInstance, L"enemy2.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hbmEnemy2) {
        MessageBox(hWnd, L"Failed to load enemy2.bmp", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    hbmEnemy3 = (HBITMAP)LoadImage(hInstance, L"enemy3.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hbmEnemy3) {
        MessageBox(hWnd, L"Failed to load enemy3.bmp", L"Error", MB_OK | MB_ICONERROR);
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

    hbmBackground = (HBITMAP)LoadImage(hInstance, L"background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hbmBackground) {
        MessageBox(hWnd, L"Failed to load background.bmp", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    hbmIntro = (HBITMAP)LoadImage(hInstance, L"intro.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hbmIntro) {
        MessageBox(hWnd, L"Failed to load intro.bmp", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    hMod = LoadLibrary(L"winmm.dll");
    if (hMod) {
        lpfn_mciSendString = (LPFN_MCISENDSTRING)GetProcAddress(hMod, "mciSendStringW");
        if (lpfn_mciSendString) {
            lpfn_mciSendString(L"open \"background.mp3\" type mpegvideo alias bgm", NULL, 0, NULL);
        }
    }

    GetClientRect(hWnd, &clientRect);
    player.x = clientRect.right / 2 - 20;
    player.y = clientRect.bottom - 100;
    player.width = 100;
    player.height = 100;

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
            currentEnemyCount = 2; // 重置敌机上限
            playerLives = 3; // 重置玩家生命值
            invincibleTime = 0;
            recoveryTime = 0;
            powerShotTime = 0;
            clearScreenTime = 0;
            player.x = clientRect.right / 2 - 20; // 重置玩家位置
            player.y = clientRect.bottom - 100;
            SpawnEnemies();
            InvalidateRect(hWnd, NULL, TRUE);
            if (lpfn_mciSendString) {
                lpfn_mciSendString(L"play bgm repeat", NULL, 0, NULL);
            }
            break;
        case 3: // 清除最高记录按钮标识符
            highScore = 0;
            SaveHighScore();
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

        // 拷贝内存HDC内容到实际HDC
        BOOL tt = BitBlt(hdc, rectClient.left, rectClient.top, wndWidth,
            wndHeight, memHDC, rectClient.left, rectClient.top, SRCCOPY);

        // 内存回收
        SelectObject(memHDC, pOldBMP);
        DeleteObject(bmpBuff);
        DeleteDC(memHDC);

        EndPaint(hWnd, &ps);
    }
    break;
    case WM_KEYDOWN:
        if (showIntro && wParam == VK_RETURN) {
            showIntro = 0;
            InvalidateRect(hWnd, NULL, TRUE); // 重新绘制窗口
            typedef MCIERROR(WINAPI* LPFN_MCISENDSTRING)(LPCWSTR, LPWSTR, UINT, HANDLE);
            LPFN_MCISENDSTRING lpfn_mciSendString = (LPFN_MCISENDSTRING)GetProcAddress(hMod, "mciSendStringW");
            if (lpfn_mciSendString) {
                lpfn_mciSendString(L"play bgm repeat", NULL, 0, NULL);
            }
        }
        else {
            switch (wParam)
            {
            case VK_SPACE:
                FireBullet();
                break;
            case 'I': // 处理I键按下事件
                if (invincibleSkills > 0) {
                    invincibleSkills--;
                    invincibleTime = manualInvincibleTime;
                }
                break;
            case 'R':
                if (recoverySkills > 0 && playerLives < 3) {
                    recoverySkills--;
                    playerLives++;
                    recoveryTime = recoveryStartTime;
                }
                break;
            case 'P': // 处理P键按下事件
                if (powerShotSkills > 0 && powerShotTime == 0) {
                    powerShotSkills--;
                    powerShotTime = powerShotDuration;
                }
                break;
            case 'C': // 处理C键按下事件
                if (clearScreenSkills > 0 && clearScreenTime == 0) {
                    clearScreenSkills--;
                    clearScreenTime = clearScreenDuration; // 开启清空屏幕状态
                    ShakeWindow(hWnd, 5, 3); // 在清空屏幕时间内抖动窗口
                    enemy_count = 0; // 清空所有敌机
                    enemyBullet_count = 0; // 清空所有敌机子弹
                }
                break;
            }
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
            UpdateGame(hWnd);
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    case WM_SIZE:
    {
        wndWidth = LOWORD(lParam);
        wndHeight = HIWORD(lParam);
        InvalidateRect(hWnd, NULL, TRUE); // 重新绘制窗口
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

void UpdateGame(HWND hWnd)
{
    if (showIntro) {
        return;
    }

    static int skillInvincibleCooldown = 500; // 每隔500帧给予一个无敌技能
    static int skillRecoveryCooldown = 1000; // 每隔1000帧给予一个恢复技能
    static int skillPowerShotCooldown = 300; // 每隔300帧给予一个强力技能
    static int skillClearScreenCooldown = 800; // 每隔1200帧给予一个清空技能

    if (gameOver) {
        skillInvincibleCooldown = 500;
        skillRecoveryCooldown = 1000;
        skillPowerShotCooldown = 300;
        skillClearScreenCooldown = 800;
        return;
    }

    if (bulletCooldown > 0) {
        bulletCooldown--;
    }

    // 时间减少
    if (invincibleTime > 0) {
        invincibleTime--;
    }

    if (recoveryTime > 0)
    {
        recoveryTime--;
    }

    if (powerShotTime > 0) {
        powerShotTime--;
    }

    if (clearScreenTime > 0) {
        clearScreenTime--;
    }

    // 每隔一段时间给予技能
    skillInvincibleCooldown--;
    if (skillInvincibleCooldown <= 0) {
        invincibleSkills++;
        skillInvincibleCooldown = 500; // 重置技能冷却时间
    }

    skillRecoveryCooldown--;
    if (skillRecoveryCooldown <= 0) {
        recoverySkills++;
        skillRecoveryCooldown = 1000; // 重置技能冷却时间
    }

    skillPowerShotCooldown--;
    if (skillPowerShotCooldown <= 0) {
        powerShotSkills++;
        skillPowerShotCooldown = 300; // 重置技能冷却时间
    }

    skillClearScreenCooldown--;
    if (skillClearScreenCooldown <= 0) {
        clearScreenSkills++;
        skillClearScreenCooldown = 800; // 重置技能冷却时间
    }

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
            int enemyHalfWidth = enemies[j].width / 2;
            int enemyHalfHeight = enemies[j].height / 2;
            int enemyCenterX = enemies[j].x + enemyHalfWidth;
            int enemyCenterY = enemies[j].y + enemyHalfHeight;

            if (bullets[i].x < enemyCenterX + enemyHalfWidth / 2 && bullets[i].x + bullets[i].width > enemyCenterX - enemyHalfWidth / 2 &&
                bullets[i].y < enemyCenterY + enemyHalfHeight / 2 && bullets[i].y + bullets[i].height > enemyCenterY - enemyHalfHeight / 2) {

                // 减少敌人的生命值
                enemies[j].health--;

                if (enemies[j].health <= 0) {
                    if (enemies[j].type == 3) {
                        score += 2; // 类型3敌机加2分
                    }
                    else {
                        score += 1; // 其他敌机加1分
                    }
                    enemies[j] = enemies[--enemy_count];

                    // 检查并调整敌机上限
                    if (score / 10 + 2 > currentEnemyCount && currentEnemyCount < maxEnemyCount) {
                        currentEnemyCount = score / 10 + 2;
                    }
                }

                hit = 1;
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

    // 检查敌人子弹和玩家碰撞
    if (invincibleTime <= 0) { // 当无敌时间为0时才会受到伤害
        for (i = 0; i < enemyBullet_count; ) {
            int playerHalfWidth = player.width / 2;
            int playerHalfHeight = player.height / 2;
            int playerCenterX = player.x + playerHalfWidth;
            int playerCenterY = player.y + playerHalfHeight;

            if (enemyBullets[i].x < playerCenterX + playerHalfWidth / 2 && enemyBullets[i].x + enemyBullets[i].width > playerCenterX - playerHalfWidth / 2 &&
                enemyBullets[i].y < playerCenterY + playerHalfHeight / 2 && enemyBullets[i].y + enemyBullets[i].height > playerCenterY - playerHalfHeight / 2) {
                playerLives--;
                ShakeWindow(hWnd, 10, 5);
                if (playerLives <= 0) {
                    gameOver = 1;
                }
                invincibleTime = passiveInvincibleTime; // 受到伤害后开启无敌时间
                enemyBullets[i] = enemyBullets[--enemyBullet_count];
            }
            else {
                ++i;
            }
        }

        // 检查敌人是否碰到玩家
        for (i = 0; i < enemy_count; ++i) {
            int playerHalfWidth = player.width / 2;
            int playerHalfHeight = player.height / 2;
            int playerCenterX = player.x + playerHalfWidth;
            int playerCenterY = player.y + playerHalfHeight;

            int enemyHalfWidth = enemies[i].width / 2;
            int enemyHalfHeight = enemies[i].height / 2;
            int enemyCenterX = enemies[i].x + enemyHalfWidth;
            int enemyCenterY = enemies[i].y + enemyHalfHeight;

            if (enemyCenterX - enemyHalfWidth / 2 < playerCenterX + playerHalfWidth / 2 &&
                enemyCenterX + enemyHalfWidth / 2 > playerCenterX - playerHalfWidth / 2 &&
                enemyCenterY - enemyHalfHeight / 2 < playerCenterY + playerHalfHeight / 2 &&
                enemyCenterY + enemyHalfHeight / 2 > playerCenterY - playerHalfHeight / 2) {
                playerLives -= 2;
                ShakeWindow(hWnd, 10, 5);
                if (playerLives <= 0) {
                    gameOver = 1;
                }
                invincibleTime = passiveInvincibleTime; // 受到伤害后开启无敌时间
                enemies[i] = enemies[--enemy_count];
            }
        }
    }

    // 调整生成敌人逻辑
    if (enemy_count < currentEnemyCount) {
        SpawnEnemies();
    }

    // 更新敌人子弹位置
    for (i = 0; i < enemyBullet_count; ++i) {
        enemyBullets[i].x += enemyBullets[i].speedX;
        enemyBullets[i].y += enemyBullets[i].speedY;
    }

    // 移除离开屏幕的敌人子弹
    for (i = 0; i < enemyBullet_count; ) {
        if (enemyBullets[i].y > clientRect.bottom || enemyBullets[i].x < 0 || enemyBullets[i].x > clientRect.right) {
            enemyBullets[i] = enemyBullets[--enemyBullet_count];
        }
        else {
            ++i;
        }
    }

    // 敌人发射子弹逻辑
    for (i = 0; i < enemy_count; ++i) {
        enemies[i].fireTimer--;
        if (enemies[i].fireTimer <= 0) {
            int playerCenterY = player.y + player.height / 2;
            int enemyCenterY = enemies[i].y + enemies[i].height;
            if (abs(playerCenterY - enemyCenterY) > 150) {
                FireEnemyBullet(enemies[i]);
            }
            enemies[i].fireTimer = rand() % 30 + 20;
        }
    }

    if (enemy_count < currentEnemyCount) {
        SpawnEnemies();
    }
}


void DrawGame(HDC hdc)
{
    if (showIntro) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        HGDIOBJ oldBitmap = SelectObject(hdcMem, hbmIntro);
        StretchBlt(hdc, 0, 0, wndWidth, wndHeight, hdcMem, 0, 0, 1000, 800, SRCCOPY);
        SelectObject(hdcMem, oldBitmap);
        DeleteDC(hdcMem);
        return;
    }

    if (gameOver) {
        // 清空所有子弹
        bullet_count = 0;
        enemyBullet_count = 0;

        // 停止音乐
        if (lpfn_mciSendString) {
            lpfn_mciSendString(L"stop bgm", NULL, 0, NULL);
        }

        // 更新最高记录
        if (score > highScore) {
            highScore = score;
            SaveHighScore();
        }

        // 重置无敌技能数
        invincibleSkills = 0;
        recoverySkills = 0;
        powerShotSkills = 0;
        clearScreenSkills = 0;

        // 绘制游戏结束画面
        const wchar_t* gameOverText = L"游戏结束";
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        RECT rect;
        GetClientRect(GetForegroundWindow(), &rect);
        rect.top -= 100;
        DrawText(hdc, gameOverText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 绘制得分
        wchar_t scoreText[50];
        wsprintf(scoreText, L"本局分数: %d", score);
        rect.top += 50;
        DrawText(hdc, scoreText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 显示最高记录或新的最高记录
        if (score == highScore) {
            const wchar_t* newHighScoreText = L"新的最高记录！";
            rect.top += 50;
            DrawText(hdc, newHighScoreText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        else {
            wchar_t highScoreText[50];
            wsprintf(highScoreText, L"最高记录: %d", highScore);
            rect.top += 50;
            DrawText(hdc, highScoreText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

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
            // 强制刷新按钮区域以防止闪烁
            InvalidateRect(hButtonRestart, NULL, TRUE);
        }

        // 显示清除最高记录按钮
        if (!hButtonClearHighScore) {
            hButtonClearHighScore = CreateWindow(
                L"BUTTON",
                L"清除最高记录",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                rect.right / 2 - 60, // x 坐标
                rect.bottom / 2 + 90, // y 坐标
                120,        // 宽度
                30,         // 高度
                GetForegroundWindow(),
                (HMENU)3,
                hInst,
                NULL);
            // 强制刷新按钮区域以防止闪烁
            InvalidateRect(hButtonClearHighScore, NULL, TRUE);
        }
        return;
    }

    // 如果游戏没有结束且按钮存在，销毁按钮
    if (hButtonRestart) {
        DestroyWindow(hButtonRestart);
        hButtonRestart = NULL;
    }
    if (hButtonClearHighScore) {
        DestroyWindow(hButtonClearHighScore);
        hButtonClearHighScore = NULL;
    }

    HDC hdcMem = CreateCompatibleDC(hdc);
    HGDIOBJ oldBitmap;

    oldBitmap = SelectObject(hdcMem, hbmBackground);
    StretchBlt(hdc, 0, 0, wndWidth, wndHeight, hdcMem, 0, 0, 1000, 800, SRCCOPY);

    oldBitmap = SelectObject(hdcMem, hbmPlayer);
    TransparentBlt(hdc, player.x, player.y, 100, 100, hdcMem, 0, 0, 100, 100, RGB(255, 255, 255));

    int i;
    for (i = 0; i < bullet_count; ++i) {
        SelectObject(hdcMem, hbmBullet);
        BitBlt(hdc, bullets[i].x, bullets[i].y, bullets[i].width, bullets[i].height, hdcMem, 0, 0, SRCCOPY);
    }

    for (i = 0; i < enemy_count; ++i) {
        HBITMAP hbmCurrent;
        if (enemies[i].type == 2) {
            hbmCurrent = hbmEnemy2;
        }
        else if (enemies[i].type == 3) {
            hbmCurrent = hbmEnemy3;
        }
        else {
            hbmCurrent = (enemies[i].health == 1) ? hbmEnemyDamaged : hbmEnemy;
        }
        SelectObject(hdcMem, hbmCurrent);
        TransparentBlt(hdc, enemies[i].x, enemies[i].y, 100, 100, hdcMem, 0, 0, 100, 100, RGB(255, 255, 255));
    }

    for (i = 0; i < enemyBullet_count; ++i) {
        SelectObject(hdcMem, hbmEnemyBullet);
        TransparentBlt(hdc, enemyBullets[i].x, enemyBullets[i].y, enemyBullets[i].width, enemyBullets[i].height, hdcMem, 0, 0, 50, 50, RGB(255, 255, 255));
    }

    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);

    // 绘制分数
    wchar_t scoreText[20];
    wsprintf(scoreText, L"分数: %d", score);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, 10, 10, scoreText, wcslen(scoreText));

    // 绘制生命值
    wchar_t livesText[20];
    wsprintf(livesText, L"生命值: %d", playerLives);
    SetTextColor(hdc, RGB(255, 20, 20));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, clientRect.right - 80, 10, livesText, wcslen(livesText));

    // 绘制无敌技能数量
    wchar_t invincibleSkillText[20];
    wsprintf(invincibleSkillText, L"无敌技能数(I): %d", invincibleSkills);
    SetTextColor(hdc, RGB(255, 255, 0));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, clientRect.right - 120, 30, invincibleSkillText, wcslen(invincibleSkillText));

    // 绘制恢复技能数量
    wchar_t recoverySkillText[20];
    wsprintf(recoverySkillText, L"恢复技能数(R): %d", recoverySkills);
    SetTextColor(hdc, RGB(0, 255, 0));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, clientRect.right - 125, 50, recoverySkillText, wcslen(recoverySkillText));

    // 绘制强力技能数量
    wchar_t powerShotSkillText[20];
    wsprintf(powerShotSkillText, L"强力技能数(P): %d", powerShotSkills);
    SetTextColor(hdc, RGB(120, 120, 255));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, clientRect.right - 125, 70, powerShotSkillText, wcslen(powerShotSkillText));

    // 绘制清空技能数量
    wchar_t clearScreenSkillText[20];
    wsprintf(clearScreenSkillText, L"清空技能数(C): %d", clearScreenSkills);
    SetTextColor(hdc, RGB(255, 40, 255));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, clientRect.right - 125, 90, clearScreenSkillText, wcslen(clearScreenSkillText));

    // 绘制无敌时间
    if (invincibleTime > 0) {
        wchar_t invincibleText[20];
        wsprintf(invincibleText, L"无敌时间");
        SetTextColor(hdc, RGB(255, 255, 40));
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, 10, 30, invincibleText, wcslen(invincibleText));
    }

    // 绘制恢复生命值
    if (recoveryTime > 0) {
        wchar_t recoveryText[20];
        wsprintf(recoveryText, L"恢复生命值");
        SetTextColor(hdc, RGB(40, 255, 40));
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, 10, 50, recoveryText, wcslen(recoveryText));
    }

    // 绘制强力时间
    if (powerShotTime > 0) {
        wchar_t powerShotText[20];
        wsprintf(powerShotText, L"强力时间");
        SetTextColor(hdc, RGB(160, 160, 255));
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, 10, 70, powerShotText, wcslen(powerShotText));
    }

    // 绘制清空屏幕时间
    if (clearScreenTime > 0) {
        wchar_t clearScreenText[20];
        wsprintf(clearScreenText, L"清空屏幕");
        SetTextColor(hdc, RGB(255, 80, 255));
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, 10, 90, clearScreenText, wcslen(clearScreenText));
    }
}


void FireBullet()
{
    if (bullet_count < MAX_BULLETS && bulletCooldown <= 0) {
        GameObject bullet = { player.x + player.width / 2 - 5, player.y - 10, 10, 20 };
        bullets[bullet_count++] = bullet;

        if (powerShotTime > 0) {
            if (bullet_count < MAX_BULLETS) {
                GameObject bulletLeft = { player.x + player.width / 2 - 30, player.y - 10, 10, 20 };
                bullets[bullet_count++] = bulletLeft;
            }
            if (bullet_count < MAX_BULLETS) {
                GameObject bulletRight = { player.x + player.width / 2 + 20, player.y - 10, 10, 20 };
                bullets[bullet_count++] = bulletRight;
            }
        }

        bulletCooldown = bulletCooldownTime;
    }
}


void SpawnEnemies()
{
    while (enemy_count < currentEnemyCount) {
        int enemyType = rand() % 100; // 生成0-99之间的随机数
        GameObject enemy;

        if (enemyType < 40) {
            // 第一类敌人，40%概率
            enemy = { rand() % (clientRect.right - 100), -100, 100, 100, rand() % 5 + 4, rand() % 20 + 20, 2 };
        }
        else if (enemyType < 80) {
            // 第二类敌人，40%概率
            enemy = { rand() % (clientRect.right - 100), -100, 100, 100, rand() % 5 + 4, rand() % 20 + 20, 1 };
            enemy.type = 2; // 使用类型标识区分敌人种类
        }
        else {
            // 第三类敌人，20%概率
            enemy = { rand() % (clientRect.right - 100), -100, 100, 100, rand() % 5 + 4, rand() % 20 + 20, 1 };
            enemy.type = 3; // 使用类型标识区分敌人种类
        }

        // 随机决定敌人是否发射子弹
        if (rand() % 2 == 0) { // 50% 概率发射子弹
            FireEnemyBullet(enemy);
        }

        enemies[enemy_count++] = enemy;
    }
}

void FireEnemyBullet(GameObject enemy)
{
    int playerCenterX = player.x + player.width / 2;
    int playerCenterY = player.y + player.height / 2;
    int enemyCenterX = enemy.x + enemy.width / 2;
    int enemyCenterY = enemy.y + enemy.height;

    float deltaX = playerCenterX - enemyCenterX;
    float deltaY = playerCenterY - enemyCenterY;
    float length = sqrt(deltaX * deltaX + deltaY * deltaY);

    float speed = 16.0f; // 子弹速度
    float baseSpeedX = speed * (deltaX / length);
    float baseSpeedY = speed * (deltaY / length);

    // 中间子弹
    if (enemyBullet_count < MAX_ENEMY_BULLETS) {
        Bullet bullet = { enemyCenterX - 5, enemyCenterY, 20, 20, (int)baseSpeedX, (int)baseSpeedY };
        enemyBullets[enemyBullet_count++] = bullet;
    }

    if (enemy.type == 3) {
        // 左边子弹
        if (enemyBullet_count < MAX_ENEMY_BULLETS) {
            Bullet bullet = { enemyCenterX - 15, enemyCenterY, 20, 20, (int)(baseSpeedX - 3), (int)baseSpeedY };
            enemyBullets[enemyBullet_count++] = bullet;
        }

        // 右边子弹
        if (enemyBullet_count < MAX_ENEMY_BULLETS) {
            Bullet bullet = { enemyCenterX + 5, enemyCenterY, 20, 20, (int)(baseSpeedX + 3), (int)baseSpeedY };
            enemyBullets[enemyBullet_count++] = bullet;
        }
    }
}


