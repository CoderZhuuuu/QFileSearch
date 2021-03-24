#include "kmp.h"

int kmpNext[260];

// 适用于通配符的next数组
void getNext(const char * str, const int & strL)
{
	--str;
	int i = 1;
	int j = 0;
	while (i <= strL)
	{
		while (j > 0 && '?' != str[i] && '?' != str[j] && str[i] != str[j])
		{
			j = kmpNext[j];
		}
		kmpNext[i] = j;
		++i;
		++j;
	}
}

int kmp(const char * src, const int & sL, const char * pat, const int & pL, int i)
{
	--src, --pat;
	++i;
	int pos = -1;
	int j = 1;
	while (i <= sL)
	{
		while (j > 0 && '?' != pat[j] && src[i] != pat[j])
		{
			j = kmpNext[j];
		}
		if (j == pL)
		{
			pos = i - pL;
			break;
		}
		++i;
		++j;
	}
	return pos;
}

// 通配符匹配
bool wildcardMatch(const char * src, int sL, const char * pat, int pL)
{
	bool star = false;
	int i = 0;
	bool matched = true;
	// 检查是否包含 "*"
	for (i = 0; i < pL; ++i)
	{
		if (pat[i] == '*')
		{
			star = true;
			break;
		}
	}
	// 如果没有 "*"
	if (!star)
	{
		// 如果没有星号并且模式串与原字符串长度不一样则肯定不匹配
		if (pL != sL)
		{
			matched = false;
		}
		else
		{
			// 顺序比较(? 匹配所有的，直接跳过)
			for (i = 0; i < pL && '?' != pat[i]; ++i)
			{
				if (pat[i] != src[i])
				{
					matched = false;
					break;
				}
			}
		}
	}
	else
	{
		int left = 0, right = 0;
		// 顺序
		for (i = 0; pat[i] != '*'; ++i)
		{
			if ((pat[i] != '?' && pat[i] != src[i]) || sL <= i)
			{
				matched = false;
				break;
			}
			left++;
		}
		if (matched)
		{
			// 逆序
			for (i = 0; pat[pL - 1 - i] != '*'; ++i)
			{
				if ((pat[pL - 1 - i] != '?' && pat[pL - 1 - i] != src[sL - 1 - i]) || sL <= i)
				{
					matched = false;
					break;
				}
				right++;
			}
			if (matched)
			{
				if (sL < left + right)
				{
					matched = false;
				}
				else
				{
					int patPos, srcPos;
					patPos = srcPos = left;
					// 从左边开始匹配
					for (i = left; i < pL - right; ++i)
					{
						// 如果是 "*"
						if (pat[i] == '*')
						{
							// 匹配patPos到i之间的字符串
							int patLen = i - patPos;
							if (patLen > 0)
							{
								getNext(pat + patPos, patLen);
								int pos = kmp(src, sL - right, pat + patPos, patLen, srcPos);
								if (pos == -1)
								{
									matched = false;
									break;
								}
								srcPos = pos + patLen;
							}
							patPos = i + 1;
						}
					}
				}
			}
		}
	}
	return matched;
}