// Copyright Seong Woo Lee. All Rights Reserved.

void mutex_init(Mutex* m) {
    csection_init(&m->csection);
}

void mutex_lock(Mutex* m) {
    csection_lock(&m->csection);
}

void mutex_unlock(Mutex* m) {
    csection_unlock(&m->csection);
}

void mutex_destroy(Mutex* m) {
    csection_destroy(&m->csection);
}
