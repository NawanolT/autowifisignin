/*
autowifisignin.cpp
Automatically sign in to Prince of Songkla University's network using the supplied username and password
Author: Nawanol Theera-Ampornpunt
Email: nawanol.t@phuket.psu.ac.th
*/

#include "stdafx.h"
#include <Windows.h>
#include <winhttp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define USERNAME "myusername"
#define PASSWORD "mypassword"

void print_error_and_exit(const char *s, ...)
{
	va_list arguments;

	va_start(arguments, s);
	vprintf(s, arguments);
	va_end(arguments);
	system("pause");
	exit(1);
}

void auto_login()
{
	HINTERNET  hSession = NULL, hConnect = NULL, hRequest = NULL;
	BOOL bResult;
	unsigned long ulReply_size, ulNum_bytes_read, ulNum_bytes_read_total;
	char *psReply = NULL, *p, *q, *postData;
	URL_COMPONENTS urlComp;
	wchar_t * wcstring;
	size_t numConvertChars, postData_size;
	int port, iResult;

	//open WinHTTP session
	hSession = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if(hSession == NULL)
		print_error_and_exit("WinHttpOpen failed\n");

	//check internet connection
	printf("Checking internet connection...\n");
	hConnect = WinHttpConnect(hSession, L"httpbin.org",
		INTERNET_DEFAULT_HTTP_PORT, 0);
	if(hConnect == NULL)
		print_error_and_exit("WinHttpConnect failed\n");

	hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/get", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
	if(hRequest == NULL)
		print_error_and_exit("WinHttpOpenRequest failed\n");

	bResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	if(bResult == FALSE)
		print_error_and_exit("WinHttpSendRequest failed\n");

	bResult = WinHttpReceiveResponse(hRequest, NULL);
	if(bResult == FALSE)
		print_error_and_exit("WinHttpReceiveResponse failed\n");

	ulNum_bytes_read_total = 0;
	while(1)
	{
		bResult = WinHttpQueryDataAvailable(hRequest, &ulReply_size);
		if(bResult == FALSE)
			print_error_and_exit("WinHttpQueryDataAvailable failed\n");
		if(ulReply_size == 0)
			break;

		if(psReply == NULL)
			psReply = (char *)malloc(sizeof(char) * (ulReply_size + 1));
		else
			psReply = (char *)realloc(psReply, sizeof(char) * (ulNum_bytes_read_total + ulReply_size + 1));
		if(psReply == NULL)
			print_error_and_exit("Out of memory (%lu bytes)\n", ulNum_bytes_read_total + ulReply_size + 1);
		bResult = WinHttpReadData(hRequest, psReply, ulReply_size, &ulNum_bytes_read);
		if(bResult == FALSE)
			print_error_and_exit("WinHttpReadData failed\n");
		ulNum_bytes_read_total += ulNum_bytes_read;
	}
	if(ulNum_bytes_read_total == 0)
		print_error_and_exit("Reply 1 is empty\n");
	psReply[ulNum_bytes_read_total] = 0;//null terminator
	if(hRequest) WinHttpCloseHandle(hRequest);
	if(hConnect) WinHttpCloseHandle(hConnect);

	if(strstr(psReply, "http://httpbin.org/get") != NULL)
	{
		printf("Already logged in\n");
		return;//done
	}

	//check for redirection in reply 1
	p = strstr(psReply, "href=");
	if(p == NULL)
	{
		p = strstr(psReply, "window.location=");
		if(p == NULL)
			print_error_and_exit("No redirection from reply 1\n");
		else
			p += 17;
	}
	else
		p += 6;
	q = strchr(p, '\"');
	if(q == NULL)
		print_error_and_exit("No closing quotes\n");
	*q = 0;

	//prepare to visit login page
	ZeroMemory(&urlComp, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.dwSchemeLength = (DWORD)-1;
	urlComp.dwHostNameLength = (DWORD)-1;
	urlComp.dwUrlPathLength = (DWORD)-1;
	urlComp.dwExtraInfoLength = (DWORD)-1;
	numConvertChars = 0;
	wcstring = (wchar_t *)malloc(sizeof(wchar_t) * (strlen(p) + 1));
	if(wcstring == NULL) print_error_and_exit("Out of memory\n");
	iResult = mbstowcs_s(&numConvertChars, wcstring, strlen(p) + 1, p, _TRUNCATE);
	if(iResult != 0)
		print_error_and_exit("mbstowcs_s failed\n");
	bResult = WinHttpCrackUrl(wcstring, 0, 0, &urlComp);
	if(bResult == FALSE)
		print_error_and_exit("WinHttpCrackUrl failed\n");

	//scan port
	if(urlComp.lpszHostName[urlComp.dwHostNameLength] == ':')
	{
		if(swscanf_s(urlComp.lpszHostName + urlComp.dwHostNameLength + 1, L"%d", &port) != 1)
			print_error_and_exit("Cannot scan port\n");
	}
	else
		port = strncmp(p, "https://", 8) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
	urlComp.lpszHostName[urlComp.dwHostNameLength] = 0;
	//visit login page
	printf("Visiting login page\n");
	hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, port, 0);
	if(hConnect == NULL)
		print_error_and_exit("WinHttpConnect failed\n");
	hRequest = WinHttpOpenRequest(hConnect, L"GET", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	if(hRequest == NULL)
		print_error_and_exit("WinHttpOpenRequest failed\n");
	bResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	if(bResult == FALSE)
		print_error_and_exit("WinHttpSendRequest failed\n");
	bResult = WinHttpReceiveResponse(hRequest, NULL);
	if(bResult == FALSE)
		print_error_and_exit("WinHttpReceiveResponse failed\n");

	ulNum_bytes_read_total = 0;
	while(1)
	{
		bResult = WinHttpQueryDataAvailable(hRequest, &ulReply_size);
		if(bResult == FALSE)
			print_error_and_exit("WinHttpQueryDataAvailable failed\n");
		if(ulReply_size == 0)
			break;

		if(psReply == NULL)
			psReply = (char *)malloc(sizeof(char) * (ulReply_size + 1));
		else
			psReply = (char *)realloc(psReply, sizeof(char) * (ulNum_bytes_read_total + ulReply_size + 1));
		if(psReply == NULL)
			print_error_and_exit("Out of memory (%lu bytes)\n", ulNum_bytes_read_total + ulReply_size + 1);
		bResult = WinHttpReadData(hRequest, psReply, ulReply_size, &ulNum_bytes_read);
		if(bResult == FALSE)
			print_error_and_exit("WinHttpReadData failed\n");
		ulNum_bytes_read_total += ulNum_bytes_read;
	}
	if(ulNum_bytes_read_total == 0)
		print_error_and_exit("Reply 2 is empty\n");
	psReply[ulNum_bytes_read_total] = 0;//null terminator
	if(hRequest) WinHttpCloseHandle(hRequest);
	if(hConnect) WinHttpCloseHandle(hConnect);

	//log in with username and password (prepare POST request)
	hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, port, 0);
	if(hConnect == NULL)
		print_error_and_exit("WinHttpConnect failed\n");
	free(wcstring);
	hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/php/uid.php", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	if(hRequest == NULL)
		print_error_and_exit("WinHttpOpenRequest failed\n");

	postData_size = strlen("ipv6=&login=Login&password=&username=") + strlen(USERNAME) + strlen(PASSWORD) + 1;
	postData = (char *)malloc(sizeof(char) * postData_size);
	if(postData == NULL)
		print_error_and_exit("Out of memory\n");
	_snprintf_s(postData, postData_size, _TRUNCATE, "ipv6=&login=Login&password=%s&username=%s", PASSWORD, USERNAME);

	printf("Submitting username & password\n");
	bResult = WinHttpSendRequest(hRequest, L"Content-Type: application/x-www-form-urlencoded\n", -1, postData, (DWORD)(postData_size - 1), (DWORD)(postData_size - 1), 0);
	if(bResult == FALSE)
		print_error_and_exit("WinHttpSendRequest failed\n");

	bResult = WinHttpReceiveResponse(hRequest, NULL);
	if(bResult == FALSE)
		print_error_and_exit("WinHttpReceiveResponse failed\n");

	//read response
	ulNum_bytes_read_total = 0;
	while(1)
	{
		bResult = WinHttpQueryDataAvailable(hRequest, &ulReply_size);
		if(bResult == FALSE)
			print_error_and_exit("WinHttpQueryDataAvailable failed\n");
		if(ulReply_size == 0)
			break;

		if(psReply == NULL)
			psReply = (char *)malloc(sizeof(char) * (ulReply_size + 1));
		else
			psReply = (char *)realloc(psReply, sizeof(char) * (ulNum_bytes_read_total + ulReply_size + 1));
		if(psReply == NULL)
			print_error_and_exit("Out of memory (%lu bytes)\n", ulNum_bytes_read_total + ulReply_size + 1);
		bResult = WinHttpReadData(hRequest, psReply, ulReply_size, &ulNum_bytes_read);
		if(bResult == FALSE)
			print_error_and_exit("WinHttpReadData failed\n");
		ulNum_bytes_read_total += ulNum_bytes_read;
	}
	if(ulNum_bytes_read_total == 0)
		print_error_and_exit("Reply 3 is empty\n");
	psReply[ulNum_bytes_read_total] = 0;//null terminator

	if(strstr(psReply, "Login failed.") != NULL)
		print_error_and_exit("Login failed (incorrect username or password)\n");
	if(strstr(psReply, "user login = ") != NULL)
		printf("Login completed\n");

	free(postData);
	if(hRequest) WinHttpCloseHandle(hRequest);
	if(hConnect) WinHttpCloseHandle(hConnect);
	if(hSession) WinHttpCloseHandle(hSession);
}

int main()
{
	auto_login();

	return 0;
}

