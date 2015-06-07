/*
	Parse Adobe - Parses the Adobe DB file from users.tar.gz.
	Copyright (C) 2013 Steve Thomas <steve AT tobtu DOT com>

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <string.h>

typedef unsigned char uint8_t;
typedef unsigned int  uint32_t;

int base64ToHex(const void *data, char *out, int length)
{
	const uint8_t base64[80] =
		{
			                                            62, -1, -1, -1, 63,
			52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1,
			-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
			15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
			-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
			41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
		};
	const char hex[] = "0123456789abcdef";

	if (((const char*) data)[length - 1] == '=')
	{
		length--;
	}
	if (((const char*) data)[length - 1] == '=')
	{
		length--;
	}
	if (length % 4 == 1)
	{
		return -1;
	}
	for (int i = 0; i < length; i += 4)
	{
		// Get 1st byte of output
		uint32_t ch1 = ((const uint8_t*) data)[i    ] - 43;
		uint32_t ch2 = ((const uint8_t*) data)[i + 1] - 43;
		if (ch1 > 79 || base64[ch1] == 255 ||
		    ch2 > 79 || base64[ch2] == 255)
		{
			return -1;
		}
		ch1 = base64[ch1];
		ch2 = base64[ch2];
		ch1 = ((ch1 << 2) | (ch2 >> 4)) & 0xff;
		out[0] = hex[ch1 >> 4];
		out[1] = hex[ch1 & 0xf];
		out += 2;

		// Get 2nd byte of output
		if (i + 2 >= length)
		{
			if ((ch2 & 0x0f) != 0)
			{
				return -1;
			}
			break;
		}
		uint32_t ch3 = ((const uint8_t*) data)[i + 2] - 43;
		if (ch3 > 79 || base64[ch3] == 255)
		{
			return -1;
		}
		ch3 = base64[ch3];
		ch2 = ((ch2 << 4) | (ch3 >> 2)) & 0xff;
		out[0] = hex[ch2 >> 4];
		out[1] = hex[ch2 & 0xf];
		out += 2;

		// Get 3rd byte of output
		if (i + 3 >= length)
		{
			if ((ch3 & 0x03) != 0)
			{
				return -1;
			}
			break;
		}
		uint32_t ch4 = ((const uint8_t*) data)[i + 3] - 43;
		if (ch4 > 79 || base64[ch4] == 255)
		{
			return -1;
		}
		ch4 = base64[ch4];
		ch3 = ((ch3 << 6) | ch4) & 0xff;
		out[0] = hex[ch3 >> 4];
		out[1] = hex[ch3 & 0xf];
		out += 2;
	}
	return 6 * ((length - 1) / 4) + 2 * ((length + 3) % 4);
}

int readRecord(FILE *fin, char *out)
{
	size_t len = 0;

	while (len < 1023)
	{
		if (fgets(out + len, 1024 - len, fin) == NULL)
		{
			return 0 - (int) len; // file error / eof
		}
		len += strlen(out + len);

		// Remove \r and \n
		while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r'))
		{
			len--;
		}

		// Compare end of line with "|--"
		if (len > 2 && memcmp(out + (len - 3), "|--", 3) == 0)
		{
			return (int) len;
		}
	}
	return -1023; // line too long (this should not happen since I made sure the buffer is large enough)
}

int getField(const char *record, int start, int len)
{
	for (start += 2; start < len; start++)
	{
		// Cheating there is only one record that this doesn't work on but it's good enough
		// It just means that one email address will have "|-" in front of it

		// "141660212-|--|-|--mr***********0@mail.com-|-EQ********Q=-|-|--"

		// It will be parsed as:
		// "141660212"
		// ""
		// "|--mr***********0@mail.com"
		// "EQ********Q="
		// ""

		// but should be like this (I believe):
		// "141660212"
		// "-|"
		// "-mr***********0@mail.com"
		// "EQ********Q="
		// ""
		if (memcmp(record + (start - 2), "-|-", 3) == 0)
		{
			return start + 1;
		}
	}
	return -1;
}

/**
 * ./parse [in-file [out-file [fields]]
 *
 * Default fields is 000100 (output password in hex)
 *   100000 output id
 *   010000 output name
 *   001000 output email
 *   000100 output password in hex
 *   000200 output password in base64
 *   000010 output hint
 *   000001 all fields required
 *
 * Examples:
 *   Outputs hex password
 *   ./parse < cred > cred-out
 *   ./parse cred cred-out
 *
 *   Output email addresses only
 *   ./parse cred cred-out 001000
 *
 *   Output email addresses and base64 passwords
 *   ./parse cred cred-out 001200
 *
 *   Output hex passwords and hints
 *   ./parse cred cred-out 000110
 */
int main(int argc, char *argv[])
{
	char  record[1024];
	char  buffer[1024];
	FILE *fin  = stdin;
	FILE *fout = stdout;
	int   len;
	bool  outId    = false;
	bool  outName  = false;
	bool  outEmail = false;
	bool  outPw    = true;
	bool  outPwHex = true;
	bool  outHint  = false;
	bool  allFieldsRequired = false;

	// *** Set up ***
	// File in
	if (argc > 1)
	{
		if (argv[1][0] == '-' && argv[1][1] == 0)
		{
			fin = stdin;
		}
		else
		{
			fin = fopen(argv[1], "r");
			if (fin == NULL)
			{
				perror(argv[1]);
				return 1;
			}
		}
	}

	// File out
	if (argc > 2)
	{
		if (argv[2][0] == '-' && argv[2][1] == 0)
		{
			fout = stdout;
		}
		else
		{
			fout = fopen(argv[2], "wb");
			if (fout == NULL)
			{
				perror(argv[2]);
				return 1;
			}
		}
	}

	// Extras
	if (argc > 3)
	{
		outPw = false;
		if (argv[3][0] != 0)
		{
			// id
			if (argv[3][0] == '1')
			{
				outId = true;
			}
			if (argv[3][1] != 0)
			{
				// name
				if (argv[3][1] == '1')
				{
					outName = true;
				}
				if (argv[3][2] != 0)
				{
					// email
					if (argv[3][2] == '1')
					{
						outEmail = true;
					}
					if (argv[3][3] != 0)
					{
						// pw
						if (argv[3][3] == '2')
						{
							outPw = true;
							outPwHex = false;
						}
						if (argv[3][3] == '1')
						{
							outPw = true;
						}
						if (argv[3][4] != 0)
						{
							// hint
							if (argv[3][4] == '1')
							{
								outHint = true;
							}
							// all fields required
							if (argv[3][5] == '1')
							{
								allFieldsRequired = true;
							}
						}
					}
				}
			}
		}
		if (!(outId || outName || outEmail || outPw || outHint))
		{
			outPw = true;
		}
	}

	// *** Process file ***
	len = readRecord(fin, record);
	while (len > 0)
	{
		int  recordPos, start = 0;
		int  bufferPos = 0;
		int  fieldLen;
		bool printTab = false;
		bool notEmpty = false;
		bool dontPrint = false;

		// ID
		recordPos = getField(record, start, len);
		if (recordPos < 0)
		{
			break;
		}
		if (outId)
		{
			fieldLen = recordPos - start - 3;
			if (fieldLen > 0)
			{
				notEmpty = true;
				memcpy(buffer + bufferPos, record + start, fieldLen);
				bufferPos += fieldLen;
			}
			else if (allFieldsRequired)
			{
				dontPrint = true;
			}
			printTab = true;
		}

		// Name
		start = recordPos;
		recordPos = getField(record, start, len);
		if (recordPos < 0)
		{
			break;
		}
		if (outName)
		{
			if (printTab)
			{
				buffer[bufferPos] = '\t';
				bufferPos++;
			}
			fieldLen = recordPos - start - 3;
			if (fieldLen > 0)
			{
				notEmpty = true;
				memcpy(buffer + bufferPos, record + start, fieldLen);
				bufferPos += fieldLen;
			}
			else if (allFieldsRequired)
			{
				dontPrint = true;
			}
			printTab = true;
		}

		// EMail
		start = recordPos;
		recordPos = getField(record, start, len);
		if (recordPos < 0)
		{
			break;
		}
		if (outEmail)
		{
			if (printTab)
			{
				buffer[bufferPos] = '\t';
				bufferPos++;
			}
			fieldLen = recordPos - start - 3;
			if (fieldLen > 0)
			{
				notEmpty = true;
				memcpy(buffer + bufferPos, record + start, fieldLen);
				bufferPos += fieldLen;
			}
			else if (allFieldsRequired)
			{
				dontPrint = true;
			}
			printTab = true;
		}

		// Password
		start = recordPos;
		recordPos = getField(record, start, len);
		if (recordPos < 0)
		{
			break;
		}
		if (outPw)
		{
			if (printTab)
			{
				buffer[bufferPos] = '\t';
				bufferPos++;
			}
			fieldLen = recordPos - start - 3;
			if (fieldLen > 0)
			{
				notEmpty = true;
				if (outPwHex)
				{
					fieldLen = base64ToHex(record + start, buffer + bufferPos, fieldLen);
					if (fieldLen > 0)
					{
						bufferPos += fieldLen;
					}
					else
					{
						record[len] = 0;
						// There should be only one 6666666666
						// Hopefully this is not the person that broke in and stole this data
						fprintf(stderr, "Error: Bad base64 \"%s\"\n", record);
						len = readRecord(fin, record);
						continue;
					}
				}
				else
				{
					memcpy(buffer + bufferPos, record + start, fieldLen);
					bufferPos += fieldLen;
				}
			}
			else if (allFieldsRequired)
			{
				dontPrint = true;
			}
			printTab = true;
		}

		// Hint
		start = recordPos;
		if (outHint)
		{
			if (printTab)
			{
				buffer[bufferPos] = '\t';
				bufferPos++;
			}
			len -= 3;
			fieldLen = len - start;
			if (fieldLen > 0)
			{
				notEmpty = true;
				// This is the only field that has tabs in it (there are 22 tabs in 10 records)
				if (printTab)
				{
					for (int i = start; i < len; i++)
					{
						char ch = record[i];
						buffer[bufferPos++] = ch;
						if (ch == '\t')
						{
							buffer[bufferPos - 1] = ' ';
						}
					}
				}
				else
				{
					memcpy(buffer + bufferPos, record + start, fieldLen);
					bufferPos += fieldLen;
				}
			}
			else if (allFieldsRequired)
			{
				dontPrint = true;
			}
		}

		// Print buffer
		if (notEmpty && !dontPrint)
		{
			buffer[bufferPos] = '\n';
			fwrite(buffer, bufferPos + 1, 1, fout);
		}
		len = readRecord(fin, record);
	}
	if (!feof(fin))
	{
		fprintf(stderr, "Error: This is not the file from users.tar.gz\n");
		return 1;
	}
	return 0;
}
