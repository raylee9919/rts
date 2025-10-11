#ifndef RTS_DWRITE_H
#define RTS_DWRITE_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// # Note: "dwrite_2.h" minimum: Windows 8.1
//         "dwrite_3.h" minimum: Windows 10 Build 16299
#include <dwrite_3.h>

#pragma comment(lib, "dwrite")

struct Dwrite_Text_Analysis_Source final : IDWriteTextAnalysisSource
{
    Dwrite_Text_Analysis_Source(const wchar_t* locale, const wchar_t* text, const UINT32 textLength) noexcept : _locale{locale}, _text{text}, _text_length{textLength}
    { }

    ULONG STDMETHODCALLTYPE AddRef() noexcept override
    { return 1; }

    ULONG STDMETHODCALLTYPE Release() noexcept override
    { return 1; }

    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObject) noexcept override
    {
        if (IsEqualGUID(riid, __uuidof(IDWriteTextAnalysisSource))) 
        {
            *ppvObject = this;
            return S_OK;
        }

        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE GetTextAtPosition(UINT32 textPosition, const WCHAR** textString, UINT32* textLength) noexcept override
    {
        textPosition = min(textPosition, _text_length);
        *textString = _text + textPosition;
        *textLength = _text_length - textPosition;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTextBeforePosition(UINT32 textPosition, const WCHAR** textString, UINT32* textLength) noexcept override
    {
        textPosition = min(textPosition, _text_length);
        *textString = _text;
        *textLength = textPosition;
        return S_OK;
    }

    DWRITE_READING_DIRECTION STDMETHODCALLTYPE GetParagraphReadingDirection() noexcept override
    {
        return DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    }

    HRESULT STDMETHODCALLTYPE GetLocaleName(UINT32 textPosition, UINT32* textLength, const WCHAR** localeName) noexcept override
    {
        *textLength = _text_length - textPosition;
        *localeName = _locale;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetNumberSubstitution(UINT32 textPosition, UINT32* textLength, IDWriteNumberSubstitution** numberSubstitution) noexcept override
    {
        return E_NOTIMPL;
    }

    const WCHAR* _locale;
    const WCHAR* _text;
    const UINT32 _text_length;
};

struct Dwrite_Text_Analysis_Sink_Result 
{
    Dwrite_Text_Analysis_Sink_Result *next;

    UINT32 text_position;
    UINT32 text_length;
    DWRITE_SCRIPT_ANALYSIS analysis;
};

// DirectWrite uses an IDWriteTextAnalysisSink to inform the caller of its segmentation results. The most important part are the
// DWRITE_SCRIPT_ANALYSIS results which inform the remaining steps during glyph shaping what script ("language") is used in a piece of text.
struct Dwrite_Text_Analysis_Sink final : IDWriteTextAnalysisSink 
{
    Arena *arena;
    Dwrite_Text_Analysis_Sink_Result *result_first;
    Dwrite_Text_Analysis_Sink_Result *result_last;


    ULONG STDMETHODCALLTYPE AddRef() noexcept override
    { return 1; } 

    ULONG STDMETHODCALLTYPE Release() noexcept override
    { return 1; }

    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObject) noexcept override
    {
        if (IsEqualGUID(riid, __uuidof(IDWriteTextAnalysisSink))) 
        {
            *ppvObject = this;
            return S_OK;
        }

        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE SetScriptAnalysis(UINT32 textPosition, UINT32 textLength, const DWRITE_SCRIPT_ANALYSIS* scriptAnalysis) noexcept override
    {
        Dwrite_Text_Analysis_Sink_Result *result = push_struct(arena, Dwrite_Text_Analysis_Sink_Result);
        {
            result->text_position = textPosition;
            result->text_length   = textLength;
            result->analysis      = *scriptAnalysis;
        }

        sll_push_back(result_first, result_last, result);

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetLineBreakpoints(UINT32 textPosition, UINT32 textLength, const DWRITE_LINE_BREAKPOINT* lineBreakpoints) noexcept override
    { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE SetBidiLevel(UINT32 textPosition, UINT32 textLength, UINT8 explicitLevel, UINT8 resolvedLevel) noexcept override
    { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE SetNumberSubstitution(UINT32 textPosition, UINT32 textLength, IDWriteNumberSubstitution* numberSubstitution) noexcept override
    { return E_NOTIMPL; }
};

struct Dwrite_Map_Complexity_Result
{
    UINT16 *glyph_indices;
    UINT32 mapped_length;
    BOOL is_simple;
};

struct Dwrite_Font_Fallback_Result
{
    u32 length;
    IDWriteFontFace5 *face;
};



struct Fp_Run
{
    Fp_Run *next;
    Fp_Run *prev;

    IDWriteFontFace5 *face;
    f32               font_size;
    u32               count;
    u16              *indices;
    f32              *advances;
};

struct Fp_Glyph
{
    Fp_Glyph *first;
    Fp_Glyph *last;
    Fp_Glyph *next;
    Fp_Glyph *prev;

    u16 index;
    f32 lsb;
    f32 rsb;
    f32 tsb;
    f32 bsb;
    v2 uv_min;
    v2 uv_max;
};

struct Fp_Atlas
{
    Render_Id         id;
    u8               *data;
    u32               width;
    u32               height;
    u32               pitch;
    Rpk_Context      *rpk_ctx;
    b32               dirty;
};

struct Fp_Font
{
    Fp_Font         *first;
    Fp_Font         *last;
    Fp_Font         *next;
    Fp_Font         *prev;

    IDWriteFontFace *face;

    // @Note: If font size is changed, those below should be cleared.
    f32              font_size;
    f32              ascent;
    f32              descent;
    f32              linegap;
    Arena           *arena;
    Fp_Atlas         atlas;
    Fp_Glyph        *glyph_table;
    u64              glyph_table_size;
};

struct Fp_State
{
    Arena                           *arena;

    f32                             dpi;
    WCHAR                           locale[LOCALE_NAME_MAX_LENGTH];

    IDWriteFactory5                 *factory;

    IDWriteInMemoryFontFileLoader   *in_memory_font_file_loader;

    IDWriteFontCollection           *system_font_collection;
    IDWriteFontFallback             *system_font_fallback;
    IDWriteFontFallback1            *system_font_fallback1;

    IDWriteTextAnalyzer             *text_analyzer;
    IDWriteTextAnalyzer1            *text_analyzer1;

    IDWriteRenderingParams          *rendering_params;

    Fp_Font                         *font_table;
    u64                              font_table_size;

    Arena                           *run_arena;
};
global Fp_State *fp_state;

struct Fp_Draw_String_Result
{
    AABB2 aabb;
    f32 max_ascent;
    f32 max_descent;
};

internal Fp_Draw_String_Result fp_draw_string(Utf8 string, Utf8 base_family, f32 font_size, v2 origin, Render_String_Flags flags, AABB2 cull_aabb = aabb2_infinite());

#endif // RTS_DWRITE_H
