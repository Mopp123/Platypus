#include "CustomDataTestScene.hpp"
#include <string>


using namespace platypus;


CustomDataTestScene::CustomDataTestScene()
{
}

CustomDataTestScene::~CustomDataTestScene()
{
}

static void print_custom_data(const CustomData * const pCustomData, const CustomDataManager& customDataManager)
{
    std::vector<CustomDataValue> values = customDataManager.getValues(pCustomData->offset);
    Debug::log("CustomData (total pool size: " + std::to_string(customDataManager.getStorageSize()) + " bytes) ");
    for (size_t i = 0; i < values.size(); ++i)
    {
        const CustomDataValue& value = values[i];
        std::string valueStr = "";
        Debug::log("    [" + std::to_string(i) + "] " + custom_data_value_to_string(value));
    }
}

void CustomDataTestScene::init()
{
    initBase();

    CustomDataManager& customDataManager = getCustomDataManager();

    // Test adding values to a single existing CustomComponent
    entityID_t customDataEntity1 = createEntity();
    CustomData* pCustomDataComponent1 = create_custom_data(customDataEntity1);

    int32_t val = -6;
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::INT,
        sizeof(int32_t),
        &val
    );
    uint32_t val2 = 32;
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::UINT,
        sizeof(uint32_t),
        &val2
    );

    float val3 = 12.47f;
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::FLOAT,
        sizeof(float),
        &val3
    );

    Vector2f val4(1, 2);
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::VECTOR2F,
        sizeof(Vector2f),
        &val4
    );

    Vector3f val5(4.2f, 10.5f, 7.25f);
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::VECTOR3F,
        sizeof(Vector3f),
        &val5
    );

    Vector4f val6(5.5f, 2.0f, 10.9f, 11.6f);
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::VECTOR4F,
        sizeof(Vector4f),
        &val6
    );

    std::vector<CustomDataValue> values = customDataManager.getValues(pCustomDataComponent1->offset);
    PLATYPUS_ASSERT(values.size() == 6);
    print_custom_data(pCustomDataComponent1, customDataManager);

    // Test updating single value
    uint32_t newVal = 666;
    customDataManager.update(
        pCustomDataComponent1->offset,
        1,
        sizeof(uint32_t),
        CustomDataType::UINT,
        &newVal
    );

    print_custom_data(pCustomDataComponent1, customDataManager);

    // TODO: More tests and easier way to access CustomData values

    Debug::log("Test success!");
    PLATYPUS_ASSERT(false);
}

void CustomDataTestScene::update()
{
}
