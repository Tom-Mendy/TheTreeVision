#if WITH_EDITOR

#include "ReloadPatchCommandlet.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "ReloadGuardFunctionLibrary.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	UEdGraphPin* FindPinByName(UEdGraphNode* Node, const TCHAR* PinName)
	{
		return Node ? Node->FindPin(FName(PinName)) : nullptr;
	}

	bool IsAlreadyGuardedByHasAmmo(UEdGraphNode* TargetNode)
	{
		UEdGraphPin* ExecutePin = FindPinByName(TargetNode, TEXT("execute"));
		if (!ExecutePin || ExecutePin->LinkedTo.Num() == 0)
		{
			return false;
		}

		UK2Node_IfThenElse* BranchNode = Cast<UK2Node_IfThenElse>(ExecutePin->LinkedTo[0]->GetOwningNode());
		if (!BranchNode || !BranchNode->GetConditionPin() || BranchNode->GetConditionPin()->LinkedTo.Num() == 0)
		{
			return false;
		}

		UK2Node_CallFunction* ConditionFunction = Cast<UK2Node_CallFunction>(BranchNode->GetConditionPin()->LinkedTo[0]->GetOwningNode());
		return ConditionFunction && ConditionFunction->FunctionReference.GetMemberName() == GET_FUNCTION_NAME_CHECKED(UReloadGuardFunctionLibrary, HasAmmo);
	}

	bool SaveBlueprint(UBlueprint* Blueprint)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);

		UPackage* Package = Blueprint->GetOutermost();
		const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		const bool bSaved = UPackage::SavePackage(Package, Blueprint, *PackageFilename, SaveArgs);
		UE_LOG(LogTemp, Display, TEXT("Saved %s: %s"), *PackageFilename, bSaved ? TEXT("true") : TEXT("false"));
		return bSaved;
	}

	bool InsertAmmoGuard(UEdGraph* Graph, UK2Node_CallFunction* FireBulletNode, UK2Node_CallFunction* ReloadNode)
	{
		UEdGraphPin* FireExecutePin = FindPinByName(FireBulletNode, TEXT("execute"));
		if (!FireExecutePin || FireExecutePin->LinkedTo.Num() == 0 || IsAlreadyGuardedByHasAmmo(FireBulletNode))
		{
			return false;
		}

		UEdGraphPin* PreviousExecPin = FireExecutePin->LinkedTo[0];
		PreviousExecPin->BreakLinkTo(FireExecutePin);

		UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(Graph);
		Graph->AddNode(BranchNode, true, false);
		BranchNode->CreateNewGuid();
		BranchNode->NodePosX = FireBulletNode->NodePosX - 420;
		BranchNode->NodePosY = FireBulletNode->NodePosY;
		BranchNode->PostPlacedNewNode();
		BranchNode->AllocateDefaultPins();

		UK2Node_CallFunction* HasAmmoNode = NewObject<UK2Node_CallFunction>(Graph);
		Graph->AddNode(HasAmmoNode, true, false);
		HasAmmoNode->CreateNewGuid();
		HasAmmoNode->NodePosX = FireBulletNode->NodePosX - 720;
		HasAmmoNode->NodePosY = FireBulletNode->NodePosY + 120;
		HasAmmoNode->FunctionReference.SetExternalMember(
			GET_FUNCTION_NAME_CHECKED(UReloadGuardFunctionLibrary, HasAmmo),
			UReloadGuardFunctionLibrary::StaticClass());
		HasAmmoNode->AllocateDefaultPins();

		UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
		Graph->AddNode(SelfNode, true, false);
		SelfNode->CreateNewGuid();
		SelfNode->NodePosX = FireBulletNode->NodePosX - 940;
		SelfNode->NodePosY = FireBulletNode->NodePosY + 120;
		SelfNode->PostPlacedNewNode();
		SelfNode->AllocateDefaultPins();

		PreviousExecPin->MakeLinkTo(BranchNode->GetExecPin());
		BranchNode->GetThenPin()->MakeLinkTo(FireExecutePin);
		HasAmmoNode->GetReturnValuePin()->MakeLinkTo(BranchNode->GetConditionPin());

		if (UEdGraphPin* SelfPin = FindPinByName(SelfNode, TEXT("self")))
		{
			if (UEdGraphPin* WeaponActorPin = FindPinByName(HasAmmoNode, TEXT("WeaponActor")))
			{
				SelfPin->MakeLinkTo(WeaponActorPin);
			}
		}

		if (ReloadNode)
		{
			if (UEdGraphPin* ReloadExecutePin = FindPinByName(ReloadNode, TEXT("execute")))
			{
				BranchNode->GetElsePin()->MakeLinkTo(ReloadExecutePin);
			}
		}

		UE_LOG(LogTemp, Display, TEXT("Inserted HasAmmo guard before %s.%s"), *Graph->GetName(), *FireBulletNode->GetName());
		return true;
	}
}

int32 UReloadPatchCommandlet::Main(const FString& Params)
{
	UBlueprint* WeaponBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Variant_Shooter/Blueprints/Pickups/BP_ShooterWeaponBase.BP_ShooterWeaponBase"));
	if (!WeaponBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("BP_ShooterWeaponBase not found."));
		return 1;
	}

	bool bChanged = false;
	TArray<UEdGraph*> Graphs;
	Graphs.Append(WeaponBlueprint->UbergraphPages);
	Graphs.Append(WeaponBlueprint->FunctionGraphs);
	Graphs.Append(WeaponBlueprint->MacroGraphs);
	Graphs.Append(WeaponBlueprint->DelegateSignatureGraphs);
	for (UEdGraph* Graph : Graphs)
	{
		if (Graph->GetName() != TEXT("EventGraph"))
		{
			continue;
		}

		UK2Node_CallFunction* FireBulletNode = nullptr;
		UK2Node_CallFunction* ReloadNode = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node);
			if (!CallNode)
			{
				continue;
			}

			const FName FunctionName = CallNode->FunctionReference.GetMemberName();
			if (FunctionName == TEXT("Fire Bullet"))
			{
				FireBulletNode = CallNode;
			}
			else if (FunctionName == TEXT("Reload"))
			{
				ReloadNode = CallNode;
			}
		}

		if (FireBulletNode)
		{
			bChanged |= InsertAmmoGuard(Graph, FireBulletNode, ReloadNode);
		}
	}

	if (!bChanged)
	{
		UE_LOG(LogTemp, Display, TEXT("Ammo guard patch was already applied."));
		return 0;
	}

	return SaveBlueprint(WeaponBlueprint) ? 0 : 1;
}

#endif
