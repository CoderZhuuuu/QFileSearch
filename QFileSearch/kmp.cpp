#include "kmp.h"

int kmpNext[260];

// ������ͨ�����next����
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

// ͨ���ƥ��
bool wildcardMatch(const char * src, int sL, const char * pat, int pL)
{
	bool star = false;
	int i = 0;
	bool matched = true;
	// ����Ƿ���� "*"
	for (i = 0; i < pL; ++i)
	{
		if (pat[i] == '*')
		{
			star = true;
			break;
		}
	}
	// ���û�� "*"
	if (!star)
	{
		// ���û���ǺŲ���ģʽ����ԭ�ַ������Ȳ�һ����϶���ƥ��
		if (pL != sL)
		{
			matched = false;
		}
		else
		{
			// ˳��Ƚ�(? ƥ�����еģ�ֱ������)
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
		// ˳��
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
			// ����
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
					// ����߿�ʼƥ��
					for (i = left; i < pL - right; ++i)
					{
						// ����� "*"
						if (pat[i] == '*')
						{
							// ƥ��patPos��i֮����ַ���
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