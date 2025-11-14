#pragma once
#include <string.h>
#include <string>


char *strnstr(const char *haystack, const char *needle, size_t n) {
	if (!needle || !*needle) {
		return (char *)haystack; // Empty needle always matches at the beginning
	}
	if (!haystack) {
		return nullptr; // Cannot search in a NULL haystack
	}
	size_t nNeedle = strlen(needle);
	for (size_t i = 0; i < n; ++i) {
		size_t nMatch = (nNeedle <= n - i) ? nNeedle : n - i;
		if (strncmp(&haystack[i], needle, nMatch) == 0) {
			return (char *)&haystack[i];
		}
	}
	return nullptr;
}

bool FindElement(const char *str, size_t nChars, const char *name, const char **begin, const char **end) {
	bool result = false;
	size_t nNameChars = strlen(name);
	char *token = new char[nNameChars + 4];
	strcpy(&token[1], name);
	token[0] = '<';
	token[nNameChars + 1] = '>';
	token[nNameChars + 2] = '\0';
	*begin = strnstr(str, token, nChars);
	if (*begin) {
		*begin += nNameChars + 2;
		strcpy(&token[2], name);
		token[1] = '/';
		token[nNameChars + 2] = '>';
		token[nNameChars + 3] = '\0';
		*end = strnstr(*begin, token, nChars - (*begin - str));
		if (*end) {
			result = true;
		}
	}
	delete[] token;
	return result;
}

std::string GetElementText(const char *str, size_t nChars, const char *name) {
	const char *begin, *end;
	if (FindElement(str, nChars, name, &begin, &end)) {
		return std::string(begin, end);
	}
	return "";
}

std::string GetStringAttribute(const char *str, size_t nChars, const char *element, const char *attr) {
	if (element) {
		bool bFoundElem = false;
		size_t nElemChars = strlen(element);
		char *token = new char[nElemChars + 2];
		strcpy(&token[1], element);
		token[0] = '<';
		token[nElemChars + 1] = '\0';
		while (nChars != 0) {
			str = strnstr(str, token, nChars);
			if (!str) break;
			str += nElemChars + 1;
			nChars -= nElemChars + 1;
			if (isalpha(*str) || *str == '-') continue;
			while (nChars != 0 && isspace(*str)) {
				str++;
				nChars--;
			}
			if (nChars != 0) {
				if (isalpha(*str)) {
					bFoundElem = true;
				}
				break;
			}
		}
		delete[] token;
		if (!bFoundElem) {
			return "";
		}
	}

	enum { W_ATTR, ATTR, W_EQ, W_OQ, VAL, STR, ESC, FIN } state = W_ATTR;
	bool bAttrMatch;
	char quote;
	const char *begin, *end;
	std::string result;

	for (; state != FIN && nChars != 0; str++, nChars--) {
		switch (state) {
		case W_ATTR:
			if (isspace(*str)) continue;
			if (isalpha(*str)) {
				begin = str;
				bAttrMatch = false;
				state = ATTR;
				break;
			}
			state = FIN;
			break;

		case ATTR:
			if (isalpha(*str) || *str == '-') continue;
			if (*str == '=' || isspace(*str)) {
				end = str;
				if (std::string(begin, end) == attr) {
					bAttrMatch = true;
					result.clear();
				}
				state = (*str == '=') ? W_OQ : W_EQ;
				break;
			}
			state = FIN;
			break;

		case W_EQ:
			if (isspace(*str)) continue;
			if (*str == '=') {
				state = W_OQ;
				break;
			}
			if (isalpha(*str)) {
				begin = str;
				bAttrMatch = false;
				state = ATTR;
				break;
			}
			state = FIN;
			break;

		case W_OQ:
			if (isspace(*str)) continue;
			if (*str == '"' || *str == '\'') {
				quote = *str;
				state = STR;
				break;
			}
			if (isdigit(*str) || *str == '.') {
				state = VAL;
				break;
			}
			state = FIN;
			break;

		case VAL:
			if (isdigit(*str) || *str == '.') continue;
			if (isspace(*str)) {
				state = W_ATTR;
				break;
			}
			state = FIN;
			break;

		case STR:
			if (*str == quote) {
				if (bAttrMatch) {
					return result;
				}
				state = W_ATTR;
				break;
			}
			if (*str == '\\') {
				state = ESC;
				break;
			}
			result += *str;
			break;

		case ESC:
			state = STR;
			break;
		}
	}

	return "";
}
