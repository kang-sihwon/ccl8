#include "KSIWorldParticipant.h"

// 내 클래스 함수

void FKSIWorldSectionRegistry::Register(FName Name, int32 Order, FKSIWorldSectionValidator Validator)
{
    check(!Name.IsNone() && Validator);
    auto& Values = Entries();
    check(!Values.ContainsByPredicate([Name](const FEntry& Entry) { return Entry.Name == Name; }));
    Values.Add({Name, Order, MoveTemp(Validator)});
}

const FKSIWorldSectionRegistry::FEntry* FKSIWorldSectionRegistry::Find(FName Name)
{
    return Entries().FindByPredicate([Name](const FEntry& Entry)
    {
        return Entry.Name == Name;
    });
}

TArray<const FKSIWorldSectionRegistry::FEntry*> FKSIWorldSectionRegistry::GetOrdered()
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

TArray<FKSIWorldSectionRegistry::FEntry>& FKSIWorldSectionRegistry::Entries()
{
    static TArray<FEntry> Values;
    return Values;
}

void FKSIWorldSectionRegistry::Unregister(FName Name)
{
    Entries().RemoveAll([Name](const FEntry& Entry) { return Entry.Name == Name; });
}

FKSIWorldSectionRegistrar::~FKSIWorldSectionRegistrar()
{
    FKSIWorldSectionRegistry::Unregister(SectionName);
}
