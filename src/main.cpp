#include <iostream>
#include "vulkanohno.h"

int main() {

    VulkanOhNo vkohno;

    vkohno.init();

    vkohno.run();

    vkohno.cleanup();
    return 0;
}