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
    customDataManager.addNumber(
        pCustomDataComponent1,
        CustomDataType::INT,
        &val
    );

    // Test adding empty string
    customDataManager.addString(pCustomDataComponent1, "");

    val = 420;
    customDataManager.addNumber(
        pCustomDataComponent1,
        CustomDataType::INT,
        &val
    );

    // Test altering the prior empty str
    customDataManager.updateString(
        pCustomDataComponent1->offset,
        1,
        "Now theres some data here"
    );

    print_custom_data(
        entityName1,
        pCustomDataComponent1,
        customDataManager
    );

    /*
    uint32_t val2 = 32;
    customDataManager.addNumber(
        pCustomDataComponent1,
        CustomDataType::UINT,
        &val2
    );

    std::string val3 = "Good morning!";
    customDataManager.addString(
        pCustomDataComponent1,
        val3
    );

    Vector2f val4(1, 2);
    customDataManager.addNumber(
        pCustomDataComponent1,
        CustomDataType::VECTOR2F,
        &val4
    );

    std::string val5 = "Asd a bit longer string here...";
    customDataManager.addString(
        pCustomDataComponent1,
        val5
    );

    Vector3f val6(4.2f, 10.5f, 7.25f);
    customDataManager.addNumber(
        pCustomDataComponent1,
        CustomDataType::VECTOR3F,
        &val6
    );

    Vector3f val7(4.2f, 10.5f, 7.25f);
    customDataManager.addNumber(
        pCustomDataComponent1,
        CustomDataType::VECTOR3F,
        &val7
    );

    Vector4f val8(5.5f, 2.0f, 10.9f, 11.6f);
    customDataManager.addNumber(
        pCustomDataComponent1,
        CustomDataType::VECTOR4F,
        &val8
    );


    std::vector<CustomDataValue> values = customDataManager.getValues(pCustomDataComponent1->offset);
    PLATYPUS_ASSERT(values.size() == 8);
    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test updating single value
    uint32_t newVal = 666;
    customDataManager.updateNumber(
        pCustomDataComponent1->offset,
        1,
        CustomDataType::UINT,
        &newVal
    );

    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test removing entity1 last value
    customDataManager.remove(pCustomDataComponent1, 5);
    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test removing entity1 first value
    customDataManager.remove(pCustomDataComponent1, 0);
    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test removing entity1 second to last
    customDataManager.remove(pCustomDataComponent1, 2);
    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test removing entity1 first string
    customDataManager.remove(pCustomDataComponent1, 1);
    print_custom_data(entityName1, pCustomDataComponent1, customDataManager);

    // Test adding values for another entity
    const std::string entityName2 = "Entity2";
    entityID_t customDataEntity2 = createEntity(entityName2);
    CustomData* pCustomDataComponent2 = create_custom_data(customDataEntity2);

    Vector4f entity2Val1(3, 4, 5, 12);
    customDataManager.addNumber(
        pCustomDataComponent2,
        CustomDataType::VECTOR4F,
        &entity2Val1
    );

    uint32_t entity2Val2 = 671;
    customDataManager.addNumber(
        pCustomDataComponent2,
        CustomDataType::INT,
        &entity2Val2
    );

    // Test adding string value to entity2
    std::string entity2String = "";
    customDataManager.addString(
        pCustomDataComponent2,
        "Entity2 test string"
    );
    print_custom_data(entityName2, pCustomDataComponent2, customDataManager);

    // Test updating entity2 string
    customDataManager.updateString(
        pCustomDataComponent2->offset,
        2,
        "Changed string value here"
    );
    print_custom_data(entityName2, pCustomDataComponent2, customDataManager);

    // Test readding string value to entity2
    customDataManager.remove(
        pCustomDataComponent2,
        2
    );
    print_custom_data(entityName2, pCustomDataComponent2, customDataManager);

    customDataManager.addString(
        pCustomDataComponent2,
        "Readded string value"
    );
    print_custom_data(entityName2, pCustomDataComponent2, customDataManager);

    // Test serialization
    std::vector<char> serializedData = serialize({ customDataEntity1, customDataEntity2 });
    write_file("assets/scenes/CustomDataTest.scene", serializedData);
    */

    // Test deserialization
    /*
    std::vector<char> serializedData = read_file("assets/scenes/CustomDataTest.scene");
    PLATYPUS_ASSERT(!serializedData.empty());
    std::vector<entityID_t> deserializedEntities = deserialize(serializedData, 0);
    PLATYPUS_ASSERT(deserializedEntities.size() == 2);
    for (entityID_t entityID : deserializedEntities)
    {
        std::string entityName = getEntityName(entityID);
        PLATYPUS_ASSERT(!entityName.empty());
        void* pCustomDataComponent = getComponent(entityID, ComponentType::COMPONENT_TYPE_CUSTOM_DATA);
        PLATYPUS_ASSERT(pCustomDataComponent);
        CustomData* pCustomData = reinterpret_cast<CustomData*>(pCustomDataComponent);
        print_custom_data(
            entityName,
            pCustomData,
            customDataManager
        );
    }
    */

    Debug::log("Test success!");
    PLATYPUS_ASSERT(false);
}

void CustomDataTestScene::update()
{
}
