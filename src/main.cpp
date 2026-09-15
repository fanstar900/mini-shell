#include<iostream>
#include "shell.h"
#include "signals.h"

int main(){

    setupSignalHandlers();
    Shell shell ; 
    shell.run() ; 
    return 0 ; 
}