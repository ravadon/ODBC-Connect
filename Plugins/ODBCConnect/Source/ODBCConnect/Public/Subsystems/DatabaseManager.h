// Copyright 2023 Adam Eskhan, All rights reserved.

#pragma once


#include "Windows/AllowWindowsPlatformTypes.h"
#include <sqlext.h>
#include "Windows/HideWindowsPlatformTypes.h"

#include "Objects/DatabaseObject.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "DatabaseManager.generated.h"

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnExecutionCompletedDelegate, UDatabaseObject*, DatabaseObject, bool, Success, FString, errorMsg);

/**
 * 
 */
UCLASS(NotBlueprintable, BlueprintType, meta = (Category = "ODBC Connect", DisplayName = "Database Manager"))
class ODBCCONNECT_API UDatabaseManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:

	// Do not call NativeExecuteStatement or the functions under "protected" directly, Call ExecuteStatement instead.

	static bool ConnectToDatabase(SQLHDBC& connHandle, SQLHSTMT*& stmtHandle, SQLHENV& envHandle, const FString& ConnectionString, FString& errorMsg);

	static void DisconnectFromDatabase(SQLHDBC& connHandle, SQLHSTMT*& stmtHandle, SQLHENV& envHandle);

	static bool PrepareAndExecuteStatement(SQLHDBC& connHandle, SQLHSTMT*& stmtHandle, SQLRETURN& retCode, const FString& statementString, FString& errorMsg);

	static bool GetRowsData(SQLHDBC& connHandle, SQLHSTMT*& stmtHandle, TArray <FString>& columnNames, TArray<FDatabaseColumnsData>& columnValues, FString& errorMsg);


	static bool NativeExecuteStatement(SQLHDBC& connHandle, SQLHSTMT* stmtHandle, const FString& statementString, TArray <FString>& columnNames, TArray<FDatabaseColumnsData>& columnValues, int64& affectedRowsCount, FString& errorMsg);

public:

	UFUNCTION(BlueprintCallable, Category = "DatabaseManager", meta = (KeyWords = "Query, Execute, Statement"))
		void ExecuteStatement(FString connectionString, const FString& statementString, FName statementTag, FOnExecutionCompletedDelegate OnComplete);
};
