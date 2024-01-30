#define _CRT_SECURE_NO_WARNINGS

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }

#include <iostream>
#include <string>
#include <time.h>

#include "jdbc/mysql_connection.h"
#include "jdbc/cppconn/driver.h"
#include "jdbc/cppconn/exception.h"
#include "jdbc/cppconn/resultset.h"
#include "jdbc/cppconn/statement.h"
#include "jdbc/cppconn/prepared_statement.h"

using namespace std;

// SQL 사용을 위한 라이브러리 추가
#pragma comment(lib, "debug/mysqlcppconn")

#include <Windows.h>

time_t curTime = time(NULL);

struct tm* pLocal = localtime(&curTime);

/* * @brief 웹에서 사용하는 utf-8 인코딩을 window에서 사용하는 ANSI 인코딩으로 변경 합니다. */
std::string Utf8ToMultiByte(std::string utf8_str)
{
	std::string resultString; char* pszIn = new char[utf8_str.length() + 1];
	strncpy_s(pszIn, utf8_str.length() + 1, utf8_str.c_str(), utf8_str.length());
	int nLenOfUni = 0, nLenOfANSI = 0; wchar_t* uni_wchar = NULL;
	char* pszOut = NULL;
	// 1. utf8 Length
	if ((nLenOfUni = MultiByteToWideChar(CP_UTF8, 0, pszIn, (int)strlen(pszIn), NULL, 0)) <= 0)
		return nullptr;
	uni_wchar = new wchar_t[nLenOfUni + 1];
	memset(uni_wchar, 0x00, sizeof(wchar_t) * (nLenOfUni + 1));
	// 2. utf8 --> unicode
	nLenOfUni = MultiByteToWideChar(CP_UTF8, 0, pszIn, (int)strlen(pszIn), uni_wchar, nLenOfUni);
	// 3. ANSI(multibyte) Length
	if ((nLenOfANSI = WideCharToMultiByte(CP_ACP, 0, uni_wchar, nLenOfUni, NULL, 0, NULL, NULL)) <= 0)
	{
		delete[] uni_wchar; return 0;
	}
	pszOut = new char[nLenOfANSI + 1];
	memset(pszOut, 0x00, sizeof(char) * (nLenOfANSI + 1));
	// 4. unicode --> ANSI(multibyte)
	nLenOfANSI = WideCharToMultiByte(CP_ACP, 0, uni_wchar, nLenOfUni, pszOut, nLenOfANSI, NULL, NULL);
	pszOut[nLenOfANSI] = 0;
	resultString = pszOut;
	delete[] uni_wchar;
	delete[] pszOut;
	return resultString;
}

/* * @brief 위에서 한 작업을 반대로 합니다. ANSI 인코딩을 utf-8 으로 변경합니다. */
std::string MultiByteToUtf8(std::string multibyte_str)
{
	char* pszIn = new char[multibyte_str.length() + 1];
	strncpy_s(pszIn, multibyte_str.length() + 1, multibyte_str.c_str(), multibyte_str.length());

	std::string resultString;

	int nLenOfUni = 0, nLenOfUTF = 0;
	wchar_t* uni_wchar = NULL;
	char* pszOut = NULL;

	// 1. ANSI(multibyte) Length
	if ((nLenOfUni = MultiByteToWideChar(CP_ACP, 0, pszIn, (int)strlen(pszIn), NULL, 0)) <= 0)
		return 0;

	uni_wchar = new wchar_t[nLenOfUni + 1];
	memset(uni_wchar, 0x00, sizeof(wchar_t) * (nLenOfUni + 1));

	// 2. ANSI(multibyte) ---> unicode
	nLenOfUni = MultiByteToWideChar(CP_ACP, 0, pszIn, (int)strlen(pszIn), uni_wchar, nLenOfUni);

	// 3. utf8 Length
	if ((nLenOfUTF = WideCharToMultiByte(CP_UTF8, 0, uni_wchar, nLenOfUni, NULL, 0, NULL, NULL)) <= 0)
	{
		delete[] uni_wchar;
		return 0;
	}

	pszOut = new char[nLenOfUTF + 1];
	memset(pszOut, 0, sizeof(char) * (nLenOfUTF + 1));

	// 4. unicode ---> utf8
	nLenOfUTF = WideCharToMultiByte(CP_UTF8, 0, uni_wchar, nLenOfUni, pszOut, nLenOfUTF, NULL, NULL);
	pszOut[nLenOfUTF] = 0;
	resultString = pszOut;

	delete[] uni_wchar;
	delete[] pszOut;

	return resultString;
}

int main()
{
	//char day[1024] = {0,};
	//snprintf(day, (int)sizeof(day) + 1, "%04d-%02d-%02d", pLocal->tm_year + 1900, pLocal->tm_mon + 1, pLocal->tm_mday);


	// WorkBench 역할을 하는 녀석을 생성합니다.
	// Connection 말 그대로 연결 작업입니다.
	sql::Driver* driver = nullptr;
	sql::Connection* connection = nullptr;		// 주소
	sql::Statement* statement = nullptr;		// SQL
	sql::ResultSet* resultSet = nullptr;		// 결과
	sql::PreparedStatement* preparedStatement = nullptr;	// sql + 값 추가

	// driver 를 요청해서 WorkBench 를 실행합니다.
	driver = get_driver_instance();

	// 로컬 자기 자신한테 연결 작업을 해줍니다.
	// 인자 값으로 IP 주소, 계정, 비밀번호를 전달합니다.
	connection = driver->connect("tcp://127.0.0.1", "root", "1234");

	if (connection == nullptr)
	{
		cout << "connection failed." << endl;
		exit(-1);
	}

	/*
	// mysql 에서 use world 를 이용해서 DB 사용 명시한 것과 같은 역할의 코드입니다.
	connection->setSchema("world");

	statement = connection->createStatement();
	resultSet = statement->executeQuery("select * from lunch_menu");

	// while 문을 이용하면 맨 처음 값을 못 가져옵니다.
	// 이를 해결하고자 do 를 사용할 수 있는데 이러면 또 다른 문제가 발생할 수 있습니다.
	// 그래서 아래처럼 우리는 for 문을 사용해서 값을 가져오도록 합니다.
	for (; resultSet->next();)
	{
		cout << resultSet->getInt("idx") << " : "
			<< Utf8ToMultiByte(resultSet->getString("name")) << " : "
			<< resultSet->getInt("price") << " : "
			<< Utf8ToMultiByte(resultSet->getString("position")) << endl;
	}
	*/


	try
	{
		// 만약 에러처리가 필요하다면 이런식으로 에러가 발생한 위치에 exception 를 던지게 할 수 있습니다.
		//if (Result == 0)
		//{
		//	throw exception("이것은 에러인 것이다");
		//}

		char Buffer[1024] = { 0, };

		connection->setSchema("world");
		statement = connection->createStatement();

		//char ss[1024] = { 0, };
		//snprintf(ss, 1024, "insert into guest_book (`name`, `info`, `time`) values ('이희주', '사람', '%s')", day);

		//char name[20]; 
		//cin.getline(name, 20);

		//char info[20];
		//cin.getline(info, 20);

		//snprintf(Buffer, 200, "insert into guest_book (`name`, `info`) values ('%s', '%s')", name, info);
		//statement->execute(MultiByteToUtf8(Buffer));

		preparedStatement = connection->prepareStatement("insert into guest_book (`name`, `info`) values (?, ?)");
		preparedStatement->setString(1, MultiByteToUtf8("사람이"));
		preparedStatement->setString(2, MultiByteToUtf8("먼저다"));
		preparedStatement->execute();

		resultSet = statement->executeQuery("select * from guest_book");
		for (; resultSet->next();)
		{
			cout << resultSet->getInt("idx") << " : "
				<< Utf8ToMultiByte(resultSet->getString("name")) << " : "
				<< Utf8ToMultiByte(resultSet->getString("info")) << " : "
				<< Utf8ToMultiByte(resultSet->getString("time")) << endl;
		}

		//delete resultSet;
		//delete statement;
		//delete connection;
	}
	catch (sql::SQLException e)
	{
		// SQL 관련 에러 잡기
		cout << e.getSQLStateCStr() << endl;
		cout << e.what() << endl;
	}
	catch (exception e)
	{
		// 모든 에러 잡기
		cout << e.what() << endl;
	}

	SAFE_DELETE(connection);
	SAFE_DELETE(statement);
	SAFE_DELETE(resultSet);
	SAFE_DELETE(preparedStatement);

	return 0;
}