// Copyright 2023 Adam Eskhan, All rights reserved.

#include "Objects/DatabaseObject.h"

TArray<FString> UDatabaseObject::GetColumnValuesByColumnName(const FString ColumnName)
{
	if (!(ColumnsNames.Num() > 0 && ColumnsNames.Num() == ColumnsData.Num()))
	{
		return TArray<FString> {};
	}

	int arrayIndex = 0;

	if (!ColumnsNames.Find(ColumnName, arrayIndex))
	{
		return TArray<FString> {};
	}

	TArray<FString> values;

	values.SetNum(ColumnsData[arrayIndex].ColumnsData.Num());

	for (int i = 0; i < ColumnsData[arrayIndex].ColumnsData.Num(); i++)
	{
		values[i] = ColumnsData[arrayIndex].ColumnsData[i];
	}
	return values;
}
