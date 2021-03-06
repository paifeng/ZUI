﻿#include <ZUI.h>
void Rect_Join(RECT *rc, RECT *rc1)
{
    if (rc1->left < rc->left) rc->left = rc1->left;
    if (rc1->top < rc->top) rc->top = rc1->top;
    if (rc1->right > rc->right) rc->right = rc1->right;
    if (rc1->bottom > rc->bottom) rc->bottom = rc1->bottom;
}

void * ZCALL Zui_Hash(wchar_t* str) {
    size_t hash = 0;
    wchar_t ch;
    for (long i = 0; ch = (size_t)*str++; i++)
    {
        if ((i & 1) == 0)
        {
            hash ^= ((hash << 7) ^ ch ^ (hash >> 3));
        }
        else
        {
            hash ^= (~((hash << 11) ^ ch ^ (hash >> 5)));
        }
    }
    return hash;
}

ZEXPORT ZuiBool ZCALL ZuiInit(ZuiInitConfig config) {
    if (config && config->m_hInstance) {
        m_hInstance = config->m_hInstance;
    }
#if RUN_DEBUG
    if (config && config->debug) {
        //启动调试器
        ZuiStartDebug();
    }
#endif
    /*初始化图形层*/
    if (!ZuiGraphInitialize()) {
        return FALSE;
    }
    /*初始化系统层*/
    if (!ZuiOsInitialize()) {
        return FALSE;
    }
    /*初始化全局变量*/
    if (!ZuiGlobalInit()) {
        return FALSE;
    }
    /*初始化绘制管理器*/
    if (!ZuiPaintManagerInitialize()) {
        return FALSE;
    }
    /*注册全局控件*/
    if (!ZuiControlRegister())
    {
        return FALSE;
    }
    /*初始化模版管理器*/
    if (!ZuiTemplateInit())
    {
        return FALSE;
    }
    /*初始化绑定器*/
    if (!ZuiBuilderInit())
    {
        return FALSE;
    }
    /*初始化资源池*/
    if (!ZuiResDBInit()) {
        return FALSE;
    }

    return TRUE;
}
ZEXPORT ZuiBool ZCALL ZuiUnInit() {
    /*反初始化绘制管理器*/
    ZuiPaintManagerUnInitialize();
    /*反注册全局控件*/
    ZuiControlUnRegister();
    /*反初始化模版管理器*/
    ZuiTemplateUnInit();
    /*反初始化绑定器*/
    ZuiBuilderUnInit();
    /*反初始化资源池*/
    ZuiResDBUnInit();
    /*反初始化全局变量*/
    ZuiGlobalUnInit();
    /*反初始化系统层*/
    ZuiOsUnInitialize();
    /*反初始化图形层*/
    ZuiGraphUnInitialize();
    return TRUE;
}
ZEXPORT ZuiInt ZCALL ZuiMsgLoop() {
    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
ZEXPORT ZuiVoid ZCALL ZuiMsgLoop_exit() {
    PostQuitMessage(0);
}
ZuiControl MsgBox_pRoot;
ZuiAny ZCALL MsgBox_Notify_ctl(ZuiText msg, ZuiControl p, ZuiAny UserData, ZuiAny Param1, ZuiAny Param2, ZuiAny Param3) {
    if (wcscmp(p->m_sName, L"clos") == 0) {
        ZuiMsgLoop_exit();
    }
    else if (wcscmp(p->m_sName, L"min") == 0) {
        ZuiControlCall(Proc_Window_SetWindowMin, MsgBox_pRoot, NULL, NULL, NULL);
    }
    return 0;
}

ZEXPORT ZuiVoid ZCALL ZuiMsgBox() {
    MsgBox_pRoot = NewZuiControl(L"window", NULL, NULL, NULL);
    ZuiControlRegNotify(MsgBox_pRoot, MsgBox_Notify_ctl);



    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return;
}
ZEXPORT ZuiBool ZCALL ZuiIsPointInRect(ZuiRect Rect, ZuiPoint pt) {

    return PtInRect(Rect, *(POINT *)pt);
    /*
    if (pt->x <= Rect->Left) {
        return FALSE;
    }
    if (pt->x >= Rect->Width + Rect->Left) {
        return FALSE;
    }

    if (pt->y <= Rect->Top) {
        return FALSE;
    }
    if (pt->y >= Rect->Height + Rect->Top) {
        return FALSE;
    }

    return TRUE;
    */
}