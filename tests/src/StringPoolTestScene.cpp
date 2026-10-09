#include "StringPoolTestScene.hpp"
#include <string>


using namespace platypus;


StringPoolTestScene::StringPoolTestScene()
{
}

StringPoolTestScene::~StringPoolTestScene()
{
}

static void print_string_pool_mem_order(const StringPool& pool)
{
    std::vector<std::string> poolStrings = pool.getAll();
    Debug::log("String pool(size: " + std::to_string(pool.getStorageSize()) +" bytes)");
    for (size_t i = 0; i < poolStrings.size(); ++i)
        Debug::log("    [" + std::to_string(i) + "] " + poolStrings[i]);
}

void StringPoolTestScene::init()
{
    initBase();

    int32_t strOffset1 = _stringPool.add("Test string1");
    int32_t strOffset2 = _stringPool.add("Another testing");

    std::vector<std::string> poolStrings = _stringPool.getAll();
    PLATYPUS_ASSERT(poolStrings.size() == 2);

    print_string_pool_mem_order(_stringPool);

    // Test modifying second string
    strOffset2 = _stringPool.update(strOffset2, "Changed another");

    poolStrings = _stringPool.getAll();
    print_string_pool_mem_order(_stringPool);

    // Test modifying first string
    strOffset1 = _stringPool.update(strOffset1, "First one changed to longer");

    // Test getting individual strings using their offsets
    Debug::log("Updated string pool(size: " + std::to_string(_stringPool.getStorageSize()) + " bytes)");
    Debug::log("    " + _stringPool.get(strOffset1));
    Debug::log("    " + _stringPool.get(strOffset2));

    // Test adding third
    int32_t strOffset3 = _stringPool.add("Third string");

    Debug::log("Updated string pool(size: " + std::to_string(_stringPool.getStorageSize()) + " bytes)");
    Debug::log("    " + _stringPool.get(strOffset1));
    Debug::log("    " + _stringPool.get(strOffset2));
    Debug::log("    " + _stringPool.get(strOffset3));

    // Test removing middle
    _stringPool.remove(strOffset2);

    poolStrings = _stringPool.getAll();
    PLATYPUS_ASSERT(poolStrings.size() == 2);
    print_string_pool_mem_order(_stringPool);


    // Test removing first
    _stringPool.remove(strOffset1);

    poolStrings = _stringPool.getAll();
    PLATYPUS_ASSERT(poolStrings.size() == 1);
    print_string_pool_mem_order(_stringPool);


    // Test removing the last remaining
    _stringPool.remove(strOffset3);

    poolStrings = _stringPool.getAll();
    PLATYPUS_ASSERT(poolStrings.empty());

    // Test adding to emptied pool
    strOffset1 = _stringPool.add("Added long one to empty pool");
    _stringPool.add("Little shorter one");
    _stringPool.add("yet another one");
    print_string_pool_mem_order(_stringPool);

    _stringPool.add("1234567");
    print_string_pool_mem_order(_stringPool);

    Debug::log("Test success!");

    PLATYPUS_ASSERT(false);
}

void StringPoolTestScene::update()
{
}
