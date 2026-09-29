#include "CustomDataTestScene.hpp"
#include "WaterTestScene.hpp"
#include <string>


using namespace platypus;


CustomDataTestScene::CustomDataTestScene()
{
}

CustomDataTestScene::~CustomDataTestScene()
{
}

void CustomDataTestScene::init()
{
    initBase();

    entityID_t customDataEntity = createEntity();
    CustomData* pCustomDataComponent = create_custom_data(customDataEntity);

    CustomDataManager& customDataManager = getCustomDataManager();
    customDataManager.addNumericValue<int32_t>(pCustomDataComponent, CustomDataType::INT, -4);
    customDataManager.addNumericValue<int32_t>(pCustomDataComponent, CustomDataType::INT, 128);
    customDataManager.addStringValue(pCustomDataComponent, "Test string here");
    customDataManager.addNumericValue<Vector2f>(pCustomDataComponent, CustomDataType::VECTOR2F, { 22.1f, 841.2f });
    customDataManager.addNumericValue<float>(pCustomDataComponent, CustomDataType::FLOAT, 54.12f);
    customDataManager.addNumericValue<Vector3f>(pCustomDataComponent, CustomDataType::VECTOR3F, { 12.5f, 32.0f, 11.2f });
    customDataManager.addNumericValue<Vector4f>(pCustomDataComponent, CustomDataType::VECTOR4F, { 98.0f, 76.26f, 10.2f, 89.123f });

    //std::vector<CustomDataManager::Value> values = customDataManager.getValues(pCustomDataComponent->offset);
    //PLATYPUS_ASSERT(values.size() == 7);

    int32_t val0 = customDataManager.getNumericValue<int32_t>(pCustomDataComponent, 0);
    int32_t val1 = customDataManager.getNumericValue<int32_t>(pCustomDataComponent, 1);
    std::string strVal1 = customDataManager.getStringValue(pCustomDataComponent, 2);
    Vector2f val2 = customDataManager.getNumericValue<Vector2f>(pCustomDataComponent, 3);
    float val3 = customDataManager.getNumericValue<float>(pCustomDataComponent, 4);
    Vector3f val4 = customDataManager.getNumericValue<Vector3f>(pCustomDataComponent, 5);
    Vector3f val5 = customDataManager.getNumericValue<Vector4f>(pCustomDataComponent, 6);

    Debug::log(
        "___TEST___Custom values:\n"
        "   [0] = " + std::to_string(val0) + "\n"
        "   [1] = " + std::to_string(val1) + "\n"
        "   [2] = '" + strVal1 + "'\n"
        "   [3] = " + val2.toString() + "\n"
        "   [4] = " + std::to_string(val3) + "\n"
        "   [5] = " + val4.toString() + "\n"
        "   [6] = " + val5.toString() + "\n\n"
    );


    customDataManager.updateNumericValue<int32_t>(
        pCustomDataComponent,
        0,
        666
    );

    customDataManager.updateNumericValue<Vector2f>(
        pCustomDataComponent,
        3,
        { 333, 666 }
    );

    customDataManager.updateNumericValue<float>(
        pCustomDataComponent,
        4,
        420.69f
    );

    customDataManager.updateNumericValue<Vector3f>(
        pCustomDataComponent,
        5,
        { 32, 64, 128.5f }
    );

    customDataManager.updateNumericValue<Vector4f>(
        pCustomDataComponent,
        6,
        { 0, 1.2f, 2.4f, 3.5f }
    );

    customDataManager.updateStringValue(
        pCustomDataComponent,
        2,
        "Changed"
    );

    val0 = customDataManager.getNumericValue<int32_t>(pCustomDataComponent, 0);
    val1 = customDataManager.getNumericValue<int32_t>(pCustomDataComponent, 1);
    strVal1 = customDataManager.getStringValue(pCustomDataComponent, 2);
    val2 = customDataManager.getNumericValue<Vector2f>(pCustomDataComponent, 3);
    val3 = customDataManager.getNumericValue<float>(pCustomDataComponent, 4);
    val4 = customDataManager.getNumericValue<Vector3f>(pCustomDataComponent, 5);
    val5 = customDataManager.getNumericValue<Vector4f>(pCustomDataComponent, 6);

    Debug::log(
        "___TEST___Custom values:\n"
        "   [0] = " + std::to_string(val0) + "\n"
        "   [1] = " + std::to_string(val1) + "\n"
        "   [2] = '" + strVal1 + "'\n"
        "   [3] = " + val2.toString() + "\n"
        "   [4] = " + std::to_string(val3) + "\n"
        "   [5] = " + val4.toString() + "\n"
        "   [6] = " + val5.toString() + "\n\n"
    );
}

void CustomDataTestScene::update()
{
}
