#include <stdio.h>
#include <stdlib.h>
#include <comp421/hardware.h>

int main () {

    TracePrintf(1, "ran idle");
    while(true) {

        Pause(); 
    }
}