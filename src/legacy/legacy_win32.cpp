/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



// -------------------------------------------------------------
// @Note: Good old times when everything was software rendered.


#if 0

struct Win32ScreenBuffer 
{
    BITMAPINFO bitmap_info;
    void *memory;
    int width;
    int height;
    int bpp;
};

internal void
win32_resize_dib_section(Win32ScreenBuffer *screen_buffer, int width, int height) 
{
#if 0
    if (screen_buffer->memory) 
        VirtualFree(g_screen_buffer.memory, 0, MEM_RELEASE);
#endif

    screen_buffer->width = width;
    screen_buffer->height = height;
    screen_buffer->bpp = 4;

#if 0
    BITMAPINFOHEADER *header = &screen_buffer->bitmap_info.bmiHeader;
    header->biWidth = width;
    header->biHeight = height;
    header->biSize = sizeof(screen_buffer->bitmap_info.bmiHeader);
    header->biWidth = width;
    header->biHeight = -height;
    header->biPlanes = 1;
    header->biBitCount = 32;
    header->biCompression = BI_RGB;

    screen_buffer->memory = VirtualAlloc(0, width * height * screen_buffer->bpp,
                                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#endif
}
#endif

#if 0
internal void 
win32_update_screen(HDC hdc, int windowWidth, int windowHeight) 
{
    // NOTE: For debugging purpose
    StretchDIBits(hdc,
#if 0
                  0, 0, g_screen_buffer.width, g_screen_buffer.height,
#else
                  0, 0, windowWidth, windowHeight,
#endif
                  0, 0, g_screen_buffer.width, g_screen_buffer.height,
                  g_screen_buffer.memory, &g_screen_buffer.bitmap_info, DIB_RGB_COLORS, SRCCOPY);
}

#endif



#if 0
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#else
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

#if 0
internal b32 
win32_find_chunk(Entire_File *entire_file, DWORD fourcc, u32 *chunk_size, u32 *chunk_data_position)
{
    b32 result = true;

    u32 chunk_type;
    u32 chunk_data_size;
    u32 riff_data_size = 0;
    u32 file_type;
    u32 bytesRead = 0;
    u32 offset = 0;

    u8 *at = (u8 *)entire_file->contents;

    for (;;)
    {
        chunk_type = *(u32 *)at;
        at += sizeof(u32);

        chunk_data_size = *(u32 *)at;
        at += sizeof(u32);

        switch (chunk_type)
        {
            case fourccRIFF:
            {
                riff_data_size = chunk_data_size;
                chunk_data_size = 4;
                file_type = *(u32 *)at;
                at += sizeof(u32);
            } break;

            default:
            {
                at += chunk_data_size;
            } break;
        }

        offset += sizeof(u32) * 2;

        if (chunk_type == fourcc)
        {
            *chunk_size = chunk_data_size;
            *chunk_data_position = offset;
            result = false;
            break;
        }

        offset += chunk_data_size;

        if (bytesRead >= riff_data_size)
        {
            result = false;
            break;
        }
    }

    return result;
}

internal void
win32_read_chunk_data(Entire_File *entire_file, void *buffer, u32 buffer_size, DWORD buffer_offset)
{
    u8 *src = (u8 *)entire_file->contents;
    src += buffer_offset;

    u8 *dst = (u8 *)buffer;
    for (u32 i = 0; i < buffer_size; ++i)
        *dst++ = *src++;
}

internal HRESULT
win32_init_audio() {
    HRESULT hr;

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
        Assert(!"coinit failed"); // @Todo: error-handling

    IXAudio2 *xaudio = 0;
    if (FAILED(hr = XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR)))
        Assert(!"XAudio init failed"); // @Todo: error-handling

    IXAudio2MasteringVoice *master_voice = 0;
    if (FAILED(hr = xaudio->CreateMasteringVoice(&master_voice)))
        Assert(!"master voice creation failed"); // @Todo: error-handling

    WAVEFORMATEXTENSIBLE wfx = {};
    XAUDIO2_BUFFER buffer = {};

    const char *audio_file_name = "audio/requiem.wav";
    Entire_File entire_file = win32_read_entire_file(audio_file_name);

    u32 chunk_size;
    u32 chunk_position;
    u32 filetype;
    //check the file type, should be fourccWAVE or 'XWMA'
    win32_find_chunk(&entire_file, fourccRIFF, &chunk_size, &chunk_position);
    win32_read_chunk_data(&entire_file, &filetype, sizeof(u32), chunk_position);
    Assert(filetype == fourccWAVE);

    win32_find_chunk(&entire_file, fourccFMT, &chunk_size, &chunk_position);
    win32_read_chunk_data(&entire_file, &wfx, chunk_size, chunk_position);

    //fill out the audio data buffer with the contents of the fourccDATA chunk
    win32_find_chunk(&entire_file, fourccDATA, &chunk_size, &chunk_position);
    u8 *data_buffer = (u8 *)VirtualAlloc(0, chunk_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    win32_read_chunk_data(&entire_file, data_buffer, chunk_size, chunk_position);

    buffer.AudioBytes = chunk_size;  //size of the audio buffer in bytes
    buffer.pAudioData = data_buffer;  //buffer containing audio data
    buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

    IXAudio2SourceVoice* src_voice;
    if (!FAILED(hr = xaudio->CreateSourceVoice(&src_voice, (WAVEFORMATEX *)&wfx))) {
        if (!FAILED(hr = src_voice->SubmitSourceBuffer(&buffer))) {
            if (!FAILED(hr = src_voice->Start(0))) {
                src_voice->SetVolume(0.05f);
            } else {
            }
        } else {
        }
    } else {
    }

    return hr;
}
#endif



#if 0
DWORD result;    
for (DWORD idx = 0; idx < XUSER_MAX_COUNT; idx++) 
{
    XINPUT_STATE xinput_state;
    zero_struct(&xinput_state);
    result = xinput_get_state(idx, &xinput_state);
    win32_xinput_handle_deadzone(&xinput_state);
    if (result == ERROR_SUCCESS) {

    } 
    else {
        // Todo: Diagnostic
    }
}
#endif


internal void
win32_xinput_handle_deadzone(XINPUT_STATE *state) 
{
#define XINPUT_DEAD_ZONE 2500
    f32 lx = state->Gamepad.sThumbLX;
    f32 ly = state->Gamepad.sThumbLY;

    if (sqrt(lx*lx + ly*ly) < XINPUT_DEAD_ZONE) 
    {
        state->Gamepad.sThumbLX = 0;
        state->Gamepad.sThumbLY = 0;
    }
    f32 rx = state->Gamepad.sThumbRX;
    f32 ry = state->Gamepad.sThumbRY;
    if (sqrt(rx*rx + ry*ry) < XINPUT_DEAD_ZONE) 
    {
        state->Gamepad.sThumbRX = 0;
        state->Gamepad.sThumbRY = 0;
    }
}

