#include "CCLWorldParticipant.h"

// 내 클래스 함수

void FCCLWorldSectionRegistry::Register(FName Name, int32 Order, FCCLWorldSectionValidator Validator)
{
	check(!Name.IsNone() && Validator);
	auto& Values = Entries();
	check(!Values.ContainsByPredicate([Name](const FEntry& Entry) { return Entry.Name == Name; }));
	Values.Add({Name, Order, MoveTemp(Validator)});
}

const FCCLWorldSectionRegistry::FEntry* FCCLWorldSectionRegistry::Find(FName Name)
{
	return Entries().FindByPredicate([Name](const FEntry& Entry)
	{
		return Entry.Name == Name;
	});
}

TArray<const FCCLWorldSectionRegistry::FEntry*> FCCLWorldSectionRegistry::GetOrdered()
{
	TArray<const FEntry*> Result;
	for (const auto& Entry : Entries())
	{
		Result.Add(&Entry);
	}

	Result.Sort([](const FEntry& A, const FEntry& B)
	{
		return A.Order != B.Order ? A.Order < B.Order : A.Name.LexicalLess(B.Name);
	});
	return Result;
}

TArray<FCCLWorldSectionRegistry::FEntry>& FCCLWorldSectionRegistry::Entries()
{
	static TArray<FEntry> Values;
	return Values;
}
