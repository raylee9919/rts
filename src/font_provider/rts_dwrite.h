#ifndef RTS_DWRITE_H
#define RTS_DWRITE_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

//----------------------------------------------
// @Note: 
// "dwrite_2.h" minimum: Windows 8.1
// "dwrite_3.h" minimum: Windows 10 Build 16299

#include <dwrite_3.h>

#pragma comment(lib, "dwrite.lib")

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

    const wchar_t* _locale;
    const wchar_t* _text;
    const UINT32 _text_length;
};

struct Dwrite_Run
{
    Dwrite_Run *next;

    DWRITE_GLYPH_RUN e;
};

struct Dwrite_Unit
{
    Dwrite_Unit *next;

    Arena       *arena;
    Dwrite_Run  *run_first;
    Dwrite_Run  *run_last;
    u32         run_count;
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


struct Dwrite_Map_Characters_Result 
{
    IDWriteFontFace5 *mapped_font_face;
    UINT32 mapped_length;
};

struct Dwrite_Map_Complexity_Result
{
    UINT16 *glyph_indices;
    UINT32 mapped_length;
    BOOL is_simple;
};

struct Glyph_Cel
{
    b32 is_empty;
    v2  uv_min;
    v2  uv_max;
    f32 width_px;
    f32 height_px;
    v2  offset_px; // offset of a pen from baseline origin of a glyph in px.
};
typedef Dynamic_Array(Glyph_Cel) Glyph_Cel_Array;

struct Dwrite_Font_Metrics
{
    f32 du_per_em;
    f32 advance_height_px;
};

struct Dwrite_Font_Fallback_Result
{
    u32 length;
    IDWriteFontFace5 *font_face;
};

struct Dwrite_Glyph_Table_Entry
{
    b8 occupied;
    b8 tombstone;
    UINT16      idx;
    Glyph_Cel   cel;
};

struct Dwrite_Glyph_Table
{
    u32 entry_count;
    Dwrite_Glyph_Table_Entry *entries;
};

struct Dwrite_Font_Table_Entry
{
    b8 occupied;
    b8 tombstone;
    IDWriteFontFace *key; // = 
    Dwrite_Font_Metrics metrics;
    Dwrite_Glyph_Table glyph_table;
};

struct Dwrite_Font_Table
{
    u32 entry_count;
    Dwrite_Font_Table_Entry *entries;
};

struct Dwrite_State
{
    Arena *arena;

    IDWriteFactory3         *factory;
    IDWriteFontCollection   *font_collection;
    IDWriteFontFallback     *font_fallback;
    IDWriteFontFallback1    *font_fallback1;
    IDWriteTextAnalyzer     *text_analyzer;
    IDWriteTextAnalyzer1    *text_analyzer1;

    wchar_t locale[LOCALE_NAME_MAX_LENGTH]; // @Todo: safe?
    IDWriteRenderingParams *rendering_params;

    Dwrite_Font_Table font_table;

    Arena       *unit_arena;
    Dwrite_Unit *first_free_unit;
    Dwrite_Unit *last_free_unit;
};

struct Dwrite_Get_Base_Font_Family_Index_Result
{
    u32 index;
    b32 exists;
};

// -------------------------------------
// @Note: Code
internal u64 dwrite_hash_glyph_index(u16 idx);
internal Dwrite_Glyph_Table_Entry *dwrite_get_glyph_entry_from_table(Dwrite_Glyph_Table glyph_table, u16 glyph_index);
internal void dwrite_insert_glyph_cel_to_table(Dwrite_Glyph_Table *glyph_table, u16 glyph_index, Glyph_Cel cel);
internal u64 dwrite_hash_font(IDWriteFontFace *key);
internal Dwrite_Font_Table_Entry *dwrite_get_entry_from_font_table(IDWriteFontFace *font_face);
internal void dwrite_insert_font_to_table(IDWriteFontFace *font_face, Dwrite_Font_Metrics metrics);
internal Dwrite_Map_Complexity_Result dwrite_map_complexity(IDWriteTextAnalyzer1 *text_analyzer, IDWriteFontFace *font_face, WCHAR *text, u32 text_length);
internal Dwrite_Font_Fallback_Result dwrite_font_fallback(IDWriteFontFallback *font_fallback, IDWriteFontCollection *font_collection, WCHAR *base_family, WCHAR *locale, WCHAR *text, UINT32 text_length);
internal void dwrite_abort(wchar_t *message);
internal void dwrite_init(void);
internal Dwrite_Get_Base_Font_Family_Index_Result dwrite_get_base_font_family_index(wchar_t *base_font_family_name);
internal Dwrite_Unit *dwrite_alloc_unit(void);
internal void dwrite_release_unit(Dwrite_Unit *unit);
internal Dwrite_Unit *dwrite_map_text_to_glyphs(IDWriteFontFallback1 *font_fallback, IDWriteFontCollection *font_collection, IDWriteTextAnalyzer1 *text_analyzer, WCHAR *locale, WCHAR *base_family, FLOAT pt_per_em, FLOAT px_per_inch, WCHAR *text, u32 text_length);

global Dwrite_State dwrite;

#endif // RTS_DWRITE_H
