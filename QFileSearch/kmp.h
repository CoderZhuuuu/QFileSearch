#pragma once
#include "Global.h"
using namespace std;

// Í¨Åä·ûËÑË÷
void getNext(const char *, const int &);
int kmp(const char *, const int &, const char *, const int &, int);
bool wildcardMatch(const char *, int, const char *, int);