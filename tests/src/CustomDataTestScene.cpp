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

    CustomDataManager& customDataManager = getCustomDataManager();

    // Test adding values to a single existing CustomComponent
    entityID_t customDataEntity1 = createEntity();
    CustomData* pCustomDataComponent1 = create_custom_data(customDataEntity1);

    customDataManager.addNumericValue<Vector4f>(pCustomDataComponent1, CustomDataType::VECTOR4F, { 0.2f, 1.2f, 4.3f, 2.0f });
    customDataManager.addNumericValue<int32_t>(pCustomDataComponent1, CustomDataType::INT, -4);
    customDataManager.addNumericValue<float>(pCustomDataComponent1, CustomDataType::FLOAT, 1.24f);
    customDataManager.addNumericValue<Vector2f>(pCustomDataComponent1, CustomDataType::VECTOR2F, { 1, 2 });
    customDataManager.addNumericValue<uint32_t>(pCustomDataComponent1, CustomDataType::UINT, 32);
    customDataManager.addNumericValue<Vector3f>(pCustomDataComponent1, CustomDataType::VECTOR3F, { 0.3f, 2.123f, 3.25f });
    PLATYPUS_ASSERT(pCustomDataComponent1->offset == 0);
    PLATYPUS_ASSERT(pCustomDataComponent1->elementCount == 6);

    PLATYPUS_ASSERT(customDataManager.getNumericValue<Vector4f>(pCustomDataComponent1, 0) == Vector4f(0.2f, 1.2f, 4.3f, 2.0f));
    PLATYPUS_ASSERT(customDataManager.getNumericValue<int32_t>(pCustomDataComponent1, 1) == -4);
    PLATYPUS_ASSERT(customDataManager.getNumericValue<float>(pCustomDataComponent1, 2) == 1.24f);
    PLATYPUS_ASSERT(customDataManager.getNumericValue<Vector2f>(pCustomDataComponent1, 3) == Vector2f(1, 2));
    PLATYPUS_ASSERT(customDataManager.getNumericValue<uint32_t>(pCustomDataComponent1, 4) == 32);
    PLATYPUS_ASSERT(customDataManager.getNumericValue<Vector3f>(pCustomDataComponent1, 5) == Vector3f(0.3f, 2.123f, 3.25f));

    const size_t entity1StorageSize = customDataManager.getStorageSize(pCustomDataComponent1);
    const size_t expectedEntity1StorageSize = sizeof(uint32_t) + // value count
        customDataManager.getValueBaseSize() + sizeof(Vector4f) +
        customDataManager.getValueBaseSize() + sizeof(int32_t) +
        customDataManager.getValueBaseSize() + sizeof(float) +
        customDataManager.getValueBaseSize() + sizeof(Vector2f) +
        customDataManager.getValueBaseSize() + sizeof(uint32_t) +
        customDataManager.getValueBaseSize() + sizeof(Vector3f);

    PLATYPUS_ASSERT(entity1StorageSize == expectedEntity1StorageSize);

    // Test adding another CustomData component for another entity
    entityID_t customDataEntity2 = createEntity();
    CustomData* pCustomDataComponent2 = create_custom_data(customDataEntity2);
    // Wasn't originally supposed to call this for pCustomDataComponent1 (was supposed to call
    // for pCustomDataComponent2) -> but this somehow fucks this shit up!?!???!!
    CONTINUE HERE, SHIT's FUCKED!
    customDataManager.addNumericValue<float>(pCustomDataComponent1, CustomDataType::FLOAT, 45.6f);

    PLATYPUS_ASSERT(pCustomDataComponent2->offset == entity1StorageSize);

    Debug::log("Test was successful!");
    PLATYPUS_ASSERT(false);
}

void CustomDataTestScene::update()
{
}
