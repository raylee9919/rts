// Copyright Seong Woo Lee. All Rights Reserved.

void init(Mutex* m) {
    csection_init(&m->csection);
}

void lock(Mutex* m) {
    csection_lock(&m->csection);
}

void unlock(Mutex* m) {
    csection_unlock(&m->csection);
}

void destroy(Mutex* m) {
    csection_destroy(&m->csection);
}
