#include "ACC.h"
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <bitset>
#include <unistd.h>

using namespace std;

int main(int argn, char * argv[])
{
	ACC acc;

	if(argn > 1) acc.versionCheck(true);
	else         acc.versionCheck(false);
}
