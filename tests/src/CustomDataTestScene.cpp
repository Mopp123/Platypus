#include "CustomDataTestScene.hpp"
#include <string>


using namespace platypus;


CustomDataTestScene::CustomDataTestScene()
{
}

CustomDataTestScene::~CustomDataTestScene()
{
}

static void print_custom_data(
    const std::string& entityName,
    const CustomData * const pCustomData,
    const CustomDataManager& customDataManager
)
{
    std::vector<CustomDataValue> values = customDataManager.getValues(pCustomData->offset);
    Debug::log(
        "Entity: " + entityName + " (base offset = " + std::to_string(pCustomData->offset) + ") "
        "(total pool size: " + std::to_string(customDataManager.getStorageSize()) + " bytes) "
    );
    for (size_t i = 0; i < values.size(); ++i)
    {
        const CustomDataValue& value = values[i];
        std::string valueStr = "";
        Debug::log("    [" + std::to_string(i) + "] " + customDataManager.valueToString(value));
    }
}

void CustomDataTestScene::init()
{
    initBase();

    CustomDataManager& customDataManager = getCustomDataManager();

    // Test adding values to a single existing CustomComponent
    const std::string entityName1 = "Entity1";
    entityID_t customDataEntity1 = createEntity(entityName1);
    CustomData* pCustomDataComponent1 = create_custom_data(customDataEntity1);

    int32_t val = -6;
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::INT,
        &val
    );
    uint32_t val2 = 32;
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::UINT,
        &val2
    );

    float val3 = 12.47f;
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::FLOAT,
        &val3
    );

    Vector2f val4(1, 2);
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::VECTOR2F,
        &val4
    );

    Vector3f val5(4.2f, 10.5f, 7.25f);
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::VECTOR3F,
        &val5
    );

    Vector4f val6(5.5f, 2.0f, 10.9f, 11.6f);
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::VECTOR4F,
        &val6
    );


    std::vector<CustomDataValue> values = customDataManager.getValues(pCustomDataComponent1->offset);
    PLATYPUS_ASSERT(values.size() == 6);
    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test updating single value
    uint32_t newVal = 666;
    customDataManager.update(
        pCustomDataComponent1->offset,
        1,
        CustomDataType::UINT,
        &newVal
    );

    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test removing the last value
    customDataManager.remove(pCustomDataComponent1, 5);
    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test removing entity1 first value
    customDataManager.remove(pCustomDataComponent1, 0);
    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test removing entity1 second to last
    customDataManager.remove(pCustomDataComponent1, 2);
    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test adding values for another entity
    const std::string entityName2 = "Entity2";
    entityID_t customDataEntity2 = createEntity(entityName2);
    CustomData* pCustomDataComponent2 = create_custom_data(customDataEntity2);

    Vector4f entity2Val1(3, 4, 5, 12);
    customDataManager.add(
        pCustomDataComponent2,
        CustomDataType::VECTOR4F,
        &entity2Val1
    );

    uint32_t entity2Val2 = 671;
    customDataManager.add(
        pCustomDataComponent2,
        CustomDataType::INT,
        &entity2Val2
    );

    // Test adding string value to entity2
    std::string entity2String = "Test string";
    customDataManager.addString(
        pCustomDataComponent2,
        entity2String
    );
    print_custom_data(entityName2, pCustomDataComponent2, customDataManager);

    // TODO:
    // *Test updating string values
    // *Test removing string values
    // *Test readding string values
    CONTINUE HERE!


    Debug::log("Test success!");
    PLATYPUS_ASSERT(false);
}

void CustomDataTestScene::update()
{
}
