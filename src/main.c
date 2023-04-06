#include <mes.h>
#include <gpu.h>

uint8_t start(void) {
    char* helloworld = "Hello, World!";
    gpu_blank(FRONT_BUFFER, 0);
    gpu_print_text(FRONT_BUFFER, 1, 1, 1, 0, helloworld);
    return CODE_FREEZEFRAME;
}
