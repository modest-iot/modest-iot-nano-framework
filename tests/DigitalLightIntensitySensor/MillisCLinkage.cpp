extern unsigned long fakeMillis;

extern "C" unsigned long millis() {
    return fakeMillis;
}