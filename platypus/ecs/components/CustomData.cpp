#include "CustomData.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    size_t get_custom_data_type_size(CustomDataType type)
    {
        switch (type)
        {
            case CustomDataType::NONE: return 0;
            case CustomDataType::INT: return sizeof(int32_t);
            case CustomDataType::UINT: return sizeof(uint32_t);
            case CustomDataType::FLOAT: return sizeof(float);
            case CustomDataType::STRING: return sizeof(uint32_t);
            case CustomDataType::VECTOR2F: return sizeof(Vector2f);
            case CustomDataType::VECTOR3F: return sizeof(Vector3f);
            case CustomDataType::VECTOR4F: return sizeof(Vector4f);
        }
        return 0;
    }

    std::string custom_data_type_to_string(CustomDataType type)
    {
        switch (type)
        {
            case CustomDataType::NONE: return "NONE";
            case CustomDataType::INT: return "INT";
            case CustomDataType::UINT: return "UINT";
            case CustomDataType::FLOAT: return "FLOAT";
            case CustomDataType::STRING: return "STRING";
            case CustomDataType::VECTOR2F: return "VECTOR2F";
            case CustomDataType::VECTOR3F: return "VECTOR3F";
            case CustomDataType::VECTOR4F: return "VECTOR4F";
        }
        return "Invalid type";
    }

    std::vector<CustomDataType> get_available_custom_data_types()
    {
        return {
            CustomDataType::INT,
            CustomDataType::UINT,
            CustomDataType::FLOAT,
            CustomDataType::STRING,
            CustomDataType::VECTOR2F,
            CustomDataType::VECTOR3F,
            CustomDataType::VECTOR4F
        };
    }


    CustomData* create_custom_data(
        entityID_t target,
        const std::vector<SerializedCustomDataValue>& values,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_custom_data"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_CUSTOM_DATA;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "Failed to allocate Transform component for entity: " + std::to_string(target),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (!useExplicitComponentMask)
            pUseScene->addToComponentMask(target, componentType);

        CustomData* pCustomData = reinterpret_cast<CustomData*>(pComponent);

        CustomDataManager& customDataManager = pScene->getCustomDataManager();
        int32_t offset = -1;
        for (const SerializedCustomDataValue& value : values)
        {
            offset = customDataManager.add(
                pCustomData,
                value.type,
                value.dataSize,
                value.data.data()
            );
        }
        if (!values.empty())
        {
            PLATYPUS_ASSERT(offset != -1);
        }

        pCustomData->offset = offset;
        pCustomData->elementCount = static_cast<uint32_t>(values.size());

        return pCustomData;
    }

    size_t get_serialized_custom_data_value_size(const CustomDataValue * const pCustomDataValue)
    {
        return sizeof(CustomDataType) +
            sizeof(uint32_t) +
            pCustomDataValue->usedDataSize;
    }

    size_t get_serialized_custom_data_size(const CustomData * const pCustomData)
    {
        // NOTE: WARNING! Issue if this func is used outside of the current scene!!!
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        CustomDataManager& customDataManager = pScene->getCustomDataManager();
        std::vector<CustomDataValue> values = customDataManager.getValues(pCustomData->offset);

        size_t serializedValuesSize = 0;
        for (const CustomDataValue& value : values)
            serializedValuesSize += get_serialized_custom_data_value_size(&value);

        return sizeof(ComponentType) +
            sizeof(uint32_t) + // value(element) count
            serializedValuesSize;
    }

    size_t get_serialized_custom_data_size(const char* pSerializedData, size_t dataSize)
    {
        PLATYPUS_ASSERT(dataSize >= serialized_custom_data_base_size);
        ComponentType componentType;
        memcpy(&componentType, pSerializedData, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_CUSTOM_DATA);
        size_t offset = sizeof(ComponentType);

        uint32_t elementCount;
        memcpy(&elementCount, pSerializedData + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        size_t valueOffset = offset;
        size_t totalSize = serialized_custom_data_base_size;
        for (uint32_t i = 0; i < elementCount; ++i)
        {
            valueOffset += sizeof(CustomDataType);
            uint32_t valueDataSize = 0;
            memcpy(&valueDataSize, pSerializedData + valueOffset, sizeof(uint32_t));
            totalSize += sizeof(CustomDataType) +
                sizeof(uint32_t) + // element count
                static_cast<size_t>(valueDataSize); // value data size
        }

        return totalSize;
    }

    /*
        Serialized format:
            CustomDataType type
            uint32_t dataSize
                *NOTE: this is the actual storage size(used size)
                    -> doesn't make sense to serialize usedDataSize and maxDataSize separately!
            uint8_t dataBuffer[dataSize]
    */
    std::vector<char> serialize(const CustomDataValue * const pCustomDataValue)
    {
        const size_t serializedSize = get_serialized_custom_data_value_size(pCustomDataValue);
        std::vector<char> serializedData(serializedSize);
        char* pBuf = serializedData.data();
        memcpy(pBuf, &pCustomDataValue->type, sizeof(CustomDataType));
        size_t offset = sizeof(CustomDataType);

        const uint32_t serializedDataSize = pCustomDataValue->usedDataSize;
        memcpy(pBuf + offset, &serializedDataSize, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        memcpy(pBuf + offset, pCustomDataValue->pData, serializedDataSize);
        offset += serializedDataSize;
        PLATYPUS_ASSERT(offset == serializedSize);

        return serializedData;
    }

    /*
        Serialized format:
            ComponentType type
            uint32_t elementCount
            CustomDataValue(serialized form) values[elementCount]
    */
    std::vector<char> serialize(const CustomData * const pCustomData)
    {
        // NOTE: WARNING! Issue if this func is used outside of the current scene!!!
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        CustomDataManager& customDataManager = pScene->getCustomDataManager();

        const size_t serializedSize = get_serialized_custom_data_size(pCustomData);
        std::vector<char> serializedData(serializedSize);

        char* pBuf = serializedData.data();
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_CUSTOM_DATA;
        memcpy(pBuf, &componentType, sizeof(ComponentType));
        size_t offset = sizeof(ComponentType);

        memcpy(pBuf + offset, &pCustomData->elementCount, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        std::vector<CustomDataValue> values = customDataManager.getValues(pCustomData->offset);
        std::vector<char> fullSerializedValuesData;
        // NOTE: No idea does this work, did this quite tired..
        for (const CustomDataValue& value : values)
        {
            std::vector<char> serializedValueData = serialize(&value);
            memcpy(pBuf + offset, serializedValueData.data(), serializedValueData.size());
            offset += serializedValueData.size();
        }

        return serializedData;
    }

    void deserialize(
        Scene* pScene,
        CustomData** ppCustomData,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        const size_t baseSize = sizeof(ComponentType) +
            sizeof(uint32_t); // elementCount(valueCount)

        PLATYPUS_ASSERT(dataSize > baseSize);

        const uint8_t* pBuf = reinterpret_cast<const uint8_t*>(pData);

        ComponentType componentType;
        memcpy(&componentType, pBuf, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_TRANSFORM);
        size_t offset = sizeof(ComponentType);

        uint32_t elementCount;
        memcpy(&elementCount, pBuf + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        std::vector<SerializedCustomDataValue> serializedValues(elementCount);
        for (uint32_t i = 0; i < elementCount; ++i)
        {
            CustomDataType valueType;
            memcpy(&valueType, pBuf + offset, sizeof(CustomDataType));
            offset += sizeof(CustomDataType);

            uint32_t valueSize;
            memcpy(&valueSize, pBuf + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            std::vector<uint8_t> valueData(valueSize);
            memcpy(valueData.data(), pBuf + offset, valueSize);
            offset += valueSize;

            serializedValues[i] = {
                valueType,
                valueSize,
                valueData
            };
        }

        // TODO:
        // How the fuck assemble the actual CustomData components in Scene's "post deserialization"
        // -> needed for constructing the actual component:
        //  *elem count
        //  *serialized value data

        // NOTE: While adding this noticed that the Scene's deserialization "special cases" needs
        // to be more streamlined and coherent!
        // TODO: Make "special cases" of Scene's deserialization more streamlined
        // *Additional note: don't make this an additional "hard coded deserialization case"
        //CONTINUE HERE + READ THE FUCKING COMMENTS ABOVE!

        *ppCustomData = create_custom_data(
            entityID,
            serializedValues,
            pScene,
            true
        );
    }


    CustomDataManager::CustomDataManager() :
        _memoryPool(
            free_storage_func,
            this,
            validate_free_range_func,
            this
        )
    {
    }

    int32_t CustomDataManager::add(
        CustomData* pCustomData,
        CustomDataType type,
        size_t dataSize,
        const void* pData
    )
    {
        const size_t dataTypeSize = get_custom_data_type_size(type);
        if (dataSize != dataTypeSize)
        {
            Debug::log(
                "Invalid dataSize(" + std::to_string(dataSize) + ") for type: " + custom_data_type_to_string(type),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return -1;
        }

        if ((pCustomData->offset == -1 && pCustomData->elementCount != 0) ||
            (pCustomData->offset != -1 && pCustomData->elementCount == 0))
        {
            Debug::log(
                "If pCustomData->offset == -1 elementCount must be 0 and vice versa",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return -1;
        }

        if (pCustomData->offset == -1 && pCustomData->elementCount == 0)
        {
            std::vector<uint8_t> toAdd(sizeof(uint32_t) + sizeof(CustomDataType) + dataSize);
            const uint32_t elemCount = 1;
            memcpy(toAdd.data(), &elemCount, sizeof(uint32_t));
            memcpy(toAdd.data() + sizeof(uint32_t), &type, sizeof(CustomDataType));
            memcpy(toAdd.data() + sizeof(uint32_t) + sizeof(CustomDataType), pData, dataSize);
            const int32_t newOffset = _memoryPool.add(
                -1,
                0,
                toAdd.size(),
                toAdd.data()
            );
            PLATYPUS_ASSERT(newOffset != -1);
            pCustomData->offset = newOffset;
            ++pCustomData->elementCount;
            return newOffset;
        }

        std::vector<StoredCustomDataValue> currentValues = copyValues(pCustomData->offset);
        StoredCustomDataValue newValue = { type };
        newValue.data.resize(dataSize);
        memcpy(newValue.data.data(), pData, dataSize);
        currentValues.push_back(newValue);

        const uint32_t newElemCount = pCustomData->elementCount + 1;
        PLATYPUS_ASSERT(newElemCount == currentValues.size());

        // DynamicElementSizeMemoryPool should handle this well enough?
        // NOT: NOT TESTED!
        // TODO: Make sure the mem is used efficiently enought with this in all cases!
        //
        // TODO: TOP PRIO!
        // *if the mem pool has free offsets at the end of the storage but the size isn't enough
        //  ->use those free offsets(should only have one at the end since packing) and add the
        //  remaining required size!!

        const uint8_t* pStorage = _memoryPool.getStorage().data();
        const size_t currentOffset = static_cast<const size_t>(pCustomData->offset);
        uint32_t currentSize = 0;
        memcpy(&currentSize, pStorage + currentOffset, sizeof(uint32_t));

        _memoryPool.freeRange(pCustomData->offset, currentSize);

        std::vector<uint8_t> toAdd(sizeof(uint32_t));
        memcpy(toAdd.data(), &newElemCount, sizeof(uint32_t));
        for (size_t i = 0; i < newElemCount; ++i)
        {
            const StoredCustomDataValue valueToAdd = currentValues[i];
            size_t valueSize = get_custom_data_type_size(valueToAdd.type);
            size_t prevToAddSize = toAdd.size();
            toAdd.resize(prevToAddSize + sizeof(CustomDataType) + valueSize);
            memcpy(
                toAdd.data() + prevToAddSize,
                &valueToAdd.type,
                sizeof(CustomDataType)
            );
            memcpy(
                toAdd.data() + prevToAddSize + sizeof(CustomDataType),
                valueToAdd.data.data(),
                valueToAdd.data.size()
            );
        }

        int32_t newOffset = _memoryPool.add(
            -1,
            0,
            toAdd.size(),
            toAdd.data()
        );
        PLATYPUS_ASSERT(newOffset != -1);
        pCustomData->elementCount = newElemCount;
        pCustomData->offset = newOffset;
        return newOffset;
    }

    int32_t CustomDataManager::update(int32_t offset, const std::string& newStr)
    {
        PLATYPUS_UNIMPLEMENTED;
        return -1;
    }

    void CustomDataManager::remove(int32_t offset)
    {
        PLATYPUS_UNIMPLEMENTED;
    }

    std::vector<CustomDataValue> CustomDataManager::getValues(int32_t offset) const
    {
        PLATYPUS_UNIMPLEMENTED;
        return { };
    }

    CustomDataManager::StoredCustomDataValue CustomDataManager::toStoredCustomDataValue(
        int32_t offset
    ) const
    {
        const size_t totalStorageSize = _memoryPool.getTotalSize();
        if (!validateOffset(offset))
        {
            Debug::log(
                "Invalid offset " + std::to_string(offset) + " "
                "MemoryPool size is " + std::to_string(totalStorageSize),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return { };
        }

        const uint8_t* pStorage = _memoryPool.getStorage().data();

        CustomDataType type;
        memcpy(&type, pStorage + offset, sizeof(CustomDataType));
        offset += sizeof(CustomDataType);
        PLATYPUS_ASSERT(offset <= totalStorageSize);

        const size_t dataSize = get_custom_data_type_size(type);
        PLATYPUS_ASSERT(dataSize > 0);

        std::vector<uint8_t> data(dataSize);
        memcpy(data.data(), pStorage + offset, dataSize);

        return {
            type,
            data
        };
    }

    std::vector<CustomDataManager::StoredCustomDataValue> CustomDataManager::copyValues(
        int32_t offset
    ) const
    {
        const size_t totalStorageSize = _memoryPool.getTotalSize();
        if (!validateOffset(offset))
        {
            Debug::log(
                "Invalid offset " + std::to_string(offset) + " "
                "MemoryPool size is " + std::to_string(totalStorageSize),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return { };
        }

        const uint8_t* pStorage = _memoryPool.getStorage().data();

        uint32_t count = 0;
        memcpy(&count, pStorage + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        PLATYPUS_ASSERT(offset <= totalStorageSize);

        std::vector<StoredCustomDataValue> result(static_cast<size_t>(count));
        for (uint32_t i = 0; i < count; ++i)
        {
            PLATYPUS_ASSERT(offset < totalStorageSize);
            result[i] = toStoredCustomDataValue(offset);
            offset += sizeof(CustomDataType) + result[i].data.size();
        }
        return result;
    }

    bool CustomDataManager::validateOffset(int32_t offset) const
    {
        return (offset != -1) && (offset < _memoryPool.getTotalSize());
    }

    void CustomDataManager::free_storage_func(size_t offset, size_t size, void* pUserData)
    {
        CustomDataManager* pCustomDataManager = reinterpret_cast<CustomDataManager*>(pUserData);
        DynamicElementSizeMemoryPool& memoryPool = pCustomDataManager->_memoryPool;
        PLATYPUS_ASSERT(offset + size <= memoryPool.getTotalSize());
        memset(memoryPool.accessStorage().data() + offset, 0, size);
    }

    bool CustomDataManager::validate_free_range_func(size_t offset, size_t size, void* pUserData)
    {
        const CustomDataManager* pCustomDataManager = reinterpret_cast<const CustomDataManager*>(pUserData);
        const DynamicElementSizeMemoryPool& memoryPool = pCustomDataManager->_memoryPool;
        PLATYPUS_ASSERT(offset + size <= memoryPool.getTotalSize());
        std::vector<uint8_t> emptyBytes(size);
        memset(emptyBytes.data(), 0, size);
        return memcmp(memoryPool.getStorage().data() + offset, emptyBytes.data(), size) == 0;
    }
}
