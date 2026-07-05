// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_THREAD_PRIMITIVE_H
#define RTS_THREAD_PRIMITIVE_H



internal void csection_init(Critical_Section* csection);
internal void csection_lock(Critical_Section* csection);
internal void csection_unlock(Critical_Section* csection);
internal void csection_destroy(Critical_Section* csection);


// @Todo: Should I use OS native mutex? SRW Lock?
struct Mutex {
    Critical_Section csection;
};

internal void mutex_init(Mutex* m);
internal void mutex_lock(Mutex* m);
internal void mutex_unlock(Mutex* m);
internal void mutex_destroy(Mutex* m);












#endif // RTS_THREAD_PRIMITIVE_H
