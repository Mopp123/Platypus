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

}

void CustomDataTestScene::update()
{
}
