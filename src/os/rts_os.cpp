// Copyright Seong Woo Lee. All Rights Reserved.

#if OS_WINDOWS
#  include "os/win32/rts_os_win32.cpp"
#else
#  error Undefined OS
#endif


internal
OS_INIT(os_init)
{
#if OS_WINDOWS
    os_win32_init();
#else
#  error Undefined OS
#endif
}
