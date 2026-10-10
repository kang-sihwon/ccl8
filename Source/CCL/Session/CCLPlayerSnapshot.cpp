#include "CCLPlayerSnapshot.h"

#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
constexpr uint32 Magic = 0x504C4343; // CCLP

bool Reject(FString& Error)
{
    Error = TEXT("Invalid, oversized or inconsistent world player profile.");
    return false;
}

bool String(FArchive& Ar, FString& Text, int32 Limit)
{
    FTCHARToUTF8 Utf8(*Text);
    int32 Count = Ar.IsSaving() ? Utf8.Length() : 0;
    Ar << Count;
    if (Ar.IsError() || Count < 0 || Count > Limit || (Ar.IsLoading() && Count > Ar.TotalSize() - Ar.Tell()))
    {
        return false;
    }
    if (Ar.IsSaving())
    {
        Ar.Serialize(const_cast<ANSICHAR*>(Utf8.Get()), Count);
    }
    else
    {
        TArray<ANSICHAR> Buffer;
        Buffer.SetNumZeroed(Count + 1);
        Ar.Serialize(Buffer.GetData(), Count);
        const FUTF8ToTCHAR Converted(Buffer.GetData(), Count);
        Text = FString(Converted.Length(), Converted.Get());
        const FTCHARToUTF8 Canonical(*Text);
        if (Canonical.Length() != Count || (Count && FMemory::Memcmp(Canonical.Get(), Buffer.GetData(), Count)))
        {
            return false;
        }
    }
    return !Ar.IsError();
}

bool Count(FArchive& Ar, int32& Value, int32 Maximum, int32 MinimumBytes)
{
    Ar << Value;
    return !Ar.IsError() && Value >= 0 && Value <= Maximum &&
           (!Ar.IsLoading() || int64(Value) * MinimumBytes <= Ar.TotalSize() - Ar.Tell());
}

bool Name(FArchive& Ar, FName& Value)
{
    FString Text = Ar.IsSaving() ? Value.ToString() : FString();
    if (!String(Ar, Text, 64))
    {
        return false;
    }
    if (Ar.IsLoading())
    {
        Value = FName(*Text);
    }
    return true;
}

bool Serialize(FArchive& Ar, TArray<FCCLPlayerSnapshot>& Profiles)
{
    uint32 Tag = Magic;
    uint16 Version = 1;
    Ar << Tag << Version;
    int32 Num = Profiles.Num();
    if (Ar.IsError() || Tag != Magic || Version != 1 || !Count(Ar, Num, CCLPlayerSnapshot::MaximumPlayers, 106))
    {
        return false;
    }
    Profiles.SetNum(Num);
    int32 TotalItems = 0;
    for (auto& P : Profiles)
    {
        if (!String(Ar, P.IdentityKey, 192))
        {
            return false;
        }
        Ar << P.AccountId << P.Capacity << P.Points << P.Quest << P.HealthFraction << P.StaminaFraction;
        auto& B = P.Environment;
        uint8 Source = uint8(B.Source);
        Ar << Source << B.SourceId << B.SampleWorldSeconds << B.Conditions.TemperatureCelsius
           << B.Conditions.RelativeHumidity << B.Conditions.SpoilageMultiplier << B.Conditions.MoldExposure;
        B.Source = ECCLFoodEnvironment(Source);
        int32 Items = P.Items.Num();
        if (!Count(Ar, Items, 136, 36) || (TotalItems += Items) > 4096)
        {
            return false;
        }
        P.Items.SetNum(Items);
        for (auto& Item : P.Items)
        {
            Ar << Item.Id << Item.Quantity << Item.Slot << Item.LoadedAmmo;
            if (!Name(Ar, Item.Definition) || !String(Ar, Item.Food, 20000))
            {
                return false;
            }
        }
        int32 Equipment = P.EquipmentSlots.Num();
        if (!Count(Ar, Equipment, 8, 20))
        {
            return false;
        }
        P.EquipmentSlots.SetNum(Equipment);
        for (auto& Slot : P.EquipmentSlots)
        {
            FString TagName = Ar.IsSaving() ? Slot.Slot.ToString() : FString();
            if (!String(Ar, TagName, 128))
            {
                return false;
            }
            if (Ar.IsLoading())
            {
                Slot.Slot = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
            }
            Ar << Slot.Id;
        }
        int32 Skills = P.Skills.Num();
        if (!Count(Ar, Skills, 2, 4))
        {
            return false;
        }
        P.Skills.SetNum(Skills);
        for (auto& Skill : P.Skills)
        {
            if (!Name(Ar, Skill))
            {
                return false;
            }
        }
        if (Ar.IsError() || Ar.Tell() > CCLPlayerSnapshot::MaximumBytes)
        {
            return false;
        }
    }
    return !Ar.IsError();
}

bool ValidKey(const FString& Key)
{
    if (Key.IsEmpty() || Key.Len() > 192)
    {
        return false;
    }
    for (const TCHAR Character : Key)
    {
        if (!((Character >= 'a' && Character <= 'z') || (Character >= '0' && Character <= '9') || Character == ':' ||
              Character == '-' || Character == '_' || Character == '.'))
        {
            return false;
        }
    }
    return true;
}
} // namespace

bool CCLPlayerSnapshot::Validate(const TArray<FCCLPlayerSnapshot>& Profiles, FGuid WorldId, double WorldSeconds,
                                 FString& Error)
{
    Error.Reset();
    if (!WorldId.IsValid() || !FMath::IsFinite(WorldSeconds) || WorldSeconds < 0. || Profiles.Num() > MaximumPlayers)
    {
        return Reject(Error);
    }
    TSet<FString> Keys;
    TSet<FGuid> Accounts, Items;
    int32 TotalItems = 0;
    int64 Size = 10;
    for (const auto& P : Profiles)
    {
        if (!ValidKey(P.IdentityKey) || Keys.Contains(P.IdentityKey) || !P.AccountId.IsValid() ||
            Accounts.Contains(P.AccountId) || !FMath::IsFinite(P.HealthFraction) || P.HealthFraction < 0.f ||
            P.HealthFraction > 1.f || !FMath::IsFinite(P.StaminaFraction) || P.StaminaFraction < 0.f ||
            P.StaminaFraction > 1.f || !CCLFoodEnvironment::Validate(P.Environment, WorldSeconds, Error) ||
            (TotalItems += P.Items.Num()) > 4096)
        {
            return Reject(Error);
        }
        FCCLSessionRecord Record;
        Record.AccountId = P.AccountId;
        Record.Items = P.Items;
        Record.EquipmentSlots = P.EquipmentSlots;
        Record.Skills = P.Skills;
        Record.Points = P.Points;
        Record.Quest = P.Quest;
        Record.Coins = 0;
        if (!FCCLSessionCodec::ValidateProfile(Record, WorldId, WorldSeconds, P.Capacity))
        {
            return Reject(Error);
        }
        Keys.Add(P.IdentityKey);
        Accounts.Add(P.AccountId);
        Size += 106 + P.IdentityKey.Len();
        for (const auto& Item : P.Items)
        {
            FCCLFoodState Food;
            if (Items.Contains(Item.Id) || Item.Food.Len() > 20000 || !CCLFood::Decode(Item.Food, Food, Error) ||
                (Food.IsPresent() && P.Environment.IsDynamic() &&
                 (Food.UpdatedWorldSeconds < P.Environment.SampleWorldSeconds ||
                  !(Food.Storage == P.Environment.Conditions))))
            {
                return Reject(Error);
            }
            Items.Add(Item.Id);
            Size += 36 + FTCHARToUTF8(*Item.Definition.ToString()).Length() + FTCHARToUTF8(*Item.Food).Length();
        }
        for (const auto& Slot : P.EquipmentSlots)
        {
            Size += 20 + FTCHARToUTF8(*Slot.Slot.ToString()).Length();
        }
        for (const auto Skill : P.Skills)
        {
            Size += 4 + FTCHARToUTF8(*Skill.ToString()).Length();
        }
        if (Size > MaximumBytes)
        {
            return Reject(Error);
        }
    }
    return true;
}

bool CCLPlayerSnapshot::Encode(const TArray<FCCLPlayerSnapshot>& Profiles, FGuid WorldId, double WorldSeconds,
                               TArray<uint8>& Bytes, FString& Error)
{
    if (!Validate(Profiles, WorldId, WorldSeconds, Error))
    {
        return false;
    }
    auto Candidate = Profiles;
    TArray<uint8> Output;
    FMemoryWriter Writer(Output);
    if (!Serialize(Writer, Candidate) || Output.Num() > MaximumBytes)
    {
        return Reject(Error);
    }
    Bytes = MoveTemp(Output);
    return true;
}

bool CCLPlayerSnapshot::Decode(const TArray<uint8>& Bytes, FGuid WorldId, double WorldSeconds,
                               TArray<FCCLPlayerSnapshot>& Profiles, FString& Error)
{
    if (Bytes.Num() < 10 || Bytes.Num() > MaximumBytes)
    {
        return Reject(Error);
    }
    TArray<FCCLPlayerSnapshot> Candidate;
    FMemoryReader Reader(Bytes);
    if (!Serialize(Reader, Candidate) || Reader.Tell() != Reader.TotalSize() ||
        !Validate(Candidate, WorldId, WorldSeconds, Error))
    {
        return Reject(Error);
    }
    Profiles = MoveTemp(Candidate);
    return true;
}
