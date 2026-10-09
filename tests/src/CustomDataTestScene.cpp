#include "CustomDataTestScene.hpp"
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

    int32_t val = -6;
    customDataManager.add(
        pCustomDataComponent1,
        CustomDataType::INT,
        sizeof(int32_t),
        &val
    );

    std::vector<CustomDataValue> values = customDataManager.getValues(pCustomDataComponent1->offset);
    PLATYPUS_ASSERT(values.size() == 1);

    int32_t testVal = *(reinterpret_cast<const int32_t*>(values[0].pData));
    Debug::log("testVal = " + std::to_string(testVal));

    // TODO: More tests and easier way to access CustomData values
    CONTINUE HERE!

    Debug::log("Test success!");
    PLATYPUS_ASSERT(false);
}

void CustomDataTestScene::update()
{
}
