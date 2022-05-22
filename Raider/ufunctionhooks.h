#pragma once

#include "game.h"
#include "replication.h"
#include "ue4.h"
#include <functional>

//#define LOGGING
#define DEVELOPERCHEATS

inline std::vector<UFunction*> toHook;
inline std::vector<std::function<void(UObject*, void*)>> toCall;

#define DEFINE_PEHOOK(ufunctionName, func)                           \
    toHook.push_back(UObject::FindObject<UFunction>(ufunctionName)); \
    toCall.push_back([](UObject * Object, void* Parameters) -> void func);

namespace UFunctionHooks
{
    auto Initialize()
    {
        DEFINE_PEHOOK("Function GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbility", {
            auto AbilitySystemComponent = (UAbilitySystemComponent*)Object;
            auto Params = (UAbilitySystemComponent_ServerTryActivateAbility_Params*)Parameters;

            TryActivateAbility(AbilitySystemComponent, Params->AbilityToActivate, Params->InputPressed, &Params->PredictionKey, nullptr);
        })

        DEFINE_PEHOOK("Function GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbilityWithEventData", {
            auto AbilitySystemComponent = (UAbilitySystemComponent*)Object;
            auto Params = (UAbilitySystemComponent_ServerTryActivateAbilityWithEventData_Params*)Parameters;

            TryActivateAbility(AbilitySystemComponent, Params->AbilityToActivate, Params->InputPressed, &Params->PredictionKey, &Params->TriggerEventData);
        })

        DEFINE_PEHOOK("Function GameplayAbilities.AbilitySystemComponent.ServerAbilityRPCBatch", {
            auto AbilitySystemComponent = (UAbilitySystemComponent*)Object;
            auto Params = (UAbilitySystemComponent_ServerAbilityRPCBatch_Params*)Parameters;

            TryActivateAbility(AbilitySystemComponent, Params->BatchInfo.AbilitySpecHandle, Params->BatchInfo.InputPressed, &Params->BatchInfo.PredictionKey, nullptr);
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerPawn.ServerHandlePickup", { HandlePickup((AFortPlayerPawn*)Object, Parameters, true); })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerCreateBuildingActor", {
            auto PC = (AFortPlayerControllerAthena*)Object;

            auto Params = (AFortPlayerController_ServerCreateBuildingActor_Params*)Parameters;
            auto CurrentBuildClass = Params->BuildingClassData.BuildingClass;

            if (CurrentBuildClass)
            {
                if (auto BuildingActor = (ABuildingSMActor*)SpawnActor(CurrentBuildClass, Params->BuildLoc, Params->BuildRot, PC))
                {
                    BuildingActor->DynamicBuildingPlacementType = EDynamicBuildingPlacementType::CountsTowardsBounds;
                    BuildingActor->SetMirrored(Params->bMirrored);
                    BuildingActor->PlacedByPlacementTool();
                    BuildingActor->InitializeKismetSpawnedBuildingActor(BuildingActor, PC);
                }
            }
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerBeginEditingBuildingActor", {
            auto Params = (AFortPlayerController_ServerBeginEditingBuildingActor_Params*)Parameters;
            auto Controller = (AFortPlayerControllerAthena*)Object;
            auto Pawn = (APlayerPawn_Athena_C*)Controller->Pawn;
            bool bFound = false;
            auto EditToolEntry = FindItemInInventory<UFortEditToolItemDefinition>(Controller, bFound);

            if (Controller && Pawn && Params->BuildingActorToEdit && bFound)
            {
                auto EditTool = (AFortWeap_EditingTool*)EquipWeaponDefinition(Pawn, (UFortWeaponItemDefinition*)EditToolEntry.ItemDefinition, EditToolEntry.ItemGuid);

                if (EditTool)
                {
                    EditTool->EditActor = Params->BuildingActorToEdit;
                    EditTool->OnRep_EditActor();
                    Params->BuildingActorToEdit->EditingPlayer = (AFortPlayerStateZone*)Pawn->PlayerState;
                    Params->BuildingActorToEdit->OnRep_EditingPlayer();
                }
            }
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerEditBuildingActor", {
            auto Params = (AFortPlayerController_ServerEditBuildingActor_Params*)Parameters;
            auto PC = (AFortPlayerControllerAthena*)Object;

            if (PC && Params)
            {
                auto BuildingActor = Params->BuildingActorToEdit;
                auto NewBuildingClass = Params->NewBuildingClass;

                if (BuildingActor && NewBuildingClass)
                {
                    FTransform SpawnTransform;
                    SpawnTransform.Rotation = RotToQuat(BuildingActor->K2_GetActorRotation());
                    SpawnTransform.Translation = BuildingActor->K2_GetActorLocation();
                    SpawnTransform.Scale3D = BuildingActor->GetActorScale3D();

                    BuildingActor->K2_DestroyActor();

                    if (auto NewBuildingActor = (ABuildingSMActor*)SpawnActorTrans(NewBuildingClass, SpawnTransform, PC))
                    {
                        NewBuildingActor->SetMirrored(Params->bMirrored);
                        NewBuildingActor->InitializeKismetSpawnedBuildingActor(NewBuildingActor, PC);
                    }
                }
            }
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerEndEditingBuildingActor", {
            auto Params = (AFortPlayerController_ServerEndEditingBuildingActor_Params*)Parameters;
            auto PC = (AFortPlayerControllerAthena*)Object;

            if (Params->BuildingActorToStopEditing)
            {
                Params->BuildingActorToStopEditing->EditingPlayer = nullptr;
                Params->BuildingActorToStopEditing->OnRep_EditingPlayer();

                auto EditTool = (AFortWeap_EditingTool*)((APlayerPawn_Athena_C*)PC->Pawn)->CurrentWeapon;

                if (EditTool)
                {
                    EditTool->bEditConfirmed = true;
                    EditTool->EditActor = nullptr;
                    EditTool->OnRep_EditActor();
                }
            }
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerRepairBuildingActor", {
            auto Params = (AFortPlayerController_ServerRepairBuildingActor_Params*)Parameters;
            auto Controller = (AFortPlayerControllerAthena*)Object;
            auto Pawn = (APlayerPawn_Athena_C*)Controller->Pawn;

            if (Controller && Pawn && Params->BuildingActorToRepair)
            {
                Params->BuildingActorToRepair->RepairBuilding(Controller, 10); // figure out how to get the repair amount
            }
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerControllerAthena.ServerAttemptAircraftJump", {
            auto Params = (AFortPlayerControllerAthena_ServerAttemptAircraftJump_Params*)Parameters;
            auto PC = (AFortPlayerControllerAthena*)Object;
            auto GameState = (AAthena_GameState_C*)GetWorld()->AuthorityGameMode->GameState;

            if (PC && Params && !PC->Pawn && PC->IsInAircraft()) // TODO: Teleport the player's pawn instead of making a new one.
            {
                // ((AAthena_GameState_C*)GetWorld()->AuthorityGameMode->GameState)->Aircrafts[0]->PlayEffectsForPlayerJumped();
                auto Aircraft = (AFortAthenaAircraft*)GameState->Aircrafts[0];

                if (Aircraft)
                {
                    auto ExitLocation = Aircraft->K2_GetActorLocation();

                    // ExitLocation.Z -= 500;

                    InitPawn(PC, ExitLocation);
                    PC->ActivateSlot(EFortQuickBars::Primary, 0, 0, true); // Select the pickaxe

                    bool bFound = false;
                    auto PickaxeEntry = FindItemInInventory<UFortWeaponMeleeItemDefinition>(PC, bFound);

                    if (bFound)
                        EquipInventoryItem(PC, PickaxeEntry.ItemGuid);

                    // PC->Pawn->K2_TeleportTo(ExitLocation, Params->ClientRotation);
                }
            }
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerPawn.ServerReviveFromDBNO", {
            auto Params = (AFortPlayerPawn_ServerReviveFromDBNO_Params*)Parameters;
            auto DBNOPawn = (APlayerPawn_Athena_C*)Object;
            auto DBNOPC = (AFortPlayerControllerAthena*)DBNOPawn->Controller;
            auto InstigatorPC = (AFortPlayerControllerAthena*)Params->EventInstigator;

            if (InstigatorPC && DBNOPawn && DBNOPC)
            {
                DBNOPawn->bIsDBNO = false;
                DBNOPawn->OnRep_IsDBNO();

                DBNOPC->ClientOnPawnRevived(InstigatorPC);
                DBNOPawn->SetHealth(100);
            }
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerAttemptInteract", {
            auto Params = (AFortPlayerController_ServerAttemptInteract_Params*)Parameters;
            auto PC = (AFortPlayerControllerAthena*)Object;

            if (Params->ReceivingActor)
            {
                auto DBNOPawn = (APlayerPawn_Athena_C*)Params->ReceivingActor;
                auto DBNOPC = (AFortPlayerControllerAthena*)DBNOPawn->Controller;

                if (DBNOPawn && DBNOPC && DBNOPawn->IsA(APlayerPawn_Athena_C::StaticClass()))
                {
                    DBNOPawn->ReviveFromDBNO(PC);
                }
            }
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerPlayEmoteItem", {
            auto CurrentPC = (AFortPlayerControllerAthena*)Object;
            auto CurrentPawn = (APlayerPawn_Athena_C*)CurrentPC->Pawn;

            auto EmoteParams = (AFortPlayerController_ServerPlayEmoteItem_Params*)Parameters;

            if (CurrentPC && !CurrentPC->IsInAircraft() && CurrentPawn && EmoteParams->EmoteAsset)
            {
                if (auto Montage = EmoteParams->EmoteAsset->GetAnimationHardReference(CurrentPawn->CharacterBodyType, CurrentPawn->CharacterGender))
                {
                    CurrentPawn->PlayLocalAnimMontage(Montage, 1.0f, FName(0));
                    CurrentPawn->PlayAnimMontage(Montage, 1.0f, FName(0));
                    CurrentPawn->OnRep_CharPartAnimMontageInfo();
                    CurrentPawn->OnRep_ReplicatedAnimMontage();
                }
            }
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerAttemptInventoryDrop", {
            auto PC = (AFortPlayerControllerAthena*)Object;

            if (!PC->IsInAircraft())
            {
                auto Pawn = (APlayerPawn_Athena_C*)PC->Pawn;
                HandleInventoryDrop(Pawn, Parameters);
            }
        })

        DEFINE_PEHOOK("Function BP_VictoryDrone.BP_VictoryDrone_C.OnSpawnOutAnimEnded", {
            if (Object->IsA(ABP_VictoryDrone_C::StaticClass()))
            {
                auto Drone = (ABP_VictoryDrone_C*)Object;

                if (Drone)
                {
                    Drone->K2_DestroyActor();
                }
            }
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerExecuteInventoryItem", {
            EquipInventoryItem((AFortPlayerControllerAthena*)Object, *(FGuid*)Parameters);
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerReturnToMainMenu", {
            ((AFortPlayerController*)Object)->ClientTravel(L"Frontend", ETravelType::TRAVEL_Absolute, false, FGuid());
        })

        DEFINE_PEHOOK("Function FortniteGame.FortPlayerController.ServerLoadingScreenDropped", {
            auto Pawn = (APlayerPawn_Athena_C*)((AFortPlayerController*)Object)->Pawn;

            if (Pawn && Pawn->AbilitySystemComponent)
            {
                static auto AbilitySet = UObject::FindObject<UFortAbilitySet>("FortAbilitySet GAS_DefaultPlayer.GAS_DefaultPlayer");
                for (int i = 0; i < AbilitySet->GameplayAbilities.Num(); i++)
                {
                    auto Ability = AbilitySet->GameplayAbilities[i];

                    if (!Ability)
                        continue;

                    if (Ability->GetName().find("DBNO") == -1)
                    {
                        GrantGameplayAbility(Pawn, Ability);
                    }
                }
            }
        })

        DEFINE_PEHOOK("Function Engine.GameMode.ReadyToStartMatch", {
            if (!bListening)
            {
                Game::OnReadyToStartMatch();
                Listen();
                bListening = true;
            }
        })

        printf("[+] Hooked %zu UFunction(s)\n", toHook.size());
    }
}