#include "fasatomve.h"

int main(int argc, char** argv){
    FTM ftm;
    ftm.loadFASA(argv[1]);
    ftm.writeMVE(argv[2]);
    return 0;
}
