#include <mod/amlmod.h>
#include <mod/logger.h>
#include "GTASA_STRUCTS.h"

MYMOD(net.juniordjjr.rusjj.male01, FixMALE01, 1.0, JuniorDjjr & RusJJ)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.0.2.2)
END_DEPLIST()

uintptr_t pGTASA;
void* hGTASA;

bool (*PedStreamedInForThisGang)(int);
bool (*PickStreamedInPedForThisGang)(int, int*);
int (*PickPedMIToStreamInForCurrentZone)();
void (*RequestModel)(int, int);
void (*LoadAllRequestedModels)(bool);
void (*SetModelIsDeletable)(int);

CBaseModelInfo** ms_modelInfoPtrs;
uint16_t **m_PedGroups;
tPedGroupTranslationData* m_TranslationArray;
uint32_t *CurrentWorldZone;



int LoadSomePedModel(int gangId, bool loadNow)
{
    int model = MODEL_MALE01;
    if (gangId >= 0)
    {
        if (PedStreamedInForThisGang(gangId)) // any ped loaded for this gang
        {
            if (PickStreamedInPedForThisGang(gangId, &model))
            {
                return model;
            }
        }
        model = m_PedGroups[m_TranslationArray[gangId + 18].pedGroupIds[0]][0];
    }
    else
    {
        CPedModelInfo *modelInfo;
        int tries = 0;
        do
        {
            tries++;
            if (tries > 30)
            {
                model = MODEL_MALE01;
                break;
            }
            model = PickPedMIToStreamInForCurrentZone();
            modelInfo = (CPedModelInfo *)ms_modelInfoPtrs[model];
            if (!modelInfo) continue;
        }
        while (
            !modelInfo ||
            (modelInfo->m_defaultPedStats >= 4  && modelInfo->m_defaultPedStats <= 13) ||
            (modelInfo->m_defaultPedStats <= 26 && modelInfo->m_defaultPedStats <= 32) ||
            (modelInfo->m_defaultPedStats <= 38 && modelInfo->m_defaultPedStats <= 41)
        );
    }

    if (model <= 0) model = MODEL_MALE01;

    if (loadNow && model != MODEL_MALE01)
    {
        RequestModel(model, eStreamingFlags::STREAMING_GAME_REQUIRED | eStreamingFlags::STREAMING_PRIORITY_REQUEST);
        LoadAllRequestedModels(true);
        SetModelIsDeletable(model);
        //SetModelTxdIsDeletable(model); // Empty on Android <- all textures are always loaded?
    }
    logger->Info("At the end it's %d!", model);
    return model;
}

DECL_HOOK(int, ChooseCivilianOccupation, bool a1, bool a2, int a3, int a4, int a5, bool a6, bool a7, bool a8, char* a9)
{
    int model = ChooseCivilianOccupation(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    if(model == MODEL_MALE01)
    {
        model = LoadSomePedModel(a5 - PEDSTAT_GANG1, true);
    }
    return model;
}
DECL_HOOK(int, ChooseCivilianOccupationForVehicle, bool a1, CVehicle* a2)
{
    int model = ChooseCivilianOccupationForVehicle(a1, a2);
    if(model == MODEL_MALE01) return LoadSomePedModel(-1, true);
    return model;
}

extern "C" void OnModLoad()
{
    logger->SetTag("Male01Fix");

    pGTASA = aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");

    SET_TO(PedStreamedInForThisGang, aml->GetSym(hGTASA, "_ZN9CGangWars24PedStreamedInForThisGangEi"));
    SET_TO(PickStreamedInPedForThisGang, aml->GetSym(hGTASA, "_ZN9CGangWars28PickStreamedInPedForThisGangEiPi"));
    SET_TO(PickPedMIToStreamInForCurrentZone, aml->GetSym(hGTASA, "_ZN9CPopCycle33PickPedMIToStreamInForCurrentZoneEv"));
    SET_TO(RequestModel, aml->GetSym(hGTASA, "_ZN10CStreaming12RequestModelEii"));
    SET_TO(LoadAllRequestedModels, aml->GetSym(hGTASA, "_ZN10CStreaming22LoadAllRequestedModelsEb"));
    SET_TO(SetModelIsDeletable, aml->GetSym(hGTASA, "_ZN10CStreaming19SetModelIsDeletableEi"));
    
    SET_TO(ms_modelInfoPtrs, *(uintptr_t*)(pGTASA + 0x6796D4));
    SET_TO(m_PedGroups, *(uintptr_t*)(pGTASA + 0x6773A4));
    SET_TO(m_TranslationArray, *(uintptr_t*)(pGTASA + 0x678990));
    SET_TO(CurrentWorldZone, aml->GetSym(hGTASA, "_ZN11CPopulation16CurrentWorldZoneE"));

    HOOK(ChooseCivilianOccupation, aml->GetSym(hGTASA, "_ZN11CPopulation24ChooseCivilianOccupationEbbiiibbbPc"));
    HOOK(ChooseCivilianOccupationForVehicle, aml->GetSym(hGTASA, "_ZN11CPopulation34ChooseCivilianOccupationForVehicleEbP8CVehicle"));
}
