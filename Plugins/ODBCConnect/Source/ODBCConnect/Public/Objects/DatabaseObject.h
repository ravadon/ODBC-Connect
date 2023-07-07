// Copyright 2023 Adam Eskhan, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DatabaseObject.generated.h"

USTRUCT()
struct FDatabaseColumnsData
{
	GENERATED_BODY()
public:

		TArray<FString> ColumnsData = {};
};

/**
 * 
 */
UCLASS(BlueprintType)
class ODBCCONNECT_API UDatabaseObject : public UObject
{
	GENERATED_BODY()

private:

	int64 AffectedRowsCount = -1;

	FName StatementTag = "";

	TArray<FString> ColumnsNames = {};

	TArray<FDatabaseColumnsData> ColumnsData = {};

public:

	FORCEINLINE void SetAffectedRowsCount(const int64& count) { AffectedRowsCount = count; }

	FORCEINLINE void SetColumns(const TArray<FString>& columnsNames) { ColumnsNames = columnsNames; }

	FORCEINLINE void SetColumnsData(const TArray<FDatabaseColumnsData>& columnsData) { ColumnsData = columnsData; }

	FORCEINLINE void SetStatementTag(FName statementTag = "") { StatementTag = statementTag; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DatabaseObject", meta = (KeyWords = "Affected Rows, Updated"))
		FORCEINLINE int64& GetAffectedRowsCount() { return AffectedRowsCount; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DatabaseObject", meta = (KeyWords = "Tag"))
		FORCEINLINE FName& GetStatementTag() { return StatementTag; }

	UFUNCTION(BlueprintCallable, Category = "DatabaseObject", meta = (KeyWords = "Data, Rows, Columns"))
		FORCEINLINE TArray<FString> GetColumnValuesByColumnName(const FString ColumnName = "ColumnName");
};
