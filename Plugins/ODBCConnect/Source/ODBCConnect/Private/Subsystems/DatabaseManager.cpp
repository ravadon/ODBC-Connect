// Copyright 2023 Adam Eskhan, All rights reserved.


#include "Subsystems/DatabaseManager.h"
#include "Async/Async.h"

bool UDatabaseManager::ConnectToDatabase(SQLHDBC& connHandle, SQLHSTMT*& stmtHandle, SQLHENV& envHandle, const FString& ConnectionString, FString& errorMsg)
{
	SQLRETURN RetCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &envHandle);

	if (RetCode != SQL_SUCCESS && RetCode != SQL_SUCCESS_WITH_INFO)
	{
		errorMsg = "DatabaseManager Error: Failed to allocate an environment handle.";
		return false;
	}

	RetCode = SQLSetEnvAttr(envHandle, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);

	if (RetCode != SQL_SUCCESS && RetCode != SQL_SUCCESS_WITH_INFO)
	{
		errorMsg = "DatabaseManager Error: Failed to set the ODBC version.";
		SQLFreeHandle(SQL_HANDLE_ENV, envHandle);
		return false;
	}

	RetCode = SQLAllocHandle(SQL_HANDLE_DBC, envHandle, &connHandle);

	if (RetCode != SQL_SUCCESS && RetCode != SQL_SUCCESS_WITH_INFO)
	{
		errorMsg = "DatabaseManager Error: Failed to allocate a connection handle.";
		SQLFreeHandle(SQL_HANDLE_ENV, envHandle);
		return false;
	}

	const TCHAR* connectionStringChar = *ConnectionString;
	SQLWCHAR* _ConnectionString = (SQLWCHAR*)connectionStringChar;

	// Connect to the SQL Server database
	RetCode = SQLDriverConnect(connHandle, NULL, _ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);

	if (RetCode == SQL_SUCCESS || RetCode == SQL_SUCCESS_WITH_INFO)
	{
		// Allocate a statement handle
		stmtHandle = new SQLHSTMT;
		SQLAllocHandle(SQL_HANDLE_STMT, connHandle, stmtHandle);

		// Free the connection handle
		SQLFreeHandle(SQL_HANDLE_DBC, connHandle);

		// Free the environment handle
		SQLFreeHandle(SQL_HANDLE_ENV, envHandle);

		return true;
	}
	else
	{
		SQLWCHAR SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER NativeError;
		SQLSMALLINT MsgLen;

		SQLGetDiagRec(SQL_HANDLE_DBC, connHandle, 1, SqlState, &NativeError, Msg, SQL_MAX_MESSAGE_LENGTH, &MsgLen);

		errorMsg = FString::Printf(TEXT("DatabaseManager Error: Failed to connect to SQL Server database, ODBC Error: %s"), Msg);

		// Free the connection handle
		SQLFreeHandle(SQL_HANDLE_DBC, connHandle);
		connHandle = nullptr;

		// Free the environment handle
		SQLFreeHandle(SQL_HANDLE_ENV, envHandle);
		envHandle = nullptr;

		return false;
	}
}

void UDatabaseManager::DisconnectFromDatabase(SQLHDBC& connHandle, SQLHSTMT*& stmtHandle, SQLHENV& envHandle)
{
	if (stmtHandle)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, stmtHandle);
	}
	if (connHandle)
	{
		SQLDisconnect(connHandle);
		SQLFreeHandle(SQL_HANDLE_DBC, connHandle);
	}
	if (envHandle)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, envHandle);
	}
}

bool UDatabaseManager::PrepareAndExecuteStatement(SQLHDBC& connHandle, SQLHSTMT*& stmtHandle, SQLRETURN& retCode, const FString& statementString, FString& errorMsg)
{
	const TCHAR* statementStringChar = *statementString;
	SQLWCHAR* SQLWCHARStatementString = (SQLWCHAR*)statementStringChar;

	SQLWCHAR SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER NativeError;
	SQLSMALLINT MsgLen;

	SQLRETURN RetCode = SQLPrepare(*stmtHandle, SQLWCHARStatementString, SQL_NTS);

	if (!SQL_SUCCEEDED(RetCode))
	{
		SQLGetDiagRec(SQL_HANDLE_DBC, connHandle, 1, SqlState, &NativeError, Msg, SQL_MAX_MESSAGE_LENGTH, &MsgLen);
		errorMsg = FString::Printf(TEXT("DatabaseManager Error: Failed to prepare the statement, ODBC Error: %s"), Msg);
		return false;
	}

	// Execute the SQL statement
	RetCode = SQLExecute(*stmtHandle);

	if (!SQL_SUCCEEDED(RetCode))
	{
		SQLGetDiagRec(SQL_HANDLE_DBC, connHandle, 1, SqlState, &NativeError, Msg, SQL_MAX_MESSAGE_LENGTH, &MsgLen);
		errorMsg = FString::Printf(TEXT("DatabaseManager Error: Failed to execute the statement, ODBC Error: %s"), Msg);
		return false;
	}
	return true;
}

bool UDatabaseManager::GetRowsData(SQLHDBC& connHandle, SQLHSTMT*& stmtHandle, TArray <FString>& columnNames, TArray<FDatabaseColumnsData>& columnValues, FString& errorMsg)
{
	SQLWCHAR SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER NativeError;
	SQLSMALLINT MsgLen;

	SQLSMALLINT NumCols = 0;
	SQLRETURN retCode = SQLNumResultCols(*stmtHandle, &NumCols);

	if (!SQL_SUCCEEDED(retCode))
	{
		SQLGetDiagRec(SQL_HANDLE_DBC, connHandle, 1, SqlState, &NativeError, Msg, SQL_MAX_MESSAGE_LENGTH, &MsgLen);
		errorMsg = FString::Printf(TEXT("DatabaseManager Error: Could not get the number of columns, ODBC Error: %s"), Msg);
		return false;
	}

	columnNames.SetNum(NumCols);
	columnValues.SetNum(NumCols);
	for (SQLSMALLINT i = 1; i <= NumCols; i++)
	{
		SQLWCHAR ColName[256];
		SQLSMALLINT NameLen;
		SQLSMALLINT DataType;
		SQLULEN ColSize;
		SQLSMALLINT DecimalDigits, Nullable;

		retCode = SQLDescribeColW(*stmtHandle, i, ColName, 256, &NameLen, &DataType, &ColSize, &DecimalDigits, &Nullable);

		if (!SQL_SUCCEEDED(retCode))
		{
			SQLGetDiagRec(SQL_HANDLE_DBC, connHandle, 1, SqlState, &NativeError, Msg, SQL_MAX_MESSAGE_LENGTH, &MsgLen);
			errorMsg = FString::Printf(TEXT("DatabaseManager Error: Could not get column %s, ODBC Error: %s"), i, Msg);
			return false;
		}

		FString ColumnNameString((TCHAR*)ColName);
		ColumnNameString.TrimEndInline();
		columnNames[i - 1] = ColumnNameString;
	}

	while (SQLFetch(*stmtHandle) == SQL_SUCCESS)
	{
		for (SQLSMALLINT i = 1; i <= NumCols; i++)
		{
			SQLCHAR Value[50];
			SQLLEN indicator;
			FString ColumnValue;
			retCode = SQLGetData(*stmtHandle, i, SQL_CHAR, Value, sizeof(Value), &indicator);
			if (!SQL_SUCCEEDED(retCode))
			{
				SQLGetDiagRec(SQL_HANDLE_DBC, connHandle, 1, SqlState, &NativeError, Msg, SQL_MAX_MESSAGE_LENGTH, &MsgLen);
				errorMsg = FString::Printf(TEXT("DatabaseManager Error: Could not get column %s data, ODBC Error: %s"), i, Msg);
				return false;
			}

			if (indicator == SQL_NULL_DATA)
			{
				ColumnValue = "NULL";
			}
			else
			{
				// convert to C-style string
				char* cString = (char*)Value;
				// convert to FString
				ColumnValue = FString(UTF8_TO_TCHAR(cString));
			}

			ColumnValue.TrimEndInline();
			columnValues[i - 1].ColumnsData.Add(ColumnValue);
		}
	}
	return true;
}


bool UDatabaseManager::NativeExecuteStatement(SQLHDBC& connHandle, SQLHSTMT* stmtHandle, const FString& statementString, TArray <FString>& columnNames, TArray<FDatabaseColumnsData>& columnValues, int64& affectedRowsCount, FString& errorMsg)
{
	if (statementString.Len() < 1)
	{
		errorMsg = "DatabaseManager Error: Invalid statement.";
		return false;
	}

	SQLRETURN RetCode;

	if (!PrepareAndExecuteStatement(connHandle, stmtHandle, RetCode, statementString, errorMsg))
	{
		return false;
	}

	SQLLEN rowCount = -1;
	RetCode = SQLRowCount(*stmtHandle, &rowCount);
	if (SQL_SUCCEEDED(RetCode) && rowCount >= 0)
	{
		affectedRowsCount = rowCount;
	}
	
	if (!GetRowsData(connHandle, stmtHandle, columnNames, columnValues, errorMsg))
	{
		return false;
	}
	return true;
}


////-----------

void UDatabaseManager::ExecuteStatement(FString connectionString, const FString& statementString, FName statementTag, FOnExecutionCompletedDelegate OnComplete)
{
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, connectionString, statementString, statementTag, OnComplete]()
		{
			SQLHDBC connHandle = nullptr;
			SQLHSTMT* stmtHandle = nullptr;
			SQLHENV envHandle = nullptr;

			TArray<FString> columns = {};
			TArray<FDatabaseColumnsData> columnsData = {};

			int64 affectedRowsCount = -1;
			FString errorMsg = "";
			bool result = false;

			if (!ConnectToDatabase(connHandle, stmtHandle, envHandle, connectionString, errorMsg))
			{
				result = false;
			}
			else if (!connHandle && !stmtHandle)
			{
				result = false;
			}
			else
			{
				result = UDatabaseManager::NativeExecuteStatement(connHandle, stmtHandle, statementString, columns, columnsData, affectedRowsCount, errorMsg);

				DisconnectFromDatabase(connHandle, stmtHandle, envHandle);
			}

			if (this == nullptr || !IsValid(this))
			{
				return;
			}

			AsyncTask(ENamedThreads::GameThread, [this, result, columns, columnsData, affectedRowsCount, errorMsg, statementTag, OnComplete]()
				{
					if (!result && OnComplete.IsBound())
					{
						OnComplete.Execute(nullptr, result, errorMsg);
						return;
					}

					if (OnComplete.IsBound())
					{
						UDatabaseObject* databaseObject = NewObject<UDatabaseObject>(this, UDatabaseObject::StaticClass());
						databaseObject->SetColumns(columns);
						databaseObject->SetColumnsData(columnsData);
						databaseObject->SetAffectedRowsCount(affectedRowsCount);
						databaseObject->SetStatementTag(statementTag);
						OnComplete.Execute(databaseObject, result, errorMsg);
						return;
					}

					UE_LOG(LogTemp, Warning, TEXT("ODBCConnect: Statement execution completed but nothing was bound to OnComplete."));
				});
		});
}

