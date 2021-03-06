﻿#include <ZUI.h>
#if MEM_DEBUG || RUN_DEBUG
#define snprintf _snprintf

MArray *marray_create()
{
    int i = 0;
    MArray *darray = (MArray *)malloc(sizeof(MArray));
    if (darray != NULL)
    {
        darray->count = 0;
        darray->size = 0;
        darray->msize = 0;
        InitializeCriticalSection(&darray->cs);
        darray->data = (void **)malloc(sizeof(void *) * DEFAULT_A_SIZE);

        if (darray->data != NULL)
        {
            darray->size = DEFAULT_A_SIZE;
            for (i = 0; i < darray->size; i++)
            {
                darray->data[i] = NULL;
            }
        }
        return darray;
    }

    return NULL;
}
static BOOL marray_expand(MArray *darray, int needone)
{
    int newallocsize = 0;

    if (needone == 2)
    {
        newallocsize = darray->count + (darray->count >> 1) + DEFAULT_A_SIZE;
    }
    else
    {
        newallocsize = darray->count + 1;
    }
    void **data = (void **)realloc(darray->data, sizeof(void *) * newallocsize);
    if (data != NULL)
    {
        darray->data = data;
        darray->size = newallocsize;

    }
    return TRUE;

}
BOOL marray_append(MArray *darray, void *_Ptr, size_t _Size, const char *_Func, const char *_File, unsigned int _Line)
{
    MEM* p = ((MEM *)((char *)_Ptr - sizeof(MEM)));
    if ((darray->count + 1) >= darray->size)
    {
        marray_expand(darray, 2);

    }

    darray->data[darray->count] = _Ptr;
    p->_File = _File;
    p->_Func = _Func;
    p->_Line = _Line;
    p->_Size = _Size;
    p->ptr = _Ptr;
    p->timer = GetTickCount();
    darray->count++;
    darray->msize += _Size;
    return TRUE;

}


BOOL marray_shrink(MArray *darray)
{
    if ((darray->count >> 1) < darray->size && (darray->size > DEFAULT_A_SIZE))
    {
        int newallocsize = darray->count + darray->count >> 1;
        void **data = (void **)realloc(darray->data, sizeof(void *) * newallocsize);
        if (data != NULL)
        {
            darray->data = data;
            darray->size = newallocsize;
        }
        return TRUE;
    }
    return FALSE;
}
BOOL marray_delete(MArray *darray, int index)
{
    int i = 0;
    darray->msize -= ((MEM *)((char *)darray->data[index] - sizeof(MEM)))->_Size;
    for (i = index; (i + 1) < darray->count; i++)
    {
        darray->data[i] = darray->data[i + 1];
    }

    darray->count--;

    marray_shrink(darray);

    return TRUE;
}
int marray_find(MArray * darray, void * data) {
    for (size_t i = 0; i < darray->count; i++)
    {
        if (darray->data[i] == data)
            return i;
    }
    return -1;
}

MArray *mem = NULL;
void *zui_malloc(unsigned int _Size, const char *_Func, const char *_File, unsigned int _Line) {
    if (!mem) {
        mem = marray_create();
    }
    EnterCriticalSection(&mem->cs);       // 进入临界区
    void *p = (char *)malloc(_Size + sizeof(MEM) + 8) + sizeof(MEM);
    marray_append(mem, p, _Size, _Func, _File, _Line);
    //写入效验数据
    memcpy((char *)p + _Size, "checkmem", 8);
    LeaveCriticalSection(&mem->cs);       // 离开临界区
    return p;
}
void zui_free(void *_Ptr) {
    if (!_Ptr)
        return;
    EnterCriticalSection(&mem->cs);       // 进入临界区
    //检查效验数据
    if (memcmp((char*)_Ptr + ((MEM*)((char *)_Ptr - sizeof(MEM)))->_Size, "checkmem", 8) == 0) {
        marray_delete(mem, marray_find(mem, _Ptr));
        free((char *)_Ptr - sizeof(MEM));
    }
    else {
        MessageBoxA(NULL, ((MEM*)((char *)_Ptr - sizeof(MEM)))->_Func, "memory write-overflow", MB_ICONWARNING);
    }
    LeaveCriticalSection(&mem->cs);       // 离开临界区
}

void* zui_realloc(void*  _Block, size_t _Size, const char *_Func, const char *_File, unsigned int _Line) {
    if (_Block) {
        EnterCriticalSection(&mem->cs);       // 进入临界区
        MEM *old = (char*)_Block - sizeof(MEM);
        void *new = (char *)zui_malloc(_Size, _Func, _File, _Line);
        memcpy(new, _Block, old->_Size);
        zui_free(_Block);
        LeaveCriticalSection(&mem->cs);       // 离开临界区
        return new;
    }
    else
        return zui_malloc(_Size, _Func, _File, _Line);
}
char* __cdecl zui_strdup(char const* _Source, const char *_Func, const char *_File, unsigned int _Line) {
    size_t  len = strlen(_Source) + 1;
    char *new = zui_malloc(len * 2, _Func, _File, _Line);
    if (new == NULL)
        return NULL;
    new[len-1] = L"\0";
    return (char *)memcpy(new, _Source, len);
}
wchar_t* __cdecl zui_wcsdup(wchar_t const* _String, const char *_Func, const char *_File, unsigned int _Line) {
    size_t  len = wcslen(_String) + 1;
    wchar_t *new = zui_malloc(len * 2, _Func, _File, _Line);
    if (new == NULL)
        return NULL;
    new[len-1] = L"\0";
    return (char *)memcpy(new, _String, len * 2);
}

#endif