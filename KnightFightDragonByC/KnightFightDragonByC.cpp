
#include <windows.h>
#include <tchar.h>
#include  <time.h> 


#pragma comment(lib,"winmm.lib")			//调用PlaySound函数所需库文件
#pragma  comment(lib,"Msimg32.lib")		//添加使用TransparentBlt函数所需的库文件


#define WINDOW_WIDTH	800							
#define WINDOW_HEIGHT	600							
#define WINDOW_TITLE		L"勇者斗恶龙"	
#define PARTICLE_NUMBER	50							


//角色结构体
struct CHARACTER
{
	int		NowHp;		
	int		MaxHp;			
	int		NowMp;		
	int		MaxMp;		
	int		Level;			
	int		Strength;		
	int		Intelligence; 
	int		Agility;			
};

struct SNOW
{
	int x;  
	int y; 
	BOOL exist;  
};


//定义一个动作枚举体
enum ActionTypes
{
	ACTION_TYPE_NORMAL = 0,		
	ACTION_TYPE_CRITICAL = 1,		
	ACTION_TYPE_MAGIC = 2,		
	ACTION_TYPE_MISS = 3,				
	ACTION_TYPE_RECOVER = 4,	
};

HDC				g_hdc = NULL, g_mdc = NULL, g_bufdc = NULL;      //全局设备环境句柄与全局内存DC句柄
DWORD		g_tPre = 0, g_tNow = 0;				
RECT				g_rect;				//定义一个RECT结构体，用于储存内部窗口区域的坐标
int					g_iFrameNum, g_iTxtNum;  
wchar_t			text[8][100];  
BOOL			g_bCanAttack, g_bGameOver;   
SNOW			SnowFlowers[PARTICLE_NUMBER];   
int					g_SnowNum = 0; 
CHARACTER	Hero, Boss; 
ActionTypes	HeroActionType, BossActionType;  
//一系列位图句柄的定义
HBITMAP		g_hBackGround, g_hGameOver, g_hVictory, g_hSnow;  
HBITMAP		g_hMonsterBitmap, g_hHeroBitmap, g_hRecoverSkill;  
HBITMAP		g_hSkillButton1, g_hSkillButton2, g_hSkillButton3, g_hSkillButton4;  
HBITMAP		g_hHeroSkill1, g_hHeroSkill2, g_hHeroSkill3;  
HBITMAP		g_hBossSkill1, g_hBossSkill2, g_hBossSkill3;  

LRESULT CALLBACK	WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL						Game_Init(HWND hwnd);			
VOID							Game_Main(HWND hwnd);		
BOOL						Game_ShutDown(HWND hwnd);	
VOID							Die_Check(int NowHp, bool isHero);   
VOID							Message_Insert(wchar_t* str);  
VOID							HeroAction_Logic();  
VOID							HeroAction_Paint();  
VOID							BossAction_Logic(); 
VOID							BossAction_Paint(); 
VOID							Snow_Paint();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX wndClass = { 0 };							//用WINDCLASSEX定义了一个窗口
	wndClass.cbSize = sizeof(WNDCLASSEX);			
	wndClass.style = CS_HREDRAW | CS_VREDRAW;	
	wndClass.lpfnWndProc = WndProc;					
	wndClass.cbClsExtra = 0;								
	wndClass.cbWndExtra = 0;							
	wndClass.hInstance = hInstance;						
	wndClass.hIcon = (HICON)::LoadImage(NULL, L"icon.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);  
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);    
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);  
	wndClass.lpszMenuName = NULL;						
	wndClass.lpszClassName = L"ForTheDreamOfGameDevelop";		

	
	if (!RegisterClassEx(&wndClass))				
		return -1;

	
	HWND hwnd = CreateWindow(L"ForTheDreamOfGameDevelop", WINDOW_TITLE,				
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH,
		WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

	
	MoveWindow(hwnd, 250, 80, WINDOW_WIDTH, WINDOW_HEIGHT, true);		
	ShowWindow(hwnd, nShowCmd);    
	UpdateWindow(hwnd);						

	//游戏资源的初始化
	if (!Game_Init(hwnd))
	{
		MessageBox(hwnd, L"资源初始化失败", L"消息窗口", 0); 
		return FALSE;
	}
	PlaySound(L"GameMedia\\梦幻西游原声-战斗1-森林.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP); //循环播放背景音乐 

	MSG msg = { 0 };				//定义并初始化msg
	while (msg.message != WM_QUIT)	
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))   //查看应用程序消息队列，有消息时将队列中的消息派发出去。
		{
			TranslateMessage(&msg);		//将虚拟键消息转换为字符消息
			DispatchMessage(&msg);			//分发一个消息给窗口程序。
		}
		else
		{
			g_tNow = GetTickCount();   //获取当前系统时间
			if (g_tNow - g_tPre >= 60)        //当此次循环运行与上次绘图时间相差0.06秒时再进行重绘操作
				Game_Main(hwnd);
		}

	}

	//【6】窗口类的注销
	UnregisterClass(L"ForTheDreamOfGameDevelop", wndClass.hInstance);  //程序准备结束，注销窗口类
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)					
	{
	case WM_KEYDOWN:					//按键消息
		if (wParam == VK_ESCAPE)		//按下【Esc】键
			PostQuitMessage(0);
		break;

	case WM_LBUTTONDOWN:			//鼠标左键消息
		if (!g_bCanAttack)
		{
			int x = LOWORD(lParam);	
			int y = HIWORD(lParam);		

			if (x >= 530 && x <= 570 && y >= 420 && y <= 470)
			{
				g_bCanAttack = true;
				HeroActionType = ACTION_TYPE_NORMAL;
			}
			if (x >= 590 && x <= 640 && y >= 420 && y <= 470)
			{
				g_bCanAttack = true;
				HeroActionType = ACTION_TYPE_MAGIC;
			}
			if (x >= 650 && x <= 700 && y >= 420 && y <= 470)
			{
				g_bCanAttack = true;
				HeroActionType = ACTION_TYPE_RECOVER;
			}

		}
		break;
	case WM_DESTROY:				
		Game_ShutDown(hwnd);			//调用自定义的资源清理函数Game_ShutDown（）进行退出前的资源清理
		PostQuitMessage(0);			//向系统表明有个线程有终止请求。用来响应WM_DESTROY消息
		break;								

	default:									
		return DefWindowProc(hwnd, message, wParam, lParam);		//调用缺省的窗口过程
	}

	return 0;									//正常退出
}



BOOL Game_Init(HWND hwnd)
{
	srand((unsigned)time(NULL));      

	HBITMAP bmp;

	//三缓冲体系的创建
	g_hdc = GetDC(hwnd);
	g_mdc = CreateCompatibleDC(g_hdc);  //创建一个和hdc兼容的内存DC
	g_bufdc = CreateCompatibleDC(g_hdc);//再创建一个和hdc兼容的缓冲DC
	bmp = CreateCompatibleBitmap(g_hdc, WINDOW_WIDTH, WINDOW_HEIGHT); //建一个和窗口兼容的空的位图对象

	SelectObject(g_mdc, bmp);//将空位图对象放到mdc中

	//载入一系列游戏资源图到位图句柄中
	g_hGameOver = (HBITMAP)LoadImage(NULL, L"GameMedia\\gameover.bmp", IMAGE_BITMAP, 1086, 396, LR_LOADFROMFILE);  //游戏结束位图
	g_hVictory = (HBITMAP)LoadImage(NULL, L"GameMedia\\victory.bmp", IMAGE_BITMAP, 800, 600, LR_LOADFROMFILE);  //游戏胜利位图
	g_hBackGround = (HBITMAP)LoadImage(NULL, L"GameMedia\\bg.bmp", IMAGE_BITMAP, 800, 600, LR_LOADFROMFILE);  //背景位图
	g_hMonsterBitmap = (HBITMAP)LoadImage(NULL, L"GameMedia\\monster.bmp", IMAGE_BITMAP, 360, 360, LR_LOADFROMFILE);  //怪物角色位图
	g_hHeroBitmap = (HBITMAP)LoadImage(NULL, L"GameMedia\\hero.bmp", IMAGE_BITMAP, 360, 360, LR_LOADFROMFILE);  //英雄角色位图
	g_hHeroSkill1 = (HBITMAP)LoadImage(NULL, L"GameMedia\\heroslash.bmp", IMAGE_BITMAP, 364, 140, LR_LOADFROMFILE);  //英雄1技能位图
	g_hHeroSkill2 = (HBITMAP)LoadImage(NULL, L"GameMedia\\heromagic.bmp", IMAGE_BITMAP, 374, 288, LR_LOADFROMFILE);  //英雄2技能位图
	g_hHeroSkill3 = (HBITMAP)LoadImage(NULL, L"GameMedia\\herocritical.bmp", IMAGE_BITMAP, 574, 306, LR_LOADFROMFILE);  //英雄3技能位图
	g_hSkillButton1 = (HBITMAP)LoadImage(NULL, L"GameMedia\\skillbutton1.bmp", IMAGE_BITMAP, 50, 50, LR_LOADFROMFILE);  //技能1图标位图
	g_hSkillButton2 = (HBITMAP)LoadImage(NULL, L"GameMedia\\skillbutton2.bmp", IMAGE_BITMAP, 50, 50, LR_LOADFROMFILE);   //技能2图标位图
	g_hSkillButton3 = (HBITMAP)LoadImage(NULL, L"GameMedia\\skillbutton3.bmp", IMAGE_BITMAP, 50, 50, LR_LOADFROMFILE);   //技能3图标位图
	g_hSkillButton4 = (HBITMAP)LoadImage(NULL, L"GameMedia\\skillbutton4.bmp", IMAGE_BITMAP, 50, 50, LR_LOADFROMFILE);   //技能4图标位图
	g_hBossSkill1 = (HBITMAP)LoadImage(NULL, L"GameMedia\\monsterslash.bmp", IMAGE_BITMAP, 234, 188, LR_LOADFROMFILE);   //怪物1技能位图
	g_hBossSkill2 = (HBITMAP)LoadImage(NULL, L"GameMedia\\monstermagic.bmp", IMAGE_BITMAP, 387, 254, LR_LOADFROMFILE);  //怪物2技能位图
	g_hBossSkill3 = (HBITMAP)LoadImage(NULL, L"GameMedia\\monstercritical.bmp", IMAGE_BITMAP, 574, 306, LR_LOADFROMFILE); //怪物3技能位图
	g_hSnow = (HBITMAP)LoadImage(NULL, L"GameMedia\\snow.bmp", IMAGE_BITMAP, 30, 30, LR_LOADFROMFILE);   //雪花位图
	g_hRecoverSkill = (HBITMAP)LoadImage(NULL, L"GameMedia\\recover.bmp", IMAGE_BITMAP, 150, 150, LR_LOADFROMFILE);    //恢复技能位图

	GetClientRect(hwnd, &g_rect);		//取得内部窗口区域的大小

	//设定玩家
	Hero.NowHp = Hero.MaxHp = 1000;	
	Hero.Level = 6;					
	Hero.NowMp = Hero.MaxMp = 60;    
	Hero.Strength = 10;			
	Hero.Agility = 20;			
	Hero.Intelligence = 10;	                                        

	//设定BOSS
	Boss.NowHp = Boss.MaxHp = 2000;	
	Boss.Level = 10;						
	Boss.Strength = 10;				
	Boss.Agility = 10;                   
	Boss.Intelligence = 10;		

	g_iTxtNum = 0;		

	//设置字体
	HFONT hFont;
	hFont = CreateFont(20, 0, 0, 0, 700, 0, 0, 0, GB2312_CHARSET, 0, 0, 0, 0, TEXT("微软雅黑"));
	SelectObject(g_mdc, hFont);
	SetBkMode(g_mdc, TRANSPARENT);    //设置文字显示背景透明

	Game_Main(hwnd);  
	return TRUE;
}

VOID Game_Main(HWND hwnd)
{
	wchar_t str[100];

	//先在mdc中贴上背景图
	SelectObject(g_bufdc, g_hBackGround);
	BitBlt(g_mdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, g_bufdc, 0, 0, SRCCOPY);

	//下雪效果的绘制
	if (!g_bGameOver)
	{
		Snow_Paint(); 
	}

	//显示对战消息
	SetTextColor(g_mdc, RGB(255, 255, 255));
	for (int i = 0; i < g_iTxtNum; i++)
		TextOut(g_mdc, 20, 410 + i * 18, text[i], wcslen(text[i]));


	if (Boss.NowHp > 0)
	{
		
		SelectObject(g_bufdc, g_hMonsterBitmap);
		TransparentBlt(g_mdc, 0, 50, 360, 360, g_bufdc, 0, 0, 360, 360, RGB(0, 0, 0));//采用TransparentBlt透明贴图函数
		
		swprintf_s(str, L"%d / %d", Boss.NowHp, Boss.MaxHp);
		SetTextColor(g_mdc, RGB(255, 10, 10));
		TextOut(g_mdc, 100, 370, str, wcslen(str));
	}

	if (Hero.NowHp > 0)
	{
		
		SelectObject(g_bufdc, g_hHeroBitmap);
		TransparentBlt(g_mdc, 400, 50, 360, 360, g_bufdc, 0, 0, 360, 360, RGB(0, 0, 0));//透明色为RGB(0,0,0)纯黑色
		
		swprintf_s(str, L"%d / %d", Hero.NowHp, Hero.MaxHp);
		SetTextColor(g_mdc, RGB(255, 10, 10));
		TextOut(g_mdc, 600, 350, str, wcslen(str));
		
		swprintf_s(str, L"%d / %d", Hero.NowMp, Hero.MaxMp);
		SetTextColor(g_mdc, RGB(10, 10, 255));
		TextOut(g_mdc, 600, 370, str, wcslen(str));
	}

	if (g_bGameOver)
	{
		if (Hero.NowHp <= 0)  
		{
			SelectObject(g_bufdc, g_hGameOver);
			BitBlt(g_mdc, 120, 50, 543, 396, g_bufdc, 543, 0, SRCAND);
			BitBlt(g_mdc, 120, 50, 543, 396, g_bufdc, 0, 0, SRCPAINT);
		}

		else  
		{
			SelectObject(g_bufdc, g_hVictory);
			TransparentBlt(g_mdc, 0, 0, 800, 600, g_bufdc, 0, 0, 800, 600, RGB(0, 0, 0));//透明色为RGB(0,0,0)
		}
	}
	else if (!g_bCanAttack)		//贴上技能按钮
	{
		SelectObject(g_bufdc, g_hSkillButton1);
		BitBlt(g_mdc, 530, 420, 50, 50, g_bufdc, 0, 0, SRCCOPY);
		SelectObject(g_bufdc, g_hSkillButton2);
		BitBlt(g_mdc, 590, 420, 50, 50, g_bufdc, 0, 0, SRCCOPY);
		SelectObject(g_bufdc, g_hSkillButton3);
		BitBlt(g_mdc, 650, 420, 50, 50, g_bufdc, 0, 0, SRCCOPY);
		SelectObject(g_bufdc, g_hSkillButton4);
		BitBlt(g_mdc, 710, 420, 50, 50, g_bufdc, 0, 0, SRCCOPY);
	}
	else
	{
		g_iFrameNum++;

		//第5~10个画面时显示玩家攻击效果图
		if (g_iFrameNum >= 5 && g_iFrameNum <= 10)
		{
			//第5个画面时根据之前的输入计算出游戏逻辑并进行消息显示
			if (g_iFrameNum == 5)
			{
				HeroAction_Logic();
				Die_Check(Boss.NowHp, false);
			}
			HeroAction_Paint();
		}

		//第15个画面时判断怪物进行哪项动作
		if (g_iFrameNum == 15)
		{
			BossAction_Logic();
		}

		//第26~30个画面时显示怪物攻击图标
		if (g_iFrameNum >= 26 && g_iFrameNum <= 30)
		{
			BossAction_Paint();
		}

		if (g_iFrameNum == 30)			//回合结束
		{
			g_bCanAttack = false;
			g_iFrameNum = 0;

			//每回合的魔法自然恢复，6点固定值加上0到智力值之间的一个随机值的三倍
			if (!g_bGameOver)
			{
				int MpRecover = 2 * (rand() % Hero.Intelligence) + 6;
				Hero.NowMp += MpRecover;

				if (Hero.NowMp >= Hero.MaxMp)
				{
					Hero.NowMp = Hero.MaxMp;
				}

				swprintf_s(str, L"回合结束，自动恢复了【%d】点魔法值", MpRecover);
				Message_Insert(str);
			}
		}
	}

	//将mdc中的全部内容贴到hdc中
	BitBlt(g_hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, g_mdc, 0, 0, SRCCOPY);
	g_tPre = GetTickCount();     //记录此次绘图时间
}


void Message_Insert(wchar_t* str)
{
	//如果未满8行消息，直接新开一条消息
	if (g_iTxtNum < 8)
	{
		swprintf_s(text[g_iTxtNum], str);
		g_iTxtNum++;
	}
	//如果满了8行消息，只有挤走最上面的一条消息
	else
	{
		for (int i = 0; i < g_iTxtNum; i++)
			swprintf_s(text[i], text[i + 1]);
		swprintf_s(text[7], str);
	}
}


void Die_Check(int NowHp, bool isHero)
{
	wchar_t str[100];

	if (NowHp <= 0)//生命值小于等于0了
	{
		g_bGameOver = true;
		if (isHero)  
		{
			PlaySound(L"GameMedia\\failure.wav", NULL, SND_FILENAME | SND_ASYNC); 
			swprintf_s(str, L": ( 胜败乃兵家常事，大侠请重新来过......");  
			Message_Insert(str);  //插入到文字消息中
		}
		else
		{
			PlaySound(L"GameMedia\\victory.wav", NULL, SND_FILENAME | SND_ASYNC); 
			swprintf_s(str, L"少年，你赢了，有两下子啊~~~~~！！！！");  
			Message_Insert(str);  //插入到文字消息中
		}
	}
}

VOID		HeroAction_Logic()
{
	int damage = 0;
	wchar_t str[100];

	switch (HeroActionType)
	{
	case ACTION_TYPE_NORMAL:

		if (1 == rand() % 4)                   // 20%几率触发幻影刺客的大招，恩赐解脱，4倍暴击伤害
		{
			HeroActionType = ACTION_TYPE_CRITICAL;
			damage = (int)(4.5f * (float)(3 * (rand() % Hero.Agility) + Hero.Level * Hero.Strength + 20));
			Boss.NowHp -= (int)damage;

			swprintf_s(str, L"恩赐解脱触发，这下牛逼了，4.5倍暴击...对怪物照成了【%d】点伤害", damage);
		}
		else       //没有触发”恩赐解脱“，还是用普通攻击”无敌斩“
		{
			damage = 3 * (rand() % Hero.Agility) + Hero.Level * Hero.Strength + 20;
			Boss.NowHp -= (int)damage;

			swprintf_s(str, L"玩家使用了普通攻击“无敌斩”，伤害一般般...对怪物照成了【%d】点伤害", damage);
		}

		Message_Insert(str);
		break;

	case ACTION_TYPE_MAGIC:  //释放烈火剑法
		if (Hero.NowMp >= 30)
		{
			damage = 5 * (2 * (rand() % Hero.Agility) + Hero.Level * Hero.Intelligence);
			Boss.NowHp -= (int)damage;
			Hero.NowMp -= 30;
			swprintf_s(str, L"玩家释放烈火剑法...对怪物照成了【%d】点伤害", damage);
		}
		else
		{
			HeroActionType = ACTION_TYPE_MISS;
			swprintf_s(str, L"你傻啊~!，魔法值不足30点，施法失败，这回合白费了~！");
		}
		Message_Insert(str);
		break;

	case ACTION_TYPE_RECOVER:  //使用气疗术

		if (Hero.NowMp >= 40)
		{
			Hero.NowMp -= 40;
			int HpRecover = 5 * (5 * (rand() % Hero.Intelligence) + 40);
			Hero.NowHp += HpRecover;
			if (Hero.NowHp >= Hero.MaxHp)
			{
				Hero.NowHp = Hero.MaxHp;
			}
			swprintf_s(str, L"玩家使用了气疗术，恢复了【%d】点生命值，感觉好多了。", HpRecover);
		}
		else
		{
			HeroActionType = ACTION_TYPE_MISS;
			swprintf_s(str, L"你傻啊~!，魔法值不足40点，施法失败，这回合白费了~！");
		}
		Message_Insert(str);
		break;
	}

}


VOID HeroAction_Paint()
{
	switch (HeroActionType)
	{
	case ACTION_TYPE_NORMAL:   //普通攻击，无敌斩
		SelectObject(g_bufdc, g_hHeroSkill1);
		TransparentBlt(g_mdc, 50, 170, 364, 140, g_bufdc, 0, 0, 364, 140, RGB(0, 0, 0));//TransparentBlt函数的透明颜色设为RGB(0,0,0)
		break;

	case ACTION_TYPE_CRITICAL:  //暴击，恩赐解脱
		SelectObject(g_bufdc, g_hHeroSkill3);
		TransparentBlt(g_mdc, 20, 60, 574, 306, g_bufdc, 0, 0, 574, 306, RGB(0, 0, 0));//TransparentBlt函数的透明颜色设为RGB(0,0,0)
		break;

	case ACTION_TYPE_MAGIC:  //魔法攻击，烈火剑法
		SelectObject(g_bufdc, g_hHeroSkill2);
		TransparentBlt(g_mdc, 50, 100, 374, 288, g_bufdc, 0, 0, 374, 288, RGB(0, 0, 0));//TransparentBlt函数的透明颜色设为RGB(0,0,0)
		break;

	case ACTION_TYPE_RECOVER:   //恢复，气疗术
		SelectObject(g_bufdc, g_hRecoverSkill);
		TransparentBlt(g_mdc, 560, 170, 150, 150, g_bufdc, 0, 0, 150, 150, RGB(0, 0, 0));//TransparentBlt函数的透明颜色设为RGB(0,0,0)
		break;
	}
}

VOID BossAction_Logic()
{
	srand((unsigned)time(NULL));      //初始化随机种子 
	if (Boss.NowHp > (Boss.MaxHp / 2))				
	{
		switch (rand() % 3)
		{
		case 0:						
			BossActionType = ACTION_TYPE_NORMAL;
			break;
		case 1:						
			BossActionType = ACTION_TYPE_CRITICAL;
			break;
		case 2:						
			BossActionType = ACTION_TYPE_MAGIC;
			break;
		}
	}
	else								
	{
		switch (rand() % 3)
		{
		case 0:						
			BossActionType = ACTION_TYPE_MAGIC;
			break;
		case 1:			
			BossActionType = ACTION_TYPE_CRITICAL;
			break;
		case 2:						
			BossActionType = ACTION_TYPE_RECOVER;
			break;
		}
	}
}

VOID		BossAction_Paint()
{
	int damage = 0, recover = 0;
	wchar_t str[100];

	switch (BossActionType)
	{
	case ACTION_TYPE_NORMAL:							//释放普通攻击
		SelectObject(g_bufdc, g_hBossSkill1);
		TransparentBlt(g_mdc, 500, 150, 234, 188, g_bufdc, 0, 0, 234, 188, RGB(0, 0, 0));//TransparentBlt函数的透明颜色设为RGB(0,0,0)
		//第30个画面时计算玩家受伤害程度并加入显示消息
		if (g_iFrameNum == 30)
		{
			damage = rand() % Boss.Agility + Boss.Level * Boss.Strength;
			Hero.NowHp -= (int)damage;

			swprintf_s(str, L"释放幽冥鬼火...对玩家照成【 %d】 点伤害", damage);
			Message_Insert(str);

			Die_Check(Hero.NowHp, true);
		}
		break;

	case ACTION_TYPE_MAGIC:							//释放嗜血咒
		SelectObject(g_bufdc, g_hBossSkill2);
		TransparentBlt(g_mdc, 450, 150, 387, 254, g_bufdc, 0, 0, 387, 254, RGB(0, 0, 0));//TransparentBlt函数的透明颜色设为RGB(0,0,0)
		//第30个画面时计算玩家受伤害程度并加入显示消息
		if (g_iFrameNum == 30)
		{
			damage = 2 * (2 * (rand() % Boss.Agility) + Boss.Strength * Boss.Intelligence);  //嗜血咒的伤害值计算
			Hero.NowHp -= damage;	   
			recover = (int)((float)damage * 0.2f);   
			Boss.NowHp += recover;   
			swprintf_s(str, L"释放嗜血咒...对玩家照成【 %d 】点伤害,自身恢复【%d】点生命值", damage, recover);   
			Message_Insert(str);   //插入文字消息

			Die_Check(Hero.NowHp, true);
		}
		break;

	case ACTION_TYPE_CRITICAL:							//释放致命一击
		SelectObject(g_bufdc, g_hBossSkill3);
		TransparentBlt(g_mdc, 280, 100, 574, 306, g_bufdc, 0, 0, 574, 306, RGB(0, 0, 0));//TransparentBlt函数的透明颜色设为RGB(0,0,0)
		//第30个画面时计算玩家受伤害程度并加入显示消息
		if (g_iFrameNum == 30)
		{
			damage = 2 * (rand() % Boss.Agility + Boss.Level * Boss.Strength);
			Hero.NowHp -= (int)damage;

			swprintf_s(str, L"致命一击...对玩家照成【%d】点伤害.", damage);
			Message_Insert(str);

			Die_Check(Hero.NowHp, true);
		}
		break;

	case ACTION_TYPE_RECOVER:							//使用梅肯斯姆补血
		SelectObject(g_bufdc, g_hRecoverSkill);
		TransparentBlt(g_mdc, 150, 150, 150, 150, g_bufdc, 0, 0, 150, 150, RGB(0, 0, 0));//TransparentBlt函数的透明颜色设为RGB(0,0,0)
		//第30个画面时怪物回复生命值并加入显示消息
		if (g_iFrameNum == 30)
		{
			recover = 2 * Boss.Intelligence * Boss.Intelligence;
			Boss.NowHp += recover;
			swprintf_s(str, L"使用梅肯斯姆...恢复了【%d】点生命值", recover);
			Message_Insert(str);
		}
		break;
	}
}


VOID Snow_Paint()
{
	//创建粒子
	if (g_SnowNum < PARTICLE_NUMBER)  
	{
		SnowFlowers[g_SnowNum].x = rand() % g_rect.right; 
		SnowFlowers[g_SnowNum].y = 0;    
		SnowFlowers[g_SnowNum].exist = true; 
		g_SnowNum++;   
	}

	//首先判断粒子是否存在，若存在，进行透明贴图操作
	for (int i = 0; i < PARTICLE_NUMBER; i++)
	{
		if (SnowFlowers[i].exist)  //粒子还存在
		{
			//贴上粒子图
			SelectObject(g_bufdc, g_hSnow);
			TransparentBlt(g_mdc, SnowFlowers[i].x, SnowFlowers[i].y, 30, 30, g_bufdc, 0, 0, 30, 30, RGB(0, 0, 0));

			//随机决定横向的移动方向和偏移量
			if (rand() % 2 == 0)
				SnowFlowers[i].x += rand() % 6;  
			else
				SnowFlowers[i].x -= rand() % 6;	 
			
			SnowFlowers[i].y += 10;  //纵坐标加上10
			//如果粒子坐标超出了窗口长度，就让它以随机的x坐标出现在窗口顶部
			if (SnowFlowers[i].y > g_rect.bottom)
			{
				SnowFlowers[i].x = rand() % g_rect.right;
				SnowFlowers[i].y = 0;
			}
		}

	}
}


BOOL Game_ShutDown(HWND hwnd)
{
	//释放资源对象
	DeleteObject(g_hBackGround);
	DeleteObject(g_hBackGround);
	DeleteObject(g_hGameOver);
	DeleteObject(g_hVictory);
	DeleteObject(g_hSnow);
	DeleteObject(g_hMonsterBitmap);
	DeleteObject(g_hHeroBitmap);
	DeleteObject(g_hRecoverSkill);
	DeleteObject(g_hSkillButton1);
	DeleteObject(g_hSkillButton2);
	DeleteObject(g_hSkillButton3);
	DeleteObject(g_hSkillButton4);
	DeleteObject(g_hHeroSkill1);
	DeleteObject(g_hHeroSkill2);
	DeleteObject(g_hHeroSkill3);
	DeleteObject(g_hBossSkill1);
	DeleteObject(g_hBossSkill2);
	DeleteObject(g_hBossSkill3);
	DeleteDC(g_bufdc);
	DeleteDC(g_mdc);
	ReleaseDC(hwnd, g_hdc);
	return TRUE;
}

